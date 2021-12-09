/*
 * Hex is a binary file editor based on the POSIX text editor ed.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define H1 1
#define H2 16
#define H3 256
#define H4 4096
#define H5 65536
#define H6 1048576
#define H7 16777216
#define H8 268435456

#define HELP_STRING \
"\
Hex Usage:\n\
  hex [-h file]\n\
Addressing:\n\
  \".\" The current offset\n\
  \"n\" The nth byte of the file\n\
  \"+/-n\" An offset relative to the current offset\n\
  \"/pat\" Search forwards for a series of bytes\n\
  \"\\pat\" Search backwards for a series of bytes\n\
Commands:\n\
  (.,.) * -- edit\n\
  (.,.) ^ (.) -- move\n\
  (.,.) / [buffer] -- delete\n\
  (.,.) = [buffer] -- copy\n\
  (.) & [buffer] -- put\n\
  (.) @ [mark] -- mark\n\
  ! -- quit\n\
  # -- write\n\
  _ -- undo\n\
  ? [pathname]-- file information\n\
"

unsigned int word_size = 8; //how many byts to print in a row

typedef struct u
{
	signed char op; //type of operation, one of 'e', 'm', 'd', 'p'
	unsigned long long int
		remove_offset, //undo by removing
	 	remove_length,
		replace_offset, //undo by inserting
		replace_length;
	unsigned char* buf; //bytes
	struct u *previous; //location of previous undo information
} UNDO;

typedef struct BUFFER //it's a.... buffer!
{
	unsigned char
		*buf, //holds file contents
		dirty, //non-zero if buffer is changed
		clipboard[16]; //buffers for copied/deleted strings

	unsigned long long int
		length, //length of buffer
		offset, //position in buffer
		flags[16]; //marked positions in buffer

	char* pathname; //holds path to file
	UNDO history[256]; //linked list of operations for undo
	unsigned char history_step;
//hint hint the buffer is a struct so I can extend hex to support multiple files if that's needed at some point
} BUFFER; //the name of a buffer is... "buffer"!

BUFFER THE_FILE; //the file buffer

unsigned char* VB; //volatile buffer for cuts and deletes

struct termios ui, restore; //terminal states
struct winsize w; //holds data about the terminal window

char* command; //string of commands
char* tmp; //temporary buffer for random things

int show (BUFFER* b, unsigned long long int p); //prints a "word" (see word_size)
void print_help (); //prints a help string to std output

int main (int argc, char** argv)
{

/* Parse Arguments ************************************************************/
	if (argc > 1)
	{
		if (argv[1][0] == '-')
		{
			switch(argv[1][1])
			{
				case 'h':
				{
					write(1, HELP_STRING, sizeof(HELP_STRING));
					return 0;
				}
				default:
				{
					write(1, HELP_STRING, sizeof(HELP_STRING));
					return 0;
				}
			}

		}
		int f = open(argv[1], O_RDONLY); //open file
		THE_FILE.length = lseek(f, 0, SEEK_END); //get length
		lseek(f, 0, SEEK_SET);
		THE_FILE.buf = malloc(sizeof(char) * THE_FILE.length); //allocate buffere
		if (read(f, THE_FILE.buf, THE_FILE.length) == -1) //fill buffer
		{
			free(THE_FILE.buf); //undo allocation on read failure
			THE_FILE.length = 0;
		}
		close(f); //guess...
	}
	else THE_FILE.length = 0;

/* Set Up Terminal Mode *******************************************************/

	tcgetattr (0, &restore);
	ui = restore; //save the original terminal mode inside 'restore'
	//set a whole bunch of flags below, "man termios" if you're curious
	ui.c_iflag  &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	ui.c_lflag  &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	ui.c_cflag &= ~(CSIZE | PARENB);
	ui.c_cflag |= CS8;

	tcsetattr (0, TCSANOW, &ui); //set the new terminal mode to 'ui'

	ioctl (0, TIOCGWINSZ, &w); //get information about terminal size

/* Get Input ******************************************************************/

	char c;//used to get characters from input
	unsigned int l;

	GET_COMMAND:

	free(command);
	l = 0;
	while(read(0, &c, 1) > 0)
	{
		if (c == 0 || c == 27)
		{
			continue;
		}
		else if (c == 8 || c == 127)
		{
			if (l > 0)
			{
				l--;
				command = realloc(command, l);
				write(1, "\x1b[2K\r", 5);
				write(1, command, l);
			}
		}
		else if (c == 10 || c == 13 || c == ';')
		{
			l++;
			command = realloc(command, l);
			command[l-1] = c;
			break;
		}
		else
		{
			l++;
			command = realloc(command, l);
			command[l-1] = c;
			write(1, &c, 1);
		}
		c = 0;
	}

/* Interpret commands *********************************************************/

	INTERPRET_COMMAND:
	if (l <= 0) goto INVALID_CCOMMAND;

	switch (command[0])
	{
		case '*':
		{
			break;
		}
		case '^':
		{
			break;
		}
		case '/':
		{
			break;
		}
		case '=':
		{
			break;
		}
		case '&':
		{
			break;
		}
		case '@':
		{
			break;
		}
		case '!':
		{
			goto QUIT;
			break;
		}
		case '#':
		{
			break;
		}
		case '-':
		{
			break;
		}
		case '?':
		{
			break;
		}
	}
	INVALID_CCOMMAND:
		write(1, "\n\r?\n\r", 5);
		goto GET_COMMAND;

	QUIT:
	{
		write(1, "\n\r", 2);
		tcsetattr(0, TCSANOW, &restore);
		return 0;
	}
}
