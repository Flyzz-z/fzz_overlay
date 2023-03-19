// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017 Red Hat, Inc.
 */

#include <linux/cred.h>
#include <linux/file.h>
#include <linux/mount.h>
#include <linux/xattr.h>
#include <linux/uio.h>
#include <linux/uaccess.h>
#include "overlayfs.h"

static char ovl_whatisit(struct inode *inode, struct inode *realinode)
{
	if (realinode != ovl_inode_upper(inode))
		return 'l';
	if (ovl_has_upperdata(inode))
		return 'u';
	else
		return 'm';
}

static struct file *ovl_open_realfile(const struct file *file,
				      struct inode *realinode)
{
	struct inode *inode = file_inode(file);
	struct file *realfile;
	const struct cred *old_cred;
	int flags = file->f_flags | O_NOATIME | FMODE_NONOTIFY;

	old_cred = ovl_override_creds(inode->i_sb);
	realfile = open_with_fake_path(&file->f_path, flags, realinode,
				       current_cred());
	revert_creds(old_cred);

	pr_debug("open(%p[%pD2/%c], 0%o) -> (%p, 0%o)\n",
		 file, file, ovl_whatisit(inode, realinode), file->f_flags,
		 realfile, IS_ERR(realfile) ? 0 : realfile->f_flags);

	return realfile;
}

#define OVL_SETFL_MASK (O_APPEND | O_NONBLOCK | O_NDELAY | O_DIRECT)

static int ovl_change_flags(struct file *file, unsigned int flags)
{
	struct inode *inode = file_inode(file);
	int err;

	/* No atime modificaton on underlying */
	flags |= O_NOATIME | FMODE_NONOTIFY;

	/* If some flag changed that cannot be changed then something's amiss */
	if (WARN_ON((file->f_flags ^ flags) & ~OVL_SETFL_MASK))
		return -EIO;

	flags &= OVL_SETFL_MASK;

	if (((flags ^ file->f_flags) & O_APPEND) && IS_APPEND(inode))
		return -EPERM;

	if (flags & O_DIRECT) {
		if (!file->f_mapping->a_ops ||
		    !file->f_mapping->a_ops->direct_IO)
			return -EINVAL;
	}

	if (file->f_op->check_flags) {
		err = file->f_op->check_flags(flags);
		if (err)
			return err;
	}

	spin_lock(&file->f_lock);
	file->f_flags = (file->f_flags & ~OVL_SETFL_MASK) | flags;
	spin_unlock(&file->f_lock);

	return 0;
}

static int ovl_real_fdget_meta(const struct file *file, struct fd *real,
			       bool allow_meta)
{
	struct inode *inode = file_inode(file);
	struct inode *realinode;

	real->flags = 0;
	real->file = file->private_data;

	if (allow_meta)
		realinode = ovl_inode_real(inode);
	else
		realinode = ovl_inode_realdata(inode);

	/* Has it been copied up since we'd opened it? */
	if (unlikely(file_inode(real->file) != realinode)) {
		real->flags = FDPUT_FPUT;
		real->file = ovl_open_realfile(file, realinode);

		return PTR_ERR_OR_ZERO(real->file);
	}

	/* Did the flags change since open? */
	if (unlikely((file->f_flags ^ real->file->f_flags) & ~O_NOATIME))
		return ovl_change_flags(real->file, file->f_flags);

	return 0;
}

static int ovl_real_fdget(const struct file *file, struct fd *real)
{
	return ovl_real_fdget_meta(file, real, false);
}

static int ovl_open(struct inode *inode, struct file *file)
{
	struct file *realfile;
	int err;

	err = ovl_maybe_copy_up(file_dentry(file), file->f_flags);
	if (err)
		return err;

	/* No longer need these flags, so don't pass them on to underlying fs */
	file->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	realfile = ovl_open_realfile(file, ovl_inode_realdata(inode));
	if (IS_ERR(realfile))
		return PTR_ERR(realfile);

	file->private_data = realfile;

	return 0;
}

static int ovl_release(struct inode *inode, struct file *file)
{
	fput(file->private_data);

	return 0;
}

static loff_t ovl_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file_inode(file);
	struct fd real;
	const struct cred *old_cred;
	ssize_t ret;

	/*
	 * The two special cases below do not need to involve real fs,
	 * so we can optimizing concurrent callers.
	 */
	if (offset == 0) {
		if (whence == SEEK_CUR)
			return file->f_pos;

		if (whence == SEEK_SET)
			return vfs_setpos(file, 0, 0);
	}

	ret = ovl_real_fdget(file, &real);
	if (ret)
		return ret;

	/*
	 * Overlay file f_pos is the master copy that is preserved
	 * through copy up and modified on read/write, but only real
	 * fs knows how to SEEK_HOLE/SEEK_DATA and real fs may impose
	 * limitations that are more strict than ->s_maxbytes for specific
	 * files, so we use the real file to perform seeks.
	 */
	inode_lock(inode);
	real.file->f_pos = file->f_pos;

	old_cred = ovl_override_creds(inode->i_sb);
	ret = vfs_llseek(real.file, offset, whence);
	revert_creds(old_cred);

	file->f_pos = real.file->f_pos;
	inode_unlock(inode);

	fdput(real);

	return ret;
}

static void ovl_file_accessed(struct file *file)
{
	struct inode *inode, *upperinode;

	if (file->f_flags & O_NOATIME)
		return;

	inode = file_inode(file);
	upperinode = ovl_inode_upper(inode);

	if (!upperinode)
		return;

	if ((!timespec64_equal(&inode->i_mtime, &upperinode->i_mtime) ||
	     !timespec64_equal(&inode->i_ctime, &upperinode->i_ctime))) {
		inode->i_mtime = upperinode->i_mtime;
		inode->i_ctime = upperinode->i_ctime;
	}

	touch_atime(&file->f_path);
}

static rwf_t ovl_iocb_to_rwf(struct kiocb *iocb)
{
	int ifl = iocb->ki_flags;
	rwf_t flags = 0;

	if (ifl & IOCB_NOWAIT)
		flags |= RWF_NOWAIT;
	if (ifl & IOCB_HIPRI)
		flags |= RWF_HIPRI;
	if (ifl & IOCB_DSYNC)
		flags |= RWF_DSYNC;
	if (ifl & IOCB_SYNC)
		flags |= RWF_SYNC;

	return flags;
}

//fzz_overlay: start
static ssize_t block_read_iter(struct dentry *dentry,struct kiocb *iocb,struct iov_iter *iter)
{
	struct ovl_inode *oi;
	struct path upper_path,lower_path;
	size_t start_block,end_block,block;
	size_t count,c_count;
	ssize_t ret,cnt;
	bool lower_end = false,upper_end = false;

	start_block = ovl_get_block(iocb->ki_pos);
	end_block = ovl_get_block(iocb->ki_pos+iter->count);
	oi = OVL_I(dentry->d_inode);
	count = iter->count;
	printk("fzz_overlay block_read_iter pos=%lld pos+count=%lld\n",iocb->ki_pos,iocb->ki_pos+iter->count);
	printk("fzz_overlay block_read_iter start_block=%ld end_block=%ld count=%ld\n",start_block,end_block,count);
	ovl_path_upper(dentry,&upper_path);
	if(IS_ERR(&upper_path))
	{
		return PTR_ERR(&upper_path);
	}

	ovl_path_lowerdata(dentry,&lower_path);
	if(IS_ERR(&lower_path))
	{
		return PTR_ERR(&lower_path);
	}

	if(start_block>oi->block_count)
	{
		if(!oi->upper_file)
		{	
			oi->upper_file = ovl_path_open(&upper_path, O_RDWR);
			if (IS_ERR(oi->upper_file))
				return PTR_ERR(oi->upper_file);
		}
		ret = vfs_iter_read(oi->upper_file, iter, &iocb->ki_pos,
				    ovl_iocb_to_rwf(iocb));
		return ret;
	}	

	ret = 0;
	for(block=start_block;block<=end_block;block++)
	{
		if(block>oi->block_count)
		{
			if(upper_end)
				break;
			iter->count = count;
			count = 0;
			if(!oi->upper_file)
			{	
				oi->upper_file = ovl_path_open(&upper_path, O_RDWR);
				if (IS_ERR(oi->upper_file))
				{
					printk("fzz_overlay: open upper file error\n");
					return PTR_ERR(oi->upper_file);
				}
			}	
			cnt = vfs_iter_read(oi->upper_file, iter, &iocb->ki_pos,
					    ovl_iocb_to_rwf(iocb));
			if(cnt<0)
				break;
			printk("fzz_overlay: read upper file %s %ld\n",upper_path.dentry->d_name.name,cnt);
			ret+=cnt;
			break;
		}

		if(block==end_block)
			c_count = count;
		else 
			c_count = BLOCK_SIZE;
		iter->count = c_count;
		count-=c_count;
		printk("fzz_overlay block_read_iter block=%ld\n",block);
		if(oi->block_status[block])
		{
			if(upper_end)
				break;
			if(!oi->upper_file)
			{	
				oi->upper_file = ovl_path_open(&upper_path, O_RDWR);
				if (IS_ERR(oi->upper_file))
				{
					printk("fzz_overlay: open upper file error\n");
					return PTR_ERR(oi->upper_file);
				}
			}	
			cnt = vfs_iter_read(oi->upper_file, iter, &iocb->ki_pos,
					    ovl_iocb_to_rwf(iocb));
			if(cnt<0) 
				break;	
			else if(cnt == 0)
				upper_end = true;
			printk("fzz_overlay: read upper file %s %ld\n",upper_path.dentry->d_name.name,cnt);
			ret+=cnt;	
		} 
		else 
		{
			/*lower file reach end*/
			if(lower_end) 
				break;
			if(!oi->lower_file)
			{
				oi->lower_file = ovl_path_open(&lower_path, O_RDONLY);
				if (IS_ERR(oi->lower_file))
				{
					printk("fzz_overlay: open lower file error\n");
					return PTR_ERR(oi->lower_file);
				}
			}
			cnt = vfs_iter_read(oi->lower_file, iter, &iocb->ki_pos,
					    ovl_iocb_to_rwf(iocb));
			if(cnt<0)
				break;
			else if(cnt==0)
				lower_end = true;
			printk("fzz_overlay: read lower file %s %ld\n",lower_path.dentry->d_name.name,cnt);
			ret+=cnt;
		}
	}

	return ret;
}
//fzz_overlay: end

static ssize_t ovl_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct file *file = iocb->ki_filp;
	struct fd real;
	//fzz_overlay: start
	struct dentry *dentry;
	//fzz_overlay: end
	const struct cred *old_cred;
	ssize_t ret;

	if (!iov_iter_count(iter))
		return 0;

	ret = ovl_real_fdget(file, &real);
	if (ret)
		return ret;

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	// ret = vfs_iter_read(real.file, iter, &iocb->ki_pos,
	// 		    ovl_iocb_to_rwf(iocb));
	//fzz_overlay: start
	dentry = file_dentry(file);
	if(OVL_I(dentry->d_inode)->cow_status)
	{
		ret = block_read_iter(dentry,iocb,iter);
	}
	else
	{
		ret = vfs_iter_read(real.file, iter, &iocb->ki_pos,
			ovl_iocb_to_rwf(iocb));
	}
	//faa_overlay: end
	revert_creds(old_cred);

	ovl_file_accessed(file);

	fdput(real);

	return ret;
}

//fzz_overlay: start
static ssize_t block_write_iter(struct dentry *dentry,struct kiocb *iocb,struct iov_iter *iter)
{
	size_t start_block,end_block,i;
	bool partial = false;
	struct ovl_inode *oi = OVL_I(dentry->d_inode);
	struct path upper_path,lower_path;
	ssize_t ret;
	
	start_block = ovl_get_block(iocb->ki_pos);
	end_block = ovl_get_block(iocb->ki_pos + iter->count);
	ovl_path_upper(dentry,&upper_path);
	ovl_path_lower(dentry,&lower_path);

	printk("fzz_overlay block_write_iter: start_block %ld end_block %ld\n",start_block,end_block);
	if(iocb->ki_pos>(start_block-1)*BLOCK_SIZE+1)
	{
		partial = true;
		if(!oi->upper_file)
		{	
			oi->upper_file = ovl_path_open(&upper_path, O_RDWR);
			if (IS_ERR(oi->upper_file))
			{
				printk("fzz_overlay block_write_iter: open upper file error\n");
				return PTR_ERR(oi->upper_file);
			}
		}
		if(!oi->lower_file)
		{
			oi->lower_file = ovl_path_open(&lower_path, O_RDONLY);
			if (IS_ERR(oi->lower_file))
			{
				printk("fzz_overlay block_write_iter: open lower file error\n");
				return PTR_ERR(oi->lower_file);
			}
		}
		ret = ovl_copy_up_block(oi->lower_file,oi->upper_file,
			(start_block-1)*BLOCK_SIZE,BLOCK_SIZE);
		if(ret<0)
		{
			printk("fzz_overlay block_write_iter: copy up block error upper:%s\n",oi->upper_file->f_path.dentry->d_name.name);
			return ret;
		}
	}

	if(end_block*BLOCK_SIZE<(iocb->ki_pos + iter->count))
	{
		if(!(partial&&end_block==start_block))
		{
			if(!oi->upper_file)
			{	
				oi->upper_file = ovl_path_open(&upper_path, O_RDWR);
				if (IS_ERR(oi->upper_file))
				{
					printk("fzz_overlay block_write_iter: open upper file error\n");
					return PTR_ERR(oi->upper_file);
				}
			}
			if(!oi->lower_file)
			{
				oi->lower_file = ovl_path_open(&lower_path, O_RDONLY);
				if (IS_ERR(oi->lower_file))
				{
					printk("fzz_overlay block_write_iter: open lower file error\n");
					return PTR_ERR(oi->lower_file);
				}
			}
			ovl_copy_up_block(oi->lower_file,oi->upper_file,
				(end_block-1)*BLOCK_SIZE,BLOCK_SIZE);
			if(ret<0)
			{
				printk("fzz_overlay block_write_iter: copy up block error upper:%s\n",oi->upper_file->f_path.dentry->d_name.name);
				return ret;
			}
		}
	}
	vfs_fsync(oi->upper_file, 0);
	ret = vfs_iter_write(oi->upper_file, iter, &iocb->ki_pos,
		ovl_iocb_to_rwf(iocb));
	//TODO: how to handle ret<len?
	for(i=start_block;i<=end_block;i++)
	{
		if(i>oi->block_count)
			break;
		oi->block_status[i] = 1;
	}
	return ret;
}
//fzz_overlay: end

static ssize_t ovl_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	struct fd real;
	const struct cred *old_cred;
	ssize_t ret;

	if (!iov_iter_count(iter))
		return 0;

	inode_lock(inode);
	/* Update mode */
	ovl_copyattr(ovl_inode_real(inode), inode);
	ret = file_remove_privs(file);
	if (ret)
		goto out_unlock;

	ret = ovl_real_fdget(file, &real);
	if (ret)
		goto out_unlock;

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	file_start_write(real.file);
	// ret = vfs_iter_write(real.file, iter, &iocb->ki_pos,
	// 		     ovl_iocb_to_rwf(iocb));
	//fzz_overlay: start
	if(OVL_I(inode)->cow_status)
	{
		ret = block_write_iter(file_dentry(file),iocb,iter);
	}
	else
	{
		ret = vfs_iter_write(real.file, iter, &iocb->ki_pos,
			ovl_iocb_to_rwf(iocb));
	}
	//fzz_overlay: end
	file_end_write(real.file);
	revert_creds(old_cred);

	/* Update size */
	ovl_copyattr(ovl_inode_real(inode), inode);

	fdput(real);

out_unlock:
	inode_unlock(inode);

	return ret;
}

static int ovl_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	struct fd real;
	const struct cred *old_cred;
	int ret;

	ret = ovl_real_fdget_meta(file, &real, !datasync);
	if (ret)
		return ret;

	/* Don't sync lower file for fear of receiving EROFS error */
	if (file_inode(real.file) == ovl_inode_upper(file_inode(file))) {
		old_cred = ovl_override_creds(file_inode(file)->i_sb);
		ret = vfs_fsync_range(real.file, start, end, datasync);
		revert_creds(old_cred);
	}

	fdput(real);

	return ret;
}

static int ovl_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct file *realfile = file->private_data;
	const struct cred *old_cred;
	int ret;

	if (!realfile->f_op->mmap)
		return -ENODEV;

	if (WARN_ON(file != vma->vm_file))
		return -EIO;

	vma->vm_file = get_file(realfile);

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	ret = call_mmap(vma->vm_file, vma);
	revert_creds(old_cred);

	if (ret) {
		/* Drop reference count from new vm_file value */
		fput(realfile);
	} else {
		/* Drop reference count from previous vm_file value */
		fput(file);
	}

	ovl_file_accessed(file);

	return ret;
}

static long ovl_fallocate(struct file *file, int mode, loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	struct fd real;
	const struct cred *old_cred;
	int ret;

	ret = ovl_real_fdget(file, &real);
	if (ret)
		return ret;

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	ret = vfs_fallocate(real.file, mode, offset, len);
	revert_creds(old_cred);

	/* Update size */
	ovl_copyattr(ovl_inode_real(inode), inode);

	fdput(real);

	return ret;
}

static int ovl_fadvise(struct file *file, loff_t offset, loff_t len, int advice)
{
	struct fd real;
	const struct cred *old_cred;
	int ret;

	ret = ovl_real_fdget(file, &real);
	if (ret)
		return ret;

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	ret = vfs_fadvise(real.file, offset, len, advice);
	revert_creds(old_cred);

	fdput(real);

	return ret;
}

static long ovl_real_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct fd real;
	const struct cred *old_cred;
	long ret;

	ret = ovl_real_fdget(file, &real);
	if (ret)
		return ret;

	old_cred = ovl_override_creds(file_inode(file)->i_sb);
	ret = vfs_ioctl(real.file, cmd, arg);
	revert_creds(old_cred);

	fdput(real);

	return ret;
}

static long ovl_ioctl_set_flags(struct file *file, unsigned int cmd,
				unsigned long arg, unsigned int iflags)
{
	long ret;
	struct inode *inode = file_inode(file);
	unsigned int old_iflags;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	ret = mnt_want_write_file(file);
	if (ret)
		return ret;

	inode_lock(inode);

	/* Check the capability before cred override */
	ret = -EPERM;
	old_iflags = READ_ONCE(inode->i_flags);
	if (((iflags ^ old_iflags) & (S_APPEND | S_IMMUTABLE)) &&
	    !capable(CAP_LINUX_IMMUTABLE))
		goto unlock;

	ret = ovl_maybe_copy_up(file_dentry(file), O_WRONLY);
	if (ret)
		goto unlock;

	ret = ovl_real_ioctl(file, cmd, arg);

	ovl_copyflags(ovl_inode_real(inode), inode);
unlock:
	inode_unlock(inode);

	mnt_drop_write_file(file);

	return ret;

}

static unsigned int ovl_fsflags_to_iflags(unsigned int flags)
{
	unsigned int iflags = 0;

	if (flags & FS_SYNC_FL)
		iflags |= S_SYNC;
	if (flags & FS_APPEND_FL)
		iflags |= S_APPEND;
	if (flags & FS_IMMUTABLE_FL)
		iflags |= S_IMMUTABLE;
	if (flags & FS_NOATIME_FL)
		iflags |= S_NOATIME;

	return iflags;
}

static long ovl_ioctl_set_fsflags(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	unsigned int flags;

	if (get_user(flags, (int __user *) arg))
		return -EFAULT;

	return ovl_ioctl_set_flags(file, cmd, arg,
				   ovl_fsflags_to_iflags(flags));
}

static unsigned int ovl_fsxflags_to_iflags(unsigned int xflags)
{
	unsigned int iflags = 0;

	if (xflags & FS_XFLAG_SYNC)
		iflags |= S_SYNC;
	if (xflags & FS_XFLAG_APPEND)
		iflags |= S_APPEND;
	if (xflags & FS_XFLAG_IMMUTABLE)
		iflags |= S_IMMUTABLE;
	if (xflags & FS_XFLAG_NOATIME)
		iflags |= S_NOATIME;

	return iflags;
}

static long ovl_ioctl_set_fsxflags(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	struct fsxattr fa;

	memset(&fa, 0, sizeof(fa));
	if (copy_from_user(&fa, (void __user *) arg, sizeof(fa)))
		return -EFAULT;

	return ovl_ioctl_set_flags(file, cmd, arg,
				   ovl_fsxflags_to_iflags(fa.fsx_xflags));
}

static long ovl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	switch (cmd) {
	case FS_IOC_GETFLAGS:
	case FS_IOC_FSGETXATTR:
		ret = ovl_real_ioctl(file, cmd, arg);
		break;

	case FS_IOC_SETFLAGS:
		ret = ovl_ioctl_set_fsflags(file, cmd, arg);
		break;

	case FS_IOC_FSSETXATTR:
		ret = ovl_ioctl_set_fsxflags(file, cmd, arg);
		break;

	default:
		ret = -ENOTTY;
	}

	return ret;
}

static long ovl_compat_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	switch (cmd) {
	case FS_IOC32_GETFLAGS:
		cmd = FS_IOC_GETFLAGS;
		break;

	case FS_IOC32_SETFLAGS:
		cmd = FS_IOC_SETFLAGS;
		break;

	default:
		return -ENOIOCTLCMD;
	}

	return ovl_ioctl(file, cmd, arg);
}

enum ovl_copyop {
	OVL_COPY,
	OVL_CLONE,
	OVL_DEDUPE,
};

static loff_t ovl_copyfile(struct file *file_in, loff_t pos_in,
			    struct file *file_out, loff_t pos_out,
			    loff_t len, unsigned int flags, enum ovl_copyop op)
{
	struct inode *inode_out = file_inode(file_out);
	struct fd real_in, real_out;
	const struct cred *old_cred;
	loff_t ret;

	ret = ovl_real_fdget(file_out, &real_out);
	if (ret)
		return ret;

	ret = ovl_real_fdget(file_in, &real_in);
	if (ret) {
		fdput(real_out);
		return ret;
	}

	old_cred = ovl_override_creds(file_inode(file_out)->i_sb);
	switch (op) {
	case OVL_COPY:
		ret = vfs_copy_file_range(real_in.file, pos_in,
					  real_out.file, pos_out, len, flags);
		break;

	case OVL_CLONE:
		ret = vfs_clone_file_range(real_in.file, pos_in,
					   real_out.file, pos_out, len, flags);
		break;

	case OVL_DEDUPE:
		ret = vfs_dedupe_file_range_one(real_in.file, pos_in,
						real_out.file, pos_out, len,
						flags);
		break;
	}
	revert_creds(old_cred);

	/* Update size */
	ovl_copyattr(ovl_inode_real(inode_out), inode_out);

	fdput(real_in);
	fdput(real_out);

	return ret;
}

static ssize_t ovl_copy_file_range(struct file *file_in, loff_t pos_in,
				   struct file *file_out, loff_t pos_out,
				   size_t len, unsigned int flags)
{
	return ovl_copyfile(file_in, pos_in, file_out, pos_out, len, flags,
			    OVL_COPY);
}

static loff_t ovl_remap_file_range(struct file *file_in, loff_t pos_in,
				   struct file *file_out, loff_t pos_out,
				   loff_t len, unsigned int remap_flags)
{
	enum ovl_copyop op;

	if (remap_flags & ~(REMAP_FILE_DEDUP | REMAP_FILE_ADVISORY))
		return -EINVAL;

	if (remap_flags & REMAP_FILE_DEDUP)
		op = OVL_DEDUPE;
	else
		op = OVL_CLONE;

	/*
	 * Don't copy up because of a dedupe request, this wouldn't make sense
	 * most of the time (data would be duplicated instead of deduplicated).
	 */
	if (op == OVL_DEDUPE &&
	    (!ovl_inode_upper(file_inode(file_in)) ||
	     !ovl_inode_upper(file_inode(file_out))))
		return -EPERM;

	return ovl_copyfile(file_in, pos_in, file_out, pos_out, len,
			    remap_flags, op);
}

const struct file_operations ovl_file_operations = {
	.open		= ovl_open,
	.release	= ovl_release,
	.llseek		= ovl_llseek,
	.read_iter	= ovl_read_iter,
	.write_iter	= ovl_write_iter,
	.fsync		= ovl_fsync,
	.mmap		= ovl_mmap,
	.fallocate	= ovl_fallocate,
	.fadvise	= ovl_fadvise,
	.unlocked_ioctl	= ovl_ioctl,
	.compat_ioctl	= ovl_compat_ioctl,

	.copy_file_range	= ovl_copy_file_range,
	.remap_file_range	= ovl_remap_file_range,
};
