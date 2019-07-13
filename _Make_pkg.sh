#!/bin/sh
echo "Building webMAN_MOD_1.47.xx_Updater.pkg ..."

mv webftp_server_lite.sprx                _Projects_/updater/pkgfiles/USRDIR/webftp_server_lite.sprx
mv webftp_server_full.sprx                _Projects_/updater/pkgfiles/USRDIR/webftp_server_full.sprx
mv webftp_server.sprx                     _Projects_/updater/pkgfiles/USRDIR/webftp_server.sprx
mv webftp_server_english.sprx             _Projects_/updater/pkgfiles/USRDIR/webftp_server_english.sprx
mv webftp_server_ps3mapi.sprx             _Projects_/updater/pkgfiles/USRDIR/webftp_server_ps3mapi.sprx
mv webftp_server_noncobra.sprx            _Projects_/updater/pkgfiles/USRDIR/webftp_server_noncobra.sprx
mv webftp_server_ccapi.sprx               _Projects_/updater/pkgfiles/USRDIR/webftp_server_ccapi.sprx
mv webftp_server_rebug_cobra_english.sprx _Projects_/updater/pkgfiles/USRDIR/webftp_server_rebug_cobra_english.sprx
mv webftp_server_rebug_cobra_ps3mapi.sprx _Projects_/updater/pkgfiles/USRDIR/webftp_server_rebug_cobra_ps3mapi.sprx
mv webftp_server_rebug_cobra_multi23.sprx _Projects_/updater/pkgfiles/USRDIR/webftp_server_rebug_cobra_multi23.sprx

clear
cd _Projects_/updater
sh Make_PKG.sh 2>&1

mv webMAN_MOD_1.47.xx_Updater.pkg ../../webMAN_MOD_1.47.xx_Updater.pkg
