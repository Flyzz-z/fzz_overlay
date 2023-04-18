sudo umount ./merged
sudo mount -t overlay overlay -olowerdir=./lower1:./lower2,upperdir=./upper,workdir=./work ./merged
rm upper/*

