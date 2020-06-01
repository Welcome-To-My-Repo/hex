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
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>

//consolidates all the data needed for maintaining a file buffer
struct file_buffer
{
	std::string buffer;
	std::string pathname;
	bool edited = false;
};

int main (int argc, char **argv)
{
	int current_offset, current_buffer, word_size;
	file_buffer *buffer;	//points to the current file buffer
	std::vector<file_buffer> files;
	char commands[3][8];
	std::string clear_screen;
	std::fstream fs;
	std::stringstream sts;
	
	if (argc > 1)
	{
		for (int i = 0; i < argc - 1; i ++)
		{
			files.emplace_back ();
			files.back ().pathname = argv[i];
			fs.open (argv[i], std::ios_base::binary | std::ios_base::in);
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
			
	return 0;
}
