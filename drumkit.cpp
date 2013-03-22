/*
 *      drumkit.cpp
 *      
 *      Copyright 2009 james <james@okami>
 *      
 */

#include <stdlib.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "platform.h"
#include "drumkit.h"


///////////////////////////////////////////////////////////////////////////////
// Drum class
///////////////////////////////////////////////////////////////////////////////

/*
// Free up drum data
Drum::~Drum()
{
	if (sampleData)
		Mix_FreeChunk(sampleData);
}
*/

///////////////////////////////////////////////////////////////////////////////
// DrumKit class
///////////////////////////////////////////////////////////////////////////////
#define KIT_DEBUG	true

/// Load a drumkit
bool DrumKit::Load(const char* kitname, void (*progressCallback)(int))
{
	char scmd[200], stemp[200];
	FILE *pfile;
	FILE *plog;

	printf("Loading drumkit '%s'...\n", kitname);

	// Init progress display
	progressCallback(0);

	char kitcfgpath[200];
	strcpy(kitcfgpath, "kits/");
	strcat(kitcfgpath, kitname);
	strcat(kitcfgpath, "/kit.cfg");
	char logpath[200];
	//strcpy(logpath, "kits/");
	//strcat(logpath, kitname);
	//strcat(logpath, "/loadkit.log");
	strcpy(logpath, "loadkit.log");			// JH 11/4/2009 - always make dk load log in program folder (dk folder may not exist!)

	if (KIT_DEBUG)
		{
		printf("Drumkit path: %s...\n", kitcfgpath);
		printf("Log file path: %s...\n", logpath);

		plog = fopen(logpath, "w");
		fprintf(plog, "Opening kit config file: %s\n", kitcfgpath);
		fflush(plog);
		}

	pfile = fopen(kitcfgpath, "r");
	// check if file open failed
	if(NULL == pfile)
		{
		if (KIT_DEBUG)
			{
			fprintf(plog, "Failed to open kit config file!\n");
			fclose(plog);
			}
        return false;
		}

	// Reset track info
	Init();

	// Parse kit file
	int lineNumber = 0;
	int numTracksRead = 0;
	bool end = false;
	while(!end)
		{
		// reset valid flags to trap errors
		bool validCmd = false;
		bool validParam = false;

		lineNumber++;
		if (!fgets(scmd, 200, pfile))
			break;
			
		if (KIT_DEBUG)
			fprintf(plog, "Command is %s\n", scmd);
			
		if(scmd[0] == '!' || scmd[0] == '#')
			{
			validCmd = true;
			validParam = true;
			if (KIT_DEBUG)
				fprintf(plog, "Comment encountered. Scanning till eol.\n");
			// read till end of line (character 10)
			fscanf(pfile, "%c", &stemp[0]);
			while(stemp[0] != 10)
				fscanf(pfile, "%c", &stemp[0]);
			}
		
		// track def?
		if(strstr(scmd, "track") == scmd)
			{
			// get param value
			char *param = strchr(scmd, '=');
			if (param)
				{
				param++;
				// get track value (= drum number/index)
				// NB: drum is associated with a "track number", so that
				//     we can have a drumkit with eg 3 drums that load into tracks
				//     1, 4 and 5.
				int trackNum = 0;
				sscanf(scmd+5, "%d", &trackNum);
				if (KIT_DEBUG) printf("Track %d:\n", trackNum);
				if (trackNum >=1 && trackNum <= MAX_DRUMS_PER_KIT)
					{
					numTracksRead++;
					validCmd = true;
					// remove "\n" off end of params
					param[strlen(param)-1] = 0;
					// parse params
					char *sname = strtok(param, "|");
					if (KIT_DEBUG) printf("   Name: %s\n", sname);
					if (sname)
						{
						char *smix = strtok(NULL, "|");
						if (KIT_DEBUG) printf("   Mix: %s\n", smix);
						if (smix)
							{
							char *span = strtok(NULL, "|");
							if (KIT_DEBUG) printf("   Pan: %s\n", span);
							if (span)
								{
								char *sfile = strtok(NULL, "|");
								if (KIT_DEBUG) printf("   File: %s\n", sfile);
								if (sfile)
									{
									// validate params
									validParam = true;
									int mix = atoi(smix);
									if (mix < 0 || mix > 255)
										{
										printf("Bad mix value, line %d\n", lineNumber);
										mix = 200;
										}
									int pan = atoi(span);
									if (pan < 0 || pan > 255)
										{
										printf("Bad pan value, line %d\n", lineNumber);
										pan = 128;
										}
									// text sample path
									char samplePath[200];
									strcpy(samplePath, "kits/");
									strcat(samplePath, kitname);
									strcat(samplePath, "/");
									strcat(samplePath, sfile);
									if (KIT_DEBUG) printf("samplepath: %s\n", samplePath);
									/*
									FILE* pf = fopen(samplePath, "rb");
									if (pf)
										fclose(pf);
									else
										printf("Bad sample filename, line %d\n", lineNumber);
									*/
									// set trackinfo
									int i = trackNum - 1;
									drums[i].vol = mix;
									drums[i].pan = pan;
									// TODO : safe name copy
									strcpy(drums[i].name, sname);
									// Load sample
									if (drums[i].sampleData)
										Mix_FreeChunk(drums[i].sampleData);
									drums[i].sampleData = Mix_LoadWAV(samplePath);
									if (KIT_DEBUG) printf("sampledata: %X\n", (unsigned int)drums[i].sampleData);

									// update progress bar
									progressCallback(trackNum * 10);
									}
								}
							}
						}
					}
				else
					{
	        		if (KIT_DEBUG)
	        			fprintf(plog, "Error - bad track number on line %d\n", lineNumber);
					}
				}
			else
				{
				if (KIT_DEBUG)
					fprintf(plog, "Error - bad parameter on line %d\n", lineNumber);
				}
				
				
			}	// end if track def

		// drumkit name?
		if(strstr(scmd, "name") == scmd)
			{
			validCmd = true;
			// get param value
			char *param = strchr(scmd, '=');
			if (param)
				{
				param++;
				// remove "\n" off end of param
				param[strlen(param)-1] = 0;
				if (strlen(param) > 0)
					{
					validParam = true;
					if (strlen(param) > 31)
						param[DRUMKIT_NAME_LEN - 1] = 0;
					strcpy(this->name, param);
					}
				}
			}

/*
		if(strcmp(scmd, "norm") == 0) {
        	if(numnorms == MAX_VERTS_PER_MODEL - 1 ) {
        		if (KIT_DEBUG) fprintf(plog, "Error - too many normals! Exiting...\n");
				fclose(pfile);
				return TOO_MANY_NORMS;		// error
			}
			validcmd = TRUE;
			fscanf(pfile, "%f %f %f",
				&x, &y, &z);
			if(KIT_DEBUG) fprintf(plog, "norm: xyz %3.3f %3.3f %3.3f\n", x, y, z);
			m_norm[numnorms].x = x;
			m_norm[numnorms].y = y;
			m_norm[numnorms].z = z;
			numnorms++;
		}
*/

		if (!validCmd)
			{
			printf("Bad command on line %d\n", lineNumber);
			if (KIT_DEBUG)
				fprintf(plog, "Bad command on line %d\n", lineNumber);
			}

		if (!validParam)
			{
			printf("Bad parameter on line %d\n", lineNumber);
			if (KIT_DEBUG)
				fprintf(plog, "Bad parameter on line %d\n", lineNumber);
			}

		// check if EOF before end
		if (feof(pfile))
			end = true;
		}

	fclose(pfile);

	if (KIT_DEBUG)
		fclose(plog);
	
	return true;
}

