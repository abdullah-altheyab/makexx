#include "makefile.hpp"

int main(int argc, char **argv) {
	BuildGraph bg;
	/*
	bg.add({"myfile1"}, {"myfile2", "myfile3"})
	    <<"cat myfile1 > myfile2"
	    <<"echo Hello World! >> myfile2"
	    <<"cat myfile2 > myfile3"
	    <<"echo Done. >> myfile3";

	auto & f=bg.add({"myfile1"});
	f<<"echo Hello World >> myfile1";
	f<<"echo Whats up >> myfile1";
	*/
	//bg.silent=false;
	//bg.echo=true;
	bg.dump_makefile();
}
