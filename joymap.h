// joystick / controller button mapping

#define JOYMAP_SIZE	64


#define LCLICK	1
#define RCLICK	2
//#define ENTER	13
//#define ESCAPE	27
//#define SPACE	32


/// class representing a joystick button mapping
class JoyMap
{
public:
	// constructor
	JoyMap()
		{
		Init();
		};
		
	//virtual ~Drum();
	
	void Init()
		{
		for (int i = 0; i < JOYMAP_SIZE; i++)
			map[i] = 0;
		};

	bool Load(const char* filename);
	unsigned short GetValueAt(int index) const;
	void SetValueAt(int index, unsigned short value);

private:
	unsigned short map[JOYMAP_SIZE];
	
};
