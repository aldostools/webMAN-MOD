//#define MAX_TRACKS	98 <- defined in cobra.h

#ifdef COBRA_ONLY
static int parse_lba(const char *templn, int ret_value)
{
	char *time = strrchr(templn, ' '); if(!time) return ret_value;
	char tcode[10]; // mm:ss:ff

	int tcode_len = snprintf(tcode, 8, "%s", time + 1); tcode[8] = NULL;
	if((tcode_len != 8) || tcode[2]!=':' || tcode[5]!=':') return ret_value;

	unsigned int tmin, tsec, tfrm;
	tmin = val(tcode);     // (tcode[0] & 0x0F)*10 + (tcode[1] & 0x0F);
	tsec = val(tcode + 3); // (tcode[3] & 0x0F)*10 + (tcode[4] & 0x0F);
	tfrm = val(tcode + 6); // (tcode[6] & 0x0F)*10 + (tcode[7] & 0x0F);

	return ((((tmin * 60) + tsec) * 75) + tfrm); // msf_to_lba
}

static int get_line(char *templn, const char *cue_buf, const int buf_size, const int start)
{
	*templn = NULL;
	int lp = start;
	u8 line_found = 0;

	for(int l = 0; l < MAX_LINE_LEN; l++)
	{
		if(l>=buf_size) break;
		if(lp<buf_size && cue_buf[lp]>0 && cue_buf[lp]!='\n' && cue_buf[lp]!='\r')
		{
			templn[l] = cue_buf[lp++];
			templn[l + 1] = NULL;
		}
		else
		{
			while(cue_buf[lp]=='\n' || cue_buf[lp]=='\r') {line_found = 1; if(lp < buf_size) lp++; else break;}
			break; //EOF
		}
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
		int lba, lp = 0, off = 0, len;

		int pregap = 0;

		while(lp < cue_size)
		{
			lp = get_line(templn, cue_buf, cue_size, lp);

			if(lp < 1) break;
			if(*templn == NULL) continue;

			lba = NONE;

			if(strstr(templn, "INDEX 1=")) lba = get_valuen32(templn, "INDEX 1="); else // ccd frames
			if(strstr(templn, "INDEX 01") || strstr(templn, "INDEX 1 "))
			{
				len = lba = parse_lba(templn, FAILED); // cue msf
				if(lba < off || !num_tracks)
					lba = off + pregap; // INDEX 01 is a track length, instead of lba in msf
				else
					len = 0;			// INDEX 01 is absolute LBA in MSF
				off = lba + len;
			}

			if(lba < 0) continue;

			if(num_tracks)
			{
				tracks[num_tracks].lba = lba;
				tracks[num_tracks].is_audio = 1;
			}

			pregap = 150; // (2 * 75) -> 2 secs * 75 frames/sec

			num_tracks++; if(num_tracks >= MAX_TRACKS) break;
		}
	}

	if(!num_tracks) num_tracks++;

	return num_tracks;
}
#endif
