#include "makefile.hpp"
using namespace std;

int main() {
    Makefile mf;
    mf.help_title = "My Project";

    // Add rules with: mf.add("target", "source") << FINAL << "shell command";
    //
    //   FINAL    — included in 'make all'
    //   OPTIONAL — run on demand only (the default)
    //
    // Use GNU make automatic variables in commands:
    //   $<  first source file
    //   $@  target file
    //   $^  all source files
    //
    // Organize help with menu groups:
    //   mf.define_menu("name")              — declare a menu group upfront
    //   mf.define_menu("name", FOLDED)      — declare folded by default in makexx -i
    //   mf.set_current_menu("name")         — subsequent rules belong to this group
    //   mf.set_current_menu("Parent/Child") — nested groups via slash separator
    //   rule << MENU("name")                — override group for a single rule
    //
    // Helper functions:
    //   stem("dir/file.cpp")            — "file"
    //   basename("dir/file.cpp")        — "file.cpp"
    //   change_ext("file.cpp", ".o")    — "file.o"
    //   join_path("obj", "file.o")      — "obj/file.o"

    mf.set_current_menu("Processing");

    mf.add("step1.out", "input.dat")
        << HELP("run step 1")
        << "your-tool --input $< --output $@";

    mf.add("step2.out", "step1.out")
        << HELP("run step 2")
        << "your-other-tool $< > $@";

    mf.set_current_menu("Reports");

    mf.add("report.pdf", {"step1.out", "step2.out"})
        << FINAL
        << HELP("generate final report")
        << "report-tool $^ -o $@";

    mf.generate();
}
