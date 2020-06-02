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
//function to convert hexadecimal string to character value
char toC (char *h);
//function to convert character to hexadecimal string
char *toH (char c);
//function to print an offset
void printo ();
//overload of printo for printing a buffer that's not the current buffer
void printo (int b);

int main (int argc, char **argv)
{
	int current_buffer = 0;
	std::string c;
	char key[1];
	std::fstream fs;
	std::stringstream sts, t;
	winsize w;

	if (argc > 1)
	{
		for (int i = 0; i < argc - 1; i ++)
		{
			if (argv[i][0] == '-')
			{
				sscanf (&argv[i][1], "%d", word_size);
			}
			else
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
	key[1] = {0};

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
			if (c.size () == 0)
			{
				printo ();
				buffer->o ++;
			}
			else
			{
				switch (c[0])
				{
					case '.':
					{
						break;
					}
					case '$':
					{
						break;
					}
					case '%':
					{
						break;
					}
					case '/':
					{
						if (c.size () < 2)
						{
							write (1, "?", 1);
							break;
						}
					}
					case '?':
					{
						if (c.size () < 2)
						{
							write (1, "?", 1);
							break;
						}
						break;
					}
					case '\'':
					{
						if (c.size () < 2)
						{
							write (1, "?", 1);
							break;
						}
						break;
					}
					case '0':
					{
						if (c.size () < 3)
						{
							write (1, "?", 1);
							break;
						}
						break;
					}
					case 'a':
					{
						break;
					}
					case 'c':
					{
						break;
					}
					case 'i':
					{
						break;
					}
					//characters that will never appear first return an error immediately
					case '-':
					case '+':
					case 'b':
					case 'e':
					case 'g':
					case 'h':
					case 'j':
					case 'k':
					case 'l':
					case 'n':
					case 'o':
					case 'q':
					case 't':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					{
						write (1, "?", 1);
						break;
					}
					default:
					{

					}
				}
				if (c[0] == '.' or c[0] == '$' or c[0] == '%')
				{

				}
				else if (c[0] == '/' and c.size () > 1)
				{

				}
				else if (c[0] == '\'' and c.size () > 1)
				{

				}
				else if (c[0] == '0' and c.size () > 3)
				{

				}
				else
				{
				}
			}
			c.clear ();
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

void printo ()
{
	std::stringstream s;
	s.str ("");
	if (buffer->b.size () > 0)
	{
		s
			<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< buffer->o
			<< '|';
		for (int i = 0; i < word_size; i ++)
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
		for (int i = 0; i < word_size; i ++)
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
			<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< b
			<< '|';
		for (int i = 0; i < word_size; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				s << toH (buffer->b[b + i]);
			}
			else
			{
				s << "~~";
			}
			if (i < word_size - 1)
				s << '-';
		}
		s << '|';
		for (int i = 0; i < word_size; i ++)
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
