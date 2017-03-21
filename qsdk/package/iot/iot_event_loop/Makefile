#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# Ralink Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iot_event_loop
PKG_VERSION:=1.0.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/iot_event_loop
	CATEGORY:=IoT
	TITLE:=Iot event loop package
  DEPENDS:=+libiot_util +libjson-c
endef
define Package/iot_event_loop/description
  Iot event loop package
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += 

define Build/Configure
endef

define Package/iot_event_loop/install
	$(INSTALL_DIR) $(1)/usr/bin	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/iot_event_loop $(1)/usr/bin/
endef


$(eval $(call BuildPackage,iot_event_loop))

