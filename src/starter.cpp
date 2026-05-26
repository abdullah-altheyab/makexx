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
    // Organize help with groups:
    //   mf.MENU("name")           — all subsequent rules belong to this group
    //   mf.MENU("Parent/Child")   — nested groups via slash separator
    //   mf.MENU("name", FOLDED)   — folded by default in makexx -i
    //   HELP("group", "description")    — override group for a single rule
    //
    // Helper functions:
    //   stem("dir/file.cpp")            — "file"
    //   basename("dir/file.cpp")        — "file.cpp"
    //   change_ext("file.cpp", ".o")    — "file.o"
    //   join_path("obj", "file.o")      — "obj/file.o"

    mf.MENU("Processing");

    mf.add("step1.out", "input.dat")
        << HELP("run step 1")
        << "your-tool --input $< --output $@";

    mf.add("step2.out", "step1.out")
        << HELP("run step 2")
        << "your-other-tool $< > $@";

    mf.MENU("Reports");

    mf.add("report.pdf", {"step1.out", "step2.out"})
        << FINAL
        << HELP("generate final report")
        << "report-tool $^ -o $@";

    mf.generate();
}
