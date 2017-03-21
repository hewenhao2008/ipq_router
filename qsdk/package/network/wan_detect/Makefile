include $(TOPDIR)/rules.mk

PKG_NAME:=wan_detect
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_BUILD_BIN:=$(PKG_BUILD_DIR)/$(MAKE_PATH)/bin

define Package/wan_detect
  SUBMENU:=Apps
  SECTION:=net
  TITLE:=WAN type auto detection
  DEPENDS:=
endef

define Package/wan_detect/description
 The wan_detect implements autodetection of wan interface. 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	mkdir -p $(PKG_BUILD_BIN)
endef

define Package/wan_detect/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_BIN)/link_type_detect_core $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_BIN)/link_status_detect_core $(1)/usr/sbin
endef

$(eval $(call BuildPackage,wan_detect))

