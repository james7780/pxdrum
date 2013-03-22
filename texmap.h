// texture map for xdrum gfx

enum TEXMAP { TM_VOL_BOX = 1,
			TM_BPM_BOX = 2,
			TM_PITCH_BOX = 3,
			TM_SONGLIST_BOX = 4,
			TM_PATLIST_BOX = 5,
			TM_ADDTOSONGBTN = 6,
			TM_TRACKINFO_BOX = 7,
			TM_NONOTEBEAT = 8,
			TM_NONOTE = 9,
			TM_NOTEON = 10,
			TM_ACCENT = 11,
			TM_NOTECUT = 12,
			TM_VOLSLIDER = 13,
			TM_BPMSLIDER = 14,
			TM_TRACKVOLSLIDER = 15, 
			TM_MAX = 16
};

SDL_Rect texmap[TM_MAX] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 32, 80 },			// vol box
	{ 32, 0, 32, 80 },			// bpm box
	{ 64, 0, 32, 80 },			// pitch box
	{ 0, 160, 96, 80 },			// songlist
	{ 0, 160, 96, 80 },			// patlist
	{ 0, 96, 12, 16 },		// add to song button
	{ 0, 128, 96, 24 },			// trackinfo box
	{ 128, 128, 24, 24 },		// no note (on the beat)
	{ 152, 128, 24, 24 },		// no note
	{ 176, 128, 24, 24 },		// note on
	{ 200, 128, 24, 24 },		// accent
	{ 224, 128, 24, 24 },		// note cut
	{ 128, 0, 16, 56},			// vol slider graphic
	{ 160, 0, 16, 56},			// BPM slider graphic
	{ 72, 104, 11, 21}			// track mix vol slider graphic
};
