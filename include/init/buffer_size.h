#define     KB		     1024UL
#define   _2KB_		     2048UL
#define   _4KB_		     4096UL
#define   _6KB_		     6144UL
#define   _8KB_		     8192UL
#define  _12KB_		    12288UL
#define  _16KB_		    16384UL
#define  _32KB_		    32768UL
#define  _48KB_		    49152UL
#define  _62KB_		    63488UL
#define  _64KB_		    65536UL
#define _128KB_		   131072UL
#define _192KB_		   196608UL
#define _256KB_		   262144UL
#define _384KB_		   393216UL
#define _512KB_		   524288UL
#define _640KB_		   655360UL
#define _768KB_		   786432UL
#define  _1MB_		0x0100000UL
#define  _2MB_		0x0200000UL
#define  _3MB_		0x0300000UL
#define _32MB_		0x2000000UL

#define _1GB_		0x40000000ULL
#define _48GB_		0xC00000000ULL

#define USE_2MB		98
#define USE_3MB		99
#define MIN_MEM		_192KB_

#define MAX_PAGES   ((BUFFER_SIZE_ALL / (_64KB_ * MAX_WWW_THREADS)) + 1)

#define SC_GET_FREE_MEM 	(352)

typedef struct {
	u32 total;
	u32 avail;
} _meminfo;

static _meminfo meminfo;

static void get_meminfo(void)
{
	{system_call_1(SC_GET_FREE_MEM, (u64)(u32) &meminfo);}
}

//////////////////////////////////////////////////////////

#define PS3 (1<<0)
#define PS2 (1<<1)
#define PS1 (1<<2)
#define PSP (1<<3)
#define BLU (1<<4)
#define DVD (1<<5)
#define ROM (1<<6)

static u32 BUFFER_SIZE_FTP;

static u32 BUFFER_SIZE;
static u32 BUFFER_SIZE_PS3;
static u32 BUFFER_SIZE_PSX;
static u32 BUFFER_SIZE_PSP;
static u32 BUFFER_SIZE_PS2;
static u32 BUFFER_SIZE_DVD;
static u32 BUFFER_SIZE_ALL;

#define BUFFER_SIZE_XML			4320

#ifdef MOUNT_ROMS
static const u32 BUFFER_SIZE_IGN = _4KB_;
#endif

static u32 get_buffer_size(u8 footprint)
{
	if(footprint == USE_2MB) return _2MB_;
	if(footprint == USE_3MB) return _3MB_;

	if(footprint == 1) //MIN
	{
		#ifndef LITE_EDITION
		return ( 320*KB);
		#else
		return ( _256KB_);
		#endif
	}
	else
	if(footprint == 2 || footprint >= 4) //MAX
	{
		return ( 1280*KB);
	}
	else
	if(footprint == 3) //MIN+
	{
		return (_512KB_);
	}
	else	//STANDARD
	{
		return ( 896*KB);
	}
}

static void set_buffer_sizes(u8 footprint)
{
	BUFFER_SIZE_ALL = get_buffer_size(footprint);
	BUFFER_SIZE_FTP = ( _256KB_);   // was 128 KB; 256 is the sweet spot, 512 plateaus at warm disk rate

	BUFFER_SIZE_PSP = ( _32KB_); //  50 games
	BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
	BUFFER_SIZE_DVD = ( _64KB_); // 100 items

	if(footprint >= USE_2MB)
	{
		BUFFER_SIZE_FTP = (footprint == 2) ? _256KB_ : _128KB_; // MAX = 256KB / default = 128KB

		u32 MEM = (footprint == USE_2MB) ? _128KB_ : 0; // 200 games
		//BUFFER_SIZE	= (896*KB to 1408*KB); // 1200-2200 games
		BUFFER_SIZE_PSX = (webman_config->foot == 5) ? _768KB_ : _512KB_ - MEM; // 800-1200 games (formerly 384KB: +128KB)
		BUFFER_SIZE_PSP = (webman_config->foot == 7) ? _768KB_ : _256KB_ - MEM; // 400-1200 games (formerly 128KB: +128KB)
		BUFFER_SIZE_PS2 = (webman_config->foot == 8) ? _768KB_ : _384KB_ - MEM; // 600-1200 games (formerly 256KB: +128KB)
		BUFFER_SIZE_DVD = (webman_config->foot == 6) ? _768KB_ : _512KB_ - MEM; // 800-1200 items (same as before)
	}
	else
	if(footprint == 1) //MIN (256 KB / 320 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= ( _128KB_);  // 200 games or 100 games on Lite edition
		BUFFER_SIZE_PSX = (  _32KB_);  //  50 items
		//BUFFER_SIZE_PSP = ( _32KB_); //  50 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		//BUFFER_SIZE_DVD = ( _64KB_); // 100 items
	}
	else
	if(footprint == 2) //MAX (1280 KB)
	{
		BUFFER_SIZE_FTP	= ( _256KB_);

		//BUFFER_SIZE	= ( _640KB_); //1000 games
		BUFFER_SIZE_PSX = ( _256KB_); // 400 games
		BUFFER_SIZE_PSP = (  _64KB_); // 100 games
		BUFFER_SIZE_PS2 = ( _128KB_); // 200 games
		BUFFER_SIZE_DVD = ( _192KB_); // 300 items
	}
	else
	if(footprint == 3) //MIN+ (512 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= ( 320*KB);   // 500 games
		BUFFER_SIZE_PSX = (  _32KB_);  //  50 games
		//BUFFER_SIZE_PSP = ( _32KB_); //  50 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		//BUFFER_SIZE_DVD = ( _64KB_); // 100 items
	}
	else
	if(footprint == 4) //MAX PS3+ (1280 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= ( 1088*KB);  //1700 games
		BUFFER_SIZE_PSX = (  _32KB_);  //  50 games
		//BUFFER_SIZE_PSP = ( _32KB_); //  50 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		//BUFFER_SIZE_DVD = ( _64KB_); // 100 items
	}
	else
	if(footprint == 5) //MAX PSX+ (1280 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= (  320*KB);  // 500 games
		BUFFER_SIZE_PSX = ( _768KB_);  //1200 games
		BUFFER_SIZE_PSP = (  _64KB_);  // 100 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		//BUFFER_SIZE_DVD = ( _64KB_); // 100 items
	}
	else
	if(footprint == 6) //MAX BLU+ (1280 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= (  320*KB);  // 500 games
		BUFFER_SIZE_PSX = (  _64KB_);  // 100 games
		BUFFER_SIZE_PSP = (  _64KB_);  // 100 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		BUFFER_SIZE_DVD = ( _768KB_);  //1200 items
	}
	else
	if(footprint == 7) //MAX PSP+ (1280 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= (  320*KB);  // 500 games
		BUFFER_SIZE_PSX = (  _64KB_);  // 100 games
		BUFFER_SIZE_PSP = ( _768KB_);  //1200 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		BUFFER_SIZE_DVD = (  _64KB_);  // 100 items
	}
	else
	if(footprint == 8) //MAX PS2+ (1280 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= (  352*KB);  // 550 games
		BUFFER_SIZE_PSX = (  _64KB_);  // 100 games
		//BUFFER_SIZE_PSP = ( _32KB_); //  50 games
		BUFFER_SIZE_PS2 = ( _768KB_);  //1200 games
		BUFFER_SIZE_DVD = (  _64KB_);  // 100 items
	}
	else	// if(footprint == 0) STANDARD (896 KB)
	{
		//BUFFER_SIZE_FTP = (_128KB_);

		//BUFFER_SIZE	= ( 448*KB);   // 700 games
		BUFFER_SIZE_PSX = ( 160*KB);   // 250 games
		//BUFFER_SIZE_PSP = ( _32KB_); //  50 games
		//BUFFER_SIZE_PS2 = ( _64KB_); // 100 games
		BUFFER_SIZE_DVD = ( _192KB_);  // 300 items
	}

	if((webman_config->cmask & PS1)) BUFFER_SIZE_PSX = (_4KB_);
	if((webman_config->cmask & PS2)) BUFFER_SIZE_PS2 = (_4KB_);
	if((webman_config->cmask & PSP)) BUFFER_SIZE_PSP = (_4KB_);
	if((webman_config->cmask & (BLU | DVD)) == (BLU | DVD)) BUFFER_SIZE_DVD = (_4KB_);

	#ifdef MOUNT_ROMS
	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD + BUFFER_SIZE_IGN);
	#else
	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD);
	#endif

	BUFFER_SIZE_PS3 = (BUFFER_SIZE - BUFFER_SIZE_XML -32);
}
