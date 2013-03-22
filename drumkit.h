/*
 *      drumkit.h
 *      
 *      Copyright 2009 james <james@okami>
 *      
 */

#define DRUM_NAME_LEN	32
#define DRUMKIT_NAME_LEN	32
#define MAX_DRUMS_PER_KIT	8

/// class representing a drum in a drumkit
class Drum
{
public:
	// constructor
	Drum()
		{
		Init();
		};
		
	//virtual ~Drum();
	
	void Init()
		{
		vol = 0;
		pan = 128;
		name[0] = 0;
		sampleData = NULL;
		};

	unsigned char vol;
	unsigned char pan;
	char name[DRUM_NAME_LEN];
	Mix_Chunk* sampleData;
};

/// Class representing a drumkit
class DrumKit
{
public:
	// constructor
	DrumKit()
		{
		Init();
		};
	
	void Init()
		{
		name[0] = 0;
		};
		
	char name[DRUMKIT_NAME_LEN];
	Drum drums[MAX_DRUMS_PER_KIT];
	
	bool Load(const char* kitname, void (*progressCallback)(int));
};

