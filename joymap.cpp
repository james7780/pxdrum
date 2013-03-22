/// joystick / controller button mapping
//
// joystick / cnotroller map maps joystick button number (index) to
// virtual keyboard key 

// define this to log joystick mapping
#define JOYMAP_DEBUG true

#include <stdlib.h>
#include "SDL.h"
//#include "SDL_mixer.h"
#include "platform.h"
#include "joymap.h"

/// Get joystick map character corresponding to the specified 
/// joystick button index
unsigned short JoyMap::GetValueAt(int index) const
{
	if (index < 0 || index >= JOYMAP_SIZE)
		return 0;
		
	return map[index];	
}

/// Set joystick map character corresponding to the specified 
/// joystick button index
void JoyMap::SetValueAt(int index, unsigned short value)
{
	if (index < 0 || index >= JOYMAP_SIZE)
		return;
		
	map[index] = value;	
}

/// Load a joyick map
bool JoyMap::Load(const char* filename)
{
	char scmd[200], stemp[200];
	FILE *pfile;
	FILE *plog;

	printf("Loading joystick/controller map '%s'...\n", filename);

	char joymappath[200];
	strcpy(joymappath, filename);
	char logpath[200];
	strcpy(logpath, "joymap.log");

	if (JOYMAP_DEBUG)
		{
		plog = fopen(logpath, "w");
		fprintf(plog, "Opening joymap config file: %s\n", joymappath);
		fflush(plog);
		}

	pfile = fopen(joymappath, "r");
	// check if file open failed
	if(NULL == pfile)
		{
		if (JOYMAP_DEBUG)
			{
			fprintf(plog, "Failed to open joymap config file!\n");
			fclose(plog);
			}
        return false;
		}

	// Reset joymap info
	Init();

	// Parse joymap file
	int lineNumber = 0;
	int numMappingsRead = 0;
	bool end = false;
	while(!end)
		{
		// reset valid flags to trap errors
		bool validCmd = false;
		bool validParam = false;

		lineNumber++;
		if (!fgets(scmd, 200, pfile))
			break;
			
		if (JOYMAP_DEBUG)
			fprintf(plog, "Command is %s\n", scmd);
			
		if (scmd[0] == '!' || scmd[0] == '#')
			{
			validCmd = true;
			validParam = true;
			if (JOYMAP_DEBUG)
				fprintf(plog, "Comment encountered. Scanning till eol.\n");
			// read till end of line (character 10)
			fscanf(pfile, "%c", &stemp[0]);
			while(stemp[0] != 10)
				fscanf(pfile, "%c", &stemp[0]);
			}
		else if (strlen(scmd) > 2)
			{
			// All other lines must be a joystick button mapping
			// "nn aaaaaaa"
			int buttonIndex = -1;	// invalid
			sscanf(scmd, "%d %s", &buttonIndex, stemp);
			if (JOYMAP_DEBUG)
				printf("Button %d = %s\n", buttonIndex, stemp);
			if (buttonIndex >= 0 && buttonIndex < JOYMAP_SIZE)
				{
				numMappingsRead++;
				validCmd = true;
				// check param
				validParam = true;
				if (1 == strlen(stemp))
					this->SetValueAt(buttonIndex, stemp[0]);
				else if (0 == strcmp(stemp, "LCLICK"))
					this->SetValueAt(buttonIndex, LCLICK);
				else if (0 == strcmp(stemp, "RCLICK"))
					this->SetValueAt(buttonIndex, RCLICK);
				else if (0 == strcmp(stemp, "ESCAPE"))
					this->SetValueAt(buttonIndex, SDLK_ESCAPE);
				else if (0 == strcmp(stemp, "SPACE"))
					this->SetValueAt(buttonIndex, SDLK_SPACE);
				else if (0 == strcmp(stemp, "PGUP"))
					this->SetValueAt(buttonIndex, SDLK_PAGEUP);
				else if (0 == strcmp(stemp, "PGDOWN"))
					this->SetValueAt(buttonIndex, SDLK_PAGEDOWN);
				else if (0 == strcmp(stemp, "UP"))
					this->SetValueAt(buttonIndex, SDLK_UP);
				else if (0 == strcmp(stemp, "DOWN"))
					this->SetValueAt(buttonIndex, SDLK_DOWN);
				else if (0 == strcmp(stemp, "LEFT"))
					this->SetValueAt(buttonIndex, SDLK_LEFT);
				else if (0 == strcmp(stemp, "RIGHT"))
					this->SetValueAt(buttonIndex, SDLK_RIGHT);
				else
					validParam = false;

				if (validParam && JOYMAP_DEBUG)
					printf("Button %d mapped to character code %d\n", buttonIndex, this->GetValueAt(buttonIndex));
			
				}
			else
				{
				if (JOYMAP_DEBUG)
					fprintf(plog, "Error - bad joystick button index on line %d\n", lineNumber);
				}
			}
			
		if (!validCmd)
			{
			printf("Bad command on line %d\n", lineNumber);
			if (JOYMAP_DEBUG)
				fprintf(plog, "Bad command on line %d\n", lineNumber);
			}

		if (!validParam)
			{
			printf("Bad parameter on line %d\n", lineNumber);
			if (JOYMAP_DEBUG)
				fprintf(plog, "Bad parameter on line %d\n", lineNumber);
			}

		// check if EOF before end
		if (feof(pfile))
			end = true;
		}

	fclose(pfile);

	if (JOYMAP_DEBUG)
		fclose(plog);
	
	return true;
}
