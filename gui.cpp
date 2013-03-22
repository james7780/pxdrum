/*
 *      menu.cpp
 *      
 *      Copyright 2009 james <james@okami>
 *      
 */

#include <stdlib.h>
#include <dirent.h> 			// for listing directory
#include <stdio.h> 
#include "SDL.h"
#include "platform.h"
#include "fontengine.h"
#include "joymap.h"
#include "gui.h"

extern JoyMap joyMap;


///////////////////////////////////////////////////////////////////////////////
// MenuItem class
///////////////////////////////////////////////////////////////////////////////

/// Setup a menu item
void MenuItem::SetItem(int id, const char* itemText, const char* description)
{
	m_id = id;
	strncpy(m_itemText, itemText, MAX_ITEMTEXT_LEN);
	m_itemText[MAX_ITEMTEXT_LEN-1] = 0;
	strncpy(m_description, description, MAX_DESCRIPTION_LEN);
	m_description[MAX_DESCRIPTION_LEN-1] = 0;
}

/// Setup a menu item (with option list)
void MenuItem::SetItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description)
{
	m_id = id;
	strncpy(m_itemText, itemText, MAX_ITEMTEXT_LEN);
	m_itemText[MAX_ITEMTEXT_LEN-1] = 0;
	
	strncpy(m_optionsText, optionsText, MAX_OPTIONSTEXT_LEN);
	m_optionsText[MAX_OPTIONSTEXT_LEN-1] = 0;
	m_selectedOption = selectedOption;
	
	strncpy(m_description, description, MAX_DESCRIPTION_LEN);
	m_description[MAX_DESCRIPTION_LEN-1] = 0;
}

/// Get the nth option in an option list menu item
/// @return					Length of option text at index index, or 0 if none
int MenuItem::GetOptionText(int index, char* buffer)
{
	const char* pc = m_optionsText;
	int len = (int)strlen(m_optionsText);
	if (0 == len)
		{
		//printf("GetOptionText(%d) - zero length!\n", index);
		return 0;			// no options
		}
			
	// find start pos
	for (int i = 0; i < index; i++)
		{
		// find next seperator ('|' character)
		while (*pc != '|' && ((pc - m_optionsText) < len))
			{
			pc++;
			}
		if (pc >= m_optionsText + len)
			break;		// not found 
			
		pc++;
		}

	// did we run out of options?	
	if (pc >= m_optionsText + len)
		{
		printf ("Menu index %d too big!\n", index);
		return 0;			// not found
		}
		
	// copy to buffer until '|' or 0'
	char* output = buffer;
	while (*pc != '|' && ((pc - m_optionsText) < len))
		{
		*output++ = *pc++;
		}

	// terminate output buffer	 	
	*output = 0;

	//printf("GetOptionText(%d) - result = '%s'\n", index, buffer);
	
	return (int)strlen(buffer);
}

// Get currently selected option in this menu item
int MenuItem::GetSelectedOption()
{
	return m_selectedOption;	
}

// Set currently selected option in this menu item
void MenuItem::SetSelectedOption(int index)
{
	// validate
	char optionText[MAX_OPTIONSTEXT_LEN];
	if (GetOptionText(index, optionText) > 0)
		m_selectedOption = index;
	else
		m_selectedOption = 0;
}


///////////////////////////////////////////////////////////////////////////////
// Menu class
///////////////////////////////////////////////////////////////////////////////

/// Add a menu item to the menu
int Menu::AddItem(int id, const char* itemText, const char* description)
{
	if (m_numItems < MAX_MENU_ITEMS)
		{
		m_menuItems[m_numItems].SetItem(id, itemText, description);
		m_numItems++;	
		}
		
	return -1;			// could not add!	
}

/// Add a menu item to the menu
int Menu::AddItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description)
{
	if (m_numItems < MAX_MENU_ITEMS)
		{
		m_menuItems[m_numItems].SetItem(id, itemText, optionsText, selectedOption, description);
		m_numItems++;	
		}
		
	return -1;			// could not add!	
}


extern void SetSDLRect(SDL_Rect& rect, int x, int y,int w, int h);

#define MENU_TITLE_X		16
#define MENU_ITEM_X			32
#define MENU_ITEM_START_Y	40

/// Display the menu and get chosen item from user
/// @return		id of item selected, or -1 if cancelled
int Menu::DoMenu(SDL_Surface* screen, SDL_Rect* extents, FontEngine* font, const char* title, int initialSelection)
{
	if (!screen || !font || !title)
		{
		printf("Error: DoMenu - bad parameter!\n");
		return -1;
		}

	// default extents if not supplied
	if (NULL == extents)
		extents = &m_extents;
		
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
												extents->w, extents->h,
												32, 0, 0, 0, 0);
	if (!surface)
		{
		printf("Error: CreateRGBSurface failed: %s\n", SDL_GetError());
		return -1;
		}

	// handle initial selection
	int selected = 0;
	if (initialSelection >= 0 && initialSelection < m_numItems)
		selected = initialSelection;

	int itemHeight = (3 * font->GetFontHeight()) / 2;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	while(!done)
		{
		bool upPressed = false;
		bool downPressed = false;
		bool leftPressed = false;
		bool rightPressed = false;
		bool escapePressed = false;
		bool selectPressed = false;
	
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					// controller buttons are mapped according to joymap.cfg file,
					// which maps controller buttons to SDL/ASCII keys
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (SDLK_ESCAPE == keyCode)
						escapePressed = true;
					else if (LCLICK == keyCode)
						selectPressed = true;
					else if (SDLK_UP == keyCode)
						upPressed = true;
					else if (SDLK_DOWN == keyCode)
						downPressed = true;
					else if (SDLK_LEFT == keyCode)
						leftPressed = true;
					else if (SDLK_RIGHT == keyCode)
						rightPressed = true;
/*					
					if(event.jbutton.button == 0) // TRIANGLE
						escapePressed = true;
					else if(event.jbutton.button == 2) // CROSS
						selectPressed = true;
					else if(event.jbutton.button == 6) // DOWN
						downPressed = true;
					else if(event.jbutton.button == 8) // UP
						upPressed = true;
					else if(event.jbutton.button == 7) // LEFT
						leftPressed = true;
					else if(event.jbutton.button == 9) // RIGHT
						rightPressed = true;
*/						
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					//SDL_Delay(1000);
					if (SDLK_UP == event.key.keysym.sym)
						upPressed = true;
					else if (SDLK_DOWN == event.key.keysym.sym)
						downPressed = true;
					else if (SDLK_LEFT == event.key.keysym.sym)
						leftPressed = true;
					else if (SDLK_RIGHT == event.key.keysym.sym)
						rightPressed = true;
					else if (SDLK_RETURN == event.key.keysym.sym)
						selectPressed = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						selectPressed = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						escapePressed = true;
					break;
				} // end switch
			} // wend pollevent

		// react to keypresses
		if (escapePressed)
			{
			selected = -1;
			done = true;
			}
		else if (selectPressed)
			{
			done = true;
			}
		else if (upPressed)
			{
			if (selected > 0)
				selected--;	
			}
		else if (downPressed)
			{
			if (selected < (m_numItems - 1))
				selected++;	
			}
		else if (leftPressed)
			{
			int selectedOption = m_menuItems[selected].GetSelectedOption();
			if (selectedOption > 0)
				m_menuItems[selected].SetSelectedOption(selectedOption - 1);
			else
				m_menuItems[selected].SetSelectedOption(0);
				
			//printf("After left, selopt = %d\n", m_menuItems[selected].GetSelectedOption());
			}
		else if (rightPressed)
			{
			int selectedOption = m_menuItems[selected].GetSelectedOption();
			char buffer[MAX_OPTIONSTEXT_LEN];
			if (m_menuItems[selected].GetOptionText(selectedOption + 1, buffer) > 0)
				selectedOption++;
			m_menuItems[selected].SetSelectedOption(selectedOption);
			
			//printf("After right, selopt = %d\n", m_menuItems[selected].GetSelectedOption());
			}
			
		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, extents->w, extents->h);
		SDL_FillRect(surface, &rect, g_bgColour);
		rect.h = 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		rect.y = extents->h - 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		SetSDLRect(rect, 0, 0, 4, extents->h);
		SDL_FillRect(surface, &rect, g_borderColour);
		rect.x = extents->w - 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		
		// Draw title
		SetSDLRect(rect, MENU_TITLE_X, 8,  extents->w - 16, itemHeight);
		font->DrawText(surface, title, rect, false);

		SetSDLRect(rect, MENU_TITLE_X, 4 + itemHeight,  extents->w - (MENU_TITLE_X*2), 2);
		SDL_FillRect(surface, &rect, g_separatorColour);

		// Draw items
		for (int i = 0; i < m_numItems; i++)
			{
			// draw item text
			SetSDLRect(rect, MENU_ITEM_X, MENU_ITEM_START_Y + (i * itemHeight), 0, 0);
			font->DrawText(surface, m_menuItems[i].GetItemText(), rect, false);

			// if this is an option item, then draw currently selected option
			// with arrow markers before and after the text
			char optionText[MAX_OPTIONSTEXT_LEN];
			optionText[0] = 126;
			optionText[1] = 32;
			int selectedOption = m_menuItems[i].GetSelectedOption();
			if (m_menuItems[i].GetOptionText(selectedOption, optionText+2) > 0)
				{
				int len = (int)strlen(optionText);
				optionText[len] = 32;
				optionText[len+1] = 127;
				optionText[len+2] = 0;		// terminate
				SetSDLRect(rect, VIEW_WIDTH/2, MENU_ITEM_START_Y + (i * itemHeight), 0, 0);
				font->DrawText(surface, optionText, rect, false);
				}
			}
	
		// Draw description of selected item
		if (selected >= 0 && selected < m_numItems)
			{
			SetSDLRect(rect, MENU_TITLE_X, extents->h - itemHeight, 0, 0);
			font->DrawText(surface, m_menuItems[selected].GetDescription(), rect, false);
			}

		// Draw selection highlight
		int y1 = MENU_ITEM_START_Y + (selected * itemHeight) - 4;
		SetSDLRect(rect, MENU_TITLE_X, y1, extents->w - (MENU_TITLE_X*2), 2);
		SDL_FillRect(surface, &rect, g_highlightColour);
		SetSDLRect(rect, MENU_TITLE_X, y1 + itemHeight - 1, extents->w - (MENU_TITLE_X*2), 2);
		SDL_FillRect(surface, &rect, g_highlightColour);
		SetSDLRect(rect, MENU_TITLE_X, y1, 2, itemHeight);
		SDL_FillRect(surface, &rect, g_highlightColour);
		SetSDLRect(rect, extents->w - MENU_TITLE_X - 2, y1, 2, itemHeight);
		SDL_FillRect(surface, &rect, g_highlightColour);
			
		// blit our menu serface to the screen
		SDL_Rect src;
		src.x = 0;
		src.y = 0;
		src.w = extents->w;
		src.h = extents->h;
		SDL_BlitSurface(surface, &src, screen, extents);

		SDL_Flip(screen);			// ?waits for vsync?

		SDL_Delay(20);					// gives time to other threads
		} // wend done
	
	// Tidy up
	SDL_FreeSurface(surface);
	
	// return id of selected item, or 
	int selectedId = -1;
	if (selected >= 0 && selected < m_numItems)
		selectedId = m_menuItems[selected].GetId();

	//printf("DoMenu: selected id = %d\n", selectedId);
	
	return selectedId;
}

/// Get which option was selected in an option item in this menu
/// @param id		Menu item id
/// @return			Selected option index for the specified menu item (if it's an option item) 
int Menu::GetItemSelectedOption(int id)
{
	// find item with specified id
	for (int i = 0; i < m_numItems; i++)
		{
		if (id == m_menuItems[i].GetId())
			{
			return m_menuItems[i].GetSelectedOption();
			}
		}

	//printf ("GetItemSelectedOption(%d) - id not found!\n", id);
	return 0;
}	




// Implementation of global GUI functions

/// Show message to user
/// @param title		Caption (title) text
/// @param prompt		Text to show user
/// @param confirm		If true, then act like a confirm dialog - confirm user action (eg delete, overwrite) 
bool DoMessage(SDL_Surface* screen, FontEngine* font, const char *title, const char *prompt, bool confirm)
{
	if (!screen || !font || !prompt)
		{
		printf("Error: DoMessage - bad parameter!\n");
		return false;
		}

	SDL_Rect extents;
	SetSDLRect(extents, 32, 32, VIEW_WIDTH - 64, VIEW_HEIGHT - 64);
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
												extents.w, extents.h,
												32, 0, 0, 0, 0);
	if (!surface)
		{
		printf("Error: CreateRGBSurface failed: %s\n", SDL_GetError());
		return false;
		}

	// split input string into lines
	char *textLine[20];
	int numLines = 0;
	char s[2000];
	strcpy(s, prompt);
	//printf("Text = %s\n", s);
	char *nextLine = strtok(s, "\n");
	while (nextLine)
		{
		//printf("Line %d: %s\n", numLines, nextLine);
		textLine[numLines++] = nextLine;
		nextLine = strtok(NULL, "\n");
		}
	// mark end of text lines
	textLine[numLines] = 0;

	bool confirmYes = false;

	int itemHeight = (3 * font->GetFontHeight()) / 2;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	while(!done)
		{
	
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyCode)
						confirmYes = true;
					else if (SDLK_ESCAPE == keyCode)
						done = true;
/*					
					//if(event.jbutton.button == 0) // TRIANGLE
					//	escapePressed = true;
					if(event.jbutton.button == 2) // CROSS
						done = true;
*/ 
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					if (SDLK_RETURN == event.key.keysym.sym)
						confirmYes = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						confirmYes = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						done = true;
					break;
				} // end switch
			} // wend pollevent

		if (confirmYes)
			done = true;

		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, extents.w, extents.h);
		SDL_FillRect(surface, &rect, g_bgColour);
		rect.h = 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		rect.y = extents.h - 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		SetSDLRect(rect, 0, 0, 4, extents.h);
		SDL_FillRect(surface, &rect, g_borderColour);
		rect.x = extents.w - 4;
		SDL_FillRect(surface, &rect, g_borderColour);
		
		// Draw title
		SetSDLRect(rect, MENU_TITLE_X, 8,  extents.w - 16, itemHeight);
		font->DrawText(surface, title, rect, false);

		SetSDLRect(rect, MENU_TITLE_X, 4 + itemHeight,  extents.w - (MENU_TITLE_X*2), 2);
		SDL_FillRect(surface, &rect, g_separatorColour);
		
		// Draw text prompt
		//SetSDLRect(rect, MENU_ITEM_X, MENU_ITEM_START_Y,  extents.w - 16, itemHeight);
		//font->DrawText(surface, prompt, rect, false);

		int y = MENU_ITEM_START_Y;
		for (int i = 0; i < numLines; i++)
			{
			SetSDLRect(rect, MENU_ITEM_X, y,  extents.w - 16, itemHeight);
			font->DrawText(surface, textLine[i], rect, false);
			y += itemHeight;
			}

		// Draw instructions
		SetSDLRect(rect, MENU_TITLE_X, extents.h - itemHeight, 0, 0);
		if (confirm)
			{
#ifdef PSP				
			font->DrawText(surface, "X = Yes, ^ = No", rect, false);
#else
			font->DrawText(surface, "ENTER = Yes, ESC = No", rect, false);
#endif
			}
		else
			{
#ifdef PSP				
			font->DrawText(surface, "Press X to continue", rect, false);
#else
			font->DrawText(surface, "Press ENTER to continue", rect, false);
#endif
			}

		// blit our msgbox serface to the screen
		SDL_Rect src;
		src.x = 0;
		src.y = 0;
		src.w = extents.w;
		src.h = extents.h;
		SDL_BlitSurface(surface, &src, screen, &extents);

		SDL_Flip(screen);			// waits for vsync

		SDL_Delay(20);					// gives time to other threads
		} // wend done
	
	// tidy up
	SDL_FreeSurface(surface);
	
	return confirmYes;	
}

// virtual key grid position to ascii char map
static char virtualKeyMap[40] = {
	'1', '2', '3', '4', '5', '6', '7','8', '9', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u','i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j','k', 'l', 8,
	'z', 'x', 'c', 'v', 'b', 'n', 'm',' ', '[', ']'
};

/// Edit text string
/// @param text			Text to edit
/// @param maxlen		Maximum length of the input/output text
/// @return				false if cancelled, else true
bool DoTextInput(SDL_Surface* screen, FontEngine* font, const char* prompt, char* text, int maxlen)
{
	// Load the virtual keyboard bitmaps
	SDL_Surface* keyboardImg = SDL_LoadBMP("gfx/vk_compact.bmp");
	if (!keyboardImg)
		{
		printf("Error loading keyboard image %s!\n", "gfx/vk_compact.bmp");
		return false;
		}

	SDL_Surface* keyboardHiliteImg = SDL_LoadBMP("gfx/vk_compact_hilite.bmp");
	if (!keyboardHiliteImg)
		{
		printf("Error loading keyboard image %s!\n", "gfx/vk_compact_hilite.bmp");
		return false;
		}

	int charPos = (int)strlen(text);
	
	int vkPos = 0;				// posn in virtual kb

	int itemHeight = (3 * font->GetFontHeight()) / 2;
	
	int counter = 0;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	bool escapePressed = false;
	while(!done)
		{
		int dx = 0;
		int dy = 0;
		bool addChar = false;
		bool backspace = false;
		
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyCode)
						addChar = true;
					else if (SDLK_ESCAPE == keyCode)
						escapePressed = true;
					if (SDLK_SPACE == keyCode)
						done = true;
					else if ('R' == keyCode)			// "backspace" [SQUARE button on PS controllers]
						backspace = true;
					else if (SDLK_LEFT == keyCode)
						dx = -1;
					else if (SDLK_RIGHT == keyCode)
						dx = 1;
					else if (SDLK_UP == keyCode)
						dy = -1;
					else if (SDLK_DOWN == keyCode)
						dy = 1;
/*					
					//if(event.jbutton.button == 0) // TRIANGLE
					//	escapePressed = true;
					switch (event.jbutton.button)
						{
						case 0 : 			// TRIANGLE
							escapePressed = true;
							break;
						case 2 : 			// CROSS
							addChar = true;
							break;
						case 3 : 			// SQUARE
							backspace = true;
							break;
						case 11 : 			// START
							done = true;
							break;
						case 7 : 	 		// LEFT
							dx = -1;
							break;
						case 8 :			// UP
							dy = -1;
							break;
						case 9 :			// RIGHT
							dx = 1;
							break;
						case 6 :     		// DOWN
							dy = 1;						
							break;
						}
*/ 
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					if (SDLK_RETURN == event.key.keysym.sym)
						done = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						addChar = true;
					else if (SDLK_BACKSPACE == event.key.keysym.sym)
						backspace = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						escapePressed = true;
					else if (SDLK_LEFT == event.key.keysym.sym)
						dx = -1;
					else if (SDLK_RIGHT == event.key.keysym.sym)
						dx = 1;
					else if (SDLK_UP == event.key.keysym.sym)
						dy = -1;
					else if (SDLK_DOWN == event.key.keysym.sym)
						dy = 1;
					break;
				} // end switch
			} // wend pollevent

		// Respond to input
		if (escapePressed)
			break;

		// update current virtual keyboard grid position
		int x = vkPos % 10;
		int y = vkPos / 10;
		if (1 == dx && x < 9)
			x++;
		else if (-1 == dx && x > 0)
			x--;
		if (1 == dy && y < 3)
			y++;
		else if (-1 == dy && y > 0)
			y--;
		vkPos = (10 * y) + x;
		
		if (addChar)
			{
			// append currently highlighted char to the text string
			char mappedChar = virtualKeyMap[vkPos];
			if (8 == mappedChar)
				{
				backspace = true;
				}
			else
				{
				text[charPos++] = mappedChar;
				text[charPos] = 0;
				}
			}
			
		if (backspace)
			{
			// remove previous char
			if (charPos > 0)
				{
				charPos--;
				text[charPos] = 0;
				}
			}
			
		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
		SDL_FillRect(screen, &rect, g_bgColour);
		rect.h = 4;
		SDL_FillRect(screen, &rect, g_borderColour);
		rect.y = VIEW_WIDTH - 4;
		SDL_FillRect(screen, &rect, g_borderColour);
		SetSDLRect(rect, 0, 0, 4, VIEW_HEIGHT);
		SDL_FillRect(screen, &rect, g_borderColour);
		rect.x = VIEW_WIDTH - 4;
		SDL_FillRect(screen, &rect, g_borderColour);
		
		// Draw text prompt
		SetSDLRect(rect, 8, 8,  VIEW_WIDTH - 16, itemHeight);
		font->DrawText(screen, prompt, rect, false);

		// Draw input text
		char s[200];
		strcpy(s, text);
		strcat(s, "_");
		SetSDLRect(rect, 8, 40,  VIEW_WIDTH - 16, itemHeight);
		font->DrawText(screen, s, rect, false);

		// Draw instructions
		SetSDLRect(rect, 8, VIEW_HEIGHT - itemHeight, 0, 0);
		if (0 == ((counter / 200) % 2))
			{
#ifdef PSP				
			font->DrawText(screen, "Use DPAD to select char, X to input char.", rect, false);
#else
			font->DrawText(screen, "Use arrows to select char, SPACE to input char.", rect, false);
#endif
			}
		else
			{
#ifdef PSP				
			font->DrawText(screen, "Press START to finish, TRIANGLE to cancel.", rect, false);
#else
			font->DrawText(screen, "Press ENTER to finish, ESC to cancel.", rect, false);
#endif
			}


		// draw virtual keyboard
		SDL_Rect src;
		SetSDLRect(src, 0, 0, 320, 96);
		SetSDLRect(rect, 80, 128, 320, 96);
		SDL_BlitSurface(keyboardImg, &src, screen, &rect);
		SetSDLRect(src, x*32, y*24, 32, 24);
		SetSDLRect(rect, 80 + x*32, 128 + y*24, 32, 24);
		SDL_BlitSurface(keyboardHiliteImg, &src, screen, &rect);
	

/*
		// blit our msgbox serface to the screen
		SDL_Rect src;
		src.x = 0;
		src.y = 0;
		src.w = VIEW_WIDTH;
		src.h =	VIEW_HEIGHT;
		SDL_BlitSurface(surface, &src, screen, &extents);
*/
		SDL_Flip(screen);			// waits for vsync
		
		SDL_Delay(20);
		
		counter++;
		} // wend done
	
	return (!escapePressed);	
}

/// Browse for file in the specified folder
/// @param promtp		User prompt
/// @param folder		The folder to select a file from
/// @param filename		The input filename, also output filename
/// @return				false if cancelled, else true
/// @note
///    File selector takes up the whole screen
bool DoFileSelect(SDL_Surface* screen, FontEngine* font, const char* prompt, const char* folder, char* filename)
{
	// array of strings for filenames
	char listnames[MAX_FILELIST_ITEMS][MAX_FILENAME_LEN];
	for (int i = 0; i < MAX_FILELIST_ITEMS; i++)
		strcpy(listnames[i], "");

	int numFiles = 0;	
	DIR           *d;
	struct dirent *dir;
	d = opendir(folder);			// opendir(".");
	if (d)
		{
		while ((dir = readdir(d)) != NULL)
			{
			//printf("%s\n", dir->d_name);
			if (0 == strcmp(dir->d_name, "."))
				continue;
			if (0 == strcmp(dir->d_name, ".."))
				continue;
			if (numFiles < MAX_FILELIST_ITEMS)
				{
				strcpy(listnames[numFiles], dir->d_name);
				numFiles++;
				}
			}

		closedir(d);
		}

	int currentItem = 0;				// posn in list
	int scrollPos = 0;					// = index of first item visible 
	int itemsPerPage = 12;
	int itemHeight = font->GetFontHeight();
	
	int counter = 0;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	bool escapePressed = false;
	while(!done)
		{
		int dy = 0;
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyCode)
						done = true;
					else if (SDLK_ESCAPE == keyCode)
						escapePressed = true;
					else if (SDLK_UP == keyCode)
						dy = -1;
					else if (SDLK_DOWN == keyCode)
						dy = 1;
/*
					switch (event.jbutton.button)
						{
						case 0 : 			// TRIANGLE
							escapePressed = true;
							break;
						case 2 : 			// CROSS
							done = true;
							break;
						case 8 :			// UP
							dy = -1;
							break;
						case 6 :     		// DOWN
							dy = 1;						
							break;
						}
*/
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					if (SDLK_RETURN == event.key.keysym.sym)
						done = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						done = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						escapePressed = true;
					else if (SDLK_UP == event.key.keysym.sym)
						dy = -1;
					else if (SDLK_DOWN == event.key.keysym.sym)
						dy = 1;
					break;
				} // end switch
			} // wend pollevent

		// Respond to input
		if (escapePressed)
			break;

		// update current selected filename
		if (1 == dy && currentItem < (numFiles-1))
			currentItem++;
		else if (-1 == dy && currentItem > 0)
			currentItem--;
			
		if (currentItem < scrollPos)
			scrollPos = currentItem;
		else if (currentItem >= (scrollPos + itemsPerPage))
			scrollPos++;
		
		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
		SDL_FillRect(screen, &rect, g_bgColour);
		rect.h = 4;
		SDL_FillRect(screen, &rect, g_borderColour);
		rect.y = VIEW_HEIGHT - 4;
		SDL_FillRect(screen, &rect, g_borderColour);
		SetSDLRect(rect, 0, 0, 4, VIEW_HEIGHT);
		SDL_FillRect(screen, &rect, g_borderColour);
		rect.x = VIEW_WIDTH - 4;
		SDL_FillRect(screen, &rect, g_borderColour);

		// Draw text prompt and separator
		SetSDLRect(rect, MENU_TITLE_X, 8,  VIEW_WIDTH - 16, itemHeight);
		font->DrawText(screen, prompt, rect, false);

		SetSDLRect(rect, MENU_TITLE_X, 8 + itemHeight, VIEW_WIDTH - (MENU_TITLE_X*2), 2);
		SDL_FillRect(screen, &rect, g_separatorColour);

		// Draw visible portion of file list
		for (int i = 0; i < itemsPerPage; i++)
			{
			//strcpy(s, text);
			//strcat(s, "_");
			SetSDLRect(rect, MENU_ITEM_X, MENU_ITEM_START_Y + i * itemHeight, VIEW_WIDTH - 16, itemHeight);
			font->DrawText(screen, listnames[scrollPos + i], rect, false);
			}

		// Draw selection highlight
		int y1 = MENU_ITEM_START_Y + ((currentItem - scrollPos) * itemHeight) - 2;
		SetSDLRect(rect, MENU_TITLE_X, y1, VIEW_WIDTH - MENU_TITLE_X * 2, 2);
		SDL_FillRect(screen, &rect, g_highlightColour);
		SetSDLRect(rect, MENU_TITLE_X, y1 + itemHeight, VIEW_WIDTH - MENU_TITLE_X * 2, 2);
		SDL_FillRect(screen, &rect, g_highlightColour);
		SetSDLRect(rect, MENU_TITLE_X, y1, 2, itemHeight);
		SDL_FillRect(screen, &rect, g_highlightColour);
		SetSDLRect(rect, VIEW_WIDTH - MENU_TITLE_X - 2, y1, 2, itemHeight);
		SDL_FillRect(screen, &rect, g_highlightColour);


		// Draw instructions
		SetSDLRect(rect, MENU_TITLE_X, VIEW_HEIGHT - 4 - itemHeight, 0, 0);
#ifdef PSP				
		font->DrawText(screen, "Use DPAD to highlight file, X to select.", rect, false);
#else
		font->DrawText(screen, "Use arrows to highlight file, ENTER to select.", rect, false);
#endif

		SDL_Flip(screen);			// waits for vsync
		
		SDL_Delay(20);
		
		counter++;
		} // wend done

	// get selected filename
	if (!escapePressed)
		strcpy(filename, listnames[currentItem]);
	
	return (!escapePressed);	
}

#define PROGBOX_WIDTH	240
#define PROGBOX_HEIGHT	80

/// Show progress bar 
/// NB: Overwritten by next screen update
bool ShowProgress(SDL_Surface* surface, FontEngine* font, const char* text, int progress)
{
	if (!surface || !font || !text || progress < 0 || progress > 100)
		{
		printf("Error: DoMessage - bad parameter!\n");
		return false;
		}

	SDL_Rect extents;
	SetSDLRect(extents, VIEW_WIDTH/2 - PROGBOX_WIDTH/2, VIEW_HEIGHT/2 - PROGBOX_HEIGHT/2, PROGBOX_WIDTH, PROGBOX_HEIGHT);

	int itemHeight = font->GetFontHeight();
	SDL_Rect rect;
	// Clear progress popup background
	SDL_FillRect(surface, &extents, g_bgColour);
	// Draw border
	SetSDLRect(rect, extents.x, extents.y, extents.w, 4);
	SDL_FillRect(surface, &rect, g_borderColour);
	rect.y = extents.y + extents.h - 4;
	SDL_FillRect(surface, &rect, g_borderColour);
	SetSDLRect(rect, extents.x, extents.y, 4, extents.h);
	SDL_FillRect(surface, &rect, g_borderColour);
	rect.x = extents.x + extents.w - 4;
	SDL_FillRect(surface, &rect, g_borderColour);
		
	// Draw text
	SetSDLRect(rect, extents.x + 16, extents.y + 16, extents.w - 16, itemHeight);
	font->DrawText(surface, text, rect, true);

	// Draw progress bar
	SetSDLRect(rect, extents.x + 20, extents.y + PROGBOX_HEIGHT/2,  progress*2, 20);
	SDL_FillRect(surface, &rect, g_highlightColour);
		
	return true;	
}
