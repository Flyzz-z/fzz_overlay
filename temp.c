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