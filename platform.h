// platform defines

#define VIEW_WIDTH 	480
#define VIEW_HEIGHT 272

#ifdef PSP
#define JOYRANGE	65536
#define JOYMID		0
#else
#define JOYRANGE	65536			// PS3 controller! (SDL should always have -32767 to +32767 range)
#define JOYMID		0
#endif

#ifdef PSP
	#include <stdio.h>
	#include <string.h>
	#include <pspdebug.h>
	#define printf pspDebugScreenPrintf
	#define main SDL_main
#else
	#include <stdio.h>
#endif

#ifdef WIN32
	#include <string.h>
#endif



