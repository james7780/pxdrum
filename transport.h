/*
 *      transport.h
 *      
 *      Copyright 2009 james <james@okami>
 *      
 */


/// Transport class (handles control of audio playback)
class Transport 
{
public:
	Transport()
		{
		playing = false;
		mode = PM_PATTERN;
		patternPos = 0;
		songPos = 0;
		shuffle = 0;
		jitter = 0;
		volrand = 0;
		flashOnBeat = false;
		}

	// playback mode
	enum PLAYBACK_MODE { PM_PATTERN = 0, PM_SONG = 1, PM_LIVE = 2 }; 

	bool playing;					// whether playback running or not
	PLAYBACK_MODE mode;				// playback mode
	int patternPos;					// current "tick" in the pattern (0 to 63)
	int songPos;					// current position in song
	int shuffle;					// shuffle amount
	int jitter;						// random "jitter" in millisecs
	int volrand;					// random hit volume in percent
	bool flashOnBeat;				// flash background on the beat
};
