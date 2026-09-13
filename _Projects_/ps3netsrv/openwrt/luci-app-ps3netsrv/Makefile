#
# Copyright (C) 2026 JCorrea
#
# This is free software, licensed under the GNU General Public License v3.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI Support for ps3netsrv
LUCI_DEPENDS:=+ps3netsrv
LUCI_PKGARCH:=all

PKG_NAME:=luci-app-ps3netsrv
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=GPLv3
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
