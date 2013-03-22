// PXDrum Portable Drum Machine
// Copyright James Higgs 2008/2009
//
#include <stdlib.h>
#include "SDL.h"
#include "SDL_main.h"
#include "SDL_mixer.h"
#include "SDL_thread.h"
#include "platform.h"
#include "pattern.h"
#include "drumkit.h"
#include "song.h"
#include "zones.h"
#include "texmap.h"
#include "fontengine.h"
#include "gui.h"
#include "transport.h"
#include "joymap.h"
#include "writewav.h"

#define XDRUM_VER	"1.2"

// xdrum "screen modes"
enum XDRUM_MODE { XM_MAIN = 0, XM_FILE = 1, XM_KIT = 2 };

// SDL buffers
SDL_Surface *screen;
SDL_Surface *backImg;
SDL_Surface *textures;
SDL_Surface *cursorImg;

// colours for use by gui
Uint32 g_bgColour = 0xFFFFFFFF;				// init to sane value
Uint32 g_borderColour = 0xFFFFFFFF;
Uint32 g_separatorColour = 0xFFFFFFFF;
Uint32 g_highlightColour = 0xFFFFFFFF;

DrumKit drumKit;
Song song;

// NB! currentPatternIndex and currentPattern MUST be kept in sync!
int currentPatternIndex = 0;
DrumPattern* currentPattern = NULL;			// currently playing pattern

int livePatternIndex = 0;

DrumPattern patternClipboard;
DrumEvent trackClipboard[STEPS_PER_PATTERN];

int songListScrollPos = 0;
int patListScrollPos = 0;
int currentTrack = 0;						// pattern cursor y
int currentStep = 0;						// pattern cursor x

// Mouse cursor
float cursorX = (VIEW_WIDTH / 2);
float cursorY = (VIEW_HEIGHT / 2);
// Do we want to lock mouse to pattern grid cursor (PSP)?
#define LOCK_MOUSE_TO_GRID_CURSOR

// PLayback control
Transport transport;

// For rendering text
FontEngine* bigFont = NULL;
FontEngine* smallFont = NULL;

// Joystick / controller button mapping
JoyMap joyMap;

// WAV output
short outputSample;
WavWriter wavWriter;

// quit/exit flag
bool quit = false;

void printJoystickInfo(SDL_Joystick *joystick)
{
	int index;

	index = SDL_JoystickIndex(joystick);

	printf( "JOYSTICK INFO\n\n"
		"Index: %d\n"
		"Name: %s\n"
		"Num axes: %d\n"
		"Num balls: %d\n"
		"Num hats: %d\n"
		"Num buttons: %d\n",
		index,
		SDL_JoystickName(index),
		SDL_JoystickNumAxes(joystick),
		SDL_JoystickNumBalls(joystick),
		SDL_JoystickNumHats(joystick),
		SDL_JoystickNumButtons(joystick)
	);
}

/// Utility func - setup and SDL_Rect
void SetSDLRect(SDL_Rect& rect, int x, int y,int w, int h)
{
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
}

/// Progress callback function
void progress_callback(int progress)
{
	ShowProgress(screen, bigFont, "Loading", progress);
	SDL_Flip(screen);
}

/// Load a bitmap image and convert it to display format
SDL_Surface* LoadImageConvertToDisplay(const char* filename, bool setColourKey)
{
	// Load the bitmap
	SDL_Surface* img = SDL_LoadBMP(filename);
	if (!img)
		{
		printf("Error loading image %s!\n", filename);
		return NULL;
		}

	// set colour key if neccessary
	if (setColourKey)
		{
		Uint32 colourKey = SDL_MapRGB(img->format, 0, 0, 0);		
		if (-1 == SDL_SetColorKey(img, SDL_SRCCOLORKEY, colourKey))
			{
			printf("Error setting cursor colour key!\n");
			SDL_FreeSurface(img);
			return NULL;
			}
		}

	// Convert bitmap to display format
	// NB: For PSP, colour key will not work if bitmap is in display format
	// (ie: We need to "force" software colourkey for the cursor bitmap)
	SDL_Surface* displayImg = img;
	if (!setColourKey)
		{
		displayImg = SDL_DisplayFormat(img);
		SDL_FreeSurface(img);
		}

	return displayImg;	
	
}

/// Load the required graphics "textures"
int LoadImages()
{
	backImg = LoadImageConvertToDisplay("gfx/bg.bmp", false);
	if (!backImg)
		return 1;

	textures = LoadImageConvertToDisplay("gfx/textures.bmp", false);
	if (!textures)
		return 1;

	// background, border, separator, highlight colours for gui come from textures.bmp
	SDL_LockSurface(textures);
	g_bgColour = *((Uint32*)textures->pixels + 128 + (188*256));
	printf("Background colour = %08X\n", g_bgColour);
	g_borderColour = *((Uint32*)textures->pixels + 128 + (196*256));
	printf("Border colour = %08X\n", g_borderColour);
	g_separatorColour = *((Uint32*)textures->pixels + 128 + (204*256));
	printf("Separator colour = %08X\n", g_separatorColour);
	g_highlightColour = *((Uint32*)textures->pixels + 128 + (212*256));
	printf("HIghlight colour = %08X\n", g_highlightColour);
	SDL_UnlockSurface(textures);
		
	cursorImg = LoadImageConvertToDisplay("gfx/cursor.bmp", true);
	if (!cursorImg)
		return 1;
		
	return 0;
}

/// Set main output volume
/// @param volPercent	Volume as percent of max (0 to 100)
void SetMainVolume(int volPercent)
{
	// clamp value to valid range
	if (volPercent < 0)
		volPercent = 0;
	else if (volPercent > 100)
		volPercent = 100;
		
	// set song vol as 0 to 255 range
	song.vol = (volPercent * 255) / 100;
}

/// Get main output volume
/// @return 	Volume as percent of max (0 to 100)
int GetMainVolume()
{
	return ((song.vol * 100) / 255);
}

/// Syncronise pattern pointer with pattern index
void SyncPatternPointer()
{
	if (NO_PATTERN_INDEX == currentPatternIndex)
		currentPattern = NULL;
	else
		currentPattern = &song.patterns[currentPatternIndex];
}

// draw vol/bpm/pitch sliders
void DrawSliders(SDL_Surface* surface)
{
	char s[32];
	// draw vol slider box
	SDL_Rect dest = zones[ZONE_VOL];
	SDL_Rect src = texmap[TM_VOL_BOX];
	SDL_BlitSurface(textures, &src, surface, &dest);
	sprintf(s, "%d", (song.vol * 100) / 255);
	dest.x += (16 - (int)strlen(s) * 8 / 2);
	dest.y += 2;
	smallFont->DrawText(surface, s, dest, true);
	// draw part of vol slider graphic corresponding to the vol level 
	src = texmap[TM_VOLSLIDER];
	int h = ((song.vol * 56) / 255);
	src.y += (src.h - h);
	src.h = h;
	dest.x = 8;
	dest.y = 69 - h;
	dest.w = src.w;
	dest.h = src.h;
	SDL_BlitSurface(textures, &src, surface, &dest);
	
	//draw BPM slider box
	dest = zones[ZONE_BPM];
	src = texmap[TM_BPM_BOX];
	SDL_BlitSurface(textures, &src, surface, &dest);
	sprintf(s, "%d", song.BPM);
	dest.x += (16 - (int)strlen(s) * 8 / 2);
	dest.y += 2;
	smallFont->DrawText(surface, s, dest, true);
	// draw part of BPM slider graphic corresponding to the BPM 
	src = texmap[TM_BPMSLIDER];
	h = ((song.BPM - 50) / 2);
	if (h < 0) h = 0;
	if (h > src.h) h = src.h;
	src.y += (src.h - h);
	src.h = h;
	dest.x = 40;
	dest.y = 69 - h;
	dest.w = src.w;
	dest.h = src.h;
	SDL_BlitSurface(textures, &src, surface, &dest);
	
	// draw pitch slider
	dest = zones[ZONE_PITCH];
	src = texmap[TM_PITCH_BOX];
	SDL_BlitSurface(textures, &src, surface, &dest);
/*
	SDL_Rect r1;
	r1.x = 74;
	r1.w = 12;
	if (song.pitch > 0)
		{
		r1.h = song.pitch * 2;
		r1.y = 40 - r1.h;
		}
	else
		{
		r1.h = abs(song.pitch * 2);
		r1.y = 40;
		}
	SDL_FillRect(surface, &r1, SDL_MapRGB(screen->format, 255, 128, 0));
	if (song.pitch > 0)
		sprintf(s, "+%d", song.pitch);
	else
		sprintf(s, "%d", song.pitch);
	dest.x += (16 - (int)strlen(s) * 8 / 2);
	dest.y += 2;
	smallFont->DrawText(surface, s, dest, true);
*/	
}

// draw track info
void DrawTrackInfo(SDL_Surface* surface)
{
	char trackName[DRUM_NAME_LEN];
		
	SDL_Rect src;
	SDL_Rect dest;
	for (int i = 0; i < NUM_TRACKS; i++)
		{
		src = texmap[TM_TRACKINFO_BOX];
		SetSDLRect(dest, 0, 80 + i*24, 96, 24);
		SDL_BlitSurface(textures, &src, surface, &dest);
		
		// Draw track name (instr name)
		dest.x += 2;
		dest.y += 2;
		dest.w = 70;
		if (0 == drumKit.drums[i].name[0])
			//sprintf(trackName, "Track %d", i+1);
			strcpy(trackName, "---");
		else
			strcpy(trackName, drumKit.drums[i].name);
		bigFont->DrawText(surface, trackName, dest, true);

		// draw part of track mix vol graphic corresponding to the track mix level 
		src = texmap[TM_TRACKVOLSLIDER];
		int h = (song.trackMixInfo[i].vol * 20) / 255;
		src.y += (src.h - h);
		src.h = h + 1;
		dest.x = 72;
		dest.y = 80 + i*24 + 22 - h;
		dest.w = src.w;
		dest.h = src.h;
		SDL_BlitSurface(textures, &src, surface, &dest);
		
		// Draw mute/solo state
		SetSDLRect(dest, 83,  80 + i*24, 12, 12);
		if (TrackMixInfo::TS_MUTE == song.trackMixInfo[i].state)
			{
			SetSDLRect(src, 100, 128, 12, 12);
			SDL_BlitSurface(textures, &src, surface, &dest);
			}
		else if (TrackMixInfo::TS_SOLO == song.trackMixInfo[i].state)
			{
			dest.y = 80 + i*24 + 12;
			SetSDLRect(src, 100, 140, 12, 12);
			SDL_BlitSurface(textures, &src, surface, &dest);
			}
		}
}

// draw song sequence list
void DrawSequenceList(SDL_Surface* surface)
{
	char s[40];
	char* patname;
	char no_pat_name[32] = "---";
	SDL_Rect src = texmap[TM_SONGLIST_BOX];
	SDL_Rect dest = zones[ZONE_SONGLIST];
	SDL_BlitSurface(textures, &src, surface, &dest);
	dest.x += 2;
	dest.y += 1;
	smallFont->DrawText(surface, "Sequence", dest, false);
	dest.y += 10;
	
	// songlist must "follow' song playback
	if (song.songPos > songListScrollPos + 6)
		songListScrollPos = song.songPos - 6;
	else if (song.songPos < songListScrollPos)
		songListScrollPos = song.songPos;

	// draw visible part of songlist		
	for (int i = songListScrollPos; i < PATTERNS_PER_SONG; i++)
		{
		// Draw song pattern index
		int patIndex = song.songList[i];
		if (NO_PATTERN_INDEX == patIndex)
			patname = no_pat_name;
		else
			patname = song.patterns[patIndex].name;
		if (i == song.songPos)
			sprintf(s, "%2d*%s", i+1, patname);
		else
			sprintf(s, "%2d %s", i+1, patname);
		smallFont->DrawText(surface, s, dest, false);
		dest.y += 8;
		// kick out if writing over bottom
		if (dest.y >= zones[ZONE_SONGLIST].h - 10 - 8)
			i = PATTERNS_PER_SONG;
		}
}

// draw pattern list and "add pattern to song" button
void DrawPatternList(SDL_Surface* surface)
{
	char s[40];
	SDL_Rect src = texmap[TM_PATLIST_BOX];
	SDL_Rect dest = zones[ZONE_PATLIST];
	SDL_BlitSurface(textures, &src, surface, &dest);
	dest.x += 2;
	dest.y += 1;
	smallFont->DrawText(surface, "Patterns", dest, false);
	dest.y += 10;

	// patlist must "follow' current pattern number
	if (NO_PATTERN_INDEX != currentPatternIndex)
		{
		if (currentPatternIndex > patListScrollPos + 6)
			patListScrollPos = currentPatternIndex - 6;
		else if (currentPatternIndex < patListScrollPos)
			patListScrollPos = currentPatternIndex;
		}

	for (int i = patListScrollPos; i < MAX_PATTERN; i++)
		{
		// draw pattern number/name
		if (i == currentPatternIndex)
			sprintf(s, "%2d#%s", i+1, song.patterns[i].name);
		else
			sprintf(s, "%2d %s", i+1, song.patterns[i].name);
		smallFont->DrawText(surface, s, dest, false);
		dest.y += 8;
		// kick out if writing over bottom
		if (dest.y >= zones[ZONE_PATLIST].h - 10 - 8)
			i = MAX_PATTERN;
		}
	
	// draw "add pattern to song" button	
	src = texmap[TM_ADDTOSONGBTN];
	dest = zones[ZONE_ADDTOSONGBTN];
	SDL_BlitSurface(textures, &src, surface, &dest);
}


// draw drum pattern
void DrawPatternGrid(SDL_Surface* surface, DrumPattern* pattern)
{
	if (!pattern)
		return;
		
	SDL_Rect src = texmap[TM_NONOTE];
	SDL_Rect src2 = texmap[TM_NONOTEBEAT];
	SDL_Rect dest;
	const int w = PATBOX.w;
	const int h = PATBOX.h;
	if (NULL == pattern)
		{
		// Draw empty grid
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{ 
				SetSDLRect(dest, 96 + j * w, 80 + i * h, w, h);
				SDL_BlitSurface(textures, &src, surface, &dest);
				}
			}
		}
	else
		{
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				unsigned char vol = pattern->events[i][j].vol;
				if (0 == vol)
					{
					if (0 == (j & 0x3))
						src.x = texmap[TM_NONOTEBEAT].x;
					else
						src.x = texmap[TM_NONOTE].x;
					}
				else if (1 == vol)		// mute drum (end note)
					src.x = texmap[TM_NOTECUT].x;
				else if (vol > 100)		// accent note
					src.x = texmap[TM_ACCENT].x;
				else					// trigger normal drum note
					src.x = texmap[TM_NOTEON].x;
				SetSDLRect(dest, 96 + j * w, 80 + i * h, w, h);
				SDL_BlitSurface(textures, &src, surface, &dest);
				}
			}
		}

	// Draw pattern name
	char s[200];
	strcpy(s, "Pattern: ");
	strcat(s, pattern->name);
	dest = 	zones[ZONE_PATNAME];
	SDL_FillRect(surface, &dest, g_bgColour);
	if (NULL != pattern)
		bigFont->DrawText(surface, s, dest, false);

}

/// Draw general info
/// - song name
/// - drumkit name
/// - playback mode (song / pattern)
void DrawGeneralInfo(SDL_Surface* surface)
{
	char s[200];
	// draw song name
	SDL_Rect dest = zones[ZONE_SONGNAME];
	strcpy(s, "Song: ");
	strcat(s, song.name);
	bigFont->DrawText(surface, s, dest, false);
	// draw drumkit name
	dest = zones[ZONE_KITNAME];
	//sprintf(s, "Kit: %s", drumKit.name);
	strcpy(s, "Kit: ");
	strcat(s, drumKit.name);
	bigFont->DrawText(surface, s, dest, false);

	// draw shuffle / volrand options values
	dest = zones[ZONE_OPTIONS];
	sprintf(s, "Shuffle %d  VolRand %d", transport.shuffle, transport.volrand);
	//smallFont->DrawText(surface, s, dest, false);
	bigFont->DrawText(surface, s, dest, false);

	// play mode display
	dest = zones[ZONE_MODE];
	//SDL_FillRect(surface, &dest, g_bgColour);
	if (Transport::PM_PATTERN == transport.mode)
		bigFont->DrawText(surface, "Mode: Pattern", dest, false);
	else if (Transport::PM_SONG == transport.mode)
		bigFont->DrawText(surface, "Mode: Song", dest, false);
	else
		bigFont->DrawText(surface, "Mode: Live", dest, false);
}
	
// draw cursor
void DrawCursor(int x, int y)
{
	SDL_Rect src;
	SDL_Rect dest;

	if (wavWriter.IsWriting())
		{
		SetSDLRect(src, 64, 0, 32, 24);
		SetSDLRect(dest, x, y, 32, 24);
		}
	else
		{
		SetSDLRect(src, 0, 0, 16, 16);
		SetSDLRect(dest, x, y, 16, 16);
		}

	SDL_BlitSurface(cursorImg, &src, screen, &dest);
}

/// Redraw everything
void DrawAll()
{
	// clear background
	SDL_FillRect(backImg, NULL, g_bgColour); //SDL_MapRGB(screen->format, 0, 0, 0));
	
	// draw various elements
	DrawSliders(backImg);
	DrawGeneralInfo(backImg);
	DrawSequenceList(backImg);
	DrawPatternList(backImg);
	DrawTrackInfo(backImg);
	//DrawPatternGrid(backImg, currentPattern);
	// Draw the pattern at index currentPatternIndex (for live mode)
	if (NO_PATTERN_INDEX == currentPatternIndex)
		DrawPatternGrid(backImg, NULL);
	else
		DrawPatternGrid(backImg, &song.patterns[currentPatternIndex]);
	
}

// blit background buffer to screen
void BlitBG()
{
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = VIEW_WIDTH;
	src.h = VIEW_HEIGHT;
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = VIEW_WIDTH;
	dest.h = VIEW_HEIGHT;
	SDL_BlitSurface(backImg, &src, screen, &dest);
}

/// Get the index of the solo'ed track
/// @return		Index number of track that is solo, else -1
int GetSoloTrack()
{
	for (int i = 0; i < NUM_TRACKS; i++)
		{
		if (TrackMixInfo::TS_SOLO == song.trackMixInfo[i].state)
			return i;
		}
		
	return -1;
}

/// mute/unmute a track if possible (ie: if no solo active)
void SetTrackMute(int track)
{
	printf("SetTrackMute(%d)\n", track);
	if (-1 == GetSoloTrack())
		{
		// Set / reset MUTE state
		if (TrackMixInfo::TS_MUTE == song.trackMixInfo[track].state)
			song.trackMixInfo[track].state = TrackMixInfo::TS_ON;
		else
			song.trackMixInfo[track].state = TrackMixInfo::TS_MUTE;
		}
}

/// solo / unsolo a track
/// @param track		Track index to solo (or -1 to remove solo)
/// Set solo state to the specified track
/// (-1 to switch off solo)
void SetTrackSolo(int track)
{
	// If track is currenty solo'ed, then we want to switch off solo
	if (track >= 0 && track < NUM_TRACKS)
		{
		if (TrackMixInfo::TS_SOLO == song.trackMixInfo[track].state)
			track = -1;				// so that we do not solo this track below
		}
		
	// Remove any current solo (restore previous track state)
	for (int i = 0; i < NUM_TRACKS; i++)
		song.trackMixInfo[i].state = song.trackMixInfo[i].prevState;



	// Apply new solo if neccessary
	if (track >= 0 && track < NUM_TRACKS)
		{
		// Backup track mix state, and mute / solo tracks
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			song.trackMixInfo[i].prevState = song.trackMixInfo[i].state;
			song.trackMixInfo[i].state = TrackMixInfo::TS_MUTE;
			}
		song.trackMixInfo[track].state = TrackMixInfo::TS_SOLO;
		}
		
}

/// Prompt user to load a drumkit
/// @return			true if drumkit selected and loaded OK
bool PromptLoadDrumkit()
{
	bool loaded = false;
	char kitname[DRUMKIT_NAME_LEN];
	strcpy(kitname, drumKit.name);
	if (DoFileSelect(screen, bigFont, "Select DrumKit to load", "kits", kitname))
		{
		// NB: WE must stop playback while loading/unloading samples (chunks)
		bool wasPlaying = transport.playing;
		transport.playing = false;
		Mix_HaltChannel(-1);			// stop all channels playing
		loaded = drumKit.Load(kitname, progress_callback);
		if (!loaded)
			DoMessage(screen, bigFont, "Error", "Error loading drumkit!", false); 
		transport.playing = wasPlaying;
		}

	return loaded;
}

/// Display the options menu and process the result 
int DoOptionsMenu()
{
	int selectedShuffleOption = transport.shuffle;
	int selectedJitterOption = transport.jitter / 5;
	int selectedVolrandOption = transport.volrand / 10;
	int selectedFlashOption = transport.flashOnBeat ? 1 : 0; 

	Menu menu;
	menu.AddItem(1, "Shuffle", "0|1|2|3", selectedShuffleOption, "Set shuffle amount (2 = standard shuffle)");	
	menu.AddItem(2, "Jitter (ms)", "0|5|10|15", selectedJitterOption, "!!! NOT IMPLEMENTED !!!"); // Randomise playback timing");	
	menu.AddItem(3, "VolRand (%)", "0|10|20|30|40|50", selectedVolrandOption, "Randomise hit volume");	
	menu.AddItem(4, "Flash BG on Beat", "No|Yes", selectedFlashOption, "Flash pattern grid background on the beat");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, "Options Menu", 0);
	
	// Update shuffle and jitter from menu item data
	if (selectedId > -1)
		{
		transport.shuffle = menu.GetItemSelectedOption(1);
		transport.jitter = menu.GetItemSelectedOption(2) * 5;
		transport.volrand = menu.GetItemSelectedOption(3) * 10;
		transport.flashOnBeat = (0 == menu.GetItemSelectedOption(4)) ? false : true;
		}
	
	DrawAll();
	
	return selectedId;
}

/// Display the Volume / BPM menu and process the result 
int DoVolBPMMenu(int initialSelection)
{
	int selectedVolOption = GetMainVolume() / 10;		// main vol 0 to 100
	int selectedBPMOption = (song.BPM / 10) - 6;		// starts at 60

	Menu menu;
	menu.AddItem(1, "Main Vol (%)", "Mute|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set main volume");	
	menu.AddItem(2, "BPM", "60|70|80|90|100|110|120|130|140|150|160|170|180", selectedBPMOption, "Set playback speed in BPM");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, "Volume/BPM Menu", initialSelection);
	
	// Update shuffle and jitter from menu item data
	if (selectedId > -1)
		{
		SetMainVolume(menu.GetItemSelectedOption(1) * 10);
		song.BPM = (menu.GetItemSelectedOption(2) * 10) + 60;
		}
	
	DrawAll();
	
	return selectedId;
}


/// Display the song context menu and process the result 
int DoFileMenu(int initialSelection)
{
	Menu menu;
	menu.AddItem(1, "Load song", "Load a song from file");	
	menu.AddItem(2, "Save song", "Save this song to a file");	
	menu.AddItem(3, "Load DrumKit", "Load a different drumkit");
	menu.AddItem(4, "Record to WAV", "Record next play to WAV file");

	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, "File Menu", initialSelection);
	// process result
	switch (selectedId)
		{
		case 1 :		// LOAD
			{
			char songname[SONG_NAME_LEN];
			strcpy(songname, song.name);
			if (DoFileSelect(screen, bigFont, "Select song to load", "songs", songname))
				{
				char filename[200];
				strcpy(filename, "songs/");
				strcat(filename, songname);
				//strcat(filename, ".xds");
				if (!song.Load(filename, progress_callback))
					DoMessage(screen, bigFont, "Error", "Error loading song!", false); 
				}
			}
			break;
		case 2 :		// SAVE
			{
			// Get name
			char songname[SONG_NAME_LEN];
			strcpy(songname, song.name);
			if (DoTextInput(screen, bigFont, "Enter song name", songname, SONG_NAME_LEN))
				{
				// TODO : implement song.SetName() for safety
				strcpy(song.name, songname);
				
				char filename[200];
				strcpy(filename, "songs/");
				strcat(filename, songname);
				strcat(filename, ".xds");
				song.Save(filename, progress_callback);
				}
			}
			break;
		case 3 :		// LOAD DRUMKIT
			{
			PromptLoadDrumkit();
			}
			break;
		case 4 :		// RECORD TO WAV
			{
			// Get wav file name
			char wavname[SONG_NAME_LEN];
			strcpy(wavname, song.name);
			if (DoTextInput(screen, bigFont, "Enter output file name", wavname, SONG_NAME_LEN))
				{
				char filename[200];
				strcpy(filename, "wav/");
				strcat(filename, wavname);
				strcat(filename, ".wav");
				wavWriter.Open(filename);
				}
			}
			break;
		}

	DrawAll();
	
	return selectedId;
}
	
/// Display the track context menu and process the result 
/// @param track 		The track that we want to show menu for
int DoTrackMenu(int track)
{
	char menuTitle[64];
	sprintf(menuTitle, "Track Menu [%s]", drumKit.drums[track].name);

	int selectedVolOption = (song.trackMixInfo[track].vol * 10) / 255;		// track vol 0 to 255

	Menu menu;
	menu.AddItem(1, "Track Vol (%)", "Mute|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set track mix volume");	
	menu.AddItem(2, "Copy track", "Copy the track data for this pattern");	
	menu.AddItem(3, "Paste track", "Paste track data");	
	menu.AddItem(4, "Clear track", "Remove all events on this track");
	if (TrackMixInfo::TS_MUTE != song.trackMixInfo[track].state)
		menu.AddItem(5, "Mute track", "Mute this track");
	else
		menu.AddItem(5, "Unmute track", "Unute this track");
	if (TrackMixInfo::TS_SOLO != song.trackMixInfo[track].state)
		menu.AddItem(6, "Solo track", "Solo this track");
	else
		menu.AddItem(6, "Unsolo track", "Switch solo mode off");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, menuTitle, 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// Set track vol
			{
			// ONLY CHANGE TRACK VOL IF IT IS SELECTED MENU ITEM
			// (because it only goes in steps of 10%)
			song.trackMixInfo[track].vol = (menu.GetItemSelectedOption(1) * 255) / 10;
			}
			break;
		case 2 :		// COPY
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				trackClipboard[i].CopyFrom(&currentPattern->events[track][i]);
				}
			}
			break;
		case 3 :		// PASTE
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				currentPattern->events[track][i].CopyFrom(&trackClipboard[i]);
				}
			}
			DrawPatternGrid(backImg, currentPattern);
			break;
		case 4 :		// CLEAR
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				currentPattern->events[track][i].Init();
				}
			}
			DrawPatternGrid(backImg, currentPattern);
			break;
		case 5 :		// MUTE / UNMUTE
			// Set / reset MUTE state
			SetTrackMute(track);
			break;
		case 6 :		// SOLO
			// Set / remove solo state
			SetTrackSolo(track);
			break;
		}

	DrawTrackInfo(backImg);

	return selectedId;
}

/// Display the pattern context menu and process the result 
int DoPatternMenu()
{
	char menuTitle[64];
	sprintf(menuTitle, "Pattern Menu [%s]", currentPattern->name);

	Menu menu;
	menu.AddItem(1, "Copy pattern", "Copy pattern data to the pattern buffer");	
	menu.AddItem(2, "Paste pattern", "Paste pattern data from the pattern buffer");	
	menu.AddItem(3, "Clear pattern", "Remove all events in this pattern");
	menu.AddItem(4, "Rename pattern", "Change name of the pattern");
	menu.AddItem(5, "Insert into song", "Insert current pattern into song");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, menuTitle, 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// COPY
			patternClipboard.CopyFrom(currentPattern);
			break;
		case 2 :		// PASTE
			currentPattern->CopyFrom(&patternClipboard);
			break;
		case 3 :		// CLEAR
			currentPattern->Clear();
			break;
		case 4 :		// RENAME
			{
			// Get name
			char patname[200];
			strcpy(patname, currentPattern->name);
			if (DoTextInput(screen, bigFont, "Enter new pattern name", patname, PATTERN_NAME_LENGTH))
				{
				// TODO : Implement pattern->SetName() (safer!)
				strcpy(currentPattern->name, patname);
				}
			}
			break;
		case 5 :		// INSERT PATTERN
			song.InsertPattern(currentPatternIndex);
			break;
		}

	DrawAll();
	
	return selectedId;
}

/// Display the songlist context menu and process the result 
int DoSequenceMenu()
{
	int currentPosOption = song.songPos / 10;		// track vol 0 to 255

	Menu menu;
	menu.AddItem(1, "Insert pattern", "Insert current pattern into song");
	menu.AddItem(2, "Remove pattern", "Remove pattern from the song");
	menu.AddItem(3, "Go to position", "1|11|21|31|41|51|61|71|81|91", currentPosOption, "Set current song position");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, "Sequence Menu", 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// INSERT
			song.InsertPattern(currentPatternIndex);
			break;
		case 2 :		// REMOVE
			song.RemovePattern();
			break;
		case 3 :		// SONG GOTO
			// ONLY CHANGE SONG POS IF IT IS SELECTED MENU ITEM
			// (because it only goes in steps of 10)
			song.songPos = menu.GetItemSelectedOption(3) * 10;
			if (song.songPos > PATTERNS_PER_SONG)
				song.songPos = 0;					// safety net
			break;
		}

	DrawAll();
	
	return selectedId;
}

/// Display the main menu (menu of menus)
int DoMainMenu()
{
	Menu menu;
	menu.AddItem(1, "File Menu", "Display the file menu");
	menu.AddItem(2, "Pattern Menu", "Display the pattern menu");
	menu.AddItem(3, "Sequence Menu", "Display the sequence menu");
	menu.AddItem(4, "Track Menu", "Display the track menu");
	menu.AddItem(5, "Options Menu", "Display the options menu");
	menu.AddItem(6, "Help Menu", "Display the help menu");
	menu.AddItem(7, "Quit", "Exit this program");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, "Main Menu", 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// FILE MENU
			DoFileMenu(0);
			break;
		case 2 :		// PATTERN MENU
			DoPatternMenu();
			break;
		case 3 :		// SONGLIST MENU
			DoSequenceMenu();
			break;
		case 4 :		// TRACK MENU
			DoTrackMenu(currentTrack);
			break;
		case 5 :		// OPTIONS MENU
			DoOptionsMenu();
			break;
		case 6 :		// HELP MENU
			{
			//DoHelpMenu();
#ifdef PSP			
			const char* text = 	"START to stop/start playing\n" \
								"SQUARE to rewind\n" \
								"SELECT to change mode\n" \
								"LSB for previous pattern\n" \
								"RSB for next pattern";
#else
			const char* text = 	"SPACE to stop/start playing\n" \
								"R to rewind\n" \
								"M to change mode\n" \
								"PGUP for previous pattern\n" \
								"PGDOWN for next pattern";
#endif								
			DoMessage(screen, bigFont, "Help - Keys / Buttons", text, false);
			}
			break;
		case 7 :		// QUIT
			// Prompt to confirm quit, then to save
			if (DoMessage(screen, bigFont, "Confirm Quit", "Sure to quit?", true))
				{
				quit = true;
				if (DoMessage(screen, bigFont, "Save On Exit", "Do you want to save?", true))
					{
					// Get name
					char songname[SONG_NAME_LEN];
					strcpy(songname, song.name);
					if (DoTextInput(screen, bigFont, "Enter song name", songname, SONG_NAME_LEN))
						{
						// TODO : implement song.SetName() for safety
						strcpy(song.name, songname);
						
						char filename[200];
						strcpy(filename, "songs/");
						strcat(filename, songname);
						strcat(filename, ".xds");
						song.Save(filename, progress_callback);
						}
					}
				}
			break;
		default :
			break;
		} // end switch

	DrawAll();
	
	return selectedId;
}

/// Display the note context menu and process the result 
/// @param track			Relevant track (if -1, then use "current track" and "current step")
/// @param step				Relevant step in the track
/// @return					Id of selected item, or -1 if menu escaped
int DoNoteMenu(int track, int step)
{
	if (!currentPattern)
		return -1;

	// use pattern grid cursor position if none specified 
	if (-1 == track)
		{
		track = currentTrack;
		step = currentStep;
		}

	char menuTitle[64];
	sprintf(menuTitle, "Note Menu [Track %d, Step %d]", track+1, step+1);

	// TODO : Handle cut notes (do not display vol menu item?) 
	int volPercent = (currentPattern->events[track][step].vol * 100) / 127;
	int selectedVolOption = (volPercent + 5) / 10;
	int selectedDecayOption = 0;
	int selectedCountOption = 0;
	int selectedSpacingOption = 0;

	Menu menu;
	menu.AddItem(1, "Note Vol (%)", "0|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set note volume (10 - 100)");	
	menu.AddItem(2, "Repeat Decay (%)", "0|10|20|30|40|50|60|70|80|90", selectedDecayOption, "Volume decay between repeat notes");	
	menu.AddItem(3, "Repeat Count", "0|1|2|3|4|5|6|7", selectedCountOption, "Number of repeat notes");
	menu.AddItem(4, "Repeat Space", "1|2|3|4", selectedSpacingOption, "Steps between repeat notes");
	SDL_Rect r1;
	SetSDLRect(r1, 64, 16, VIEW_WIDTH - 72, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(screen, &r1, bigFont, menuTitle, 0);
	
	// Update options from menu item data
	switch (selectedId)
		{
		case 1 : // note vol
			selectedVolOption = menu.GetItemSelectedOption(1);
			currentPattern->events[track][step].vol = (selectedVolOption * 127) / 10;
			break;
		case 2 :
		case 3 :
		case 4 :
			{
			selectedDecayOption = menu.GetItemSelectedOption(2);
			float repeatVol = (float)currentPattern->events[track][step].vol;
			float decay = (float)selectedDecayOption * 0.1f;
			int count = menu.GetItemSelectedOption(3);
			int space = menu.GetItemSelectedOption(4) + 1;
			int repeatStep = step;
			for (int i = 0; i < count; i++)
				{
				repeatStep += space;
				if (repeatStep < STEPS_PER_PATTERN)
					{
					repeatVol = repeatVol * (1.0f - decay);
					currentPattern->events[track][repeatStep].vol = (unsigned char)repeatVol;
					}
				}
			}
			break;
		}
	
	DrawAll();
	
	return selectedId;
}

// Get mouse zone, based on current screen mode
int GetMouseZone(int x, int y, int mode)
{
	int zone = 0;	 		// no zone / bad zone
	
	if (XM_MAIN == mode)
		{
		for (int i = 1; i < ZONE_MAX; i++)
			{
			if (x >= zones[i].x && x < (zones[i].x + zones[i].w))
				{
				if (y >= zones[i].y && y < (zones[i].y + zones[i].h))
					{
					zone = i;
					i = ZONE_MAX;
					} 
				}
			}
		}
	else if (XM_FILE == mode)
		{
		}
		
	return zone;
}

/// Jump the mouse cursor to the pattern grid cursor
void SetMouseToGridCursor()
{
	SDL_Rect rect = zones[ZONE_PATGRID];
	cursorX = rect.x + (currentStep * PATBOX.w) + (PATBOX.x / 2);
	cursorY = rect.y + (currentTrack * PATBOX.h) + (PATBOX.y / 2);
	SDL_WarpMouse((Uint16)cursorX, (Uint16)cursorY);
}

// Handle key press
void ProcessKeyPress(SDLKey sym, int zone)
{
#ifdef PSP
	// Handle PSP headphone remote control
	// The headphone remote control is treated as a keyboard that sends the
	// following keypresses:
	// F10 Play/Pause
	// F11 Forward
	// F12 Back
	// F13 Volume Up
	// F14 Volume Down
	// F15 Hold
	if (SDLK_F10 == sym)
		sym = SDLK_SPACE;
	else if (SDLK_F11 == sym)
		sym = SDLK_PAGEDOWN;
	else if (SDLK_F12 == sym)
		sym = SDLK_PAGEUP;
/*	else if (SDLK_F13 == sym)
		{
		int vol = (song.vol * 100) / 255;
		if (vol < 98)
		vol += 3;
		SetMainVolume(vol);
		DrawSliders(backImg);
		}
	else if (SDLK_F14 == sym)
		{
		int vol = (song.vol * 100) / 255;
		if (vol > 2)
		vol -= 1;
		SetMainVolume(vol);
		DrawSliders(backImg);
		}*/
#endif
	
	switch (sym)
		{
		case SDLK_LEFT :
			if (currentStep > 0)
				currentStep--;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			// Lock mouse cursor to pattern grid current pos
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDLK_RIGHT :
			currentStep++;
			if (currentStep >= STEPS_PER_PATTERN)
				currentStep = 0;			// wrap around
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDLK_UP :
			if (currentTrack > 0)
				currentTrack--;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDLK_DOWN :
			if (currentTrack < (NUM_TRACKS - 1))
				currentTrack++;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDLK_x :
			// set note on
			if (currentPattern)
				{
				unsigned char vol = currentPattern->events[currentTrack][currentStep].vol;
				if (0 == vol)
					vol = 64;			// note on
				else if (64 == vol)
					vol = 127;			// accent
				else if (127 == vol)
					vol = 1;			// cut
				else
					vol = 0;			// no note
				currentPattern->events[currentTrack][currentStep].vol = vol;
				// play sample
				if (vol > 1)
					{
					Mix_VolumeChunk(drumKit.drums[currentTrack].sampleData, vol);
					Mix_PlayChannel(-1, drumKit.drums[currentTrack].sampleData, 0);
					}
				// redraw grid
				DrawPatternGrid(backImg, currentPattern);								
				}
			break;
		case SDLK_c :
			// cut note
			if (currentPattern)
				{
				if (1 == currentPattern->events[currentTrack][currentStep].vol)
					currentPattern->events[currentTrack][currentStep].vol = 0;
				else
					currentPattern->events[currentTrack][currentStep].vol = 1;
				// redraw grid
				DrawPatternGrid(backImg, currentPattern);								
				}
			break;
		case SDLK_SPACE :
			// Start or stop playback or record to wav
			if (wavWriter.IsOpen())
				{
				if (transport.playing)
					wavWriter.Close();
				else
					wavWriter.StartWriting();
				}

			transport.playing = !transport.playing;
			break;
		case SDLK_b :
		case SDLK_r :
			transport.playing = false;
			transport.patternPos = 0;
			if (Transport::PM_SONG == transport.mode)
				{
				song.songPos = 0;
			    currentPatternIndex = song.songList[song.songPos];
			    SyncPatternPointer();
				}
			DrawAll();	
			break;
		case SDLK_m :
			// switch playback mode (song/pattern)
			if (Transport::PM_PATTERN == transport.mode)
				transport.mode = Transport::PM_SONG;
			else if (Transport::PM_SONG == transport.mode)
				transport.mode = Transport::PM_LIVE;
			else transport.mode = Transport::PM_PATTERN;
			DrawAll();
			break;
		case SDLK_LEFTBRACKET :
		case SDLK_PAGEUP:
			// previous pattern / song pos
			if (Transport::PM_PATTERN == transport.mode || Transport::PM_LIVE == transport.mode)
				{
				if (currentPatternIndex >= MAX_PATTERN)
					currentPatternIndex = 0;
				else if (currentPatternIndex > 0)
					currentPatternIndex--;
				}
			else if (Transport::PM_SONG == transport.mode)
				{
			    if (song.songPos > 0)
			    	song.songPos--;
			    currentPatternIndex = song.songList[song.songPos];
				}
			// sync pattern pointer (unless live mode)
			if (Transport::PM_LIVE != transport.mode)
				SyncPatternPointer();
			DrawAll();	
			break;
		case SDLK_RIGHTBRACKET :
		case SDLK_PAGEDOWN:
			// next pattern / song pos
			if (Transport::PM_PATTERN == transport.mode || Transport::PM_LIVE == transport.mode)
				{
				if (currentPatternIndex >= MAX_PATTERN)
					currentPatternIndex = 0;
				else if (currentPatternIndex < (MAX_PATTERN - 1))
					currentPatternIndex++;
				}
			else if (Transport::PM_SONG == transport.mode)
				{
			    if (song.songPos < (PATTERNS_PER_SONG - 1))
			    	song.songPos++;
			    currentPatternIndex = song.songList[song.songPos];
				}
			// sync pattern pointer (unless live mode)
			if (Transport::PM_LIVE != transport.mode)
				SyncPatternPointer();
			DrawAll();	
			break;
		case SDLK_ESCAPE :
			DoMainMenu();
			break;
		default :
			break;
		}
}

// Handle left mouse click
void ProcessLeftClick(int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	
	// no zone or invalid zone?
	if (0 == zone)
		return;
		
	// "local x and y" (relative to the zone clicked in)
	SDL_Rect rect = zones[zone];
	int x0 = x - rect.x;
	int y0 = y - rect.y;
	printf("ProcessLeftClick: zone = %d, x0 = %d, y0 = %d\n", zone, x0, y0);
		
	switch(zone)
		{
		case ZONE_VOL :
			{
			// If above vol meter (0 to 10), increment
			// If below vol meter (70 to 79), decrement
			// If on volmeter, set main vol (0 to 100%)
			int vol = (song.vol * 100) / 255;
			if (y0 <= 10)
				vol += 3;
			else if (y0 >= 70)
				vol -= 1;
			else if (x0 > 8 && x0 < 24)
				vol =  ((68 - y0) * 100) / 56;
				
			SetMainVolume(vol);
			DrawSliders(backImg);
			}
			break;
		case ZONE_BPM :
			{
			// BPM click
			// BMP range 20 to 200
			if (y0 <= 10)
				song.BPM += 1;
			else if (y0 >= 70)
				song.BPM -= 1;
			else if (x0 > 8 && x0 < 24)
				song.BPM =  50 + ((68 - y0) * 2);
				
			DrawSliders(backImg);
			}		
			break;
		case ZONE_PITCH :
			{
			// Pitch click
			// pitch range -12 to +12
			if (y0 < 40 && song.pitch < 12)
				song.pitch += 1;
			else if (y0 > 40 && song.pitch > -12)
				song.pitch -= 1;

			DrawSliders(backImg);
			}		
			break;
		case ZONE_SONGLIST :
			{
			if (y0 < 10)
				{
				if (songListScrollPos > 0)
					songListScrollPos--;
				}
			else if (y0 > rect.h - 11)
				{
				if (songListScrollPos < PATTERNS_PER_SONG - 7)
					songListScrollPos++;
				}
			else
				{
			    int songIndex = songListScrollPos + (y - 10) / 8;
			    if (songIndex >= 0 && songIndex < PATTERNS_PER_SONG)
			    	{
					song.songPos = songIndex;
					currentPatternIndex = song.songList[song.songPos];
					SyncPatternPointer();
					}
				}
			DrawAll();
			}
			break;
		case ZONE_PATLIST :
			{
			if (y0 < 10)
				{
				if (patListScrollPos > 0)
					patListScrollPos--;
				}
			else if (y0 > rect.h - 11)
				{
				if (patListScrollPos < MAX_PATTERN - 7)
					patListScrollPos++;
				}
			else
				{
			    currentPatternIndex = patListScrollPos + (y - 10) / 8;
    			if (Transport::PM_LIVE != transport.mode)
					SyncPatternPointer();
				}
			DrawAll();
			}
			break;
		case ZONE_ADDTOSONGBTN :
			song.InsertPattern(currentPatternIndex);
			DrawSequenceList(backImg);
			break;			
		case ZONE_TRACKINFO :
			{
			// Handle mute / solo / mix vol 
			int track = y0 / PATBOX.h;
			y0 = y0 - (track * PATBOX.h);		// get y0 local to the specific track
			if (x0 > 83)
				{
				if (y0 < 12)
					{
					// Set / reset MUTE state
					SetTrackMute(track);
					}
				else
					{
					// Set / reset solo state (affects all tracks)
					SetTrackSolo(track);
					}
				}
			else if (x0 > 73)
				{
				// adjust track vol
				unsigned char trackMixVol = song.trackMixInfo[track].vol;
				if (y0 < 12)
					{
					if (trackMixVol < 245)
						trackMixVol += 10;
					}
				else
					{
					if (trackMixVol > 10)
						trackMixVol -= 10;
					}
				song.trackMixInfo[track].vol = trackMixVol;
				}	
				
			DrawTrackInfo(backImg);
			}
			break;
		case ZONE_PATGRID :
			{
			// Pattern grid click
			int event = x0 / PATBOX.w;
			int track = y0 / PATBOX.h;
			printf("Patgrid: track = %d, event = %d\n", track, event);
			// update pattern cursor pos
			currentStep = event;
			currentTrack = track;
			if (currentPattern)
				{
				/*
				unsigned char vol = currentPattern->events[track][event].vol;
				if (0 == vol)
					vol = 64;
				else if (64 == vol)
					vol = 127;
				else
					vol = 0;
				currentPattern->events[track][event].vol = vol;
				// redraw grid
				DrawPatternGrid(backImg, currentPattern);
				*/
				ProcessKeyPress(SDLK_x, ZONE_PATGRID);							
				}
			}
			break;
		case ZONE_SONGNAME :
			break;
		case ZONE_KITNAME :
			break;
		case ZONE_OPTIONS :
			break;
		case ZONE_MODE :
			{
			// switch playback mode (song/pattern)
			ProcessKeyPress(SDLK_m, ZONE_MODE);
			}
			break;
		case ZONE_PATNAME :
			break;
		default :
			printf("No zone!\n");
		}
}

// Handle right mouse click
void ProcessRightClick(int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	printf("ProcessRightClick: zone = %d\n", zone);
	
	// no zone or invalid zone?
	if (0 == zone)
		return;
		
	// "local x and y" (relative to the zone clicked in)
	SDL_Rect rect = zones[zone];
	int x0 = x - rect.x;
	int y0 = y - rect.y;
		
	switch(zone)
		{
		case ZONE_VOL :
			{
			DoVolBPMMenu(0);
			}
			break;
		case ZONE_BPM :
			{
			DoVolBPMMenu(1);
			}		
			break;
		case ZONE_PITCH :
			{
			// What to do for right-click pitch slider?
			}		
			break;
		case ZONE_SONGLIST :
			{
			if (y0 < 10)
				{
				}
			else if (y0 > rect.h - 11)
				{
				}
			else
				{
				// do song list menu
				DoSequenceMenu();
				// NB: songlist menu does a DrawAll()
				}
			}
			break;
		case ZONE_PATLIST :
			{
			if (y0 < 10)
				{
				}
			else if (y0 > rect.h - 11)
				{
				}
			else
				{
				// pattern menu
				DoPatternMenu();
				// NB: pattern menu does a DrawAll()
				}
			}
			break;
		case ZONE_TRACKINFO :
			{
			// TODO : Pop up track menu (Use PSP menus?)
			//        - Copy
			//        - Paste
			//        - Clear
			//        - mute
			//        - solo
			// Handle mute / solo / mix vol 
			int track = y0 / PATBOX.h;
			y0 = y0 - (track * PATBOX.h);		// get y0 local to the specific track
			
			DoTrackMenu(track);
			}
			break;
		case ZONE_PATGRID :
			{
			// Show Right-click Note Menu
			int track = y0 / PATBOX.h;
			int step = x0 / PATBOX.w;
			DoNoteMenu(track, step);
			}
			break;
		case ZONE_PATNAME :
			{
			DoPatternMenu();
			}
			break;
		case ZONE_SONGNAME :
			{
			DoFileMenu(0);
			}
			break;
		case ZONE_KITNAME :
			{
			DoFileMenu(2);	
			}
			break;
		case ZONE_OPTIONS :
			{
			DoOptionsMenu();	
			}	
			break;
		default :
			printf("No zone!\n");
		}
	
}


/// Handle controller buttons
/// eg: PSP
void ProcessControllerButtonPress(Uint8 button, int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	
	// controller buttons are mapped according to joymap.cfg file,
	// which maps controller buttons to SDL/ASCII keys
	unsigned short keyCode = joyMap.GetValueAt(button);
	if (LCLICK == keyCode)
		{
		// If clicked in pattern grid, then trigger "add note", else 
		// treat like left mouse click
		if (ZONE_PATGRID == zone)
			ProcessKeyPress(SDLK_x, ZONE_PATGRID);
		else
			ProcessLeftClick(x, y);
		}
	else if (RCLICK == keyCode)
		ProcessRightClick(x, y);
	else
		ProcessKeyPress((SDLKey)keyCode, zone);
}



// thread example
int global_data = 0;
bool beatFlash = false;

// interval calcs
// beats per second (BPS) = BPM / 60
// quarter-beats per second (QBPS) = BPS * 4
// interval (msec) =  1000 / QBPS
// eg: 100 BPM = 6.666 QBPS, therefore interval = 150 ms (per quarter-beat)

// play thread
// new "float" version (as of v1.2 25/4/2009)
int play_thread_func(void *data)
{
	printf("Play thread starting...\n");
	
	// set up timing
	float nextTick = (float)SDL_GetTicks();
	
    int last_value = 0;
    while ( global_data != -1 )
    	{
        if ( global_data != last_value )
        	{
            //printf("Data value changed to %d\n", global_data);
            last_value = global_data;
        	}

		// calc tick interval (in case BPM has changed)
		// 16 ticks per beat = 64 ticks per pattern
		float bps = (float)song.BPM / 60.0f;
		float tps = bps * 16;
		float interval = (1000.0 / tps);

		// check if CPU is struggling
		global_data = (SDL_GetTicks() < (Uint32)nextTick) ? 0 : 1;			// 1 means CPU struggling
		
		// wait for next tick
		// TODO: put delay inside while!
		//SDL_Delay(10);		// !!! need this otherwise other thread will never process!
		int remaining = (Uint32)nextTick - SDL_GetTicks();
		if (remaining > 15)
			SDL_Delay(remaining - 5); 
		else
			global_data = 1;
		// note that we cannot use SDL_Delay() to wait for the next tick because
		// it's granularity is 10ms on some systems!!!
		while (SDL_GetTicks() < (Uint32)nextTick) ;

		nextTick += interval;
			
		// only process events if we are playing
		if (transport.playing)
			{
			// are we on the next quarter-beat?
			int beatPos = transport.patternPos & 0xF;
			bool onEvent = false;
			if (transport.shuffle > 0)
				onEvent = (0 == beatPos || 4 == beatPos || (8 + transport.shuffle) == beatPos || 12 == beatPos);
			else
				onEvent = (0 == (beatPos & 0x3));
			if (onEvent)
				{
				int event = transport.patternPos / 4;
				// do we have a note to play?
				if (currentPattern)
					{
					for (int track = 0; track < NUM_TRACKS; track++)
						{
						if (currentPattern->events[track][event].vol > 1)
							{
							// calculate output vol
							int trackMixVol = (TrackMixInfo::TS_MUTE == song.trackMixInfo[track].state) ? 0 : song.trackMixInfo[track].vol;
							if (trackMixVol > 0)
								{
								int chunkVol = (song.vol * trackMixVol * currentPattern->events[track][event].vol) / (512 * 128);
								// randomise output vol if neccessary
								if (transport.volrand > 0)
									{
									int range = (chunkVol * transport.volrand) / 100;
									chunkVol += (rand() % range) - range / 2;
									if (chunkVol < 0)
										chunkVol = 0;
									else if (chunkVol >= MIX_MAX_VOLUME)
										chunkVol = MIX_MAX_VOLUME - 1;
									}
								// play the sample
								if (drumKit.drums[track].sampleData)
									{
									Mix_VolumeChunk(drumKit.drums[track].sampleData, chunkVol);
									Mix_PlayChannel(-1, drumKit.drums[track].sampleData, 0);
									// FUTURE? - Set pan (get channel from Mix_Playchannel() return value)
									//Mix_SetPanning(channel, 127, 127);	// channel, left, right
									}
								}
							}
						else if (1 == currentPattern->events[track][event].vol)
							{
							// cut note
							// Find which channel (if any) this track's sample chunk was the last played on
							for (int channel = 0; channel < MIX_CHANNELS; channel++)
								{
								Mix_Chunk* chunk = Mix_GetChunk(channel);
								if (chunk && (chunk == drumKit.drums[track].sampleData))
									{
									//printf("cut track %d, channel %d\n", track, channel);
									Mix_HaltChannel(channel);
									channel = MIX_CHANNELS;
									}
								}
							}
						}
					}
				}

			// If on the beat, then set flash flag
			if (transport.flashOnBeat && (0 == beatPos))
				beatFlash = true;

			// update current pattern tick pos
			transport.patternPos++;
			if (64 == transport.patternPos)
				{
				if (Transport::PM_SONG == transport.mode)
					{
					// next song pos
					song.songPos++;
					if (PATTERNS_PER_SONG == song.songPos)
						song.songPos = 0;
					currentPatternIndex = song.songList[song.songPos];
					
					// If we hit a "No pattern" in our songlist, then rewind
					if (NO_PATTERN_INDEX == currentPatternIndex)
						{
						song.songPos = 0;
						currentPatternIndex = song.songList[0];
						}
					
					SyncPatternPointer();
					}
				else if (Transport::PM_LIVE == transport.mode)
					{
					// start playing "queued" pattern
					SyncPatternPointer();
					}
				transport.patternPos = 0;
				}
				
			} // end if playing
        	
    	} // wend
    	
    // play thread should never quit until program quit
    printf("Play thread quitting\n");
    return(0);
}

// TEST 	
// make a passthru processor function that does nothing...
void noEffect(void *udata, Uint8 *stream, int len)
{
    // Get current output "level"
	short* samples = (short *)stream;
	short maxval = 0;
	for (int i = 0; i < 200; i++)
		{
		if (samples[i] > maxval)
			maxval = samples[i];
		}

	//maxval = abs(maxval);
	if (maxval > 10000)
		maxval = 10000;
	//printf("val = %d\t", maxval);
	if (maxval > outputSample)
		outputSample = maxval;
	else if (outputSample > 0)
		{
		//outputSample = (outputSample + val);
		outputSample = (outputSample * 19) / 20;
		}

	// If we are recording, then write to record file
	if (wavWriter.IsOpen() && wavWriter.IsWriting())
		{
		wavWriter.AppendData(stream, len);
		}
}
//END TEST

// required for PSP (but not Linux !?!?)
#ifdef __cplusplus 
extern "C"
#endif
int main(int argc, char *argv[])
{
	int audio_rate;
	int audio_channels;
	// set this to any of 512,1024,2048,4096
	// the higher it is, the more FPS shown and CPU needed
	int audio_buffers = 512;
	Uint16 audio_format;

	SDL_Rect src;
	SDL_Rect dest;

	float cursorDX = 0.0;
	float cursorDY = 0.0;

	printf("PXDRUM V%s - Copyright James Higgs 2009\n", XDRUM_VER);

	// init fonts
	bigFont = new FontEngine("gfx/font_8x16.bmp", 8, 16);
	if (!bigFont)
		printf("Error - unable to init big font!\n");

	smallFont = new FontEngine("gfx/font_8x8.bmp", 8, 8);
	if (!smallFont)
		printf("Error - unable to init small font!\n");

	// init song
	song.vol = SDL_MIX_MAXVOLUME;
	
	//DrumPattern testpattern;
	//testpattern.events[0][3].vol = 100;

	currentPatternIndex = 0;
	currentPattern = &song.patterns[currentPatternIndex];
	currentPattern->events[0][0].vol = 100;
	currentPattern->events[0][4].vol = 60;

	SDL_Joystick *joystick = NULL;

	// initialize SDL for audio, video and joystick
	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        {
                printf("init error: %s\n", SDL_GetError());
                return 1;
        }


	if((screen = SDL_SetVideoMode(VIEW_WIDTH, VIEW_HEIGHT, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)) == NULL)
        {
                printf("SetVideoMode: %s\n", SDL_GetError());
                return 1;
        }

	// Set window caption for windowed versions
	char caption[64];
	sprintf(caption, "PXDrum v%s", XDRUM_VER);
	SDL_WM_SetCaption(caption, NULL);

	if (0 != LoadImages())
		{
 		printf("Error loading gfx!\n");
		return 1;
 		}

	SDL_Delay(10000);

	// Show splash screen while loading
	SetSDLRect(src, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
	SetSDLRect(dest, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
	SDL_BlitSurface(backImg, &src, screen, &dest);
	SDL_Flip(screen);			// waits for vsync

	// initialize sdl mixer, open up the audio device
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, audio_buffers) < 0)
        {
                printf("Mix_OpenAudio: %s\n", SDL_GetError());
                return 1;
        }

	// print out some info on the audio device and stream
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
	int bits = audio_format & 0xFF;
	printf("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
			bits, audio_channels > 1 ? "stereo" : "mono", audio_buffers );


	SDL_Delay(3000);
/*
	// Note: may be issue with file handles not released
	trackInfo[0].sampleData = Mix_LoadWAV("audio/Voyager/VGR Kick-low end.wav");
	
	if (!trackInfo[0].sampleData)
		{
		printf("Error loading sample!\n");
		}

	trackInfo[1].sampleData = Mix_LoadWAV("audio/Voyager/VGR Snare.wav");
	trackInfo[2].sampleData = Mix_LoadWAV("audio/Voyager/VGR CHH.wav");
*/

	// Load default drumkit
	if (!drumKit.Load("default", progress_callback))
		{
		DoMessage(screen, bigFont, "Error", "Cannot load drumkit 'default'!\nPlease select a drumkit.", false);
		if (!PromptLoadDrumkit())
			{
			DoMessage(screen, bigFont, "Error", "Cannot continue without drumkit!\nProgram will exit.", false);
			return 1;
			}
		}

	// Initial draw of everything
	DrawAll();
	
	// And play a corresponding sound
	Mix_PlayChannel(0, drumKit.drums[0].sampleData, 0);


	if(SDL_NumJoysticks())
        {
		joystick = SDL_JoystickOpen(0);
		printJoystickInfo(joystick);
#ifdef PSP
		joyMap.Load("joymap.psp.cfg");
#else
		joyMap.Load("joymap.cfg");
#endif
        }
	else
        {
		printf("No joystick detected\n");
        }

	// register noEffect as a postmix processor
	Mix_SetPostMix(noEffect, NULL);

	// Create thread to play beats in the background
	SDL_Thread *playThread = SDL_CreateThread(play_thread_func, NULL);
	if (!playThread)
		{
		printf("Unable to create play thread: %s\n", SDL_GetError());
		return 1;
		}

	// start playing
	transport.playing = true;
	
	// for detecting pattern change (when playing song mode)
	int displayedPatternIndex = currentPatternIndex;
	int displayedSongPos = song.songPos;

	SDL_ShowCursor(SDL_DISABLE);
	//Uint32 lastTick = SDL_GetTicks();
	int currentZone = 0;
	SDL_Event event;
	quit = false;
	while(!quit)
		{
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					ProcessControllerButtonPress(event.jbutton.button, (int)cursorX, (int)cursorY);	
					break;
				case SDL_JOYBUTTONUP:
					//if(event.jbutton.button == 7 /* LEFT */ && dir==0) dir=-1;
					//if(event.jbutton.button == 8 /* UP */ && dir==1) dir=-1;
					//if(event.jbutton.button == 9 /* RIGHT */ && dir==2) dir=-1;
					//if(event.jbutton.button == 6 /* DOWN */ && dir==3) dir=-1;
					break;
                case SDL_JOYAXISMOTION:
                	// Simulate a mouse
					//printf("Axis motion: j index = %d axis = %d value = %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
					if (0 == event.jaxis.axis)
						{
						float dx = (float)(event.jaxis.value - JOYMID) / (JOYRANGE / 2);
						//printf("DX = %.3f\n", cursorDX);
						if (dx > -0.15 && dx < 0.15)
							cursorDX = 0.0;
						else
							cursorDX = (cursorDX + dx) * 0.5f;			// avg of last 2
						}
					else if (1 == event.jaxis.axis)
						{
						float dy = (float)(event.jaxis.value - JOYMID) / (JOYRANGE / 2);
						if (dy > -0.15 && dy < 0.15)
							cursorDY = 0.0;
						else
							cursorDY = (cursorDY + dy) * 0.5f;			// avg of last 2
						}
					break;
				// For systems with mouses/trackballs
				case SDL_MOUSEMOTION:
					//printf("Mouse moved by %d,%d to (%d,%d)\n", 
					//	   event.motion.xrel, event.motion.yrel,
					//	   event.motion.x, event.motion.y);
					cursorX = event.motion.x;
					cursorY = event.motion.y;
					cursorDX = 0; cursorDY = 0;
					break;
				case SDL_MOUSEBUTTONDOWN:
					//printf("Mouse button %d pressed at (%d,%d)\n",
					//	   event.button.button, event.button.x, event.button.y);
					if (1 == event.button.button)		// Left mouse button
						ProcessLeftClick((int)cursorX, (int)cursorY);
					else if (3 == event.button.button)	// Right mouse button
						ProcessRightClick((int)cursorX, (int)cursorY);
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					ProcessKeyPress(event.key.keysym.sym, currentZone);
					break;
                case SDL_QUIT:
					quit = true;
					break;
				} // end switch
			}

		// get current zone
		currentZone = GetMouseZone((int)cursorX, (int)cursorY, XM_MAIN);

		// If we have chaned the pattern we're playing, we need to redraw
		if (currentPatternIndex != displayedPatternIndex)
			{
			DrawAll();
			displayedPatternIndex = currentPatternIndex;
			printf("redraw - pattern changed to %d\n", currentPatternIndex);
			}
		else if (displayedSongPos != song.songPos)
			{
			DrawSequenceList(backImg);
			displayedSongPos = song.songPos;
			}

		// blit BG "layer" to screen
		BlitBG();
		
		// OPTIONAL - flash pattern bg on beat
		if (beatFlash)
			{
			dest = zones[ZONE_PATGRID];
			SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 200, 255, 200));
			beatFlash = false;
			}

#ifdef LOCK_MOUSE_TO_GRID_CURSOR
		// Lock grid cursor to mouse if mouse is in grid
		if (ZONE_PATGRID == currentZone)
			{
			currentStep	= ((int)cursorX - zones[ZONE_PATGRID].x) / PATBOX.w;
			currentTrack = ((int)cursorY - zones[ZONE_PATGRID].y) / PATBOX.h;
			}
#endif		
		// Draw pattern cursor
		if (ZONE_PATGRID == currentZone)
			{
			SetSDLRect(src, 24, 0, PATBOX.w, PATBOX.h);
			SetSDLRect(dest, zones[ZONE_PATGRID].x + currentStep * PATBOX.w, zones[ZONE_PATGRID].y + currentTrack * PATBOX.h, PATBOX.w, PATBOX.h);
			SDL_BlitSurface(cursorImg, &src, screen, &dest);
			}

		// Draw playback bar
		// TODO : blit from textures		
		SetSDLRect(dest, 96 + (transport.patternPos * PATBOX.w / 4), 80, 4, 192);
		if (0 == global_data)
			SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 255, 0));
		else // CPU struggling!
			SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 0, 0));

		// Update "joystick mouse" position
		if (ZONE_PATGRID == currentZone)
			{
			cursorX += cursorDX * 5.0f;
			cursorY += cursorDY * 5.0f;
			}
		else
			{
			cursorX += cursorDX * 3.0f;
			cursorY += cursorDY * 3.0f;
			}
		if (cursorX < 0.0) cursorX = 0.0;
		if (cursorX > VIEW_WIDTH - 1) cursorX = VIEW_WIDTH - 1;
		if (cursorY < 0) cursorY = 0;
		if (cursorY > VIEW_HEIGHT -1) cursorY = VIEW_HEIGHT -1;
		// Draw mouse cursor
		DrawCursor((int)cursorX, (int)cursorY);

		// Draw output sample level
		SetSDLRect(dest, 72, 68 - (outputSample / 200), 16, outputSample / 200);
		SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 255, 0));

		SDL_Flip(screen);			// waits for vsync
		
		// control update rate of this loop
		//Uint32 time_remaining = 30 - (SDL_GetTicks() - lastTick);
		//if (time_remaining > 0)
		//	SDL_Delay(time_remaining);
		//lastTick = SDL_GetTicks();
		SDL_Delay(20);					// gives time to other threads
        } // wend

	// clean up
	// stop playing and close playback thread
	transport.playing = false;
	SDL_KillThread(playThread);

	if (wavWriter.IsOpen())
		wavWriter.Close();

	// close joystick
	if (joystick)
		{
		SDL_JoystickClose(joystick);
        }

	// close SDL_mixer, then quit SDL
	Mix_CloseAudio();
	SDL_Quit();

	return(0);
}

