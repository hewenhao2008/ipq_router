include $(TOPDIR)/rules.mk

PKG_NAME:=iot_startup
PKG_VERSION:=0.3
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/iot_startup
#  SECTION:=net
	CATEGORY:=IoT
	#DEPENDS:=iot_event_loop zigbee-mgmt
	DEPENDS:=iot_event_loop
	TITLE:=iot_startup script.
endef

define Package/iot_startup/description
	script to start iot apps during bootup phase.
endef

define Build/Prepare
	echo building under $(PKG_BUILD_DIR)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef 

define Build/Compile/Default

endef
Build/Compile = $(Build/Compile/Default)

define Build/InstallDev
	
endef

define Package/iot_startup/install		
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/evergrande_startup $(1)/etc/init.d/		
endef

$(eval $(call BuildPackage,iot_startup))

