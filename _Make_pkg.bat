@echo off
title Building webMAN_MOD_1.47.xx_Updater.pkg ...

copy /Y webftp_server_full.sprx                _Projects_\updater\update\dev_hdd0\plugins\webftp_server.sprx
copy /Y webftp_server_noncobra.sprx            _Projects_\updater\update\dev_hdd0\plugins\webftp_server_noncobra.sprx

move /Y webftp_server_lite.sprx                _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_full.sprx                _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server.sprx                     _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_english.sprx             _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_ps3mapi.sprx             _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_noncobra.sprx            _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_ccapi.sprx               _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_rebug_cobra_english.sprx _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_rebug_cobra_ps3mapi.sprx _Projects_\updater\pkgfiles\USRDIR
move /Y webftp_server_rebug_cobra_multi23.sprx _Projects_\updater\pkgfiles\USRDIR

cls
cd _Projects_\updater
call Make_PKG.bat
title Building webMAN_MOD_latest.pkg ...
call make_update.bat

set ver=1.47
set rev=25

move webMAN_MOD_%ver%.xx_Updater.pkg                      ..\..\webMAN_MOD_%ver%.%rev%_Installer.pkg
move webMAN_MOD_%ver%.xx_Updater_rebugification_theme.pkg ..\..\webMAN_MOD_%ver%.%rev%_Installer_rebugification_theme.pkg
move webMAN_MOD_%ver%.xx_Updater_metalification_theme.pkg ..\..\webMAN_MOD_%ver%.%rev%_Installer_metalification_theme.pkg

move webMAN_MOD_latest.pkg ..\..
