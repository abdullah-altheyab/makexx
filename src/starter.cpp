#include "makefile.hpp"
using namespace std;

int main() {
    Makefile mf;
    mf.title = "My Project";
    // mf.description      = "What this project does";     // shown in AGENTS.md header
    // mf.profile          = false;                        // opt out: disable per-rule timing
    // mf.graph            = false;                        // opt out: skip graph generation
    // mf.context_filename = "CLAUDE.md";                  // rename the generated context file
    // mf.preamble         = "CFLAGS ?= -O2\n";           // raw make text injected near top
    // mf.silent           = true;                         // prefix all commands with @

    // ── Getting started ───────────────────────────────────────────────────────
    //
    //  This file IS the build description.  Edit it to define your rules, then
    //  run  makexx  to regenerate the makefile.
    //
    //  Quick syntax:
    //    auto& rule = mf.add("target", "source");  // add a rule
    //    rule << FINAL;                             // include in 'make all'
    //    rule << HELP("description");               // document the target
    //    rule << MENU("Group");                     // organise under a group
    //    rule << "your shell command $< $@ $^";    // append a shell command
    //
    //  Flags:
    //    FINAL    — built by 'make all'
    //    OPTIONAL — run on demand only (the default)
    //    PHONY    — target is not a file
    //
    //  Nested groups:
    //    rule << MENU("Parent/Child")
    //
    //  Helpers:
    //    stem("dir/file.cpp")          → "file"
    //    change_ext("file.cpp", ".o")  → "file.o"
    //    join_path("obj", "file.o")    → "obj/file.o"
    //
    //  Full DSL reference: makefile.hpp (installed in this directory)
    // ─────────────────────────────────────────────────────────────────────────

    mf.add("start")
        << PHONY << FINAL
        << HELP("getting started — edit makefile.cpp then run makexx")
        << "@echo ''"
        << "@echo 'Welcome to makexx!  This is a fresh project.'"
        << "@echo ''"
        << "@echo 'Next steps:'"
        << "@echo '  1. Open makefile.cpp in this directory'"
        << "@echo '  2. Replace the starter rules with your own build rules'"
        << "@echo '  3. Run  makexx  to regenerate the makefile and build'"
        << "@echo ''"
        << "@echo 'Useful commands:'"
        << "@echo '  makexx -c     regenerate the makefile without building'"
        << "@echo '  make help     list all documented targets'"
        << "@echo '  makexx -i     interactive target selector (TUI)'"
        << "@echo ''"
        << "@echo 'Full DSL reference is in makefile.hpp (installed in this directory).'";

    mf.generate();
}
