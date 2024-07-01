# Installs the dependencies needed for building project

set -e
CURRENT_USER=$(whoami)
# FIXME: Update this information to use LLVM and no GNU EFI instead
sudo apt-get install make gcc nasm mtools qemu-system
cd /tmp
git clone https://git.code.sf.net/p/gnu-efi/code gnu-efi
cd gnu-efi
make
sudo make install
sudo usermod -aG kvm $CURRENT_USER
echo Consider reloading shell - Added KVM group to $CURRENT_USER
