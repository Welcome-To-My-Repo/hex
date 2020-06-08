#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <charconv>
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
	//buffer
	std::string b;
	//pathname
	std::string p;
	//edited flag
	bool e = false;
	//offset
	int o = 0;
	//list of marked addresses
	std::vector <mark>m;
};
struct clip
{
	//buffer
	std::string b;
	//buffer name
	char *n;
};
std::vector<clip>clips;
std::string vol;
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
//function to add bytes to buffer
std::string getText ();
//function to convert hexadecimal string to character value
char toC (char *h);
//function to convert character to hexadecimal string
char *toH (unsigned char c);
//function to print an offset
void printo ();
//overload of printo for printing a buffer that's not the current buffer
void printo (int b);
void printo (int a, int b);
//convert hex address to decimal
int htoa (char *h);
//refegex find
int rfind (std::string r);
//regex replace
void rrplace (std::string r);
//returns a string of input
std::string gin ();

//data for undo command
std::string ub; //holds buffer of last bytes added
std::string udb; //holds buffer of last bytes removed
int uo, ul; //holds the (o)ffset the last command acted on and the (l)ength of bytes
bool ua; //true if the last operation added bytes, false if bytes were removed

int values [8] = {268435456,16777216,1048576,65536,4096,256,16,1};

int main (int argc, char **argv)
{
	int current_buffer = 0, pos;
	std::string tmp;
	char g;
	std::fstream fs;
	std::stringstream sts, t;
	winsize w;

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
	ui.c_iflag  &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	//ui.c_oflag &= ~OPOST;
	ui.c_lflag  &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	ui.c_cflag &= ~(CSIZE | PARENB);
	ui.c_cflag |= CS8;
	tcsetattr (0, TCSANOW, &ui);

	ioctl (0, TIOCGWINSZ, &w);
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
	* 9: aggregate pathname
	*/
	bool init = true;
	int *o = new int[3] {0,0,0}, //three possible offsets in a command
		ac = 0, //number of offsets in the command
		rc = 0, //number of regular expressions
		s = 0, //state identifier
		count = 0, //general purpose counter
		confirm = 0;
		/*
		 * confirm
		 * first address +1
		 * second address +1
		 * pathname +1
		 * command +3
		 * name +6
		 * option +12
		 * third address +12
		*/
	char cmd = 0, //holds command character
		opt = 0, //holds option character
		*a = new char [8], //holds hex offset
		*name = new char[4], //holds name
		key = 0, //used for switch
		*keyword = new char[4], //used for keyword commands
		*mk = new char[4]; //used for marked addresses
	std::string *rx = new std::string[2], //holds regular expressions
		path, //holds pathname
		*prx = new std::string[2]; //holds previous regular expressions
	bool addr = false, succ = false, r = true;
	std::stringstream out;
	sts.str ("");
	sts.clear ();
	sts << DEFAULT;
	write (1, sts.str ().c_str (), sts.str ().size ());
	while (1)
	{
		t.str (gin ());

		while (t.get(key))
		{
			if (key == ' ' or key == '\t')
				continue;
			switch (s)
			{
				case -1:
				{
					confirm = -1;
					switch (key)
					{
						case '\n':
						case '\r':
						case '|':
						{
							s = 0;
						}
					}
					break;
				}
				case 0: //init
				{
					init = false;
					switch (key)
					{
						case '-':
						case '+':
						{
							cmd = key;
							confirm += 3;
							s = 0;
							break;
						}
						case 'q':
						{
							cmd = key;
							confirm += 3;
							s = 5;
							break;
						}
						case 'f':
						{
							cmd = key;
							confirm += 3;
							s = 9;
							break;
						}
						case 'a':
						case 'i':
						case 'c':
						{
							cmd = key;
							confirm += 3;
							break;
						}
						case 'd':
						{
							cmd = key;
							confirm += 3;
							count = 0;
							s = 3;
							break;
						}
						case '[':
						{
							s = 1;
							count = 0;
							break;
						}
						case '.':
						{
							s = 1;
							if (ac < 4)
							{
								o[ac] = buffer->o;
								confirm += 1;
								ac ++;
							}
							else
							{
								s = -1;
							}
							break;
						}
						case '$':
						{
							s = 1;
							if (ac < 4)
							{
								o[ac] = buffer->b.size () - 1;
								confirm += 1;
								ac ++;
							}
							else
							{
								s = -1;
							}
							break;
						}
						case '\'':
						{
							s = 4;
							count = 0;
							break;
						}
						case '\n':
						case '\r':
						case '|':
						{
							break;
						}
						default:
						{
							s = -1;
							break;
						}
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
						{
							if (ac < 2)
							{
								if (count < 8)
								{
									a[count] = key;
									count ++;
								}
								else
								{
									s = -1;
								}
							}
							else if (ac > 1 and confirm > 2)
							{
								a[count] = key;
								count ++;
							}
							else
							{
								s = -1;
							}
							break;
						}
						case ']':
						{
							if (count == 0)
							{
								s = -1;
								break;
							}
							int tmp = 7;
							o[ac] ++;
							for (int i = count - 1; i > -1; i --)
							{
								if (a[i] > 57)
								{
									o[ac] += (a[i] - 87) * values[tmp];
								}
								else
								{
									o[ac] += (a[i] - 48) * values[tmp];
								}
								tmp --;
							}
							if (o[ac] < buffer->b.size ())
							{
								ac ++;
								confirm += 1;
								s = 0;
							}
							else
							{
								s = -1;
							}
							break;
						}
						case '[':
						{
							s = -1;
							break;
						}
						case '\n':
						case '\r':
						case '|':
						{
							s = -1;
							break;
						}
					}
					break;
				}
				case 3:
				{
					switch (key)
					{
						case '\n':
						case '\r':
						case '|':
						{
							if (count < 4)
								s = -1;
							break;
						}
						default:
						{
							if (count < 4)
							{
								mk[count] = key;
								count ++;
								break;
							}
						}

					}
					break;
				}
				case 4:
				{
					switch (key)
					{
						case '\n':
						case '\r':
						case '|':
						{
							if (count < 4)
								s = -1;
							break;
						}
						default:
						{
							if (count < 4)
							{
								name[count] = key;
								count ++;
							}
							else
							{
								for (int i = 0; i < buffer->m.size (); i ++)
								{
									if (0 == strncmp (
										name,
										buffer->m.at(i).n,
										4
									))
									{
										o[ac] = buffer->m.at(i).o;
										ac ++;
										break;
									}
								}
							}
						}
					}
					break;
				}
				case 5: //gets option character
				{
					switch (key)
					{
						case 'g': break;
						case 'c': break;
						case '!':
						{
							opt = key;
							break;
						}
						case '\n':
						case '\r':
						case '|':
						{
							break;
						}
						default:
						{
							s = -1;
							break;
						}
					}
					break;
				}
				case 9: //get pathname
				{
					switch (key)
					{
						case '\r':
						case '\n':
						case '|':
						{
							break;
						}
						default:
						{
							path.push_back (key);
							break;
						}
					}
					break;
				}
			}
		}
		t.str ("");
		t.clear ();
		write (1, "\r\x1b[2K", 5);
		switch (confirm)
		{
			case -1:
			{
				write (1, "?", 1);
				break;
			}
			case 0:
			{
				if (buffer->o < buffer->b.size ())
				{
					buffer->o ++;
					printo();
				}
				else
				{
					write (1, "!", 1);
				}

				break;
			}
			case 1:
			{
				buffer->o = o[0];
				printo (o[0]);
				break;
			}
			case 2:
			{
				buffer->o = o[1];
				printo (o[0], o[1]);
				break;
			}
			case 3:
			{
				switch (cmd)
				{
					case '-':
					{
						if (buffer->o > 0)
						{
							buffer->o --;
							printo ();
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
					case '+':
					{
						if (buffer->o < buffer->b.size ())
						{
							buffer->o ++;
							printo ();
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
					case 'q':
					{
						if (buffer->e and opt == '!')
							restore ();
						else if (!buffer->e)
							restore ();
						else
							write (1, "!", 1);
						break;
					}
					case 'f':
					{
						if (path.size () > 0)
							buffer->p = path;
						sts.str ("");
						sts.clear ();
						if (buffer->p.size () > 0)
							sts << buffer->p;
						else
							sts << WARNING << "No File" << DEFAULT;
						sts << " ";
						if (buffer->e)
							sts << WARNING << "modified";
						else
							sts << OKAY << "unmodified";
						sts << DEFAULT << " " << std::hex
							<< buffer->o
							<< "/"
							<< buffer->b.size ()
							<< "(%";
						if (buffer->b.size () > 0)
							sts << (int)(buffer->o * 100) / buffer->b.size ();
						else
							sts << 0;
						sts	<< ")";
						write (1, sts.str ().c_str (), sts.str ().size ());
						break;
					}
					case 'a':
					{
						buffer->e = true;
						tmp = getText ();
						if (buffer->b.size () == 0)
						{
							buffer->b.assign(tmp);
						}
						else if (buffer->o + 1 == buffer->b.size ())
						{
							buffer->b.append (tmp);
						}
						else
						{
							buffer->b.insert (buffer->o + 1, tmp);
						}
						ub = tmp;
						udb = "";
						uo = buffer->o;
						ul = tmp.size ();
						ua = true;
						break;
					}
					case 'i':
					{
						buffer->e = true;
						tmp = getText ();
						if (buffer->b.size () == 0)
							buffer->b.assign (tmp);
						else
							buffer->b.insert (buffer->o, tmp);
						ub = tmp;
						udb = "";
						uo = buffer->o;
						ul = tmp.size ();
						ua = true;
						break;
					}
					case 'c':
					{
						buffer->e = true;
						tmp = getText ();
						if (buffer->b.size () == 0)
							buffer->b.assign(tmp);
						else
						{
							if (tmp.size () > 0)
							{
								udb.clear ();
								udb.push_back (buffer->b.at(buffer->o));
								buffer->b.at(buffer->o) = tmp.at(0);
								buffer->b.insert (buffer->o + 1, tmp.substr (1, std::string::npos));
							}
						}
						ub = tmp;
						ul = tmp.size ();
						uo = buffer->o;
						ua = true;
						break;
					}
					case 'd':
					{
						if (buffer->b.size () > 0)
						{
							buffer->e = true;
							if (name[0] != 0)
							{
								clips.push_back  (clip {buffer->b.substr (buffer->o, buffer->o + 1), name});
							}
							else
							{
								vol = buffer->b.substr (buffer->o, buffer->o + 1);
							}
							buffer->b.erase (buffer->o, 1);
							if (!(buffer->o < buffer->b.size ()))
								buffer->o = buffer->b.size () - 1;
						}
						else
							write (1, "?", 1);
						break;
					}
					default:
					{
						write (1, "?", 1);
					}
				}
				break;
			}
			case 4:
			{
				if (o[0] == -1)
				{
					write (1, "?", 1);
					break;
				}
				switch (cmd)
				{
					case '-':
					{
						if (buffer->o - o[0] >= 0)
						{
							buffer->o -= o[0];
							printo ();
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
					case '+':
					{
						if (buffer->o + o[0]< buffer->b.size ())
						{
							buffer->o += o[0];
							printo ();
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
					case 'a':
					{
						if (buffer->b.size () == 0)
						{
							buffer->b.assign(getText ());
						}
						else if (o[0] + 1 == buffer->b.size ())
						{
							buffer->b.append (getText ());
						}
						else
						{
							buffer->b.insert (o[0] + 1, getText ());
						}
						buffer->o = o[0];
						break;
					}
					case 'i':
					{
						buffer->e = true;
						if (buffer->b.size () == 0)
							buffer->b.assign (getText ());
						else if (o[0] == buffer->b.size ())
							buffer->b.append (getText ());
						else
							buffer->b.insert (o[0], getText ());
						break;
					}
					case 'c':
					{
						buffer->e = true;
						if (buffer->b.size () == 0)
							buffer->b.assign(getText ());
						else
						{
							tmp = getText ();
							if (tmp.size () > 0)
							{
								buffer->b.at(o[0]) = tmp.at(0);
								buffer->b.insert (o[0] + 1, tmp.substr (1, std::string::npos));
							}
						}
						break;
					}
					case 'd':
					{
						if (buffer->b.size () > 0)
						{
							buffer->e = true;
							if (name[0] != 0)
							{
								clips.push_back  (clip {buffer->b.substr (o[0], o[0] + 1), name});
							}
							else
							{
								vol = buffer->b.substr (o[0], o[0] + 1);
							}
							buffer->b.erase (o[0], 1);
							if (!(buffer->o < buffer->b.size ()))
								buffer->o = buffer->b.size () - 1;
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
				}
			}
			case 5:
			{
				if (o[0] == -1 or o[1] == -1)
				{
					write (1, "?", 1);
					break;
				}
				if (!(o[0] < o[1]))
				{
					write (1, "?", 1);
					break;
				}
				switch (cmd)
				{
					case 'c':
					{
						buffer->e = true;
						if (buffer->b.size () == 0)
							buffer->b.assign(getText ());
						else
						{
							tmp = getText ();
							if (tmp.size () > 0)
								buffer->b.replace (o[0], o[1], tmp);
						}
						break;
					}
					case 'd':
					{
						if (buffer->b.size () > 0)
						{
							buffer->e = true;
							if (name[0] != 0)
							{
								clips.push_back  (clip {buffer->b.substr (o[0], o[1]), name});
							}
							else
							{
								vol = buffer->b.substr (o[0], o[1] + 1);
							}
							buffer->b.erase (o[0], o[1]);
							if (!(buffer->o < buffer->b.size ()))
								buffer->o = buffer->b.size () - (o[1] - o[0]);
						}
						else
						{
							write (1, "!", 1);
						}
						break;
					}
				}
			}
		}
		write (1, "\n", 1);
		delete[] o;
		o = new int [3] {-1,-1,-1};
		ac = 0;
		rc = 0;
		s = 0;
		count = 0;
		confirm = 0;
		cmd = 0;
		opt = 0;
		delete[] a;
		a = new char[8] {0,0,0,0,0,0,0,0};
		delete[] name;
		name = new char[4] {0,0,0,0};
		key = 0;
		delete[] keyword;
		keyword = new char[4] {0,0,0,0};
		delete[] rx;
		rx = new std::string[2];
		path.clear ();
		init = true;

	}
	return 0;
}

void restore ()
{
	tcsetattr (0, TCSANOW, &preserve);
	write (1, "\n", 1);
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

char * toH (unsigned char c)
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

//16^7 = 268435456
//16^6 = 16777216
//16^5 = 1048576
//16^4 = 65536
//16^3 = 4096
//16^2 = 256
//16^1 = 16
//16^0 = 1

std::string gin ()
{
	std::string in;
	char key = 0;
	while (read (0, &key, 1) != -1)
	{
		if (key == 0)
		{
			continue;
		}
		else if (key == 27)
		{
			continue;
		}
		else if (key == 8 or key == 127)
		{
			if (in.size () > 0)
				in.pop_back ();
			write (1, "\r\x1b[2K", 5);
			write (1, in.c_str (), in.size ());
		}
		else if (key == 10 or key == 13 or key == '|')
		{
			in.push_back (key);
			break;
		}
		else
		{
			in.push_back (key);
			write (1, &key, 1);
		}
		key = 0;
	}
	return in;
}

std::string getText ()
{
	std::string buffer;
	int word_counter, hex_counter = 0;
	std::stringstream sts;
	char key = 0, hex[2] = {48, 48};
	unsigned char t[1];
	while (1)
	{
		sts.str ("");
		sts
			<< std::hex
			<< OFFSET
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< word_counter
			<< DEFAULT
			<< '|';
		write (1, sts.str ().c_str (), sts.str ().size ());
		sts.clear ();
		sts.str ("");
		while (1)
		{
			key = 0;
			while (key == 0)
			{
				if (read (0, &key, 1) == -1)
				{
					write (1, &key, 1);
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					write (1, &key, 1);
					hex[0] = key;
				}
				else if (key > 96 and key < 103)
				{
					write (1, &key, 1);
					hex[0] = key;
				}
				else if (key > 64 and key < 71)
				{
					write (1, &key, 1);
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
				if (read (0, &key, 1) == -1)
				{
					write (1, &key, 1);
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					write (1, &key, 1);
					hex[1] = key;
				}
				else if (key > 96 and key < 103)
				{
					write (1, &key, 1);
					hex[1] = key;
				}
				else if (key > 64 and key < 71)
				{
					write (1, &key, 1);
					key += 32;
					hex[1] = key;
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
			sts.clear ();
			sts.str ("");
			t[0] = toC (hex);
			buffer.push_back (t[0]);
			hex_counter ++;
			if (hex_counter < WORD_SIZE)
			{
				write (1, "-", 1);
			}
			else
			{
				hex_counter = 0;
				word_counter ++;
				char p[1];
				sts << '|';
				for (int i = 0; i < WORD_SIZE; i ++)
				{
					p[0] = buffer.at(buffer.size () - WORD_SIZE + i);
					if (p[0] > 31 and p[0] < 127)
						sts << p[0];
					else
						sts << INVISIBLE << '?' << DEFAULT;
				}
				sts << "\n";
				write (1, sts.str ().c_str (), sts.str ().size ());
				break;
			}
		}
	}
}

void printo ()
{
	std::stringstream s;
	s.str ("");
	int start, byte;
	if (buffer->b.size () > 0 and buffer->o < buffer->b.size ())
	{
		start = (buffer->o / WORD_SIZE) * WORD_SIZE;
		byte = buffer->o % WORD_SIZE;
		s
			<< OFFSET
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< buffer->o
			<< DEFAULT
			<< '|';
		for (int i = 0; i < WORD_SIZE; i ++)
		{
			if (start + i < buffer->b.size ())
			{
				if (i == byte)
					s << BYTE;
				s << toH (buffer->b[start + i]);
				if (i == byte)
					s << DEFAULT;
				if (i < WORD_SIZE - 1)
					s << '-';
			}
			else
			{
				s << EOF_MARK << "EOF";
				for (int j = 0; j < WORD_SIZE - i - 2; j++)
					s << "   ";
				s << "  " << DEFAULT;
				break;
			}
		}
		s << '|';
		for (int i = 0; i < WORD_SIZE; i ++)
		{
			if (start + i < buffer->b.size ())
			{
				if (i == byte)
					s << BYTE;
				if (buffer->b[start + i] > 31 and buffer->b[start + i] < 127)
				{
					s << buffer->b[start + i];
				}
				else
				{
					s << INVISIBLE << '?' << DEFAULT;
				}
				if (i == byte)
					s << DEFAULT;
			}
			else
			{
				s << EOF_MARK << "~" << DEFAULT;
			}
		}
		s << '|';
	}
	else
	{
		s << "!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}

void printo (int b)
{
	std::stringstream s;
	s.str ("");
	int start, byte;
	if (buffer->b.size () > 0 and b < buffer->b.size ())
	{
		start = (b / WORD_SIZE) * WORD_SIZE;
		byte = b % WORD_SIZE;
		s
			<< OFFSET
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< buffer->o
			<< DEFAULT
			<< '|';
		for (int i = 0; i < WORD_SIZE; i ++)
		{
			if (start + i < buffer->b.size ())
			{
				if (i == byte)
					s << BYTE;
				s << toH (buffer->b[start + i]);
				if (i == byte)
					s << DEFAULT;
				if (i < WORD_SIZE - 1)
					s << '-';
			}
			else
			{
				s << EOF_MARK << "EOF";
				for (int j = 0; j < WORD_SIZE - i - 2; j++)
					s << "   ";
				s << "  " << DEFAULT;
				break;
			}
		}
		s << '|';
		for (int i = 0; i < WORD_SIZE; i ++)
		{
			if (start + i < buffer->b.size ())
			{
				if (i == byte)
					s << BYTE;
				if (buffer->b[start + i] > 31 and buffer->b[start + i] < 127)
				{
					s << buffer->b[start + i];
				}
				else
				{
					s << INVISIBLE << '?' << DEFAULT;
				}
				if (i == byte)
					s << DEFAULT;
			}
			else
			{
				s << EOF_MARK << "~" << DEFAULT;
			}
		}
		s << '|';
	}
	else
	{
		s << "!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}

void printo (int a, int b)
{
	std::stringstream s;
	s.str ("");
	int word;
	int start, byte;
	if (buffer->b.size () > 0 and a < b and a < buffer->b.size () and b < buffer->b.size ())
	{
		for (int pos = a; pos < b + 1; pos += 16)
		{
			start = (pos / 16) * 16;
			byte = pos % 16;
			s
				<< OFFSET
				<< std::hex
				<< std::setw (8)
				<< std::right
				<< std::setfill ('0')
				<< buffer->o
				<< DEFAULT
				<< '|';
			for (int i = 0; i < WORD_SIZE; i ++)
			{
				if (start + i < buffer->b.size ())
				{
					if (i == byte)
						s << BYTE;
					s << toH (buffer->b[start + i]);
					if (i == byte)
						s << DEFAULT;
					if (i < WORD_SIZE - 1)
						s << '-';
				}
				else
				{
					s << EOF_MARK << "EOF";
					for (int j = 0; j < WORD_SIZE - i - 2; j++)
						s << "   ";
					s << "  " << DEFAULT;
					break;
				}
			}
			s << '|';
			for (int i = 0; i < WORD_SIZE; i ++)
			{
				if (start + i < buffer->b.size ())
				{
					if (i == byte)
						s << BYTE;
					if (buffer->b[start + i] > 31 and buffer->b[start + i] < 127)
					{
						s << buffer->b[start + i];
					}
					else
					{
						s << INVISIBLE << '?' << DEFAULT;
					}
					if (i == byte)
						s << DEFAULT;
				}
				else
				{
					s << EOF_MARK << "~" << DEFAULT;
				}
			}
			s << '|';
			if (pos < b)
				s << "\r\n";
		}
	}
	else
	{
		s << "!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}
