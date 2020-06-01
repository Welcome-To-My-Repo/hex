#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

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
	//buffer
	std::string b;
	//pathname
	std::string p;
	//edited flag
	bool e = false;
	//offset
	int o = 0;
};
struct clip
{
	//buffer
	std::string b;
	//buffer name
	std::string n;
};
//termios states
termios editor, preserve;
//word size is the length in bytes of each "line"
int word_size = 8;
//points to file being edited
file_buffer *buffer;
//list of files given in arguments
std::vector<file_buffer> files;
//list of buffers set with y and d
std::vector<clip>clipboard;
//volatile buffer for y and d when no name is specified
std::string v;

//function to restore terminal and terminate hex
void restore ();
//function to get input
std::string getText ();

int main (int argc, char **argv)
{
	int current_buffer = 0;
	std::string tmp, commands, out;
	char key[1];
	std::fstream fs;
	std::stringstream sts;

	if (argc > 1)
	{
		for (int i = 0; i < argc - 1; i ++)
		{
			files.emplace_back ();
			files.back ().p = argv[i];
			fs.open (argv[i], std::ios_base::binary | std::ios_base:in);
			if (fs.is_open ())
			{
				sts << fs.rdbuf ();
				fs.close ();
				files.back ().b.assign (sts.str ());
			}
			else
			{
				fs.close ();
				files.back ().b.clear ();
			}
		}
	}
	else
	{
		files.emplace_back ();
	}

	buffer = &files.at(current_buffer);

	tcgetattr (STDIN_FILENO, &preserve);
	editor = preserve;
	editor.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        editor.c_oflag &= ~OCRNL;
        editor.c_oflag |= OPOST;
        editor.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        editor.c_cflag |= CS8;
        editor.c_cc[VMIN] = 0;
        editor.c_cc[VTIME] = 1;
	if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &editor) == -1)
	{
		perror (terr);
		exit (1);
	}

	cout << ":";
	while (1)
	{
		char key[1] = {0};
		std::string commands;
		while (1)
		{
			key[0] = 0;
			std::cin.get(key);

			if (key[0] == 0)
			{
				continue;
			}
			else if (key[0] == 10 or key[0] == 13)
			{
				break;
			}
			else if (key[0] == 127 or kkey[0] == 8)
			{
				if (commands.size () > 0)
				{
					commands.pop_back ();
					std::cout << "\x1b[2K\r:" << commands;
				}
			}
			else
			{
				commands.push_back (key[0]);
				std::cout << key[0];
			}
		}
	}
}
