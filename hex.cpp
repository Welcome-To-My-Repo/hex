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
struct mark;
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
	//list of marked addresses
	std::vector <mark>m;
};
struct clip
{
	//buffer
	std::string b;
	//buffer name
	std::string n;
};
struct mark
{
	char n[4];
	int o;
};
//termios states
termios editor, preserve;
//points to file being edited
file_buffer *buffer;
//list of files given in arguments
std::vector<file_buffer> files;

//function to restore terminal and terminate hex
void restore ();
//function to get input
std::string getText ();
//function to convert hexadecimal string to character value
char toC (char *h);
//function to convert character to hexadecimal string
char *toH (char c);
//function to print an offset
void printo ();
//overload of printo for printing a buffer that's not the current buffer
void printo (int b);
//convert hex address to decimal
int htoa (char *h);
//refegex find
int rfind (std::string r);
//regex replace
void rrplace (std::string r);

int main (int argc, char **argv)
{
	int current_buffer = 0, pos, count, s;
	std::string c, tmp;
	char key[1], g;
	std::fstream fs;
	std::stringstream sts, t;
	winsize w;

	//command argument variables
	//addresses are
	//-1 for the current offset
	//-2 for the last line
	//-3 for the entire buffer
	int o[3];
	int o1, o2, o3; //addresses are -1 for '$'
	std::string regex1, regex2, path, pregex1, pregex2;
	char cmd[1], opt[2], a[8], name[4];
	bool addr; //true after first delimiting slash is encountered.
	int ac; //incremented every time an address is populated
	bool succ; //true if a mark or byte buffer is located
	bool cmd; //true if a command has been parsed

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
	/*
	* simple state machine that parses the input, populates the command
	* structure variables, and executes the specified commands.
	* s specifies the current state:
	* -1 error state, reads until next command
	* 0 neutral state
	* 1 read hexadecimal address
	* 2 read named address
	* 3 command
	* 4 third address
	* 5 name
	* 6 pathname
	* 7 keyword command
	*/
	key[0] = 0;
	ac = 0;
	addr = false;
	s = 0;
	count = 0;
	while (read (0, key, 1) != 0)
	{
		if (key[0] == 0)
			continue;
		switch (m)
		{
			case -1:
			{
				switch (key[0])
				{
					case '\n':
					case '\r':
					case '|':
					{
						m = 0;
					}
				}
			}
			case 0:
			{
				switch (key[0])
				{
					case '.': {break;}
					case '/': {addr = true; m = 1; break;}
					case '$': {break;}
					case '%': {break;}
					case '\'' {break;}
					case '`': {break;}
					case '?': {break;}
					case '+': {break;}
					case '-': {break;}
					case 'a':
					case 'c':
					case 'i':
					case 'd':
					case 'y':
					case 'v':
					case 'z':
					case 'm':
					case 'p':
					case 'x':
					case ''
				}
			}
			case 1:
			{
				switch (key[0])
				{
					case '/':
					{
						if (addr and count == 0)
						{
							write (1, "?", 1);
							m = -1;
						}
						if (addr and count > 0)
						{
							m = 0;
							count = 0;
							sscanf (a, "%x", &o1);
						}
						else
						{
							addr = true;
						}
					}
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case 'a':
					case 'b':
					case 'c':
					case 'd':
					case 'e':
					case 'f':
					{
						if (count < 8)
						{
							a[count] = k[0];
							count ++;
						}
						else
						{
							write (1, "?", 1);
							m = -1;
						}
					}
				}
			}
			case 2:
			{
				if (count < 4)
				{
					if (k[0] > 32 and k[0] < 127)
					{
						name[count] = k[0];
					}
					else
					{
						write (1, "?", 1);
						m = -1;
					}
				}
				else
				{
					succ = false;
					for (int i = 0; i < buffer->m.size (); i ++)
					{
						if (strncmp (name, buffer->m[i].n) == 0)
						{
							succ = true;
							if (ac < 3)
							{
								o[ac] = buffer->m[i].o;
							}
							else
							{
								write (1, "?", 1);
								m = -1;
							}
						}
					}
				}
			}
		}
		//execute command here
	}

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
			if (c.size () == 0)
			{
				printo ();
				buffer->o ++;
			}
			else
			{
				sts.str (c);
				/*
				* simple state machine that parses the input
				* and populates the
				* m speifies the current state (mode)
				* 0 = error
				* 1 = populate first address
				*/
				int m = 0, addrcount = 0;
				while (sts.get (g))
				{
					switch (g)
					{
						case '0':
						{

							break;
						}
					}
					switch (m)
				}
			}
			c.clear ();
			write (1, "\n", 1);
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
}

void restore ()
{
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &preserve);
	exit (0);
}

char toC (char *h)
{
	char r = 0;
	if (h[1] > 57)
		r += h[1] - 87;
	else
		r += h[1] - 48;
	if (h[0] > 57)
		r += (h[0] - 87) * 16;
	else
		r += (h[0] - 48) * 16;
	return r;
}

char * toH (char c)
{
	char *x = new char[2];
	x[0] = c / 16;
	x[1] = c % 16;
	if (x[0] > 9)
		x[0] += 87;
	else
		x[0] += 48;
	if (x[1] > 9)
		x[1] += 87;
	else
		x[1] += 48;
	return x;
}

int htoa (char *)
{
//16^7 = 268435456
//16^6 = 16777216
//16^5 = 1048576
//16^4 = 65536
//16^3 = 4096
//16^2 = 256
//16^1 = 16
//16^0 = 1
return 0;
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
			//<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< word_counter
			<< '|'
			<< "\x1b[0m";
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
			t[0] = toC (hex);
			buffer.push_back (t[0]);
			hex_counter ++;
			if (hex_counter < 16)
			{
				write (STDOUT_FILENO, "-", 1);
			}
			else
			{
				hex_counter = 0;
				word_counter ++;
				char p[1];
				write (STDOUT_FILENO, "|", 1);
				for (int i = 0; i < 16; i ++)
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

void printo ()
{
	std::stringstream s;
	s.str ("");
	if (buffer->b.size () > 0)
	{
		s
			//<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< buffer->o
			<< '|';
		for (int i = 0; i < 16; i ++)
		{
			if (buffer->o + i < buffer->b.size ())
			{
				s << toH (buffer->b[buffer->o + i]);
			}
			else
			{
				s << "~~";
			}
			if (i < word_size - 1)
				s << '-';
		}
		s << '|';
		for (int i = 0; i < 16; i ++)
		{
			if (buffer->o + i < buffer->b.size ())
			{
				if (buffer->b[buffer->o + i] > 31 and buffer->b[buffer->o + i] < 177)
					s << buffer->b[buffer->o + i];
				else
					s << ' ';
			}
			else
			{
				s << ' ';
			}
		}
		s << '.';
	}
	else
	{
		s << "Buffer is empty!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}

void printo (int b)
{
	std::stringstream s;
	s.str ("");
	if (buffer->b.size () > 0)
	{
		s
			//<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< b
			<< '|';
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				s << toH (buffer->b[b + i]);
			}
			else
			{
				s << "~~";
			}
			if (i < 16 - 1)
				s << '-';
		}
		s << '|';
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				if (buffer->b[b + i] > 31 and buffer->b[b + i] < 177)
					s << buffer->b[b + i];
				else
					s << ' ';
			}
			else
			{
				s << ' ';
			}
		}
		s << '.';
	}
	else
	{
		s << "Buffer is empty!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}
