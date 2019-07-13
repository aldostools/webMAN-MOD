@ECHO OFF
title make_package_custom
if not exist custom md custom

:: ----------------------------------------------
:: Simple script to build a PKG (by CaptainCPS-X)
:: ----------------------------------------------

:: Change these for your application / manual...
set CID=CUSTOM-INSTALLER_00-0000000000000000
set PKG_DIR=./custom/
set PKG_NAME=boot_mamba.pkg

make_package_custom.exe --contentid %CID% %PKG_DIR% %PKG_NAME%

pause
