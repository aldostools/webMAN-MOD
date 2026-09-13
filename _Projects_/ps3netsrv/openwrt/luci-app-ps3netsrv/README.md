# luci-app-ps3netsrv

LuCI Support for `ps3netsrv`.

> [!IMPORTANT]
> This is a configuration interface (GUI) only. It **requires** the `ps3netsrv` binary to be installed on your system to function.

<img width="1011" height="564" alt="image" src="https://github.com/user-attachments/assets/1bd2f42e-63cc-476d-b6ae-e8590fefc1b3" />

This package provides a web interface for configuring `ps3netsrv` on OpenWrt. `ps3netsrv` allows you to stream games and ISOs over the network to your CFW PlayStation® 3 system.

## Features

- **Enable/Disable**: Toggle the `ps3netsrv` service.
- **Custom User**: Run the service as a specific system user (e.g., `root`).
- **Game Directory**: Configure the path to your PS3ISO, GAMES, and other game folders.
- **Port Selection**: Change the network port (default: `38008`).
- **IP Whitelist**: Restrict access to specific IP addresses or ranges (e.g., `192.168.1.*`).

## Usage

Once installed, the configuration interface can be found in the LuCI web interface under:
**Services** -> **PS3 Net Server**

## Credits

- Based on the original `ps3netsrv` implementation.
- ps3netsrv LuCI support developed by JCorrea.
