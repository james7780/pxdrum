// song.h

#define SONG_NAME_LEN		32
#define PATTERNS_PER_SONG	100			// max length of song in patterns
#define MAX_PATTERN			50			// max number of patterns in a song
#define NO_PATTERN_INDEX	0xFF		// marker for "no pattern" in songlist

/// Class representing mix info for a track in a song
class TrackMixInfo
{
public:
	enum TRACKSTATE { TS_ON = 0, TS_MUTE = 1, TS_SOLO = 2 };
	// constructor
	TrackMixInfo()
		{
		Init();
		};
	
	void Init()
		{
		vol = 255;				// full volumne 
		pan = 128;				// centrepan
		state = TS_ON;			// track is not muted or solo'd
		prevState = TS_ON;		// track is not muted or solo'd
		};

	bool Read(FILE* pfile)
		{
		fread(&vol, sizeof(char), 1, pfile);
		fread(&pan, sizeof(char), 1, pfile);
		fread(&state, sizeof(char), 1, pfile);
		fread(&prevState, sizeof(char), 1, pfile);
		return true;
		}

	void Write(FILE* pfile)
		{
		fwrite(&vol, sizeof(char), 1, pfile);
		fwrite(&pan, sizeof(char), 1, pfile);
		fwrite(&state, sizeof(char), 1, pfile);
		fwrite(&prevState, sizeof(char), 1, pfile);
		}
		
	unsigned char vol;
	unsigned char pan;
	unsigned char state;		// current track state
	unsigned char prevState;	// state before last solo enable
};

/// class representing a song
class Song
{
public:
	// constructor
	Song()
		{
		Init();
		};
		
	unsigned char vol;					// song current vol
	unsigned char BPM;					// song current BPM
	char pitch;							// song current pitch offset
	char name[SONG_NAME_LEN];			// song name
	DrumPattern patterns[MAX_PATTERN];
	int songPos;						// current song position
	int currentPatternIndex;			// index of current pattern
	// song list (pattern indices)
	unsigned char songList[PATTERNS_PER_SONG];
	
	// track mix info
	TrackMixInfo trackMixInfo[NUM_TRACKS];
	
	// member funcs
	void Init();
	// Load song from disk
	bool Load(const char* filename, void (*progressCallback)(int));	
	// Save song to disk
	bool Save(const char* filename, void (*progressCallback)(int));
	// Insert a pattern into the songlist at the cuurent song pos
	bool InsertPattern(int patternIndex);	
	// Remove the songlist entry at the current song pos
	bool RemovePattern();
};

