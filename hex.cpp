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

//data for undo command
std::string ub; //holds buffer of last bytes added or removed
int uo, ul; //holds the (o)ffset the last command acted on and the (l)ength of bytes
bool ua; //true if the last operation added bytes, false if bytes were removed

int main (int argc, char **argv)
{
	int current_buffer = 0, pos;
	std::string c, tmp;
	char g;
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
	write (1, t.str ().c_str (), t.str ().size ());
	/*
	* simple state machine that parses the input, populates the command
	* structure variables, and executes the specified commands.
	* s specifies the current state:
	* -1: error state, continue to next command
	* 0: init
	* 1: aggregate an address
	* 2: aggregate a command
	* 3: aggregate a name
	* 4: aggregate a mark
	* 5: aggregate an option
	* 6: regular expression search address
	* 7: aggregate a regular expression
	* 8: aggregate keyword command
	* 9: execute
	* 10: aggregate pathname
	*/
	int o[3], ac = 0, rc = 0, s = 0, count = 0;
	bool addr = false, succ = false, sd;
	char cmd, opt[2], a[8], name[4], key = 0, kwc[4], mk[4];
	std::string rx[2], path, prx[2], in;
	std::stringstream out;
	while (read (0, &key, 1) != -1)
	{
		if (key == 0) continue;
		else if (key == 8 or key == 127)
		{
			if (in.size () > 0)
			{
				in.pop_back ();
				out << "\x1b[2K\r" << in;
				write (1, out.str ().c_str (), out.str ().size ());
			}
		}
		else if (key == 10 or key == 13)
		{
			in.push_back ('\n');
			write (1, "\n", 1);
			t.str (in);
			while (t.get (key))
			{
				switch (s)
				{
					case -1:
					{
						break;
					}
					case 0:
					{
						switch (key)
						{
							case '.': {o[ac] = buffer->o; ac ++; s = 1; break;}
							case '$': {o[ac] = buffer->b.size () - 1; ac ++; break;}
							case '\'': {s = 4; break;}
							case '`': {s = 6; sd = true; break;}
							case '?': {s = 6; sd = false; break;}
							case '+': {o[ac] = buffer->o; break;}
							case '-': {o[ac] -= buffer->o; break;}
							case 'a':
							case 'c':
							case 'i':
								{cmd = key; s = 9; break;}

							case 'd':
							case 'y':
							case 'v':
							case 'x':
								{cmd = key; s = 3; break;}

							case 'z': {cmd = key; s = 7; break;}

							case 'm': {cmd = key; s = 1; break;}

							case 'q': {cmd = key; count = 0; s = 5; break;}

							case 'n':
							case 'p':
							case 'r':
							case 'u':
							case 'w':
								{s = 9; break;}

							case 'o':
							case 's':
							case 'f':
								{s = 10; break;}

							default: {s = -1; write (1, "?", 1); break;}
						}
						break;
					}
					case 1:
					{
						switch (key)
						{
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
								{a[count] = key; break;}
						}
						break;
					}
					case 2:
					{
						break;
					}
					case 3:
					{
						break;
					}
					case 4:
					{
						break;
					}
					case 5:
					{
						switch (key)
						{
							case 'i': {o[count] = key; s = 9; break;}
							case 'g':
							{
								if (count < 2)
								{
									o[count] = key;
									count ++;
									s = 5;
								}
								else
								{
									write (1, "?", 1);
								}
								break;
							}
							case 'c':
							{
								if (count < 2)
								{
									o[count] = key;
									count ++;
									s = 5;
								}
								else
								{
									write (1, "?", 1);
								}
								break;
							}
							case '!':
							{
								o[count] = key;
								s = 9;
								break;
							}
							case '\n':
							{
								s = 9;
								break;
							}
						}
						break;
					}
					case 6:
					{
						break;
					}
					case 7:
					{
						break;
					}
					case 8:
					{
						break;
					}
					case 10:
					{
						break;
					}
					case 9:
					{
						switch (cmd)
						{
							case 'q':
							{
								if (buffer->e and o[0] == '!')
								{
									restore ();
								}
								else if (!buffer->e)
								{
									restore ();
								}
								else
								{
									restore ();
								}
								break;
							}
						}
						break;
					}
				}
				in.clear ();
			}
		}
		else
		{
			in.push_back (key);
			out << "\x1b[2K\r" << in;
			write (1, out.str ().c_str (), out.str ().size ());
		}
		out.str ("");
		key = 0;
	}
	return 0;
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
					p[0] = buffer.at(buffer.size () - 16 + i);
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
			if (i < 16 - 1)
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
