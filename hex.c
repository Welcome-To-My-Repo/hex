/*
 * Hex is a binary file editor based on the POSIX text editor ex.
 ******************************************************************************

ex [file] [-c commands] -R

Command Addressing:

 .  The current offset
 n  The nth byte
 +n,-n An offset relative to the current offset
 /pat  search forwards for a pattern of bytes
 \pat  search backwards for a pattern of bytes

Commands:

  (.,.) edit, e
  (.,.) move, m (.)
  (.,.) delete, d [buffer]
  (.,.) copy, c [buffer]
  (.) put, p [buffer]
  (.) flag, f
  >  next buffer
  <  previous buffer
  quit, q
  quit!, q!
  write, w
  undo, u

  multiple commands can be separated with a semicolon
 */
#include <termios.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>

#include "configuration.h"

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

} BUFFER; //the name of a buffer is... "buffer"!

unsigned char* VB; //volatile buffer for cuts and deletes
BUFFER* THE_FILES; //array of file buffers
unsigned int NUMBER_OF_FILES = 0;
BUFFER* buffer; //which file buffer is active
UNDO *history; //linked list of operations for undo
unsigned char history_length;

int show (BUFFER* b, unsigned long long int p); //prints a "word" (see word_size)

termios ui, preserve;

int main (int argc, char** argv)
{
	/*READ IN FILES*/

	/*LOOP GETTING COMMANDS*/
	GET:
	
	QUIT:
	return 0;
}
