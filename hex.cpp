#include <cstdio>
#include <cstdlib>
#include <string>
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

//consolidates all the data needed for maintaining a file buffer
struct file_buffer
{
	std::string buffer;
	std::string pathname;
	bool edited = false;
	int current_offset = 0;
};
termios editor, preserve;
int word_size = 8;

void restore ();
/**
* getText grabs new words from the user for the commands a, i, and c.
*/
std::string getText ();

int main (int argc, char **argv)
{
	int current_buffer;
	file_buffer *buffer;	//points to the current file buffer
	std::vector<file_buffer> files;
	std::string tmp, commands, out;
	char key[1];
	std::fstream fs;
	std::stringstream sts;
//get file arguments
	if (argc > 1)
	{
		for (int i = 0; i < argc - 1; i ++)
		{
			files.emplace_back ();
			files.back ().pathname = argv[i];
			fs.open (argv[i],
				std::ios_base::binary | std::ios_base::in);
			if (fs.is_open ())
			{
				sts << fs.rdbuf ();
				fs.close ();
				files.back ().buffer.assign (sts.str ());
			}
			else
			{
				fs.close ();
				files.back ().buffer.clear ();
			}

		}
	}
	else
	{
		files.emplace_back ();
	}
	current_buffer = 0;
	buffer = &files.at(current_buffer);
//initialize terminal "raw mode"
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
//clear terminal window
	winsize ws;
	if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
	{
		perror (nowinsize);
		exit (2);
	}
	if (ws.ws_col < 43 or ws.ws_row < 2)
	{
		perror (small);
		exit (3);
	}
	for (int i = 0; i < ws.ws_row; i ++)
		tmp.append ("\n");
	write (STDOUT_FILENO, tmp.c_str (), tmp.size ());
	write (STDOUT_FILENO, ":", 1);
	while (1)
	{
		key[0] = '\0';
		if (read (STDIN_FILENO, key, 1) == -1
			and errno != EAGAIN)
		{
			perror (rerr);
			exit (5);
		}
		if (key[0] == 0 or key[0] == 27)
		{
			continue;
		}
		if (key[0] == 10 or key[0] == 13)
		{
			write (STDOUT_FILENO, "\n", 1);
			if (commands.size () > 0)
			{
				switch (commands[0])
				{
					case 's':
						break;
					case 'w':
						break;
					case 'q':
					{
						if (commands[1] == '!' or !buffer->edited)
							restore ();
						else
						{
							write (STDOUT_FILENO, unsaved, sizeof (unsaved));
						}
						break;
					}
					case 'n':
						break;
					case 'r':
						break;
					case 'a':
					{
						buffer->edited = true;
						buffer->buffer.insert (buffer->current_offset, getText ());
						break;
					}
					case 'i':
					{
						buffer->edited = true;
						buffer->buffer.insert (buffer->current_offset - word_size, getText ());
						break;
					}
					case 'c':
					{
						buffer->edited = true;
						tmp = getText ();
						for (int i = 0; i < word_size; i ++)
						{
							buffer->buffer.at(i) = tmp.at(i);
						}
						buffer->buffer.insert (buffer->current_offset + word_size, tmp.substr (8, std::string::npos));
						break;
					}
					case 'd':
						break;
					case 'm':
						break;
					case 'v':
						break;
					case 'y':
						break;
					case 'p':
						break;
					default:
					{

					}
				}
			}
			else
			{
				if (buffer->buffer.size () > 0)
				{
					if (buffer->current_offset < buffer->buffer.size ())
						buffer->current_offset ++;
					if (buffer->current_offset < buffer->buffer.size ())
					{
						tmp.assign ( buffer->buffer.substr (
							buffer->current_offset,
							buffer->current_offset + word_size
						));
					}
					else
					{
						write (STDOUT_FILENO, eof, sizeof (eof));
					}
				}
				else
				{
					write (STDOUT_FILENO, empty, sizeof (empty));
				}
			}
			commands.clear ();
			write (STDOUT_FILENO, "\n:", 2);
		}
		else if (key[0] == 127 or key[0] == 8)
		{
			commands.pop_back ();
			write (STDOUT_FILENO, "\r", 1);
			write (STDOUT_FILENO, commands.c_str (), commands.size ());
		}
		else
		{
			commands.push_back (key[0]);
			write (STDOUT_FILENO, "\r", 1);
			write (STDOUT_FILENO, commands.c_str (), commands.size ());
		}
	}
	return 0;
}

void restore ()
{
	if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &preserve) == -1)
	{
		perror ("Couldn't reset terminal!");
		exit (4);
	}
	//printf ("\n");
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
