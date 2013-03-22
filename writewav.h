// WAV - writer class

/// Class representing mix info for a track in a song
class WavWriter
{
public:

	// constructor
	WavWriter()
		{
		Init();
		};
	
	void Init()
		{
		m_dataLength = 0;
		m_pfile = NULL;
		m_writing = false;
		};

	bool Open(char* filename);					// open file for writing
	bool IsOpen();								// is file open?
	void StartWriting();						// enable writing (record ON)
	void StopWriting();							// disable writing (record OFF)
	bool IsWriting();							// are we recording?
	void AppendData(void* data, int len);		// add data to the file
	int GetLength();							// get length of data in file
	void Close();								// stop recording and close file
		
	int m_dataLength;
	FILE* m_pfile;
	bool m_writing;
};
