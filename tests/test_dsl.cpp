#include "makexxfile.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Minimal test runner
// ---------------------------------------------------------------------------

static int failures = 0;
static int total = 0;
static const char *current_test = "";

#define CHECK_CONTAINS(content, substr) do { \
    ++total; \
    if((content).find(substr) == std::string::npos) { \
        std::cerr << "  FAIL [" << current_test << "]: expected to find: " << (substr) << "\n"; \
        ++failures; \
    } \
} while(0)

#define CHECK_NOT_CONTAINS(content, substr) do { \
    ++total; \
    if((content).find(substr) != std::string::npos) { \
        std::cerr << "  FAIL [" << current_test << "]: expected NOT to find: " << (substr) << "\n"; \
        ++failures; \
    } \
} while(0)

#define CHECK_EQ(actual, expected) do { \
    ++total; \
    if((actual) != (expected)) { \
        std::cerr << "  FAIL [" << current_test << "]: expected \"" << (expected) << "\" got \"" << (actual) << "\"\n"; \
        ++failures; \
    } \
} while(0)

// ---------------------------------------------------------------------------
// RAII temp directory — each test runs in its own isolated directory
// ---------------------------------------------------------------------------

struct TempDir {
    std::string path;
    std::string prev;

    TempDir() {
        char tmpl[] = "/tmp/makexx_test_XXXXXX";
        path = mkdtemp(tmpl);
        char buf[4096];
        prev = getcwd(buf, sizeof(buf));
        chdir(path.c_str());
    }
    ~TempDir() {
        chdir(prev.c_str());
        std::system(("rm -rf " + path).c_str());
    }
};

static std::string read_makefile() {
    std::ifstream f("makefile");
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string read_file(std::string const &path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_basename() {
    current_test = "test_basename";
    CHECK_EQ(basename("dir/file.cpp"), "file.cpp");
    CHECK_EQ(basename("a/b/c.txt"), "c.txt");
    CHECK_EQ(basename("nodir.txt"), "nodir.txt");
    CHECK_EQ(basename("dir\\win.txt"), "win.txt");
}

static void test_change_ext() {
    current_test = "test_change_ext";
    CHECK_EQ(change_ext("data.segy", ".bin"), "data.bin");
    CHECK_EQ(change_ext("data.segy", "bin"), "data.bin");
    CHECK_EQ(change_ext("dir/file.cpp", ".o"), "dir/file.o");
    CHECK_EQ(change_ext("noext", ".txt"), "noext.txt");
    CHECK_EQ(change_ext("file.tar.gz", ".bz2"), "file.tar.bz2");
}

static void test_join_path() {
    current_test = "test_join_path";
    CHECK_EQ(join_path("./obj", "main.o"), "./obj/main.o");
    CHECK_EQ(join_path("./obj/", "main.o"), "./obj/main.o");
    CHECK_EQ(join_path("", "main.o"), "main.o");
}

static void test_header() {
    current_test = "test_header";
    TempDir td;
    Makefile mf;
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "# This is an automatically generated makefile via makexx.");
    CHECK_CONTAINS(content, "# DO NOT EDIT!");
    CHECK_CONTAINS(content, "SHELL=/bin/bash");
    // `.DEFAULT_GOAL := all` must precede the `makefile:` regen rule, so
    // `make` with no args runs `all` instead of re-running `makexx -c`.
    CHECK_CONTAINS(content, ".DEFAULT_GOAL := all");
    auto goal_pos = content.find(".DEFAULT_GOAL := all");
    auto regen_pos = content.find("makefile: makefile.cpp");
    CHECK_EQ(goal_pos < regen_pos, true);
}

static void test_basic_rule() {
    current_test = "test_basic_rule";
    TempDir td;
    Makefile mf;
    mf.add("output.o", "input.cpp") << "g++ -c $< -o $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "output.o");
    CHECK_CONTAINS(content, "input.cpp");
    CHECK_CONTAINS(content, "g++ -c $< -o $@");
}

static void test_command() {
    current_test = "test_command";
    TempDir td;
    Makefile mf;
    mf.add("result.bin", "input.dat") << "process $< > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "process $< > $@");
}

static void test_final_in_all() {
    current_test = "test_final_in_all";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << FINAL << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "all : output.bin");
}

static void test_optional_not_in_all() {
    current_test = "test_optional_not_in_all";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << OPTIONAL << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    // 'all :' line should be empty (no targets)
    CHECK_NOT_CONTAINS(content, "all : output.bin");
}

static void test_multi_source() {
    current_test = "test_multi_source";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", {"src1.dat", "src2.dat"}) << FINAL << "merge $^ > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "src1.dat");
    CHECK_CONTAINS(content, "src2.dat");
    CHECK_CONTAINS(content, "merge $^ > $@");
}

static void test_help() {
    current_test = "test_help";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << FINAL << HELP("build the output") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "build the output");
}

static void test_menu_order_matches_help() {
    current_test = "test_menu_order_matches_help";
    TempDir td;
    Makefile mf;
    mf << MENU("Build");
    mf.add("a.o", "a.cpp") << HELP("a") << "g++ -c $< -o $@";
    mf << MENU("Reports");
    mf.add("rep.pdf") << HELP("r") << "echo r";
    mf.add("gis.qgz") << MENU("GIS") << HELP("g") << "echo g";  // per-rule MENU
    mf << MENU("Utilities");
    mf.add("util") << HELP("u") << "echo u";
    mf.generate();
    auto menu = read_file(".makexx_menu");
    CHECK_CONTAINS(menu, "!group\tBuild\t");
    CHECK_CONTAINS(menu, "!group\tReports\t");
    CHECK_CONTAINS(menu, "!group\tUtilities\t");
    CHECK_CONTAINS(menu, "!group\tGIS\t");
    // help_group_order: Build, Reports, Utilities (mf<<MENU registers
    // immediately), then GIS (per-rule MENU registers at dump_help time).
    auto pos_build = menu.find("!group\tBuild\t");
    auto pos_reports = menu.find("!group\tReports\t");
    auto pos_utils = menu.find("!group\tUtilities\t");
    auto pos_gis = menu.find("!group\tGIS\t");
    CHECK_EQ(pos_build < pos_reports, true);
    CHECK_EQ(pos_reports < pos_utils, true);
    CHECK_EQ(pos_utils < pos_gis, true);
}

static void test_menu_not_sticky() {
    current_test = "test_menu_not_sticky";
    TempDir td;
    Makefile mf;
    mf << MENU("Build");                                  // declaration only — not sticky
    mf.add("a.o", "a.cpp") << HELP("compile a") << "g++ -c $< -o $@";              // no << MENU
    mf.add("b.o", "b.cpp") << MENU("Build") << HELP("compile b") << "g++ -c $< -o $@";
    mf.generate();
    auto menu = read_file(".makexx_menu");
    CHECK_CONTAINS(menu, "Build\tb.o\tcompile b");         // joined Build explicitly
    CHECK_NOT_CONTAINS(menu, "Build\ta.o");                // did NOT inherit Build
}

static void test_undocumented_group() {
    current_test = "test_undocumented_group";
    TempDir td;
    Makefile mf;
    mf << MENU("Build");
    mf.add("app", "app.o") << MENU("Build") << HELP("link the app") << "g++ $< -o $@";
    mf.add("app.o", "app.cpp") << "g++ -c $< -o $@";          // no HELP
    mf << DESC("parse.o", "tokenizer object");
    mf.add("parse.o", "parse.cpp") << "g++ -c $< -o $@";      // no HELP, has DESC
    mf.add({"a.out", "b.out"}, "data.in") << "split $<";      // multi-target, no HELP
    mf.context = false;
    mf.generate();
    auto menu = read_file(".makexx_menu");
    // Undocumented group declared folded ("+") and listed before Built-in.
    CHECK_CONTAINS(menu, "!group\tUndocumented\t+");
    CHECK_CONTAINS(menu, "Undocumented\tapp.o\t");
    // Multi-target rules surface each target individually.
    CHECK_CONTAINS(menu, "Undocumented\ta.out\t");
    CHECK_CONTAINS(menu, "Undocumented\tb.out\t");
    // DESC is used as the description when present.
    CHECK_CONTAINS(menu, "Undocumented\tparse.o\ttokenizer object");
    // The HELP'd target stays in its own group, not Undocumented.
    CHECK_CONTAINS(menu, "Build\tapp\tlink the app");
    CHECK_NOT_CONTAINS(menu, "Undocumented\tapp\t");
    auto pos_undoc = menu.find("!group\tUndocumented");
    auto pos_builtin = menu.find("Built-in\tall");
    CHECK_EQ(pos_undoc < pos_builtin, true);

    // Undocumented targets must NOT leak into `make help` output.
    auto mk = read_file("makefile");
    auto help_pos = mk.find("\nhelp:");
    std::string help_rule = help_pos == std::string::npos ? mk : mk.substr(help_pos);
    CHECK_NOT_CONTAINS(help_rule, "'app.o'");
    CHECK_NOT_CONTAINS(help_rule, "'parse.o'");
}

static void test_graph_json() {
    current_test = "test_graph_json";
    TempDir td;
    Makefile mf;
    mf.title = "GT";
    mf << MENU("Build");
    mf.add("app", "app.o") << MENU("Build") << FINAL << HELP("the app") << TOOL("g++") << "g++ $< -o $@";
    mf.add("app.o", "app.cpp") << "g++ -c $< -o $@";
    mf << DESC("app.cpp", "entry point");
    mf.generate_with_graph();
    auto j = read_file(".makexx_graph.json");
    CHECK_CONTAINS(j, "\"title\":\"GT\"");
    CHECK_CONTAINS(j, "\"id\":\"app\"");
    CHECK_CONTAINS(j, "\"type\":\"final\"");
    CHECK_CONTAINS(j, "\"group\":\"Build\"");
    CHECK_CONTAINS(j, "\"id\":\"app.cpp\"");           // source not produced -> input
    CHECK_CONTAINS(j, "\"type\":\"input\"");
    CHECK_CONTAINS(j, "\"desc\":\"entry point\"");
    CHECK_CONTAINS(j, "\"type\":\"tool\"");            // g++ surfaced as a tool node
    CHECK_CONTAINS(j, "\"tool\":true");                // and a tool edge
    CHECK_CONTAINS(j, "{\"source\":\"app.o\",\"target\":\"app\"}");
    CHECK_NOT_CONTAINS(j, "\"id\":\"help\"");          // built-in rule excluded
    // Makefile wiring: html target + graph phony, both reach makexx.
    auto mk = read_file("makefile");
    CHECK_CONTAINS(mk, "makefile_graph.html : .makexx_graph.json");
    CHECK_CONTAINS(mk, "makexx --build-graph");
    CHECK_CONTAINS(mk, "graph : makefile_graph.html");
}

static void test_graph_tags_and_cmds() {
    current_test = "test_graph_tags_and_cmds";
    TempDir td;
    Makefile mf;
    mf << MENU("Forecast");
    mf.add("alpha_forecast.bin", "data.csv")
        << FINAL << HELP("forecast #alpha play (issue #42, not C#sharp)") << "run $< > $@";
    mf << DESC("data.csv", "raw seismic from #geo dept");
    mf.generate_with_graph();
    auto j = read_file(".makexx_graph.json");
    // HELP hashtags become tags on the rule's target; numbers-first ok; C# not matched.
    CHECK_CONTAINS(j, "\"id\":\"alpha_forecast.bin\"");
    CHECK_CONTAINS(j, "\"tags\":[\"alpha\",\"42\"]");
    CHECK_NOT_CONTAINS(j, "\"sharp\"");          // C#sharp: # not at a word boundary
    // DESC hashtag tags the input file node.
    CHECK_CONTAINS(j, "\"tags\":[\"geo\"]");
    // Recipe command text is emitted verbatim for path explanations.
    CHECK_CONTAINS(j, "\"cmds\":[\"run $< > $@\"]");
    // Input file has no recipe -> empty cmds.
    auto p = j.find("\"id\":\"data.csv\"");
    CHECK_EQ(p != std::string::npos, true);
    CHECK_CONTAINS(j.substr(p), "\"cmds\":[]");
}

static void test_graph_no_dangling_edges() {
    current_test = "test_graph_no_dangling_edges";
    TempDir td;
    Makefile mf;
    auto &r = mf.add("out.bin", "a.in");
    r << FINAL << HELP("build it") << "build $^ > $@";
    mf.add_source(r, "extra.in");   // wired as a prereq via add_source -> not in `nodes`
    mf.generate_with_graph();
    auto j = read_file(".makexx_graph.json");
    // The edge endpoint must still be emitted as a node, else the edge is
    // dangling and the viewer's graph library throws on load (blank page).
    CHECK_CONTAINS(j, "\"id\":\"extra.in\"");
    CHECK_CONTAINS(j, "{\"source\":\"extra.in\",\"target\":\"out.bin\"}");
}

static void test_profile() {
    current_test = "test_profile";
    TempDir td;
    Makefile mf;
    mf.profile = true;
    mf.add("out.bin", "in.dat") << FINAL << HELP("build out") << "cp $< $@";
    mf.add("mid.o", "in.dat") << "cp $< $@";
    mf.generate();
    auto mk = read_file("makefile");
    // A millisecond clock is selected once, at parse time.
    CHECK_CONTAINS(mk, "MXX_NOW :=");
    // Recipes are wrapped: per-target start stamp + a raw event appended to
    // the log, keyed by the literal $@.
    CHECK_CONTAINS(mk, ".makexx_prof/$(subst /,_,$@).t0");
    CHECK_CONTAINS(mk, "printf '%s\\trule\\t%s\\t%s\\n'");
    CHECK_CONTAINS(mk, ">> .makexx_hits");
    // Exactly the two user rules are instrumented — built-ins like `help`
    // are not (else this would be 3+).
    size_t n = 0, p = 0;
    while((p = mk.find(">> .makexx_hits", p)) != std::string::npos) { n++; p += 3; }
    CHECK_EQ(n == 2, true);
    // History survives cleaning: the temp dir is removed, the log is not.
    CHECK_CONTAINS(mk, "rm -rf .makexx_prof");
    CHECK_NOT_CONTAINS(mk, "rm -f \".makexx_hits\"");
}

static void test_profile_off_by_default() {
    current_test = "test_profile_off_by_default";
    TempDir td;
    Makefile mf;
    mf.add("o.bin", "i.dat") << "cp $< $@";
    mf.generate();
    auto mk = read_file("makefile");
    // No instrumentation when profiling is off. (The .makexx_hits / .makexx_prof
    // names still appear in the list_unknown ignore list — that's intentional
    // and harmless — so assert on the actual wrapping instead.)
    CHECK_NOT_CONTAINS(mk, "MXX_NOW");
    CHECK_NOT_CONTAINS(mk, ">> .makexx_hits");
    CHECK_NOT_CONTAINS(mk, ".t0");
    CHECK_NOT_CONTAINS(mk, "rm -rf .makexx_prof");
}

static void test_preamble() {
    current_test = "test_preamble";
    TempDir td;
    Makefile mf;
    mf.preamble = "CFLAGS ?= -O2 -Wall\nvpath %.cpp src";
    mf.add("foo.o", "foo.cpp") << "g++ $(CFLAGS) -c $< -o $@";
    mf.generate();
    auto content = read_makefile();
    auto shell_pos = content.find("SHELL=/bin/bash");
    auto phony_pos = content.find(".PHONY:");
    auto block = content.substr(shell_pos, phony_pos - shell_pos);
    CHECK_CONTAINS(block, "CFLAGS ?= -O2 -Wall");
    CHECK_CONTAINS(block, "vpath %.cpp src");
}

static void test_mf_phony_and_retain() {
    current_test = "test_mf_phony_and_retain";
    TempDir td;
    Makefile mf;
    // Rule defined plainly — phony / retain applied at the makefile level.
    mf.add("deploy", "myapp.bin") << "scripts/deploy.sh $<";
    mf.add("myapp.bin", "src.cpp") << FINAL << "g++ $< -o $@";
    mf << PHONY("deploy");
    mf << RETAIN("myapp.bin");
    mf.generate();
    auto content = read_makefile();
    // `deploy` should land in .PHONY even though its rule didn't say so.
    auto phony_line = content.substr(content.find(".PHONY:"), content.find('\n', content.find(".PHONY:")) - content.find(".PHONY:"));
    CHECK_CONTAINS(phony_line, "deploy");
    // `myapp.bin` should be in full_clean but NOT in soft_clean.
    auto full_pos = content.find("full_clean:");
    auto soft_pos = content.find("soft_clean:");
    auto list_pos = content.find("list:");
    CHECK_CONTAINS(content.substr(full_pos, soft_pos - full_pos), "myapp.bin");
    CHECK_NOT_CONTAINS(content.substr(soft_pos, list_pos - soft_pos), "myapp.bin");
}

static void test_tool() {
    current_test = "test_tool";
    TempDir td;
    Makefile mf;
    // Bare name → wrapped in $(shell command -v ...).
    // Path with `/` → literal.
    mf.add("out1.bin", "in1.dat") << TOOL("prog1") << "prog1 $< > $@";
    mf.add("out2.bin", "in2.dat") << TOOL("../bin/prog2") << "../bin/prog2 $< > $@";
    mf.generate();
    auto content = read_makefile();
    // Find each rule's prereq line.
    auto pos1 = content.find("out1.bin :");
    auto pos2 = content.find("out2.bin :");
    auto line1 = content.substr(pos1, content.find('\n', pos1) - pos1);
    auto line2 = content.substr(pos2, content.find('\n', pos2) - pos2);
    CHECK_CONTAINS(line1, "in1.dat");
    CHECK_CONTAINS(line1, "$(shell command -v prog1 2>/dev/null)");
    CHECK_CONTAINS(line2, "in2.dat");
    CHECK_CONTAINS(line2, "../bin/prog2");
    // Path-form must NOT have been wrapped.
    CHECK_NOT_CONTAINS(line2, "$(shell command -v ../bin/prog2");
}

static void test_agents_md() {
    current_test = "test_agents_md";
    TempDir td;
    Makefile mf;
    mf.title = "Demo";
    mf.description = "A small demo project.";
    mf << MENU("Build", "Compilation");
    mf.add("main.o", "main.cpp") << HELP("compile main") << TOOL("g++") << "g++ -c $< -o $@";
    mf.add("install", "main.o") << PHONY << HELP("install") << "cp $< /usr/local/bin/";
    // Nested group exposed in the heading.
    mf << MENU("Build/Tests");
    mf.add("unit.run", "main.o") << HELP("run unit tests") << "./unit";
    // Un-HELP'd intermediate rule — should appear in the Intermediate section.
    mf.add("scratch.dat", "main.cpp") << "preprocess $< > $@";
    mf.generate();
    auto content = read_file("AGENTS.md");
    // Build system intro + corrected URL.
    CHECK_CONTAINS(content, "abdullah-altheyab/makexx");
    CHECK_NOT_CONTAINS(content, "ab-10/makexx");
    CHECK_CONTAINS(content, "makexx -i");
    // DSL reference URLs for AI agents.
    CHECK_CONTAINS(content, "raw.githubusercontent.com/abdullah-altheyab/makexx/main/include/makexxfile.hpp");
    // Inline DSL cheat sheet for offline agents.
    CHECK_CONTAINS(content, "DSL quick reference");
    CHECK_CONTAINS(content, "auto& r = mf.add");
    CHECK_CONTAINS(content, "open_file(\"report.pdf\")");
    // PHONY rule-of-thumb in the cheat sheet (eval-driven addition).
    CHECK_CONTAINS(content, "REQUIRED whenever the target name");
    // Phony flag on the install target.
    CHECK_CONTAINS(content, "`make install` (phony)");
    // Tool dep on the compile target.
    CHECK_CONTAINS(content, "(uses `g++`)");
    // Nested group heading uses full slash-path + extra `#`.
    CHECK_CONTAINS(content, "#### Build/Tests");
    // Intermediate-targets section lists un-HELP'd user rules with deps.
    CHECK_CONTAINS(content, "## Intermediate targets");
    CHECK_CONTAINS(content, "`scratch.dat` (from `main.cpp`)");
    // Built-in `help` rule is auto-added without HELP() — must be filtered
    // out of the Intermediate section so it doesn't double-list.
    auto interm_pos = content.find("## Intermediate targets");
    auto builtin_pos = content.find("## Built-in targets");
    auto interm_block = content.substr(interm_pos, builtin_pos - interm_pos);
    CHECK_NOT_CONTAINS(interm_block, "`help`");
    // Complete built-in target list.
    CHECK_CONTAINS(content, "make list_input");
    CHECK_CONTAINS(content, "make list_unknown");
}

static void test_desc() {
    current_test = "test_desc";
    TempDir td;
    Makefile mf;
    // Makefile-level DESC.
    mf << DESC("dept_a_prices.csv", "Annual price forecasts for the next 10 years.");
    // Rule-level DESC (colocated with the rule that consumes the file).
    mf.add("forecast.t", "geology_wells.t")
        << HELP("run forecast") << DESC("geology_wells.t", "Well inventory from the geology team.")
        << "process $< > $@";
    // DESC on a target file (output) — describes the artifact contents.
    mf.add("priced.t", "dept_a_prices.csv")
        << HELP("apply prices") << DESC("priced.t", "Tab-separated table: well_id, year, price.")
        << "price $< > $@";
    // DESC on an un-HELP'd intermediate.
    mf.add("aux.t", "dept_a_prices.csv")
        << DESC("aux.t", "Intermediate normalisation table.")
        << "normalize $< > $@";
    mf.generate();
    auto content = read_file("AGENTS.md");
    // Inputs section: DESC inline.
    auto inputs_pos = content.find("## Input files");
    auto targets_pos = content.find("## Targets");
    auto inputs_block = content.substr(inputs_pos, targets_pos - inputs_pos);
    CHECK_CONTAINS(inputs_block, "`dept_a_prices.csv`");
    CHECK_CONTAINS(inputs_block, "Annual price forecasts for the next 10 years.");
    CHECK_CONTAINS(inputs_block, "`geology_wells.t`");
    CHECK_CONTAINS(inputs_block, "Well inventory from the geology team.");
    // Target listing: DESC as indented sub-bullet, distinct from HELP.
    CHECK_CONTAINS(content, "- File: Tab-separated table: well_id, year, price.");
    // Intermediate listing: DESC appended.
    CHECK_CONTAINS(content, "`aux.t`: Intermediate normalisation table.");
}

static void test_desc_accumulates() {
    current_test = "test_desc_accumulates";
    TempDir td;
    Makefile mf;
    // mf-level DESC: base annotation.
    mf << DESC("shared.csv", "Quarterly NPV scenarios from finance.");
    // A second mf-level DESC for the same file: should append.
    mf << DESC("shared.csv", "Contact: finance-quant@company.");
    // Rule-level DESC for the same file: should append after mf-level.
    mf.add("model.t", "shared.csv")
        << HELP("build model")
        << DESC("shared.csv", "Schema: scenario, year, npv.")
        << "model $< > $@";
    mf.generate();
    auto content = read_file("AGENTS.md");
    auto inputs_pos = content.find("## Input files");
    auto targets_pos = content.find("## Targets");
    auto inputs_block = content.substr(inputs_pos, targets_pos - inputs_pos);
    // All three fragments must appear, joined by spaces.
    CHECK_CONTAINS(inputs_block,
        "Quarterly NPV scenarios from finance. Contact: finance-quant@company. Schema: scenario, year, npv.");
}

static void test_open_file() {
    current_test = "test_open_file";
    TempDir td;
    Makefile mf;
    mf.add("show", "report.pdf") << HELP("open the report") << open_file("$<");
    mf.generate();
    auto content = read_makefile();
    // The snippet should mention all four openers and expand `$<` verbatim.
    CHECK_CONTAINS(content, "wslview \"$<\"");
    CHECK_CONTAINS(content, "xdg-open \"$<\"");
    CHECK_CONTAINS(content, "open \"$<\"");
    CHECK_CONTAINS(content, "start \"$<\"");
    // wslview must come before xdg-open: on WSL, xdg-open exists but mis-
    // routes Office formats. The wslview-first ordering is load-bearing.
    auto pos_wslview = content.find("wslview \"$<\"");
    auto pos_xdg     = content.find("xdg-open \"$<\"");
    CHECK_EQ(pos_wslview < pos_xdg, true);
}

static void test_user_phony() {
    current_test = "test_user_phony";
    TempDir td;
    Makefile mf;
    mf.add("install") << PHONY << HELP("install the thing") << "cp foo /usr/local/bin/";
    mf.add("regular", "src.dat") << HELP("regular build") << "cp $< $@";
    // Multi-target rule with one phony output and one real file output.
    mf.add({"deploy", "deploy.log"}, "myapp")
        << PHONY("deploy")
        << "scripts/deploy.sh $< > deploy.log";
    mf.generate();
    auto content = read_makefile();
    auto phony_line = content.substr(content.find(".PHONY:"), content.find('\n', content.find(".PHONY:")) - content.find(".PHONY:"));
    CHECK_CONTAINS(phony_line, "install");
    CHECK_CONTAINS(phony_line, "deploy");
    CHECK_NOT_CONTAINS(phony_line, "regular");
    CHECK_NOT_CONTAINS(phony_line, "deploy.log");
    // Phony rule must NOT get the `### GENERATING` decoration.
    auto install_rule_start = content.find("install :");
    auto install_rule_end = content.find("\n\n", install_rule_start);
    auto install_block = content.substr(install_rule_start, install_rule_end - install_rule_start);
    CHECK_NOT_CONTAINS(install_block, "### GENERATING");
}

static void test_menu_group_description() {
    current_test = "test_menu_group_description";
    TempDir td;
    Makefile mf;
    mf << MENU("Build", "Compile sources");
    mf.add("foo.o", "foo.cpp") << HELP("compile foo") << "g++ -c $< -o $@";
    mf.generate();
    auto makefile = read_makefile();
    auto context = read_file("AGENTS.md");
    auto menu = read_file(".makexx_menu");
    CHECK_CONTAINS(makefile, "Build: \xe2\x80\x94 Compile sources");
    CHECK_CONTAINS(context, "### Build");
    CHECK_CONTAINS(context, "_Compile sources_");
    CHECK_CONTAINS(menu, "!desc\tBuild\tCompile sources");
}

static void test_nested_groups_emit_parents() {
    current_test = "test_nested_groups_emit_parents";
    TempDir td;
    Makefile mf;
    mf.add("report.pdf", "filtered.bin")
        << MENU("Processing/QC")
        << HELP("Generate QC report")
        << "qcplot $< $@";
    mf.generate();
    auto makefile = read_makefile();
    auto context = read_file("AGENTS.md");
    CHECK_CONTAINS(makefile, "echo 'Processing:';");
    CHECK_CONTAINS(makefile, "echo '  QC:';");
    // AGENTS.md uses heading-level + full slash-path (no leaf-only indent).
    CHECK_CONTAINS(context, "### Processing");
    CHECK_CONTAINS(context, "#### Processing/QC");
}

static void test_temp_in_full_clean() {
    current_test = "test_temp_in_full_clean";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << FINAL << TEMP("scratch.tmp") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    // Find the full_clean section and check scratch.tmp appears there
    auto pos = content.find("full_clean:");
    CHECK_CONTAINS(content.substr(pos), "scratch.tmp");
}

static void test_temp_in_soft_clean() {
    current_test = "test_temp_in_soft_clean";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << FINAL << TEMP("scratch.tmp") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    auto pos = content.find("soft_clean:");
    CHECK_CONTAINS(content.substr(pos), "scratch.tmp");
}

static void test_softclean_retain() {
    current_test = "test_softclean_retain";
    TempDir td;
    Makefile mf;
    mf.add("expensive.bin", "input.dat") << FINAL << "run $< > $@";
    mf.on_softclean_retain("expensive.bin");
    mf.generate();
    auto content = read_makefile();
    auto full_pos = content.find("full_clean:");
    auto soft_pos = content.find("soft_clean:");
    auto list_pos = content.find("list:");
    // appears in full_clean section
    CHECK_CONTAINS(content.substr(full_pos, soft_pos - full_pos), "expensive.bin");
    // absent from soft_clean section (between soft_clean: and list:)
    CHECK_NOT_CONTAINS(content.substr(soft_pos, list_pos - soft_pos), "expensive.bin");
}

static void test_retain_forms() {
    current_test = "test_retain_forms";
    TempDir td;
    Makefile mf;
    // No-paren form: retain all of rule's targets.
    mf.add({"a.bin", "b.bin"}, "in.dat") << FINAL << RETAIN << "run $< > $@";
    // Variadic form: retain named files.
    mf.add("c.bin", "in.dat") << FINAL << RETAIN("c.bin", "extra.log") << "run $< > $@";
    // Braced form: still works.
    mf.add("d.bin", "in.dat") << FINAL << RETAIN({"d.bin", "other.log"}) << "run $< > $@";
    // Single string: still works.
    mf.add("e.bin", "in.dat") << FINAL << RETAIN("e.bin") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    auto soft_pos = content.find("soft_clean:");
    auto list_pos = content.find("list:");
    auto soft = content.substr(soft_pos, list_pos - soft_pos);
    // None of the retained files should appear in soft_clean.
    CHECK_NOT_CONTAINS(soft, "a.bin");
    CHECK_NOT_CONTAINS(soft, "b.bin");
    CHECK_NOT_CONTAINS(soft, "c.bin");
    CHECK_NOT_CONTAINS(soft, "extra.log");
    CHECK_NOT_CONTAINS(soft, "d.bin");
    CHECK_NOT_CONTAINS(soft, "other.log");
    CHECK_NOT_CONTAINS(soft, "e.bin");
}

static void test_silent() {
    current_test = "test_silent";
    TempDir td;
    Makefile mf;
    mf.silent = true;
    mf.add("output.bin", "input.dat") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_CONTAINS(content, "\t@run $< > $@");
}

static void test_no_echo() {
    current_test = "test_no_echo";
    TempDir td;
    Makefile mf;
    mf.echo = false;
    mf.add("output.bin", "input.dat") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    CHECK_NOT_CONTAINS(content, "### GENERATING");
}

static void test_phony() {
    current_test = "test_phony";
    TempDir td;
    Makefile mf;
    mf.generate();
    auto content = read_makefile();
    // Built-in phony targets are emitted in sorted order.
    CHECK_CONTAINS(content, ".PHONY: all full_clean help list list_input list_unknown soft_clean");
}

static void test_byproduct() {
    current_test = "test_byproduct";
    TempDir td;
    Makefile mf;
    mf.add("output.bin", "input.dat") << FINAL << BYPRODUCT("output.log") << "run $< > $@";
    mf.generate();
    auto content = read_makefile();
    auto full_pos = content.find("full_clean:");
    CHECK_CONTAINS(content.substr(full_pos), "output.log");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    test_basename();
    test_change_ext();
    test_join_path();
    test_header();
    test_basic_rule();
    test_command();
    test_final_in_all();
    test_optional_not_in_all();
    test_multi_source();
    test_help();
    test_menu_order_matches_help();
    test_menu_not_sticky();
    test_undocumented_group();
    test_graph_json();
    test_graph_tags_and_cmds();
    test_graph_no_dangling_edges();
    test_profile();
    test_profile_off_by_default();
    test_user_phony();
    test_open_file();
    test_tool();
    test_desc();
    test_desc_accumulates();
    test_agents_md();
    test_mf_phony_and_retain();
    test_preamble();
    test_menu_group_description();
    test_nested_groups_emit_parents();
    test_temp_in_full_clean();
    test_temp_in_soft_clean();
    test_softclean_retain();
    test_retain_forms();
    test_silent();
    test_no_echo();
    test_phony();
    test_byproduct();

    int passed = total - failures;
    std::cout << passed << "/" << total << " checks passed";
    if(failures == 0) {
        std::cout << " — all good.\n";
        return 0;
    } else {
        std::cout << " — " << failures << " FAILED.\n";
        return 1;
    }
}
