It is possible to use a 8/16/32/+++ USB memory stick with one game.
When you plug the USB - the game disc will appear and the game can be started.

It can be as simple as:
1) Write the ISO to the USB (no file systems - just raw sector copy)
2) Insert the USB stick in the PS3
3) Play the game

it works with a USB as block device :)

1) Get an ISO file
2) Use Win32DiskImager and write the ISO to the USB stick
3) Load the game with REAL-DISC.ntfs[PS3ISO] - http://deanbg.com/REAL-DISC.ntfs[PS3ISO] (this is set for the right usb port - 0x010300000000000A).

p.s. I edited the file for 58GB ISO size (0x07400000 sectors), so it will work for any game without modifications.


deank
