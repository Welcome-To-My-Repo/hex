#include "hex.h"

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
