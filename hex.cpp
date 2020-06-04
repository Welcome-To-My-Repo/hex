#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <charconv>

#include <termios.h>
#include <unistd.h>

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

//terminal states
termios ui, preserve;

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

void execute (
	int *o = 0,
	int ac = 0,
	char cmd = 0,
	char *opt = 0,
	char *name = 0,
	char *keyword = 0,
	std::string *rx = 0,
	std::string path = "",
	std::string *prx = 0);

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

	if (argc > 1)
	{
		for (int i = 1; i < argc; i ++)
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

	buffer = &files.front ();

	tcgetattr (0, &preserve);
	ui = preserve;
	ui.c_lflag &= ~ECHO;
	ui.c_iflag |= ICRNL;
	tcsetattr (0, TCSANOW, &ui);

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
	* 9: aggregate pathname
	*/
	//void execute (int *o, int ac, char cmd, char *name, char *keyword, std::string *rx, std::string path, std::string *prx)
	int o[3], ac = 0, rc = 0, s = 0, count = 0;
	bool addr = false, succ = false, r = true;
	int confirm = 0;
	/* confirm signifies which components exist in the command
	* first address +1
	* second address +1
	* command +3
	*/
	char 	cmd = 0,
		opt = 0,
		a[8] = {0,0,0,0,0,0,0,0},
		name[4] = {0,0,0,0},
		key = 0,
		keyword[4] = {0,0,0,0},
		mk[4] = {0,0,0,0};
	std::string rx[2], path, prx[2], in;
	std::stringstream out;
	while (1)
	{
		//while (read (0, &key, 1) != -1)
		while (read (0, &key, 1) != -1)
		{
			t.str ("");
			//in.push_back (key);
			if (key == 8 or key == 127)
			{
				if (in.size () > 0)
					in.pop_back ();
				t << "\x1b[2K\r" << in;
				write (1, t.str().c_str (), t.str().size ())
			}
			else if (key == 10 or key == 13)
			{
				//std::cout << '\r';
				break;
			}
			else
			{
				in.push_back (key);
				//std::cout << key;
			}
		}
		t.str (in);
		while (t >> key)
		{
			switch (s)
			{
				case -1:
				{
					switch (key)
					{
						case '\n':
						case '\r':
						case '|':
						{
							s = 0;
							break;
						}
					}
				}
				case 0:
				{
					switch (key)
					{
						case '.': {s = 0; o[ac] = buffer->o; ac ++; confirm += 1; break;}
						case 'q': {cmd = key; s = 5; confirm += 3; break;}
						default: {s = -1; break;}
					}
					break;
				}
				case 1: break;
				case 2: break;
				case 3: break;
				case 4: break;
				case 5:
				{
					switch (key)
					{
						case 'i': {opt = key; break;}
					}
					r = false;
					break;
				}
				case 6: break;
				case 7: break;
				case 8: break;
				case 9: break;
			}
		}
		t.clear ();
		switch (confirm)
		{
			case 0:
			{
				printo();
				if (buffer->o + 16 < buffer->b.size ())
					buffer->o += 16;
				break;
			}
			case 1:
			{
				break;
			}
			case 2: break;
			case 3:
			{
				switch (cmd)
				{
					case 'q':
					{
						if (buffer->e and opt == '!')
							restore ();
						else if (!buffer->e)
							restore ();
						else
							std::cout << "!";
					}
				}
			}
			case 4: break;
			case 5: break;
		}
		o[0] = 0;
		o[1] = 0;
		o[2] = 0;
		confirm = 0;
		cmd = 0;
	}
	return 0;
}

void restore ()
{
	tcsetattr (0, TCSANOW, &preserve);
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
	char *x = new char[3];
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
	x[2] = '\0';
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
	char key = 0, hex[2] = {48, 48}, t[1];
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
		std::cout << sts.str ();
		sts << std::flush;
		while (1)
		{
			key = 0;
			while (key == 0)
			{
				if (!std::cin.get(key))
				{
					perror ("Read error");
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					hex[0] = key;
				}
				else if (key > 96 and key < 103)
				{
					hex[0] = key;
				}
				else if (key > 64 and key < 71)
				{
					key += 32;
					hex[0] = key;
				}
				else if (key == '.')
				{
					return buffer;
				}
				else
				{
					key = 0;
				}
			}
			key = 0;
			while (key == 0)
			{
				if (!std::cin.get (key))
				{
					perror ("Read error!");
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					hex[0] = key;
				}
				else if (key > 96 and key < 103)
				{
					hex[0] = key;
				}
				else if (key > 64 and key < 71)
				{
					key += 32;
					hex[0] = key;
				}
				else if (key == '.')
				{
					return buffer;
				}
				else
				{
					key = 0;
				}
			}
			t[0] = toC (hex);
			buffer.push_back (t[0]);
			hex_counter ++;
			if (hex_counter < 16)
			{
				std::cout << '-';
			}
			else
			{
				hex_counter = 0;
				word_counter ++;
				char p[1];
				std::cout << '|';
				for (int i = 0; i < 16; i ++)
				{
					p[0] = buffer.at(buffer.size () - 16 + i);
					if (p[0] > 31 and p[0] < 177)
						std::cout << p;
					else
						std::cout << '.';
				}
				std::cout << '\n';
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
				s << "EOF";
				for (int j = 0; j < 16 - i; j++)
					s << "   ";
				break;
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
				s << '-';
			}
		}
		s << '.';
	}
	else
	{
		s << "!";
	}
	//s << "\n";
	write (1, s.str ().c_str (), s.str ().size ());
}

void printo (int b)
{
	std::stringstream s;
	char h[2];
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
		std::cout << s.str ();
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				std::cout << toH (buffer->b[b + i]);
			}
			else
			{
				std::cout << "  ";
			}
			if (i < 16 - 1)
				std::cout << '-';
		}
		std::cout << '|';
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				if (buffer->b[b + i] > 31 and buffer->b[b + i] < 177)
					std::cout << buffer->b[b + i];
				else
					std::cout << ' ';
			}
			else
			{
				std::cout << ' ';
			}
		}
		std::cout << '.';
	}
	else
	{
		std::cout << "!";
	}
}

void execute (
	int *o,
	int ac,
	char cmd,
	char *opt,
	char *name,
	char *keyword,
	std::string *rx,
	std::string path,
	std::string *prx)
{
	if (ac == 0 and cmd == 0)
	{
		std::cout << "?";
		return;
	}
	else if (ac = 0 and cmd != 0)
	{
		switch (cmd)
		{
			case 'q':
			{
				if (buffer->e and opt[0] == '!')
					restore ();
				else if (!buffer->e)
					restore ();
				else
				{
					std::cout << "?";
					return;
				}
			}
		}
	}
	else
	{

	}
}
