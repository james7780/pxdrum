// song.cpp

#include <stdlib.h>
#include "SDL.h"
#include "platform.h"
#include "fontengine.h"
#include "gui.h"
#include "pattern.h"
#include "song.h"

extern SDL_Surface *screen;
extern FontEngine* bigFont;

/// BInary file i/o helper functions
/// Read a 4-byte integer from a file, in LSB-first order,
/// converting to big-endian if neccessary
int freadInt(FILE* pf)
{
	int value = 0;
	fread(&value, sizeof(int), 1, pf);

#if __BYTE_ORDER == __LITTLE_ENDIAN
	return value;
#else
	// convert to big-endian
	int b = ((value&0xff)<<24)+((value&0xff00)<<8)+((value&0xff0000)>>8)+((value>>24)&0xff);
	return b;
#endif
}

/// Read a 2-byte short from a file, in LSB-first order,
/// converting to big-endian if neccessary
short freadShort(FILE* pf)
{
	short value = 0;
	fread(&value, sizeof(short), 1, pf);

#if __BYTE_ORDER == __LITTLE_ENDIAN
	return value;
#else
	// convert to big-endian
	int b = ((value>>8)&0xff)+((value<<8)&0xff00);
	return b;
#endif
}

/// Write a 4-byte integer to a file, in LBB-first order,
/// converting FROM big-endian to little-endian if neccessary
void fwriteInt(FILE* pf, int value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	fwrite(&value, sizeof(int), 1, pf);
#else
	// convert to big-endian
	int b = ((value&0xff)<<24)+((value&0xff00)<<8)+((value&0xff0000)>>8)+((value>>24)&0xff);
	fwrite(&b, sizeof(int), 1, pf);
#endif
}

/// Write a 2-byte short to a file, in LBB-first order,
/// converting FROM big-endian to little-endian if neccessary
void fwriteShort(FILE* pf, short value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	fwrite(&value, sizeof(short), 1, pf);
#else
	// convert to big-endian
	int b = ((value>>8)&0xff)+((value<<8)&0xff00);
	fwrite(&b, sizeof(short), 1, pf);
#endif
}


void Song::Init()
{
	vol = 200;
	BPM = 100;
	pitch = 0;
	strcpy(name, "<empty>");
	songPos = 0;
	currentPatternIndex = 0;

	// init songlist
	for (int i = 0; i < PATTERNS_PER_SONG; i++)
		songList[i] = NO_PATTERN_INDEX;

	// init pattern names
	for (int i = 0; i < MAX_PATTERN; i++)
		{
		sprintf(patterns[i].name, "P%d", i+1);
		}
}

// File format:
	// char[32]				songname
	// uchar vol
	// uchar bpm
	// char pitch
	// uint currentpattern
	// uint songpos
	// uint songList_length
	// uchar songlist[songList_length]
	// uint numPatterns
	// DrumPattern patterns[numPatterns]
	// uint numTracks
	// TrackMixInfo trackMixInfo[numTracks]

/// Load a songfrom disk	
bool Song::Load(const char* filename, void (*progressCallback)(int))
{
	FILE* pfile = fopen(filename, "rb");
	// check if file open failed
	if(NULL == pfile)
		{
		printf("Failed to open song file %s!\n", filename);
        return false;
		}

	// init progress display
	progressCallback(0);

	fread(&name, SONG_NAME_LEN * sizeof(char), 1, pfile);
	fread(&vol, sizeof(char), 1, pfile);
	fread(&BPM, sizeof(char), 1, pfile);
	fread(&pitch, sizeof(char), 1, pfile);
	currentPatternIndex = freadInt(pfile);
	songPos = freadInt(pfile);
	progressCallback(10);

	// read song list (sequence)
	int songListLength = freadInt(pfile);
	if (songListLength > PATTERNS_PER_SONG)
		{
		DoMessage(screen, bigFont, "Song Load Warning", "Song sequence length too long,\npossibly from later version.\n \nSong may be truncated.", false);
		fread(&songList[0], PATTERNS_PER_SONG * sizeof(char), 1, pfile);
		// skip over extra
		fseek(pfile, songListLength - PATTERNS_PER_SONG, SEEK_CUR);
		}
	else
		{
		fread(&songList[0], songListLength * sizeof(char), 1, pfile);
		}
	progressCallback(30);

	// read patterns
	int numPatterns = freadInt(pfile);
	if (numPatterns > MAX_PATTERN)
		{
		DoMessage(screen, bigFont, "Song Load Warning", "Too many patterns,\npossibly from later version.\n \nSome patterns may not be loaded.", false);
		for (int i = 0; i < MAX_PATTERN; i++)
			patterns[i].Read(pfile);
		// skip over extra
		DrumPattern tempPat;
		for (int i = 0; i < (numPatterns - MAX_PATTERN); i++)
			tempPat.Read(pfile);
		}
	else
		{
		for (int i = 0; i < numPatterns; i++)
			patterns[i].Read(pfile);
		}
	progressCallback(60);

	// read patterns
	int numTracks = freadInt(pfile);
	if (numTracks > NUM_TRACKS)
		{
		DoMessage(screen, bigFont, "Song Load Warning", "Too many tracks!\nSome tracks may not be loaded.", false);
		for (int i = 0; i < NUM_TRACKS; i++)
			trackMixInfo[i].Read(pfile);
		// skip over extra
		TrackMixInfo tempTrackInfo;
		for (int i = 0; i < (numTracks - NUM_TRACKS); i++)
			tempTrackInfo.Read(pfile);
		}
	else
		{
		for (int i = 0; i < numTracks; i++)
			trackMixInfo[i].Read(pfile);
		}

	progressCallback(100);
	
	fclose(pfile);
	
	return true;	
}

/// Save the song to disk	
bool Song::Save(const char* filename, void (*progressCallback)(int))
{
	FILE* pfile = fopen(filename, "wb");
	// check if file open failed
	if(NULL == pfile)
		{
		printf("Failed to open song file %s!\n", filename);
        return false;
		}

	// init progress display
	progressCallback(0);

	fwrite(&name, SONG_NAME_LEN * sizeof(char), 1, pfile);
	fwrite(&vol, sizeof(char), 1, pfile);
	fwrite(&BPM, sizeof(char), 1, pfile);
	fwrite(&pitch, sizeof(char), 1, pfile);
	fwrite(&currentPatternIndex, sizeof(int), 1, pfile);
	fwrite(&songPos, sizeof(int), 1, pfile);

	progressCallback(10);

	unsigned int songListLength = PATTERNS_PER_SONG;
	fwrite(&songListLength, sizeof(int), 1, pfile);
	fwrite(&songList[0], songListLength * sizeof(char), 1, pfile);

	progressCallback(30);

	unsigned int numPatterns = MAX_PATTERN;
	fwrite(&numPatterns, sizeof(int), 1, pfile);
	fwrite(&patterns[0], numPatterns * sizeof(DrumPattern), 1, pfile);

	progressCallback(60);

	unsigned int numTracks = NUM_TRACKS;
	fwrite(&numTracks, sizeof(int), 1, pfile);
	fwrite(&trackMixInfo[0], numTracks * sizeof(TrackMixInfo), 1, pfile);

	progressCallback(100);
	
	fclose(pfile);
	
	return true;	
}

/// Insert a pattern into the songlist at the cuurent song pos
/// @param patternIndex			Index of pattern to insert into songlist
/// @return						true is ok, false if fail
bool Song::InsertPattern(int patternIndex)
{
	// validate
	if (patternIndex < 0 || patternIndex >= MAX_PATTERN)
		return false;
		
	// shift up songlist entries after the current  songpos
	for (int i = PATTERNS_PER_SONG-1; i > songPos; i--)
		{
		songList[i] = songList[i-1];
		}
		
	// insert pattern index into songlist
	songList[songPos] = patternIndex;
	
	// move up songpos
	if (songPos < PATTERNS_PER_SONG-1)
		songPos++;
		
	return true;
}

/// Remove the songlist entry at the current song pos
/// @return						true is ok, false if fail
bool Song::RemovePattern()
{
	// shift down songlist entries after this songpos
	for (int i = songPos; i < PATTERNS_PER_SONG-1; i++)
		{
		songList[i] = songList[i+1];
		}

	songList[PATTERNS_PER_SONG-1] = NO_PATTERN_INDEX;
	
	return true;
}
