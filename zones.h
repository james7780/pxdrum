// xdrum screen zones

enum ZONES { ZONE_VOL = 1,
			ZONE_BPM = 2,
			ZONE_PITCH = 3,
			ZONE_SONGLIST = 4,
			ZONE_PATLIST = 5,
			ZONE_ADDTOSONGBTN = 6,
			ZONE_TRACKINFO = 7,
			ZONE_PATGRID = 8,
			ZONE_SONGNAME = 9,
			ZONE_KITNAME = 10,
			ZONE_OPTIONS = 11,
			ZONE_MODE = 12,
			ZONE_PATNAME = 13,
			ZONE_MAX = 14
		};

SDL_Rect zones[ZONE_MAX] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 32, 80 },			// vol
	{ 32, 0, 32, 80 },			// bpm
	{ 64, 0, 32, 80 },			// pitch
	{ 384, 0, 96, 80 },			// song list
	{ 272, 0, 96, 80 },			// pattern list
	{ 370, 32, 12, 16 },		// add-to-song button
	{ 0, 80, 96, 192 },			// track info
	{ 96, 80, 384, 192 },		// pattern grid
	{ 98, 0, 172, 16 },			// song name
	{ 98, 16, 172, 16 },		// drumkit name
	{ 98, 32, 172, 16 },		// options display (shuffle / volrand)
	{ 98, 48, 172, 16 },		// mode 
	{ 98, 64, 172, 16 },		// pattern name
};
	
// pattern grid box size
SDL_Rect PATBOX = { 0, 0, 24, 24 };

