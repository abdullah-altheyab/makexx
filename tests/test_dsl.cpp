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
    CHECK_CONTAINS(context, "### Processing");
    CHECK_CONTAINS(context, "  ### QC");
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
    CHECK_CONTAINS(content, ".PHONY: all full_clean soft_clean list list_unknown list_input help");
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
    test_nested_groups_emit_parents();
    test_temp_in_full_clean();
    test_temp_in_soft_clean();
    test_softclean_retain();
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
