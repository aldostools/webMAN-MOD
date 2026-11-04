Unlock FPS (PSXPlace) [Tested]
0
Zolika1351/illusion
0 00F18840 00000000
#
Lot Of Money On Gain [Tested]
0
jgduff1
0 0018C5D8 92F904D0 
#
1P Invincible Mode [Tested]
0
0 0066C908 419EFF40
#
Infinite Ammo/No Reload [Tested]
0
/*Dynamic Memory Codes*/
/0 33D0777C 00110066
/
////////////////////////////
/*EBOOT Codes*/
/*ON*/
/0 0066BFD4 380B0000 
/addi       r0,r11,-0x1
/0 0066BFE4 39290000 
/addi       r9,r9,-0x1
/
/*OU*/
/
/0 0066BFDC 60000000 
/sth        r0,0x1C(r24)
/0 0066BFF4 60000000 
/sth        r9,0x1E(r24)
/
/*OFF*/
/0 0066BFD4 380BFFFF 
/0 0066BFE4 3929FFFF 
/0 0066BFDC B018001C 
/0 0066BFF4 B138001E 
#
Star Cop Never Increase [Tested]
0
/*Dynamic Memory Codes*/
/0 338928A8 00000000
/0 32F23268 00000000
////////////////////////////
/*EBOOT Codes*/
/*Desactivate This Codes*/
/*If The Mission Need*/
/*Cop Pursuit*/
/*ON*/
/**GTA IV Original**/
/0 0022B980 60000000 
/stw        r11,0x68(r31)
/
/**Ballad Of Gay Tony**/
/**The Lost & Damned**/
/0 0022B824 60000000 
/0 0022B864 60000000 
/0 0022B8F4 60000000 
/0 0022B954 60000000 
/0 0022BA14 60000000 
/stw        r11,0x68(r31)
/
/*OFF*/
/0 0022B980 917F0068
/0 0022B824 917F0068 
/0 0022B864 917F0068 
/0 0022B8F4 917F0068 
/0 0022B954 917F0068 
/0 0022BA14 917F0068
#
 [Tested]





1P Invincible Mode
0
xtatu
0 0066C908 419EFF40
#
AoB 1P Invincible Mode [Tested]
0
xtatu
B 00010000 04000000
B 409EFF404089034C812100A43B60000039E00000827E8078 419EFF404089034C812100A43B60000039E00000827E8078
#
