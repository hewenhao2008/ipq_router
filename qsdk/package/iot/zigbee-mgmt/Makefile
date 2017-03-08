include $(TOPDIR)/rules.mk

PKG_NAME:=zigbee-mgmt
PKG_VERSION:=0.3
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/zigbee-mgmt
#  SECTION:=net
	CATEGORY:=IoT
	DEPENDS:=+libpthread 
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
endef

TARGET_LDFLAGS += -lpthread

$(eval $(call BuildPackage,zigbee-mgmt))

