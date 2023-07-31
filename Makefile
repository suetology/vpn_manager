include $(TOPDIR)/rules.mk

PKG_NAME:=vpn_manager
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/vpn_manager
	DEPENDS:=+libubus +libubox +libblobmsg-json +libjson-c
	CATEGORY:=Base system
	TITLE:=Program which is used to manage openvpn server via ubus
endef

define Package/vpn_manager/install
	$(INSTALL_DIR) $(1)/usr/bin
	# $(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vpn_manager $(1)/usr/bin/
	# $(INSTALL_BIN) ./files/vpn_manager.init $(1)/etc/init.d/vpn_manager
endef

$(eval $(call BuildPackage,vpn_manager))
