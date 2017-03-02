
QSDK_PATH=$(pwd)/..
QCS_SPF_PATH=~/qca-networking-2016-spf-4-0_qca_oem.git

#cd $QCS_SPF_PATH

echo "copy files to build dir..."
cp -rf $QSDK_PATH/qca/src/uboot-1.0/tools/pack.py $QCS_SPF_PATH/NHSS.QSDK.4.0/apss_proc/out/
cp -rf $QSDK_PATH/bin/ipq806x/openwrt* $QCS_SPF_PATH/IPQ4019.ILQ.4.0/common/build/ipq

cp -rf $QCS_SPF_PATH/TZ.BF.2.7/trustzone_images/build/ms/bin/MAZAANAA/* $QCS_SPF_PATH/IPQ4019.ILQ.4.0/common/build/ipq
cp -rf $QCS_SPF_PATH/BOOT.BF.3.1.1/boot_images/build/ms/bin/40xx/misc/tools/config/boardconfig_premium  $QCS_SPF_PATH/IPQ4019.ILQ.4.0/common/build/ipq 
cp -rf $QCS_SPF_PATH/BOOT.BF.3.1.1/boot_images/build/ms/bin/40xx/misc/tools/config/appsboardconfig_premium  $QCS_SPF_PATH/IPQ4019.ILQ.4.0/common/build/ipq


sed -i 's#</linux_root_path>#/</linux_root_path>#' $QCS_SPF_PATH/IPQ4019.ILQ.4.0/contents.xml 
sed -i 's#</windows_root_path>#\\</windows_root_path>#' $QCS_SPF_PATH/IPQ4019.ILQ.4.0/contents.xml

cd $QCS_SPF_PATH/IPQ4019.ILQ.4.0/common/build

sed '/debug/d' -i update_common_info.py 
sed '/Required/d' -i update_common_info.py 
sed '/streamboost/d' -i update_common_info.py

python update_common_info.py

cp -f bin/nand-ipq40xx-single.img $QSDK_PATH/
cd $QSDK_PATH

