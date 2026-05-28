	if(!is_busy && (islike(param, "/index.ps3?") || islike(param, "/refresh.ps3"))) ; else

	if(!is_busy && sys_admin && (islike(param, "/mount.ps3?http")
 #ifdef DEBUG_MEM
					|| islike(param, "/peek.lv2?")
					|| islike(param, "/poke.lv2?")
					|| islike(param, "/find.lv2?")
					|| islike(param, "/peek.lv1?")
					|| islike(param, "/poke.lv1?")
					|| islike(param, "/find.lv1?")
					|| islike(param, "/dump.ps3")
					|| islike(param, "/hexview.ps3/")
 #endif

 #ifndef LITE_EDITION
					|| islike(param, "/delete.ps3")
					|| islike(param, "/delete_ps3")
 #endif

 #ifdef PS3MAPI
					|| islike(param, "/ps3mapi.ps3")
					|| islike(param, "/home.ps3mapi")
					|| islike(param, "/getmem.ps3mapi") || islike(param, "/patch.ps3")
					|| islike(param, "/setmem.ps3mapi")
					|| islike(param, "/payload.ps3mapi")
					|| islike(param, "/led.ps3mapi")
					|| islike(param, "/buzzer.ps3mapi") || islike(param, "/beep.ps3")
					|| islike(param, "/notify.ps3mapi")
					|| islike(param, "/mappath.ps3mapi")
					|| islike(param, "/syscall.ps3mapi")
					|| islike(param, "/syscall8.ps3mapi")
					|| islike(param, "/setidps.ps3mapi")
					|| islike(param, "/vshplugin.ps3mapi")
					|| islike(param, "/kernelplugin.ps3mapi")
					|| islike(param, "/gameplugin.ps3mapi")
 #endif

 #ifdef COPY_PS3
					|| islike(param, "/copy.ps3/")
 #endif
	)) keep_alive = 0;

	else if(islike(param, "/cpursx.ps3")
		||  islike(param, "/sman.ps3")
		||  islike(param, "/mount_ps3/")
		||  islike(param, "/mount.ps3/")
 #ifdef PS2_DISC
		||  islike(param, "/mount.ps2/")
		||  islike(param, "/mount_ps2/")
 #endif
 #ifdef VIDEO_REC
		||  islike(param, "/videorec.ps3")
 #endif
 #ifdef EXT_GDATA
		||  islike(param, "/extgd.ps3")
 #endif
 #ifdef NOBD_PATCH
		|| islike(param, "/nobd.ps3")
 #endif
 #ifdef SYS_BGM
		||  islike(param, "/sysbgm.ps3")
 #endif
 #ifdef LOAD_PRX
		||  islike(param, "/loadprx.ps3")
		||  islike(param, "/loadprx_ps3")
		||  islike(param, "/unloadprx.ps3")
 #endif
 #ifdef COBRA_ONLY
		||  islike(param, "/reloadprx.ps3")
 #endif
		||  islike(param, "/eject.ps3")
		||  islike(param, "/insert.ps3")) keep_alive = 0;
	else if(islike(param, "/index.ps3")) ;
	else
