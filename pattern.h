// xdrum pattern definition
//
#define STEPS_PER_PATTERN 16
#define NUM_TRACKS	8
#define PATTERN_NAME_LENGTH	32

class DrumEvent
{
public:
	// constructor
	DrumEvent()
		{
		Init();
		};
	
	void Init()
		{
		vol = 0;
		pan = 128;
		};
		
	void CopyFrom(const DrumEvent* event)
		{
		vol = event->vol;
		pan = event->pan;	
		}

	unsigned char vol;
	unsigned char pan;
};

class DrumPattern
{
public:
	// constructor
	DrumPattern()
		{
		name[0] = 0;
/*			
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				events[i][j].Init();
				}
			}
*/ 
		};

	void Clear()
		{
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				events[i][j].Init();
				}
			}
		}
		
	void CopyFrom(DrumPattern* pattern)
		{
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				events[i][j].CopyFrom(&pattern->events[i][j]);
				}
			}
		}

	bool Read(FILE* pfile)
		{
		fread(&name, PATTERN_NAME_LENGTH * sizeof(char), 1, pfile);
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				unsigned char vol;
				unsigned char pan;
				fread(&vol, sizeof(char), 1, pfile);
				fread(&pan, sizeof(char), 1, pfile);
				events[i][j].vol = vol;
				events[i][j].pan = pan;
				}
			}
			
		return true;
		}

	void Write(FILE* pfile)
		{
		fwrite(&name, PATTERN_NAME_LENGTH * sizeof(char), 1, pfile);
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				unsigned char vol = events[i][j].vol;
				unsigned char pan = events[i][j].pan;
				fwrite(&vol, sizeof(char), 1, pfile);
				fwrite(&pan, sizeof(char), 1, pfile);
				}
			}
			
		}

	char name[PATTERN_NAME_LENGTH];
	DrumEvent events[NUM_TRACKS][STEPS_PER_PATTERN];
	
};
