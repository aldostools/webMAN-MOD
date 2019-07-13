# webMAN MOD - Fork of original webMAN plugin by Deank for Playstation 3

__README IS STILL IN PROGRESS, IF YOU SEE ANY ERROR PLEASE TELL ME OR PUSH A COMMIT__

webMAN MOD is a homebrew plugin with many features added on the original webMAN/sMAN by Deank.

Our goal was to create an AIO plugin all that integrates all existing features available on the PS3 Scene
in an intuitive way, and webMAN MOD was born.

## Current functionality
### General
- Support on all custom firmwares with Cobra feature enabled (ver 4.46-4.84 CEX, DEX & DECR)
- Support on REBUG firmware with Cobra feature disabled (ver 4.84.2 CEX & DEX) 
- Support on REBUG firmware with Mamba loaded via boot_plugins_nocobra_kernel.txt (ver 4.84.2 CEX & DEX) 
- Support on PS3HEN on 4.82 OFW and 4.84 HFW CEX & DEX
- Support on classic custom firmware with Mamba loaded via IRISMAN (ver 3.41-4.84 CEX & DEX)
- Support on classic custom firmware with Mamba/PRXLoader (ver 3.41-4.84 CEX & DEX)
- Support on classic custom firmware with PRXLoader (ver 3.41-4.84 CEX & DEX)

- All PS3 Models (including all fat, Slim 20xx, 21xx, 25xx, 3xxx & SuperSlims 4xxx) are supported via PS3HEN payload
- All PS3 Models capable to downgrade to 3.56 or lower are supported via PS3Xploit Flash Writer (aka PS3Xploit 2.0)
  See *http://www.psdevwiki.com/ps3/SKU_Models* for compatibility with CFW

### webMAN vanilla features
- FTP server with remote control functions (shutdown/restart)
- WWW server with remote control functions (scroll down for the complete list of shortcuts)
- Support for loading and browsing of [local] PS3 games in ISO and folder format, DVD videos in ISO format, Blu-ray movies in ISO format, PS1/PS2/PSP games in ISO format with cover display
- NETISO support for network loading and browsing of PS3 games in ISO and folder format, DVD videos in ISO format, Blu-ray movies in ISO format, PS1 and PSP games
- NTFS support for PS3 and PS1 games in ISO format, Blu-ray movies in ISO format and DVD Video in ISO format
- Dynamic Fan Control and in-game temperature monitoring
- PAD shortcuts (*open include/combos.h for a complete list of shortcuts)
- Keep USB device awake
- Mount last game or AUTOBOOT.ISO to system startup
- Support direct access to NTFS devices through web & ftp (1.45 / 1.45.11)
- XMB integration XMB proxy (1.46 / 1.46.00)
- Integrated prepNTFS (1.47)

### webMAN MOD additional features
- Easy installer/updater
- Translated to 23 languages
- VSH Menu integration (hold SELECT to show the menu)
- sLaunch GUI integration (hold START or R2+L2 on XMB to show the GUI)
- New folder icons (by Brunolee & Berion)
- It can mount PS2 Classics games on PS2 Classic Launcher (.BIN.ENC)
- Automatic CONFIG creation for PS2ISOs and PS2 Classic using config database from ManaGunZ or the database of CONFIG created by the installer
- ROMS support through PKG/ROM Launcher and RetroArch
- It can auto-mount any custom folder, ISO or open an URL on startup. Official only can mount AUTOBOOT.ISO
- Title ID can be displayed on XMB menu
- Covers are shown using the Title ID on the file name of the ISO. Official needs to mount the game to show the covers.
- Option for online covers display (free service provided by DeViL303)
- It can rip a game from disc to hdd0 or copy from hdd0 to usb000 or from usb00x to hdd0.
- FTP server includes new SITE commands to allow copy/paste files locally, unmount game, toggle external gamedata, turn on/off dev_blind, change file attributes
- /dev_blind and /dev_hdd1 are automounted when accessed via FTP or URL
- Safe upload mode prevents a brick/semi-brick if power fails during ftp uploads to /dev_blind.
- Increased security: ADMIN mode blocks access to critical functions like /setup.ps3, /delete.ps3; Password for FTP server access; limit remote access to specific IP
- Integrated external gameDATA allows installation of packages & game data on external USB drives
- Web Debugger (remote peek/poke/find bytes, dump lv1 & lv2 memory)
- Support for automatic or manual removal of CFW syscalls and spoof console id (IDPS/PSID)
- All LV2 peek/pokes are done through syscalls 8/9 (CFW only) - syscalls 6/7 used only by PS3HEN
- Extended support up to 5 remote network servers
- Several shortcuts to toggle Cobra, swap Rebug files, mount net0/ or net1/, show IDPS/PSID, etc.
- Support for user defined combos (pad shortcuts)
- Enable screen capture on CFW that don't has the feature.
- Enable selection of emulator for PS1 and PS2 on B/C consoles
- Various improvements on File Manager (file & folder icons, links to navigate faster, mount ISO, mount net0/ or net1/, preview images, copy/paste/delete files & folders)
- MIN+ memory profile (same as MIN but uses 512K and 2.5X more buffer for PS3 games)
- MAX+ memory profile (same as MAX 1280K for PS3 games, others buffer is reduced, eg: 2X less buffer for ftp and 4X for DVD etc...)
- Copy operations use shadow copy on hdd0 for faster copy operations
- Scan for games on the stealth folder "/video"
- Support last_game.txt / autoboot on nonCobra edition
- "Offline" mode (blocks some PSN/tracking servers) and automatic restore when CFW syscalls are removed. Game updates still work in this mode. (v1.33.03)
- XMBM+ integration when grouping of XMB content is disabled (v1.33.03)
- Extended Content Profile Grouping (v1.33.07) - common files + individual content (4 profiles)
- PS3 Manager API Support (PS3MAPI) compatible with RTM tools
- Integrated Mysis video_rec plugin and get klicensee
- Support for *.ntfs[BDFILE]* (fake ISO created by IRISMAN or prepNTFS) - Used to play movies or install large packages on NTFS
- Support to mount NTFS games using raw_iso.sprx (rawseciso by Estwald) - Supports fake ISO
- Support for auto-fix games that require higher FW version (4.20 and later)
- Optional Video subfolder to "Bluray™ and DVD" folder (Display RetroXMB videos, videos on USB devices and Data Disc icon)
- Coverflow-like webGUI aka "slider" provides a mobile/desktop friendly GUI for fast game selection.
- 2 GUI Themes: sMAN-like graphical interface & webMAN original theme
- Extended system information (Title ID, game icon, APP Version, IDPS/PSID, CFW version, last played game)
- Display of Play time & startup time to SELECT+START and /cpursx.ps3 (Use SELECT+START+R2 to display Game ID, Title, play time and more in-Game info)
- Virtual pad allows send button events remotely via http://pad.aldostools.org on web browser or with webPAD software (Windows only)
- dev_bdvd/PS3_UPDATE now is redirected when the plugin is loaded (and when a game is mounted) [This is intended to prevent an accidental update if a game disc is inserted in the drive]
- Added /play.ps3 to launch XMB Functions (e.g: /play.ps3?col=network&seg=seg_premo) <- this will start Remote Play server from XMB.
- Once a game is mounted via html, if you click on the displayed icon the game will be launched on the PS3. This is nice to start the game once it's mounted from your mobile This option uses the new command /play.ps3
- Support for auto-play any supported ISO, game folder or auto-open an URL link
- Support for change BD/DVD region
- NETISO server on PS3 (ISO only) lets share games among PS3 consoles in a LAN
- Support for local web chat (source code only)
- Support edition of small text files (<2KB) via web
- Download files & install PKG remotely or with pad shortcuts
- Support batch script automation at startup (boot_init.txt or autoexec.bat) or played at any time (/play.ps3/<script-file>.bat)
- Use "home" path to define default path for /app_home/PS3_GAME on start up and for R2+START (e.g. make app_home start multiMAN or IRISMAN)

## How to build
Requirements for Windows:
- git, clone this repository with the following command: *git clone https://github.com/aldostools/webMAN-MOD.git*
- Official PS3 SDK v3.40 or 4.00 complete leaked version, google is your friend to find it
- GCC (for Windows [MinGW](http://sourceforge.net/projects/mingw) with mingw32-base will be fine) or [Cygwin (x86/x64)](https://cygwin.com/install.html)
- Open Source PSL1GHT SDK to compile prepNTFS and PKG Updater only

Requirements for GNU/Linux:
- An x86 linux distribution, Fedora 20 is tested working
- git, clone this repository with the following command: *git clone https://github.com/aldostools/webMAN-MOD.git*
- Official PS3 SDK v400.001 leaked version
- Official Cell OS Lv-2 leaked toolchain (a 4.1M patched GCC 4.1.1 version)
- wine for the missing linux tools
- A compiled Scetool binary, ps3 keys
- Open Source PSL1GHT SDK to compile prepNTFS and PKG Updater only

## Credits
- All the documentation on *http://www.psdevwiki.com*, and to all the devs who contributed
- Cobra team, for their work on Cobra payload and sharing the source code in public (thanks to the request of STLcardsWS)
- Deank as the creator of webMAN, sMAN, sLaunch, multiMAN / mmCM and many other contributions to the scene
- Estwald for NTFS library, rawseciso, fake iso, Mamba payload, etc.
- aldostools for all his works on this project!
- Zar & m@tsumot0 for starting the modding project
- NzV for PS3 Manager API (aka PS3MAPI), Mamba/PRX Loader & Mamba improvements
- bguerville for web downloader & package installer modules and port of ntfslib
- The team that ported NTFS library from PSL1ght to PS3 SDK (freddy38510, bguerville, Zar, deank, Joonie)
- Mysis, who wrote some useful libs and reverse engineering VSH Exports functions: http://www.ps3devwiki.com/ps3/VSH#Exports
- 3141card for VSH Menu POC & Littlebalup for his enhancement/new features
- jjolano (John Olano) for OpenFTP server
- OsirisX for PS3XPAD and the source code needed for gamepad emulation
- Berion & Brunolee for the graphics & icons
- PSX-SCENE, PSX-PLACE, PLAYSTATIONHAX, PS3HAX & other scene websites/users, who translated, helped in the testing process

Special thanks to Joonie, Habib & Rebug Team, flatz, haxxxen, devil303, Rancid-O, EvilNat, KW, naehrwert, MiralaTijera


## License
### webMAN MOD

webMAN MOD is a FREE software and all its components are distributed and protected under GNU General Public License version 3
(GPL v3) as published by the Free Software Foundation, or (at your option) any later version..

That means that any change made to the source code, binaries or resources of this software should be made public.
A detailed list of the changes and the credits to the original authors are required.

It is not permitted to distribute modified versions with the same name of this software.
A similar name or misleading name should NOT be used, to avoid confusion about the origin or the version in use.

For futher information about GPL v3, refer to: https://www.gnu.org/licenses/gpl-3.0.en.html

The software is distributed "as is". __No warranty of any kind is expressed or implied. You use at your own risk.__
Neither the author, the licensor nor the agents of the licensor will be liable for data loss, damages, loss of profits
or any other kind of loss while using or misusing this software or its components.

Furthermore, the author and his ASSOCIATES shall assume NO responsibility, legal or otherwise implied, for any misuse of,
or for any loss that may occur while using the SOFTWARE or its components.

Installing and using the software signifies acceptance of these terms and conditions of the license.
If you do not agree with the terms of this license, you must remove all software files from your storage devices
and cease to use the software.
