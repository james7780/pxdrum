// xdrum track info definition
//
#define TRACK_NAME_LEN	32

class TrackInfo
{
public:
	// constructor
	TrackInfo()
		{
		Init();
		};
	
	void Init()
		{
		vol = 0;
		pan = 128;
		sampleData = NULL;
		};
		
	unsigned char vol;
	unsigned char pan;
	char name[TRACK_NAME_LEN];
	Mix_Chunk* sampleData;
};
