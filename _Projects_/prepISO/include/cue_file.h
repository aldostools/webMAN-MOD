#define NONE -1
#define MAX_TRACKS	98
#define MAX_LINE_LEN	640

#define FAILED -1

#define ISDIGIT(a)		('0' <= (unsigned char)(a) && (unsigned char)(a) <= '9')

static int64_t val(const char *c)
{
	int64_t previous_result = 0, result = 0;
	int64_t multiplier = 1;

	if(c && *c == '-')
	{
		multiplier = -1;
		c++;
	}

	while(*c)
	{
		if(!ISDIGIT(*c)) return result * multiplier;

		result *= 10;
		if(result < previous_result)
			return(0); // overflow
		else
			previous_result = result;

		result += (*c & 0x0F);
		if(result < previous_result)
			return(0); // overflow
		else
			previous_result = result;

		c++;
	}
	return(result * multiplier);
}

static void get_value(char *value, const char *url, u16 max_size)
{
	u16 n;
	for(n = 0; n < max_size; n++)
	{
		if(url[n] == '&' || url[n] == 0) break;
		value[n] = url[n];
	}
	value[n] = 0;
}

static uint32_t get_valuen32(const char *param, const char *label)
{
	char value[12], *pos = strstr(param, label);
	if(pos)
	{
		get_value(value, pos + strlen(label), 11);
		return (uint32_t)val(value);
	}
	return 0;
}

static int parse_lba(const char *templn, int use_pregap, int ret_value)
{
	char *time = strrchr(templn, ' '); if(!time) return ret_value;
	char tcode[10]; // mm:ss:ff

	int tcode_len = snprintf(tcode, 8, "%s", time + 1); tcode[8] = 0;
	if((tcode_len != 8) || tcode[2]!=':' || tcode[5]!=':') return ret_value;

	unsigned int tmin, tsec, tfrm;
	tmin = val(tcode);     // (tcode[0] & 0x0F)*10 + (tcode[1] & 0x0F);
	tsec = val(tcode + 3); // (tcode[3] & 0x0F)*10 + (tcode[4] & 0x0F);
	tfrm = val(tcode + 6); // (tcode[6] & 0x0F)*10 + (tcode[7] & 0x0F);

	return ((((tmin * 60) + tsec) * 75) + tfrm + use_pregap); // msf_to_lba
}

static int get_line(char *templn, const char *cue_buf, const int buf_size, const int start)
{
	*templn = 0;
	int lp = start;
	u8 line_found = 0;

	for(int l = 0; l < MAX_LINE_LEN; l++)
	{
		if(l>=buf_size) break;
		if(lp<buf_size && cue_buf[lp]>0 && cue_buf[lp]!='\n' && cue_buf[lp]!='\r')
		{
			templn[l] = cue_buf[lp++];
			templn[l + 1] = 0;
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

static int parse_cue(char *templn, const char *cue_buf, const unsigned int cue_size, TrackDef *tracks)
{
	unsigned int num_tracks = 0;

	tracks[0].lba = 0;
	tracks[0].is_audio = 0;

	if(cue_size > 16)
	{
		//int use_pregap = 0;
		int lba, lp = 0, prev = 0;

		while(lp < cue_size)
		{
			lp = get_line(templn, cue_buf, cue_size, lp);

			if(lp < 1) break;
			if(*templn == 0) continue;

			lba = NONE;

			//if(strstr(templn, "PREGAP")) {use_pregap = parse_lba(templn, 0, 2); continue;}
			if(strstr(templn, "INDEX 1=")) lba = get_valuen32(templn, "INDEX 1="); else // ccd frames
			if(strstr(templn, "INDEX 01") || strstr(templn, "INDEX 1 "))
			{
				lba = parse_lba(templn, /*num_tracks ? use_pregap :*/ 0, FAILED); // cue msf
				if(lba <= prev) lba += prev; // INDEX is a track length, instead of lba in msf
				prev = lba;
			}

			if(lba < 0) continue;

			tracks[num_tracks].lba = lba;
			if(num_tracks) tracks[num_tracks].is_audio = 1;

			num_tracks++; if(num_tracks >= MAX_TRACKS) break;
		}
	}

	if(!num_tracks) num_tracks++;

	return num_tracks;
}
