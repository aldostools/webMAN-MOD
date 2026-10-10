#ifdef COBRA_ONLY
	if(islike(param, "/fps.ps3"))
	{
		// /fps.ps3    toggle VshCounterFps plugin
		toggle_fps();
		strcpy(param, "/cpursx.ps3");
	}
#endif
