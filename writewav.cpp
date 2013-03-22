/*
     WAV File Specification
     FROM http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
    The canonical WAVE format starts with the RIFF header:
    0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                                   (0x52494646 big-endian form).
    4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                                   4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                                   This is the size of the rest of the chunk 
                                   following this number.  This is the size of the 
                                   entire file in bytes minus 8 bytes for the
                                   two fields not included in this count:
                                   ChunkID and ChunkSize.
    8         4   Format           Contains the letters "WAVE"
                                   (0x57415645 big-endian form).

    The "WAVE" format consists of two subchunks: "fmt " and "data":
    The "fmt " subchunk describes the sound data's format:
    12        4   Subchunk1ID      Contains the letters "fmt "
                                   (0x666d7420 big-endian form).
    16        4   Subchunk1Size    16 for PCM.  This is the size of the
                                   rest of the Subchunk which follows this number.
    20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                                   Values other than 1 indicate some 
                                   form of compression.
    22        2   NumChannels      Mono = 1, Stereo = 2, etc.
    24        4   SampleRate       8000, 44100, etc.
    28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
    32        2   BlockAlign       == NumChannels * BitsPerSample/8
                                   The number of bytes for one sample including
                                   all channels. I wonder what happens when
                                   this number isn't an integer?
    34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.

    The "data" subchunk contains the size of the data and the actual sound:
    36        4   Subchunk2ID      Contains the letters "data"
                                   (0x64617461 big-endian form).
    40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                                   This is the number of bytes in the data.
                                   You can also think of this as the size
                                   of the read of the subchunk following this 
                                   number.
    44        *   Data             The actual sound data.
*/

// song.cpp

#include <stdlib.h>
#include "SDL.h"
#include "platform.h"
//#include "fontengine.h"
//#include "gui.h"
//#include "pattern.h"
//#include "song.h"
#include "writewav.h"

void WriteWav16SHeader(FILE* pfile, int numSamples)
{
	if (!pfile)
		return;

	// Format chunk
	int fmtChunkSize = 16;
	short format = 1;
	short numChannels = 2;
	int sampleRate = 44100;
	short bitsPerSample = 16;
	int byteRate = sampleRate * numChannels * bitsPerSample/8;
	short blockAlign = numChannels * bitsPerSample/8;
	// Data chunk
	int dataChunkSize = numSamples * numChannels * bitsPerSample/8;

	// RIFF chunk (outer chunk)
	int riffChunkSize = 4 + (8 + fmtChunkSize) + (8 + dataChunkSize);

	// write header
	fwrite("RIFF", 4, 1, pfile);
	fwrite((void*)&riffChunkSize, 4, 1, pfile);
	fwrite("WAVE", 4, 1, pfile);
	// write format chunk
	fwrite("fmt ", 4, 1, pfile);
	fwrite((void*)&fmtChunkSize, 4, 1, pfile);
	fwrite((void*)&format, 2, 1, pfile);
	fwrite((void*)&numChannels, 2, 1, pfile);
	fwrite((void*)&sampleRate, 4, 1, pfile);
	fwrite((void*)&byteRate, 4, 1, pfile);
	fwrite((void*)&blockAlign, 2, 1, pfile);
	fwrite((void*)&bitsPerSample, 2, 1, pfile);
	// write data chunk info
	fwrite("data", 4, 1, pfile);
	fwrite((void*)&dataChunkSize, 4, 1, pfile);

}

/// Start writing WAV file
bool WavWriter::Open(char* filename)
{
	if (m_pfile)
		return false;

	m_pfile = fopen(filename, "wb");
	if (!m_pfile)
		return false;

	// Write WAV header (with info for 0-length data)
	WriteWav16SHeader(m_pfile, 0);

	m_dataLength = 0;

	return true;
}

/// IS the WAV writer open?
bool WavWriter::IsOpen()
{
	return (m_pfile != NULL);
}

/// Trigger state to "WRITING"
void WavWriter::StartWriting()
{
	m_writing = true;
}

/// Trigger state to "NOT WRITING"
void WavWriter::StopWriting()
{
	m_writing = false;
}

bool WavWriter::IsWriting()
{
	return m_writing;
}

/// Append WAV data to the WAV file
void WavWriter::AppendData(void* data, int len)
{
	if (!m_pfile)
		return;

	fwrite(data, len, 1, m_pfile);
	m_dataLength += len;
}

/// Get current data length of wav file
int WavWriter::GetLength()
{
	return m_dataLength;
}

/// Stop writing WAV file
void WavWriter::Close()
{
	if (!m_pfile)
		return;

	 m_writing = false;

	// rewind
	fseek(m_pfile, 0, SEEK_SET);
	// Overwrite original WAV header (with info for final data length)
	WriteWav16SHeader(m_pfile, m_dataLength / 4);
	// close file
	fclose(m_pfile);

	m_pfile = NULL;
}

/* REFERENCE CODE
void WriteWav16Mono(char* filename, void *data, int numSamples)
{
	// Format chunk
	int fmtChunkSize = 16;
	short format = 1;
	short numChannels = 1;
	int sampleRate = 44100;
	short bitsPerSample = 16;
	int byteRate = sampleRate * numChannels * bitsPerSample/8;
	short blockAlign = numChannels * bitsPerSample/8;
	// Data chunk
	int dataChunkSize = numSamples * numChannels * bitsPerSample/8;

	// RIFF chunk (outer chunk)
	int riffChunkSize = 4 + (8 + fmtChunkSize) + (8 + dataChunkSize);

	FILE* pfile = fopen(filename, "wb");
	// write header
	fwrite("RIFF", 4, 1, pfile);
	fwrite((void*)&riffChunkSize, 4, 1, pfile);
	fwrite("WAVE", 4, 1, pfile);
	// write format chunk
	fwrite("fmt ", 4, 1, pfile);
	fwrite((void*)&fmtChunkSize, 4, 1, pfile);
	fwrite((void*)&format, 2, 1, pfile);
	fwrite((void*)&numChannels, 2, 1, pfile);
	fwrite((void*)&sampleRate, 4, 1, pfile);
	fwrite((void*)&byteRate, 4, 1, pfile);
	fwrite((void*)&blockAlign, 2, 1, pfile);
	fwrite((void*)&bitsPerSample, 2, 1, pfile);
	// write data chunk
	fwrite("data", 4, 1, pfile);
	fwrite((void*)&dataChunkSize, 4, 1, pfile);
	fwrite(data, dataChunkSize, 1, pfile);
	fclose(pfile);
}
*/

