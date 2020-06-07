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
