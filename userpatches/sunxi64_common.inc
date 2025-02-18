ARCH=arm64
ATF_TARGET_MAP="PLAT=$ATF_PLAT DEBUG=1 bl31;;build/$ATF_PLAT/debug/bl31.bin"
BOOTDELAY=-2
BOOTPATCHDIR='u-boot-sunxi'
UBOOT_TARGET_MAP=';;u-boot-sunxi-with-spl.bin'
BOOTSCRIPT='boot-sun50i-next.cmd:boot.cmd'
LINUXFAMILY=sunxi64

KERNELPATCHDIR='sunxi-'$BRANCH

family_tweaks()
{
	# execute specific tweaks function if present
	[[ $(type -t family_tweaks_s) == function ]] && family_tweaks_s

    [[ $RELEASE == bookworm ]] && dpkg_install_deb_chroot "$EXTER/cache/debs/extra/${RELEASE}-utils/wpasupplicant_2.10-9_arm64.deb"
    [[ $RELEASE == jammy ]] && dpkg_install_deb_chroot "$EXTER/cache/debs/extra/${RELEASE}-utils/wpasupplicant_2.10-6ubuntu1_arm64.deb"

	rm $SDCARD/root/*.deb >/dev/null 2>&1
}

write_uboot_platform()
{
	if [[ -f $1/boot0_sdcard.fex ]]; then # legacy h616/h6 loader
		dd if=$1/boot0_sdcard.fex of=$2 bs=8k seek=1 conv=fsync > /dev/null 2>&1
		dd if=$1/boot_package.fex of=$2 bs=8k seek=2050 conv=fsync > /dev/null 2>&1 || true
	elif [[ -f $1/u-boot-with-dtb.bin ]]; then # legacy a64 loader
		dd if=/dev/zero of=$2 bs=1k count=1023 seek=1 status=noxfer > /dev/null 2>&1
		dd if=$1/u-boot-with-dtb.bin of=$2 bs=1k seek=8 conv=fsync > /dev/null 2>&1 || true
	elif [[ -f $1/u-boot-sunxi-with-spl.bin ]]; then
		dd if=/dev/zero of=$2 bs=1k count=1023 seek=1 status=noxfer > /dev/null 2>&1
		dd if=$1/u-boot-sunxi-with-spl.bin of=$2 bs=1k seek=8 conv=fsync > /dev/null 2>&1 || true
	else
		[[ -f $1/sunxi-spl.bin ]] && dd if=$1/sunxi-spl.bin of=$2 bs=8k seek=1 conv=fsync > /dev/null 2>&1
		[[ -f $1/u-boot.itb ]] && dd if=$1/u-boot.itb of=$2 bs=8k seek=5 conv=fsync > /dev/null 2>&1 || true
	fi
}

setup_write_uboot_platform()
{
	if grep -q "ubootpart" /proc/cmdline; then
		# mainline with new boot script
		local tmp=$(cat /proc/cmdline)
		tmp="${tmp##*ubootpart=}"
		tmp="${tmp%% *}"
		[[ -n $tmp ]] && local part=$(findfs PARTUUID=$tmp 2>/dev/null)
		[[ -n $part ]] && local dev=$(lsblk -n -o PKNAME $part 2>/dev/null)
		[[ -n $dev ]] && DEVICE="/dev/$dev"
	else
		# legacy or old boot script
		local tmp=$(cat /proc/cmdline)
		tmp="${tmp##*root=}"
		tmp="${tmp%% *}"
		[[ -n $tmp ]] && local part=$(findfs $tmp 2>/dev/null)
		[[ -n $part ]] && local dev=$(lsblk -n -o PKNAME $part 2>/dev/null)
		# do not try to write u-boot to USB devices
		[[ -n $dev && $dev == mmcblk* ]] && DEVICE="/dev/$dev"
	fi
}
