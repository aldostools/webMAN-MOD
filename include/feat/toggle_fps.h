#ifdef COBRA_ONLY
static void toggle_fps(void)
{
	#ifdef FPS_OVERLAY
	if(overlay_enabled)
		disable_progress();
	else
	{
		unload_vsh_plugin("VshFpsCounter"); // unload any loaded VshFpsCounter
		show_msg2("FPS", STR_ENABLED);
		if(!webman_config->nobeep) play_sound_id(5); // trophy sound
	}
	#else
	if(!webman_config->nobeep) BEEP1;
	#endif
	if(isCobraDebug)
		toggle_vsh_plugin("/dev_hdd0/tmp/wm_res/VshFpsCounterM.sprx");
	else
		toggle_vsh_plugin("/dev_hdd0/tmp/wm_res/VshFpsCounter.sprx");
}
#endif
