# miniupnpd
 -= MiniUPnP project =-  
Main author : Thomas BERNARD
 
Github : https://github.com/miniupnp/miniupnp/

miniupnpd/ : MiniUPnP daemon - an implementation of a UPnP IGD                                
             + NAT-PMP / PCP gateway

#Makefile For Openwrt
```Makefile
#
# Copyright (C) 2006-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=miniupnpd
PKG_VERSION:=1.9.20150609
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/dnetlab/miniupnpd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=94aa36c73ae8f1534809f6e838ddb5df7c131e37
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz


PKG_MAINTAINER:=Markus Stenberg <fingon@iki.fi>
PKG_LICENSE:=BSD-3-Clause

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Dnetlab
  DEPENDS:=+iptables +libip4tc +IPV6:libip6tc +IPV6:ip6tables
  TITLE:=Lightweight UPnP IGD, NAT-PMP & PCP daemon
  SUBMENU:=Firewall
  URL:=http://miniupnp.free.fr/
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/upnpd
endef

define Package/$(PKG_NAME)/postinst
#!/bin/sh

if [ -z "$$IPKG_INSTROOT" ]; then
  ( . /etc/uci-defaults/99-miniupnpd )
  rm -f /etc/uci-defaults/99-miniupnpd
fi

exit 0
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	echo "OpenWrt" | tr \(\)\  _ >$(PKG_BUILD_DIR)/os.openwrt
endef

MAKE_FLAGS += \
	TEST=0 \
	LIBS="" \
	CC="$(TARGET_CC) -DIPTABLES_143 -lip4tc $(if $(CONFIG_IPV6),-lip6tc)" \
	CONFIG_OPTIONS="$(if $(CONFIG_IPV6),--ipv6) --leasefile --vendorcfg" \
	-f Makefile.linux \
	miniupnpd

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/etc/init.d $(1)/etc/config $(1)/etc/uci-defaults $(1)/etc/hotplug.d/iface $(1)/usr/share/miniupnpd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/miniupnpd $(1)/usr/sbin/miniupnpd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/miniupnpd.init $(1)/etc/init.d/miniupnpd
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/upnpd.config $(1)/etc/config/upnpd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/files/miniupnpd.hotplug $(1)/etc/hotplug.d/iface/50-miniupnpd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/files/miniupnpd.defaults $(1)/etc/uci-defaults/99-miniupnpd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/files/firewall.include $(1)/usr/share/miniupnpd/firewall.include
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
```