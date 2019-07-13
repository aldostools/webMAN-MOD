#ifdef COBRA_ONLY
static int parse_lba(const char *templn, bool use_pregap)
{
	char *time = strrchr(templn, ' '); if(!time) return FAILED;
	char tcode[10];

	int tcode_len = snprintf(tcode, 9, "%s", time + 1); tcode[8] = NULL;
	if((tcode_len != 8) || tcode[2]!=':' || tcode[5]!=':') return FAILED;

	unsigned int tmin, tsec, tfrm;
	tmin = (tcode[0] & 0x0F)*10 + (tcode[1] & 0x0F);
	tsec = (tcode[3] & 0x0F)*10 + (tcode[4] & 0x0F);
	tfrm = (tcode[6] & 0x0F)*10 + (tcode[7] & 0x0F);

	if(use_pregap) tsec += 2;

	return ((tmin * 60 + tsec) * 75 + tfrm);
}

static int get_line(char *templn, const char *cue_buf, const int buf_size, const int start)
{
	*templn = NULL;
	int lp = start;
	u8 line_found = 0;

	for(int l = 0; l < MAX_LINE_LEN; l++)
	{
		if(l>=buf_size) break;
		if(lp<buf_size && cue_buf[lp] && cue_buf[lp]!='\n' && cue_buf[lp]!='\r')
		{
			templn[l] = cue_buf[lp];
			templn[l + 1] = NULL;
		}
		else
		{
			templn[l] = NULL;
		}
		while(cue_buf[lp]=='\n' || cue_buf[lp]=='\r') {line_found = 1, lp++;}

		if(templn[l] == NULL) break; //EOF
	}

	if(!line_found) return NONE;

	return lp;
}

static unsigned int parse_cue(char *templn, const char *cue_buf, const int cue_size, TrackDef *tracks)
{
	unsigned int num_tracks = 0;

	tracks[0].lba = 0;
	tracks[0].is_audio = 0;

	if(cue_size > 16)
	{
		u8 use_pregap = 0;
		int lba, lp = 0;

		while(lp < cue_size)
		{
			lp = get_line(templn, cue_buf, cue_size, lp);
			if(lp < 1) break;

			if(strstr(templn, "PREGAP")) {use_pregap = 1; continue;}
			if(!strstr(templn, "INDEX 01") && !strstr(templn, "INDEX 1 ")) continue;

			lba = parse_lba(templn, use_pregap && num_tracks); if(lba < 0) continue;

			tracks[num_tracks].lba = lba;
			if(num_tracks) tracks[num_tracks].is_audio = 1;

			num_tracks++; if(num_tracks >= 32) break;
		}
	}

	if(!num_tracks) num_tracks++;

	return num_tracks;
}
#endif
