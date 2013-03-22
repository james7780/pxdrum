/*
 *      menu.h
 *      
 *      Copyright 2009 james <james@okami>
 *      
 */

// menu defines
#define MAX_MENU_ITEMS		10
#define MAX_ITEMTEXT_LEN	32
#define MAX_OPTIONSTEXT_LEN	256
#define MAX_DESCRIPTION_LEN	64

// file list defines
#define MAX_FILELIST_ITEMS	100
#define MAX_FILENAME_LEN	32		

// colours (must be defined in main app)
extern Uint32 g_bgColour;
extern Uint32 g_borderColour;
extern Uint32 g_separatorColour;
extern Uint32 g_highlightColour;

// GUI functions
extern bool DoMessage(SDL_Surface* screen, FontEngine* font, const char *title, const char *prompt, bool confirm);
extern bool DoTextInput(SDL_Surface* screen, FontEngine* font, const char* prompt, char* text, int maxlen);
extern bool DoFileSelect(SDL_Surface* screen, FontEngine* font, const char* prompt, const char* folder, char* filename);
extern bool ShowProgress(SDL_Surface* screen, FontEngine* font, const char* text, int progress);

/// Class representing an item in a menu
class MenuItem
{
public:
	// constructor
	MenuItem()
		{
		Init();
		};
	
	void Init()
		{
		m_id = 0;
		strcpy(m_itemText, "<item>");
		strcpy(m_optionsText, "");
		m_selectedOption = 0;
		strcpy(m_description, "<desc>");
		};

	void SetItem(int id, const char* itemText, const char* description);
	void SetItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description);

	int GetId() { return m_id; }
	const char* GetItemText() { return m_itemText; }
	int GetOptionText(int index, char* buffer);
	int GetSelectedOption();
	void SetSelectedOption(int index);
	const char* GetDescription() { return m_description; }

private:		
	int m_id;
	char m_itemText[MAX_ITEMTEXT_LEN];
	char m_optionsText[MAX_OPTIONSTEXT_LEN];
	int m_selectedOption;
	char m_description[MAX_DESCRIPTION_LEN];
	
};

class Menu
{
public:
	// constructor
	Menu()
		{
		Init();
		};
	
	void Init()
		{
		m_numItems = 0; 
		m_selected = 0;
		// default menu extents to full view
		m_extents.x = 0;
		m_extents.y = 0;
		m_extents.w = VIEW_WIDTH;
		m_extents.h = VIEW_HEIGHT;
		};

	int AddItem(int id, const char* itemText, const char* description);	
	int AddItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description);	
	int DoMenu(SDL_Surface* screen, SDL_Rect* extents, FontEngine* font, const char* title, int initialSelection);
	int GetItemSelectedOption(int id);	

private:		
	int m_numItems; 
	int m_selected;
	MenuItem m_menuItems[MAX_MENU_ITEMS];
	SDL_Rect m_extents;
};

