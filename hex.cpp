#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "configuration.h"

struct clip;
struct mark;
struct file_buffer
{
	std::string 
		b, //buffer
		p; //pathname
	bool e = false; //edited flag
	long int o = 0; //current offset
};
struct clip
{
	std::string b; //buffer
	char n[4]; //name
};

std::vector<clips>clips; //holds clips from 'y' or 'd'
std::string vol; //holds a clip from 'y' or 'd' without a name

//terminal states
termios ui, preserve

//points to file being edited
file_buffer *buffer;
//list of files given in arguments
std::vector <file_buffer> files;

//funtion to restore terminal and exit hex
void restore ();

//function to add bytes to buffer
std::string getText ();

//function to convert hex to string
char toC (char *h);

//function to convert char to hex string
char *toH (unsigned char c);

//function to print an offset
void printo ();

//overload of printo for printing not-current-buffer
void printo (long int o);

//custom range offset print
void printo (long int o, long int l);

//convert hex address to decimal
int hto (char *h);

int main (int argc, char **argv)
{
	int current_buffer = 0, pos;
	std::string tmp;
