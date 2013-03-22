// fontengine
// Copyright James Higgs 2009

/*
#define FONT_CHARS_PER_LINE	32
#define FONT_CHAR_WIDTH		8
#define FONT_CHAR_HEIGHT	16
#define OUT_CHAR_WIDTH		8
#define OUT_CHAR_HEIGHT		16
*/

class FontEngine
{
public:
	// constructor
	FontEngine(const char* fontfile, int font_char_width, int font_char_height);
	
	// operations
	void DrawGlyph(SDL_Surface* surface, char c, int destx, int desty);
	void DrawText(SDL_Surface* surface, const char *s, SDL_Rect& rect, bool clip);
	int GetFontHeight()
		{
		return fontCharHeight;
		}
private:	
	SDL_Surface *fontImg;
	int fontCharsPerline;
	int fontCharWidth;
	int fontCharHeight;
};
