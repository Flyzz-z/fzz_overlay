sudo modprobe -r overlay
make
kmodsign sha512 ~/sign/MOK.priv ~/sign/MOK.der overlay.ko
sudo rm /lib/modules/5.3.5-050305-generic/kernel/fs/overlayfs/overlay.ko
sudo cp overlay.ko /lib/modules/5.3.5-050305-generic/kernel/fs/overlayfs/overlay.ko
sudo modprobe overlay
