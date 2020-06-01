#include "hex.h"

//consolidates all the data needed for maintaining a file buffer

termios editor, preserve;
//size in bytes of each "line" to be edited/displayed
int word_size = 8;
//pointer to the current file buffer
file_buffer *buffer;
//list of files given in arguments
std::vector<file_buffer> files;
//list of word strings set by yank or delete
std::vector<clip> clipboard;
//volatile word string for yank or delete when no name is specified
std::string v;

int main (int argc, char **argv)
{
	int current_buffer;
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
			files.back ().p = argv[i];
			fs.open (argv[i],
				std::ios_base::binary | std::ios_base::in);
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
		commands.assign (input ());
		cmd (commands);
		write (STDOUT_FILENO, "\n:", 2);
	}
	return 0;
}
