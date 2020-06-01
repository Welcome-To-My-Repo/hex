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

//consolidates all the data needed for maintaining a file buffer
struct file_buffer
{
	std::string buffer;
	std::string pathname;
	bool edited = false;
	int current_offset;
};
termios editor, preserve;

void restore ();

int main (int argc, char **argv)
{
	int current_buffer, word_size = 8;
	file_buffer *buffer;	//points to the current file buffer
	std::vector<file_buffer> files;
	std::string clear_screen, commands, out;
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
		perror ("Couldn't set terminal attributes!");
		exit (1);
	}
//clear terminal window
	winsize ws;
	if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
	{
		perror ("Couldn't get window size!");
		exit (2);
	}
	if (ws.ws_col < 43 or ws.ws_row < 2)
	{
		perror ("Window is too small!");
		exit (3);
	}
	for (int i = 0; i < ws.ws_row; i ++)
		clear_screen.append ("\n");
	write (STDOUT_FILENO, clear_screen.c_str (), clear_screen.size ());
	while (1)
	{
		key[0] = '\0';
		if (read (STDIN_FILENO, key, 1) == -1
			and errno != EAGAIN)
		{
			perror ("Read error!");
			exit (5);
		}
		if (key[0] == 0)
		{
			continue;
		}
		if (key[0] == 10 or key[0] == 13)
		{
			write (STDOUT_FILENO, "\n", 1);

			switch (commands[0])
			{
				case 'a':
				{
					char hex[3], a[1];
					hex[2] = '\0';
					int counter = 0;
					bool app;
					while (app)
					{
						sts
							<< '\r'
							<< "0x"
							<< std::hex
							<< std::setw (8)
							<< std::left
							<< std::setfill ('0')
							<< buffer->current_offset
							<< '|'
							<< std::resetiosflags (std::ios_base::fmtflags ());
						for (int j = 0; j < word_size; j ++)
						{
							if (buffer->buffer.size () > 0)
							{
								if (buffer->current_offset + j < buffer->buffer.size ())
								{
									sts << std::hex << std::setw (2) << (int)buffer->buffer.at(buffer->current_offset + j);
								}
								else
								{
									sts << "  ";
								}
							}
							else
								sts << "  ";
							if (j < word_size - 1)
								sts << '-';
						}
						sts << '|';
						for (int j = 0; j < word_size; j ++)
						{
							if (buffer->buffer.size () > 0)
							{
								if (buffer->current_offset + j < buffer->buffer.size ())
								{
									if (buffer->buffer.at(buffer->current_offset + j) > 31 and buffer->buffer.at(buffer->current_offset + j) < 128)
										sts << buffer->buffer.at(buffer->current_offset + j);
									else
										sts << ' ';
								}
								else
								{
									sts << ' ';
								}
							}
							else
								sts << ' ';
						}
						write (STDOUT_FILENO, sts.str().c_str (), sts.str().size());
						key[0] == 0;
						while (key[0] == 0)
						{
							key[0] = 0;
							if (read (STDIN_FILENO, key, 1) == -1 and errno != EAGAIN)
							{
								perror ("Read error!");
								exit (5);
							}
							if ((key[0] > 47 and key[0] < 58) or (key[0] > 64 and key[0] < 71) or (key[0] > 96 and key[0] < 103))
							{
								hex[0] = key[0];
							}
							else if (key[0] == 46)
							{
								app = false;
							}
							else
							{
								key[0] = 0;
							}
						}
						while (key[0] == 0 and app)
                                                {
							key[0] = 0;
                                                        if (read (STDIN_FILENO, key, 1) == -1 and errno != EAGAIN)
                                                        {
                                                                perror ("Read error!");
                                                                exit (5);
                                                        }
                                                        if ((key[0] > 47 and key[0] < 58) or (key[0] > 64 and key[0] < 71) or (key[0] > 96 and key[0] < 103))
                                                        {
								hex[1] = key[0];
                                                        }
                                                        else
                                                        {
                                                                key[0] = 0;
                                                        }
                                                }
						if (app)
						{
							sscanf (hex, "%h", a);
							buffer->buffer.insert (buffer->current_offset + counter, a);
							counter ++;
						}
					}
					break;
				}
				case 'i':
				{
					break;
				}
				case 'c':
				{
					break;
				}
				case 'd':
				{
					break;
				}
				case 'm':
				{
					break;
				}
				case 'n':
				{
					break;
				}
				case 'r':
				{
					break;
				}
				case 'v':
				{
					if (commands.size () > 1)
					{
					}
					else
					{
						if (buffer->buffer.size () > 0)
						{
						}
						else
						{
							write (STDOUT_FILENO,
								"Buffer is empty!\n",
								sizeof ("BUFFER is empty!\n"));
						}
					}
					break;
				}
				case 'w':
				{
					break;
				}
				case 'q':
				{
					if (commands.size () > 1)
					{
						if (commands.at(1) == '!')
						{
							restore ();
						}
					}
					else if (!buffer->edited)
					{
						restore ();
					}
					else
					{
						write (STDOUT_FILENO,
							"Most recent changes haven't been saved!",
							sizeof ("Most recent changes haven't been saved!"));
					}
					break;
				}
				case 'y':
				{
					break;
				}
				case 'p':
				{
					break;
				}
				case 's':
				{
					if (commands.size () < 2)
					{
						write (STDOUT_FILENO,
							"Nothing to set...\n",
							sizeof ("Nothing to set...\n"));
					}
					else
					{
					}
					break;
				}
				default:
				{
					if (commands.size () == 0)
					{
						buffer->current_offset + word_size;
					}
				}
			}
			commands.clear ();
		}
		else if (key[0] == 127 or key[0] == 8)
		{
			commands.pop_back ();
		}
		else
		{
			commands.push_back (key[0]);
			write (STDOUT_FILENO, key, 1);
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
	printf ("\n");
	exit (0);
}
