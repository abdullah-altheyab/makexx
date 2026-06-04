#include "makefile.hpp"
using namespace std;

int main() {
    Makefile mf;
    mf.title = "My Project";

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
    // Organize help with menu groups — a rule joins a group via its OWN << MENU:
    //   rule << MENU("name")                — put this rule in a group
    //   rule << MENU("Parent/Child")        — nested groups via slash separator
    //   mf   << MENU("name", FOLDED)        — declare a group folded by default in makexx -i
    //   mf   << MENU("name", "description") — declare a group's description (help/AGENTS/TUI)
    //
    // Helper functions:
    //   stem("dir/file.cpp")            — "file"
    //   basename("dir/file.cpp")        — "file.cpp"
    //   change_ext("file.cpp", ".o")    — "file.o"
    //   join_path("obj", "file.o")      — "obj/file.o"

    mf.add("step1.out", "input.dat")
        << MENU("Processing")
        << HELP("run step 1")
        << "your-tool --input $< --output $@";

    mf.add("step2.out", "step1.out")
        << MENU("Processing")
        << HELP("run step 2")
        << "your-other-tool $< > $@";

    mf.add("report.pdf", {"step1.out", "step2.out"})
        << MENU("Reports")
        << FINAL
        << HELP("generate final report")
        << "report-tool $^ -o $@";

    mf.generate();
}
