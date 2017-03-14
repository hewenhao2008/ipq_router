#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# Ralink Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iot_event_demo
PKG_VERSION:=1.0.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/iot_event_demo
    CATEGORY:=IoT
	TITLE:=Iot wifi subdev manager Package
  DEPENDS:=+libpthread  +libiot_util
endef
define Package/iot_event_demo/description
  Iot wifi subdev manager Package
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += 

define Build/Configure
endef

define Package/iot_event_demo/install
	$(INSTALL_DIR) $(1)/usr/bin	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/iot_event_demo  $(1)/usr/bin/
endef


$(eval $(call BuildPackage,iot_event_demo))

