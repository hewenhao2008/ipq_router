#
# Copyright (C) 2011 OpenWrt.org
#

USE_REFRESH=1

. /lib/ipq806x.sh
. /lib/upgrade/common.sh

RAMFS_COPY_DATA=/lib/ipq806x.sh
RAMFS_COPY_BIN="/usr/bin/dumpimage /bin/mktemp /usr/sbin/mkfs.ubifs /usr/bin/truncate 
	/usr/sbin/ubiattach /usr/sbin/ubidetach /usr/sbin/ubiformat /usr/sbin/ubimkvol
	/usr/sbin/ubiupdatevol /usr/bin/basename /bin/rm /usr/bin/find"


get_full_section_name() {
	local img=$1
	local sec=$2

	dumpimage -l ${img} | grep "^ Image.*(${sec})" | \
		sed 's,^ Image.*(\(.*\)),\1,'
}

image_contains() {
	local img=$1
	local sec=$2
	dumpimage -l ${img} | grep -q "^ Image.*(${sec}.*)" || return 1
}

#yzg: 通过dumpimage 打印FIT文件的sections。
print_sections() {
	local img=$1

	dumpimage -l ${img} | awk '/^ Image.*(.*)/ { print gensub(/Image .* \((.*)\)/,"\\1", $0) }'
}

image_has_mandatory_section() {
	local img=$1
	local mandatory_sections=$2

	for sec in ${mandatory_sections}; do
		image_contains $img ${sec} || {\
			return 1
		}
	done
}

image_demux() {
	local img=$1

	for sec in $(print_sections ${img}); do
		local fullname=$(get_full_section_name ${img} ${sec})

		dumpimage -i ${img} -o /tmp/${fullname}.bin ${fullname} > /dev/null || { \
			echo "Error while extracting \"${sec}\" from ${img}"
			return 1
		}
	done
	return 0
}

image_is_FIT() {
	if ! dumpimage -l $1 > /dev/null 2>&1; then
		echo "$1 is not a valid FIT image"
		return 1
	fi
	return 0
}

switch_layout() {
	local layout=$1
	local boot_layout=`find / -name boot_layout`

	# Layout switching is only required as the  boot images (up to u-boot)
	# use 512 user data bytes per code word, whereas Linux uses 516 bytes.
	# It's only applicable for NAND flash. So let's return if we don't have
	# one.

	[ -n "$boot_layout" ] || return

	case "${layout}" in
		boot|1) echo 1 > $boot_layout;;
		linux|0) echo 0 > $boot_layout;;
		*) echo "Unknown layout \"${layout}\"";;
	esac
}

#do_flash_mtd ${sec} "0:HLOS"
#[root@OpenWrt:/proc# cat mtd 
#dev:    size   erasesize  name
#mtd0: 00100000 00020000 "0:SBL1"
#mtd1: 00100000 00020000 "0:MIBIB"
#mtd2: 00100000 00020000 "0:BOOTCONFIG"
#]
# /proc/mtd 文件记录各分区大小。
do_flash_mtd() {
	local bin=$1
	local mtdname=$2
	local append=""
	local img_md5
	local img_size
	local part_md5

	local mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')
	local pgsz=$(cat /sys/class/mtd/${mtdpart}/writesize)
	[ -f "$CONF_TAR" -a "$SAVE_CONFIG" -eq 1 -a "$2" == "rootfs" ] && append="-j $CONF_TAR"

	#yzg: dd output the content to stdout, mtd write the content from stdin ("write -") to mtd part. (-e /dev/${mtdpart}, erase device before writting.)
	dd if=/tmp/${bin}.bin bs=${pgsz} conv=sync | mtd $append write - -e "/dev/${mtdpart}" "/dev/${mtdpart}"
		
	#### verify flashing success ##################
	img_md5=$(md5sum /tmp/${bin}.bin | awk '{print $1}')
	img_size=$(ls -l /tmp/${bin}.bin | awk '{print $5}')
	
	dd if=/dev/${mtdpart} of=/tmp/partitionN.img bs=2048
	truncate -s ${img_size} /tmp/partitionN.img
	
	part_md5=$(md5sum /tmp/partitionN.img | awk '{print $1}')
	
	if [ ${img_md5} == ${part_md5} ]; then
		echo "--md5 verification passed...${img_md5}"	
	else
		echo "--md5 verification fail....image_md5: [${img_md5}]  part_md5:[${part_md5}]"	
		sleep 1000
		#reboot
	fi
	
	rm -f /tmp/partitionN.img
	###############################################
}

# /proc/mtd 文件记录各分区大小。
# do_flash_mtd_by_mtdpart image.bin mtdx 
do_flash_mtd_by_mtdpart() {
	local bin=$1
	local mtdpart=$2
	local append=""
	local img_md5
	local img_size
	local part_md5
	local pgsz
	
	echo "------ do_flash_mtd_by_mtdpart: $1 to ${mtdpart} -----------"
	
	pgsz=$(cat /sys/class/mtd/${mtdpart}/writesize)
	[ -f "$CONF_TAR" -a "$SAVE_CONFIG" -eq 1 -a "$2" == "rootfs" ] && append="-j $CONF_TAR"
		

	#yzg: dd output the content to stdout, mtd write the content from stdin ("write -") to mtd part. (-e /dev/${mtdpart}, erase device before writting.)
	dd if=/tmp/${bin}.bin bs=${pgsz} conv=sync | mtd $append write - -e "/dev/${mtdpart}" "/dev/${mtdpart}"
		
	#### verify flashing success ##################
	img_md5=$(md5sum /tmp/${bin}.bin | awk '{print $1}')
	img_size=$(ls -l /tmp/${bin}.bin | awk '{print $5}')
	
	dd if=/dev/${mtdpart} of=/tmp/partitionN.img bs=2048
	truncate -s ${img_size} /tmp/partitionN.img
	
	part_md5=$(md5sum /tmp/partitionN.img | awk '{print $1}')
	
	if [ ${img_md5} == ${part_md5} ]; then
		echo "--md5 verification passed...${img_md5}"	
	else
		echo "--md5 verification fail....image_md5: [${img_md5}]  part_md5:[${part_md5}]"	
		sleep 1000
		#reboot
	fi
	
	rm -f /tmp/partitionN.img
	###############################################
}


do_flash_emmc() {
	local bin=$1
	local emmcblock=$2

	dd if=/dev/zero of=${emmcblock}
	dd if=/tmp/${bin}.bin of=${emmcblock}
}

do_flash_partition() {
	local bin=$1
	local mtdname=$2
	local emmcblock="$(find_mmc_part "$mtdname")"

	if [ -e "$emmcblock" ]; then
		do_flash_emmc $bin $emmcblock
	else
		do_flash_mtd $bin $mtdname
	fi
}

do_flash_bootconfig() {
	local bin=$1
	local mtdname=$2

	# Fail safe upgrade
	if [ -f /proc/boot_info/getbinary_${bin} ]; then
		cat /proc/boot_info/getbinary_${bin} > /tmp/${bin}.bin
		do_flash_partition $bin $mtdname
	fi
}

#yzg: do_flash_failsafe_partition ${sec} "0:HLOS"
#[root@OpenWrt:/proc/boot_info# ls
#0:APPSBL               0:QSEE                 rootfs
#0:CDT                  getbinary_bootconfig
#0:HLOS                 getbinary_bootconfig1
#]
do_flash_failsafe_partition() {
	local bin=$1
	local mtdname=$2
	local emmcblock
	local primaryboot
	local mtdpart

	########### changes: 统一修改primaryboot flag，不单独更新 ###############
	# Fail safe upgrade
	#[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
	#	default_mtd=$mtdname
	#	mtdname=$(cat /proc/boot_info/$mtdname/upgradepartition)
	#	primaryboot=$(cat /proc/boot_info/$default_mtd/primaryboot)
	#	if [ $primaryboot -eq 0 ]; then
	#		echo 1 > /proc/boot_info/$default_mtd/primaryboot
	#	else
	#		echo 0 > /proc/boot_info/$default_mtd/primaryboot
	#	fi
	#}
	
	echo "---- debug: mtdname is ${mtdname} ----"
	
	if [ ${mtdname} == "0:APPSBL" ]; then
			## upgrading uboot now. switch  0:APPSBL and 0:APPSBL_1
			# check whether we are boot by primary parititon, rootfs on mtd12. then uboot upgarde to mtd10--APPSBL_1
			mtdpart=$(grep "\"rootfs"\" /proc/mtd | awk -F: '{print $1}')
			
			echo "---- debug: mtdpart is ${mtdpart} ----"
			
			if [ $mtdpart = "mtd12" ]; then
				mtdpart="mtd10"
			else				
				mtdpart="mtd9"			   
		  fi
		 
		 do_flash_mtd_by_mtdpart $bin ${mtdpart}
		 
		 return 0
	else
		[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
		  default_mtd=$mtdname
		  mtdname=$(cat /proc/boot_info/$mtdname/upgradepartition)
	  }
 fi
	#############changes end ####################################################

	emmcblock="$(find_mmc_part "$mtdname")"

	if [ -e "$emmcblock" ]; then
		do_flash_emmc $bin $emmcblock
	else
		do_flash_mtd $bin $mtdname
	fi

}

## 统一更新primaryboot标记。
do_force_primaryboot_change()
{
	local mtdnames="rootfs 0:HLOS 0:APPSBL 0:CDT 0:QSEE"
	
	echo '----- upgrade 'primaryboot'=$1 flag begin ---------'
	
	for mtdname in ${mtdnames}; do 
		#mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')
		
		#debug
		ls -l /proc/boot_info/$mtdname/upgradepartition
		
		[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
			echo $1 > /proc/boot_info/$mtdname/primaryboot
		}	
	done
	
	echo '----- upgrade 'primaryboot' flag done ---------'
}

#yzg: do_flash_failsafe_partition ${sec} "rootfs"
do_flash_ubi() {
	local bin=$1
	local mtdname=$2
	local mtdpart
	local primaryboot
	local img_md5
	local img_size
	local part_md5

	mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')
	#yzg: -f : force, -p x: <path to device> 
	ubidetach -f -p /dev/${mtdpart}

	echo "-------mtdname:${mtdname}  mtdpart:${mtdpart}-------"
	
	########## change: 最后修改primaryboot标记 ####################
	# Fail safe upgrade
	[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
		#primaryboot=$(cat /proc/boot_info/$mtdname/primaryboot)
		#if [ $primaryboot -eq 0 ]; then
		#	echo 1 > /proc/boot_info/$mtdname/primaryboot
		#else
		#	echo 0 > /proc/boot_info/$mtdname/primaryboot
		#fi
		
		#Now mtdname is rootfs_1, going to upgrade this part...
		mtdname=$(cat /proc/boot_info/$mtdname/upgradepartition)
	}

	mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')

	#yzg: -y : yes for all questions. -f flash_image_file.
	ubiformat /dev/${mtdpart} -y -f /tmp/${bin}.bin
	
	############# changes: add image verifing...############
	img_md5=$(md5sum /tmp/${bin}.bin | awk '{print $1}')
	img_size=$(ls -l /tmp/${bin}.bin | awk '{print $5}')
	
	echo "------ image size: ${img_size}"
	
	dd if=/dev/${mtdpart} of=/tmp/partitionN.img bs=2048
	truncate -s ${img_size} /tmp/partitionN.img
		
	part_md5=$(md5sum /tmp/${bin}.bin | awk '{print $1}')
	
	if [ ${img_md5} == ${part_md5} ]; then
		echo "--md5 verification passed...${img_md5}"	
	else
		echo "--md5 verification fail....image_md5: [${img_md5}]  part_md5:[${part_md5}]"	
		sleep 1000
		#reboot
	fi
	rm -f /tmp/partitionN.img
	#########################################
	
	##### changes:  最后修改primaryboot标记 ####################
	echo "now mtdname is: ${mtdname}"
	
	#[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
		
		primaryboot=$(cat /proc/boot_info/rootfs/primaryboot)
		if [ $primaryboot -eq 0 ]; then
			do_force_primaryboot_change 1
			echo "change primaryboot to 1."
		else
			do_force_primaryboot_change 0
			echo "change primaryboot to 0."
		fi		
	#}
	
	#if [ !-f /proc/boot_info/rootfs/upgradepartition ]; then
	#	echo "error, /proc/boot_info/$mtdname/upgradepartition not exists."
	#fi
	
}

do_flash_tz() {
	local sec=$1
	local mtdpart=$(grep "\"0:QSEE\"" /proc/mtd | awk -F: '{print $1}')
	local emmcblock="$(find_mmc_part "0:QSEE")"

	if [ -n "$mtdpart" -o -e "$emmcblock" ]; then
		do_flash_failsafe_partition ${sec} "0:QSEE"
	else
		do_flash_failsafe_partition ${sec} "0:TZ"
	fi
}

do_flash_ddr() {
	local sec=$1
	local mtdpart=$(grep "\"0:CDT\"" /proc/mtd | awk -F: '{print $1}')
	local emmcblock="$(find_mmc_part "0:CDT")"

	if [ -n "$mtdpart" -o -e "$emmcblock" ]; then
		do_flash_failsafe_partition ${sec} "0:CDT"
	else
		do_flash_failsafe_partition ${sec} "0:DDRPARAMS"
	fi
}

to_upper ()
{
	echo $1 | awk '{print toupper($0)}'
}

#yzg 真正的刷写分区。
flash_section() {
	local sec=$1

	local board=$(ipq806x_board_name)
	case "${sec}" in
		hlos*) switch_layout linux; do_flash_failsafe_partition ${sec} "0:HLOS";;
		rootfs*) switch_layout linux; do_flash_failsafe_partition ${sec} "rootfs";;
		fs*) switch_layout linux; do_flash_failsafe_partition ${sec} "rootfs";;
		ubi*) switch_layout linux; do_flash_ubi ${sec} "rootfs";;
		#sbl1*) switch_layout boot; do_flash_partition ${sec} "0:SBL1";;
		#sbl2*) switch_layout boot; do_flash_failsafe_partition ${sec} "0:SBL2";;
		#sbl3*) switch_layout boot; do_flash_failsafe_partition ${sec} "0:SBL3";;
		#mibib*) switch_layout boot; do_flash_partition ${sec} "0:MIBIB";;
		#dtb-$(to_upper $board)*) switch_layout boot; do_flash_partition ${sec} "0:DTB";;
		u-boot*) switch_layout boot; do_flash_failsafe_partition ${sec} "0:APPSBL";;
		#ddr-$(to_upper $board)*) switch_layout boot; do_flash_ddr ${sec};;
		#ddr-${board}-*) switch_layout boot; do_flash_failsafe_partition ${sec} "0:DDRCONFIG";;
		#ssd*) switch_layout boot; do_flash_partition ${sec} "0:SSD";;
		#tz*) switch_layout boot; do_flash_tz ${sec};;
		#rpm*) switch_layout boot; do_flash_failsafe_partition ${sec} "0:RPM";;
		*) echo "Section ${sec} ignored"; return 1;;
	esac

	echo "Flashed ${sec}"
}

erase_emmc_config() {
	local emmcblock="$(find_mmc_part "rootfs_data")"
	if [ -e "$emmcblock" -a "$SAVE_CONFIG" -ne 1 ]; then
		dd if=/dev/zero of=${emmcblock}
	fi
}

platform_check_image() {
	local board=$(ipq806x_board_name)

	local mandatory_nand="ubi"
	local mandatory_nor_emmc="hlos fs"
	local mandatory_nor="hlos"
	local mandatory_section_found=0
	local optional="sb11 sbl2 u-boot ddr-${board} ssd tz rpm"
	local ignored="mibib bootconfig"

	image_is_FIT $1 || return 1

	image_has_mandatory_section $1 ${mandatory_nand} && {\
		mandatory_section_found=1
	}

	image_has_mandatory_section $1 ${mandatory_nor_emmc} && {\
		mandatory_section_found=1
	}

	image_has_mandatory_section $1 ${mandatory_nor} && {\
		mandatory_section_found=1
	}

	if [ $mandatory_section_found -eq 0 ]; then
		echo "Error: mandatory section(s) missing from \"$1\". Abort..."
		return 1
	fi

	for sec in ${optional}; do
		image_contains $1 ${sec} || {\
			echo "Warning: optional section \"${sec}\" missing from \"$1\". Continue..."
		}
	done

	for sec in ${ignored}; do
		image_contains $1 ${sec} && {\
			echo "Warning: section \"${sec}\" will be ignored from \"$1\". Continue..."
		}
	done

	image_demux $1 || {\
		echo "Error: \"$1\" couldn't be extracted. Abort..."
		return 1
	}

	[ -f /tmp/hlos_version ] && rm -f /tmp/*_version
	dumpimage -c $1
	return $?
}

platform_version_upgrade() {
	local version_files="appsbl_version sbl_version tz_version hlos_version rpm_version"
	local sys="/sys/devices/system/qfprom/qfprom0/"
	local tmp="/tmp/"

	for file in $version_files; do
		[ -f "${tmp}${file}" ] && {
			echo "Updating "${sys}${file}" with `cat "${tmp}${file}"`"
			echo `cat "${tmp}${file}"` > "${sys}${file}"
			rm -f "${tmp}${file}"
		}
	done
}

#yzg: do_upgrade -> common.sh : platform_do_upgrade "$ARGV"
platform_do_upgrade() {
	local board=$(ipq806x_board_name)

	# verify some things exist before erasing
	if [ ! -e $1 ]; then
		echo "Error: Can't find $1 after switching to ramfs, aborting upgrade!"
		reboot
	fi

	for sec in $(print_sections $1); do
		if [ ! -e /tmp/${sec}.bin ]; then
			echo "Error: Cant' find ${sec} after switching to ramfs, aborting upgrade!"
			reboot
		fi
	done

	case "$board" in
	db149 | ap148 | ap145 | ap148_1xx | db149_1xx | db149_2xx | ap145_1xx | ap160 | ap160_2xx | ap161 | ak01_1xx | ap-dk01.1-c1 | ap-dk01.1-c2 | ap-dk04.1-c1 | ap-dk04.1-c2 | ap-dk04.1-c3 | ap-dk04.1-c4 | ap-dk04.1-c5 | ap-dk05.1-c1 |  ap-dk06.1-c1 | ap-dk07.1-c1 | ap-dk07.1-c2)
		for sec in $(print_sections $1); do
			echo "---- flash_section ${sec} now...."
			flash_section ${sec}
		done

		switch_layout linux
		# update bootconfig to register that fw upgrade has been done
		do_flash_bootconfig bootconfig "0:BOOTCONFIG"
		do_flash_bootconfig bootconfig1 "0:BOOTCONFIG1"
		platform_version_upgrade

		erase_emmc_config
		return 0;
		;;
	esac

	echo "Upgrade failed!"
	return 1;
}

platform_copy_config() {
	local nand_part="$(find_mtd_part "ubi_rootfs")"
	local emmcblock="$(find_mmc_part "rootfs_data")"

	if [ -e "$nand_part" ]; then
		local mtdname=rootfs
		local mtdpart

		[ -f /proc/boot_info/$mtdname/upgradepartition ] && {
			mtdname=$(cat /proc/boot_info/$mtdname/upgradepartition)
		}

		mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')
		ubiattach -p /dev/${mtdpart}
		mount -t ubifs ubi0:rootfs_data /tmp/overlay
		cp /tmp/sysupgrade.tgz /tmp/overlay/
		sync
		umount /tmp/overlay
		echo "-----platform_copy_config complete. [$nand_part] (mtdname is ${mtdname}) (mtdpart is ${mtdpart})  -------"
	elif [ -e "$emmcblock" ]; then
		mount -t ext4 "$emmcblock" /tmp/overlay
		cp /tmp/sysupgrade.tgz /tmp/overlay/
		sync
		umount /tmp/overlay
		echo "-----platform_copy_config complete [$emmcblock] -------"
	fi
}

