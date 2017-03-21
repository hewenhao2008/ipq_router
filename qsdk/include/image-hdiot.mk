include $(TOPDIR)/product_config/$(PR_NAME)/version.config

SYS_TIME :=$(shell date +%y%m%d)
BUILD_TIME :=$(shell date +%y%m%d%H%M%S)
UCI_FILE :=$(TARGET_DIR)/etc/config/device_info


define Image/hdiot/prepare
	$(call Image/hdiot/prepare/device_info)
endef

define Image/hdiot/prepare/device_info	
	touch $(UCI_FILE)
	cat /dev/null > $(UCI_FILE)
	echo "config info "\'info\' > $(UCI_FILE)
	if [ -f $(TOPDIR)/product_config/$(PR_NAME)/version.config ]; then \
		echo   "	option sw_version "\'$(SYS_SOFTWARE_REVISION_MAJOR).$(SYS_SOFTWARE_REVISION_MIDDLE).$(SYS_SOFTWARE_REVISION_MINOR) build $(SYS_TIME)\' >> $(UCI_FILE); \
		echo   "	option build_time "\'$(BUILD_TIME)\' >> $(UCI_FILE); \
	fi
endef