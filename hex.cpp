#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <charconv>

#include <fcntl.h>
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
struct clip;
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
	//clipboard for the file
	std::vector<clip>c;
	//volatile clipboard for unnamed y and d operations
	std::string v;
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

//function to restore terminal and terminate hex
void restore ();
//function to get input
std::string getText ();

int main (int argc, char **argv)
{
	int current_buffer = 0;
	std::string commands;
	char key[1];
	std::fstream fs;
	std::stringstream sts, t;
	winsize w;

	if (argc > 1)
	{
		for (int i = 0; i < argc - 1; i ++)
		{
			files.emplace_back ();
			files.back ().p = argv[i];
			fs.open (argv[i], std::ios_base::binary | std::ios_base::in);
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
	ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
	t.str ("");
	for (int i = 0; i < w.ws_row; i ++)
		t << '\n';
	t << ':';
	write (1, t.str ().c_str (), t.str ().size ());
	while (1)
	{
		char key[1] = {0};
		std::string c;
		while (1)
		{
			t.str ("");
			key[0] = 0;
			read (0, key, 1);
			if (key[0] == 0)
			{
				continue;
			}
			else if (key[0] == 10 or key[0] == 13)
			{
				write (1, "\n", 1);
				break;
			}
			else if (key[0] == 127 or key[0] == 8)
			{
				if (c.size () > 0)
				{
					c.pop_back ();
					t << "\x1b[2K\r:" << c;
					write (1, t.str ().c_str (), t.str ().size ());
				}
			}
			else
			{
				c.push_back (key[0]);
				write (1, key, 1);
			}
		}
		if (c[0] == '.')
		{

		}
		switch (c[0])
		{
			case '.':
			{
				t.str (c.substr (1, std::string::npos));

				break;
			}
			case '0':
			{
				break;
			}
			case 'q':
			{
				break;
			}
			case 'a':
			{
				break;
			}
		}
	}
}

void restore ()
{
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &preserve);
	exit (0);
}

std::string getText ()
{
	std::string buffer;
	int word_counter, hex_counter = 0;
	std::stringstream sts;
	char key[1] = {0}, hex[2] = {48, 48}, t[1];
	while (1)
	{
		sts.str ("");
		sts
			<< "0x"
			<< std::hex
			<< std::setw (7)
			<< std::right
			<< std::setfill ('0')
			<< word_counter
			<< '0'
			<< '|';
		write (STDOUT_FILENO, sts.str ().c_str (), sts.str ().size ());
		sts << std::flush;
		while (1)
		{
			key[0] = 0;
			while (key[0] == 0)
			{
				if (read (STDIN_FILENO, key, 1) == -1 and errno != EAGAIN)
				{
					perror ("Read error");
					exit (5);
				}
				if (key[0] > 47 and key[0] < 58)
				{

					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] > 96 and key[0] < 103)
				{
					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] > 64 and key[0] < 71)
				{
					key[0] += 32;
					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] == '.')
				{
					write (STDOUT_FILENO, ".", 1);
					return buffer;
				}
				else
				{
					key[0] = 0;
				}
			}
			key[0] = 0;
			while (key[0] == 0)
			{
				if (read (STDIN_FILENO, key, 1) == -1 and errno != EAGAIN)
				{
					perror ("Read error!");
					exit (5);
				}
				if (key[0] > 47 and key[0] < 58)
				{

					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] > 96 and key[0] < 103)
				{
					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] > 64 and key[0] < 71)
				{
					key[0] += 32;
					write (STDOUT_FILENO, key, 1);
					hex[0] = key[0];
				}
				else if (key[0] == '.')
				{
					write (STDOUT_FILENO, ".", 1);
					return buffer;
				}
				else
				{
					key[0] = 0;
				}
			}
			sscanf (hex, "%x", t);
			buffer.push_back (t[0]);
			hex_counter ++;
			if (hex_counter < word_size)
			{
				write (STDOUT_FILENO, "-", 1);
			}
			else
			{
				hex_counter = 0;
				word_counter ++;
				char p[1];
				write (STDOUT_FILENO, "|", 1);
				for (int i = 0; i < word_size; i ++)
				{
					p[0] = buffer.at(buffer.size () - word_size + i);
					if (p[0] > 31 and p[0] < 177)
						write (STDOUT_FILENO, p, 1);
					else
						write (STDOUT_FILENO, ".", 1);
				}
				write (STDOUT_FILENO, "\n", 1);
				break;
			}
		}
	}
}
