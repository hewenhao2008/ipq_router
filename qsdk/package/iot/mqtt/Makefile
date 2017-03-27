#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# Ralink Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt
PKG_VERSION:=1.00

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/mqtt
  CATEGORY:=Custom packages
  SUBMENU:=common
  DEPENDS:=+libmosquitto +libjson-c +libiot_util
  TITLE:=Translate uci config into ralink wifi dat format
endef

define Package/mqtt/description
  Translate uci config into ralink wifi dat format
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += -lmosquitto -ljson-c

define Build/Configure
endef

define Package/mqtt/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/client1 $(PKG_BUILD_DIR)/client2 $(PKG_BUILD_DIR)/client3 $(PKG_BUILD_DIR)/server_test $(1)/usr/bin
endef


$(eval $(call BuildPackage,mqtt))

