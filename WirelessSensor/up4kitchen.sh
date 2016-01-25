#!/bin/sh

echo "Copy for the rom0.bin"
scp out/firmware/rom0.bin root@10.0.1.22:/usr/share/openhab/webapps/ota/
echo "Copy for the spiff_rom.bin"
scp out/firmware/spiff_rom.bin root@10.0.1.22:/usr/share/openhab/webapps/ota/kitchen/
