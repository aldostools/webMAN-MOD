# ps3netsrv-openwrt

OpenWrt ps3netsrv feed for use with the OpenWrt SDK.

You can find binaries for some platforms on "Releases" tab.

<b>After install to openwrt procedure</b>:
- After install, you need to setup it. The easiest way is through terminal, over ssh with these commands:
```
uci set ps3netsrv.main.enabled='1'
uci set ps3netsrv.main.dir='mnt/sda1'
uci set ps3netsrv.main.port='38008'
uci commit
```
Notes:
- "ps3netsrv.main.enabled='1'" means it will start on boot. If you don't want it, set the value to '0' (so you will need to start it manually through terminal);
- "ps3netsrv.main.dir" is used to set the root dir of your games directories (which contains "PS3ISO", "GAMES",...);
- "ps3netsrv.main.port" is used to set the port used by ps3netsrv.
