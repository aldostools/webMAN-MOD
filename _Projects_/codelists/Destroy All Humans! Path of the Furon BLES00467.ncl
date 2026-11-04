Unlock FPS
1
0 010b9e68 6000
#
Unlock FPS (PSXPlace) [Tested]
0
PSXPlace
0 010B9E68 60000000
0 010B9EC0 60000000
#
Button Bindings [Tested]
0
IDontModLobbies
/*
In /USRDIR/KronosGame/Config/PS3/Cooked/Coalesced.ini
you can bind commands to
buttons even though they
are labeled as Xbox 360
buttons. Some I have seen
are:

Bindings=(Name="XboxTypeS_Back",Command="god") 
Bindings=(Name="XboxTypeS_DPad_Up",Command="teleport")             
Bindings=(Name="XboxTypeS_DPad_Down",Command="stat FPS | killbots")        
Bindings=(Name="XboxTypeS_DPad_Left",Command="loaded | changesize 1")
Bindings=(Name="XboxTypeS_DPad_Right",Command="playersonly | changesize 2") 

You will need to edit
/USRDIR/KronosGame/PS3TOC.txt
to adjust things if the
byte size of the file has
changed, which can be
very tricky. You might
need the Gears Of War
Editor or some Unreal
Engine Tools to make it
easier. It is
specifically line 2 that
has it, so you would need
to adjust nearly the
entire file.
*/
#
