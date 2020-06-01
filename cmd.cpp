#include "hex.h"

void cmd (std::string s)
{
	std::string b;
	std::vector <std::string> c;
	std::stringstream t;
	t.str (s);
	while (t >> b)
	{
		c.push_back (b);
	}
	if (c[0][0] == ".")
	{
		
	}
	else if (c[0][0] == '0')
	{

	}
	else
	{
		switch (c[0][0])
		{
			case 'a':
			{

			}
			case 'c':
			{

			}
			case 'd':
			{

			}
			case 'f':
			{

			}
			case 'i':
			{

			}
			case 'm':
			{

			}
			case 'n':
			{

			}
			case 'p':
			{

			}
			case 'q':
			{

			}
			case 'r':
			{

			}
			case 's':
			{

			}
			case 'u':
			{

			}
			case 'v':
			{

			}
			case 'w':
			{

			}
			case 'y':
			{

			}
			default:
			{

			}
		}
	}
}
