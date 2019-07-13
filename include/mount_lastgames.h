	// ----------
	// last game
	// ----------
	if(action)
	{
		// load last_games.bin
		_lastgames lastgames;
		if(read_file(LAST_GAMES_BIN, (char*)&lastgames, sizeof(_lastgames), 0) == 0) lastgames.last = 0xFF;

		// find game being mounted in last_games.bin
		bool _prev = false, _next = false;

		_next = IS(_path, "_next");
		_prev = IS(_path, "_prev");

		if(_next || _prev)
		{
			if(lastgames.last >= MAX_LAST_GAMES) lastgames.last = 0;

			if(_prev)
			{
				if(lastgames.last == 0) lastgames.last = LAST_GAMES_UPPER_BOUND; else lastgames.last--;
			}
			else
			if(_next)
			{
				if(lastgames.last >= LAST_GAMES_UPPER_BOUND) lastgames.last = 0; else lastgames.last++;
			}
			if(*lastgames.game[lastgames.last].path!='/') lastgames.last = 0;
			if(*lastgames.game[lastgames.last].path!='/' || strlen(lastgames.game[lastgames.last].path) < 7) goto exit_mount;

			sprintf(_path, "%s", lastgames.game[lastgames.last].path);
		}
		else
		if(lastgames.last >= MAX_LAST_GAMES)
		{
			lastgames.last = 0;
			snprintf(lastgames.game[lastgames.last].path, STD_PATH_LEN, "%s", _path);
		}
		else
		{
			u8 n;

			for(n = 0; n < MAX_LAST_GAMES; n++)
			{
				if(IS(lastgames.game[n].path, _path)) break;
			}

			if(n >= MAX_LAST_GAMES)
			{
				lastgames.last++;
				if(lastgames.last >= MAX_LAST_GAMES) lastgames.last = 0;
				snprintf(lastgames.game[lastgames.last].path, STD_PATH_LEN, "%s", _path);
			}
		}

		// save last_games.bin
		save_file(LAST_GAMES_BIN, (char *)&lastgames, sizeof(_lastgames));
	}

	// -----------------------
	// save last mounted game
	// -----------------------

	if(*_path != '/') goto exit_mount;
	else
	{
		save_file(LAST_GAME_TXT, _path, SAVE_ALL);
	}
