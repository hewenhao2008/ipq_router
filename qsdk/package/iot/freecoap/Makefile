include $(TOPDIR)/rules.mk

PKG_NAME:=freecoap
PKG_VERSION:=0.3
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/freecoap
#  SECTION:=net
	CATEGORY:=IoT
	DEPENDS:=+libgmp +libnettle +libgnutls
	TITLE:=FreeCoap library and test programs.
	URL:=https://github.com/keith-cullen/FreeCoAP/releases
endef

define Package/freecoap/description
	COAP (Constrained Application Protocol) is used by IoT Device to access Internet. it is base on UDP.
	FreeCoap is one implementation of COAP.
endef

define Build/Prepare
	echo building under $(PKG_BUILD_DIR)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef 

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/lib/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libfreecoap.so $(1)/usr/lib/
endef

define Package/freecoap/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libfreecoap.so $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test_coap_server $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test_coap_client $(1)/usr/bin/
endef

CFLAGS += -Ilib/include

$(eval $(call BuildPackage,freecoap))

