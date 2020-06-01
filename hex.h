#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/param.h>
#include <sys//stat.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <stdio_ext.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>
#include <vector>
#include <pwd.h>
#include <dirent.h>
#include <exception>
#include <iterator>
#include <sstream>
#include <fstream>
#include <iomanip>

//output strings
#define unsaved "Buffer has unsaved changes!"
#define empty "Buffer is empty!"
#define eof "Offset is at the End of File!"
#define nowinsize "Couldn't get window size!"
#define small "Window is too small!"
#define rerr "Read error!"
#define terr "Couldn't set terminal attributes!"
#define cerr "Bad command formatting!"

struct file_buffer
{
	std::string b;
	std::string p;
	bool e = false;
	int o = 0;
};
struct clip
{
	std::string b;
	std::string n;
};
extern termios editor, preserve;
extern int word_size;
extern file_buffer *buffer;
extern std::vector<file_buffer> files;
extern std::vector<clip> clipboard;
extern std::string v;

void restore ();

std::string getText ();

std::vector<std::string> parse (std::string);

std::string input ();

void cmd (std::string s);

bool iscmd (std::string s);
bool isadd (std::string s);

//big function handles
void manip (int a1, int a2, char c, int r, char n[]);
