# Load webMAN MOD

This is a simple addon used to load webMAN MOD from its default path.

## Requisites

Requires webMAN MOD installed in the PS3 system.

If `/dev_hdd0/plugins/webftp_server.sprx` is not found it will beep 3 times.

Cobra/MAMBA/PS3HEN payload must be enabled and loaded.

By default the plugin will load in VSH slot 1.

If webMAN MOD did not load after return to XMB, run the addon again with the button combos.

## Combos

If you press and hold these buttons:
```
L1 = Load webMAN MOD plugin in slot 2
L2 = Load webMAN MOD plugin in slot 3
R1 = Load webMAN MOD plugin in slot 4
R2 = Load webMAN MOD plugin in slot 5

SQUARE = Create /dev_hdd0/boot_plugins.txt and reboots (if the plugin exists)
```

## Use Cases
- Load webMAN MOD on demand.
- Load webMAN MOD if boot_plugins.txt was removed or edited to don't load the plugin.
- Reload webMAN MOD after unload it.
- Re-Create /dev_hdd0/boot_plugins.txt if the file was deleted or damaged.
