MANPATH = /usr/share/man/man1
INSTALL = /usr/local/bin

default: debug

debug:
	cc -ohex hex.c -g3

release:
	cc -ohex hex.c -O3



install: release
	chmod a+x hex;
	mkdir -p $(MANPATH) $(INSTALL)
	mv hex $(INSTALL)/
	cp hex.1 $(MANPATH)

uninstall:
	rm -f $(INSTALL)/hex

clean:
	rm -rf hex
