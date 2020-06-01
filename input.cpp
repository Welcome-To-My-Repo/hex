#include "hex.h"

std::string input ()
{
	char key[1] = {0};
	std::string commands;
	write (STDOUT_FILENO, ":", 1);
	while (1)
	{
		if (read (STDIN_FILENO, key, 1) == -1 and errno != EAGAIN)
		{
			perror (rerr);
			exit (5);
		}
		if (key[0] == 0)
			continue;
		else if (key[0] == 10 or key[0] == 13)
		{
			if (commands.size () > 0)
				return commands;
		}
		else if (key[0] == 127 or key[0] == 8)
		{
			if (commands.size () > 0)
			{
				commands.pop_back ();
				write (STDOUT_FILENO, "\r\x1b[2K:", 6);
				write (STDOUT_FILENO, commands.c_str (), commands.size ());
			}
		}
		else
		{
			commands.push_back (key[0]);
			write (STDOUT_FILENO, "\r\x1b[2K:", 6);
			write (STDOUT_FILENO, commands.c_str (), commands.size ());
		}
	}
}

bool iscmd (std::string s)
{
	for (int i = 0; i < s.size (); i ++)
	{
		if (s[i] < 65 and s[i] > 90 and s[i] < 97 and s[i] > 122)
			return false;
	}
	return true;
}

bool isadd (std::string s)
{
	for (int i = 0; i < s.size (); i ++)
	{
		if ((s[i] < 48 and s[i] > 57) and s[i] != 88 and s[i] != 120)
			return false;
	}
	return true;
}
