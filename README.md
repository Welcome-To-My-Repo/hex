HEX
===

About
-----
Hex is a scriptable hexadecimal editor modeled after the POSIX ex utility. I
created it because I couldn't find a hex editor that was fast enough for editing
Lisp images. I ended up creating an entire editor with regular expression
support and basic scripting capabilities.

Build Requirements
------------------
Hex is built for Linux.
It only requires the Posix C libraries and the C++ standard libraries to work.
A makefile is provided with the project so Hex also requires Make only for
convenience.

Installation Instructions
-------------------------
To install Hex, use your command line to open the project folder and enter the
command, "sudo make clean install". To uninstall hex, enter
"sudo make uninstall".

To create a debugging build of hex, enter "make debug".
