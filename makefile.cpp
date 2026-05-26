
#include "makefile.hpp"
using namespace std;

int main() {
    BuildGraph bg;

    // Add rules with: bg.add("target", "source") << FINAL << "shell command";
    //
    //   FINAL    — included in 'make all'
    //   OPTIONAL — run on demand only (the default)
    //
    // Use GNU make automatic variables in commands:
    //   $<  first source file
    //   $@  target file
    //   $^  all source files

    bg.add("step1.out", "input.dat")
        << FINAL
        << HELP("run step 1")
        << "your-tool --input $< --output $@";

    bg.add("step2.out", "step1.out")
        << FINAL
        << HELP("run step 2")
        << "your-other-tool $< > $@";

    bg.add("report.pdf", {"step1.out", "step2.out"})
        << FINAL
        << HELP("generate final report")
        << "report-tool $^ -o $@";

    bg.dump_makefile();
}

