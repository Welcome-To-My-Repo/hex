default:
	c++ -ohex hex.cpp -g3

release:
	c++ -ohex hex.cpp -O3

clean:
	rm -r hex
