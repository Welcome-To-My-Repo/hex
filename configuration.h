/*
 * This header defines several parameters that users can change on compilation.
 * The first section defines some colors that hex uses. If you don't know what
 * these strings mean, look up "ANSI Escape Codes".
 * The last section defines the number of bytes that hex prints at a time.
 */

 /*
const char
//COLOR THEME
* DEFAULT = "\x1b[0m", 		//used for normal text
* OFFSET = "\x1b[104m",		//used for displaying the offset
* BYTE = "\x1b[44m",		//used for marking the current byte
* EOF_MARK = "\x1b[31m",//used for marking the EOF
* OKAY = "\x1b[92m",		//used to signify good things
* WARNING = "\x1b[91m",		//used to signify bad things
* INVISIBLE = "\x1b[34m",	//marks non visible values
*/


const char
//MONO THEME
*DEFAULT = "\x1b[0m",
*OFFSET = "\x1b[47m\x1b[30m",
*BYTE = "\x1b[47m\x1b[30m",
*EOF_MARK = "\x1b[90m",
*OKAY = "",
*WARNING = "",
*INVISIBLE = "\x1b[100m";


int

WORD_SIZE = 16;			//the number of bytes to display on one line

#define H1 1
#define H2 16
#define H3 256
#define H4 4096
#define H5 65536
#define H6 1048576
#define H7 16777216
#define H8 268435456

#define HELP_STRING \
"Hex Usage:\n\
 hex [-h] [file]\n\
Addressing:\n\
 \".\" The current offset\n\
 \"n\" The nth byte of the file\n\
 \"+/-n\" An offset relative to the current offset\n\
 \"/pat\" Search forwards for a series of bytes\n\
 \"\\pat\" Search backwards for a series of bytes\n\
Commands:\n\
 (.,.) edit, e\n\
 (.,.) move, m (.)\n\
 (.,.) delete, d [buffer]\n\
 (.,.) copy, c [buffer]\n\
 (.) put, p [buffer]\n\
 (.) flag, f\n\
 next, n\n\
 file, f [pathname]\n\
 quit, q [!]\n\
 write, w\n\
 undo, u\
\nPlease see the manual for more detailed information\
"

/*
 edit *
 move ^
 delete /
 copy =
 put &
 flag @
 quit !
 write #
 undo -
 file ?
 */
