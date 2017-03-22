include $(TOPDIR)/rules.mk

PKG_NAME:=zigbee-mgmt
PKG_VERSION:=0.3
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/zigbee-mgmt
#  SECTION:=net
	CATEGORY:=IoT
	DEPENDS:=+libpthread +libjson-c +libiot_util +libreadline +libncurses +libsqlite3
	TITLE:=zigbee-mgmt PC host programs.
endef

define Package/zigbee-mgmt/description
	zigbee-mgmt is the application running on Router side to handle the zigbee device.
endef

define Build/Prepare
	echo building under $(PKG_BUILD_DIR)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef 

define Build/InstallDev
	
endef

define Package/zigbee-mgmt/install	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/zigbee-mgmt $(1)/usr/bin/	
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/evergrande_startup $(1)/etc/init.d/
	
	#used for test phase
	$(INSTALL_DIR) $(1)/root
	$(CP) $(PKG_BUILD_DIR)/zigbee_dev.db $(1)/root
	
	#for unit test only.
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/zigbee-mgmt-ut $(1)/usr/bin/	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/zigbee-cli $(1)/usr/bin/	
endef

TARGET_CFLAGS += -g -I$(PKG_BUILD_DIR)
TARGET_LDFLAGS += -lpthread  -ljson-c -liot_util -lreadline -lncurses -lsqlite3

$(eval $(call BuildPackage,zigbee-mgmt))

