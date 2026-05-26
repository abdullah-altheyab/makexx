#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
using std::cout;
using std::endl;
using std::string;
using std::vector;
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#ifdef _WIN64
		//define something for Windows (64-bit only)
		string const runsys = "win64";
	#else
		//define something for Windows (32-bit only)
	#endif
#elif __APPLE__
	#if TARGET_IPHONE_SIMULATOR
		// iOS Simulator
	#elif TARGET_OS_IPHONE
		// iOS device
	#elif TARGET_OS_MAC
		// Other kinds of Mac OS
		string const runsys = "macos";
	#else
		#   error "Unknown Apple platform"
	#endif
#elif __linux__
	// linux
	string const runsys = "linux";
#elif __unix__ // all unices not caught above
	// Unix
	string const runsys = "unix";
#elif defined(_POSIX_VERSION)
	// POSIX
	string const runsys = "posix";
#else
	#error "Unknown compiler"
#endif

#include "makexxfile_embed.hpp"
#include "starter_embed.hpp"
#include <cstring>
#include <cerrno>
#include <cassert>

bool exists(char const *file) {
	FILE *fb = fopen(file, "r");
	if(fb == NULL) {
		return false;
	} else {
		fclose(fb);
		return true;
	}
}
void write_file(char const *file, const unsigned char *str, unsigned int len) {
	FILE *fb = fopen(file, "w");
	if(fb == NULL) {
		std::cerr << "error: cannot write '" << file << "': " << strerror(errno) << std::endl;
		exit(1);
	}
	fwrite(str, len, sizeof(unsigned char), fb);
	fclose(fb);
}
void write_file(char const *file, const char *str, int len) {
	FILE *fb = fopen(file, "w");
	if(fb == NULL) {
		std::cerr << "error: cannot write '" << file << "': " << strerror(errno) << std::endl;
		exit(1);
	}
	fwrite(str, len, sizeof(char), fb);
	fclose(fb);
}
#include "makexxfile.hpp"

char *read_file(char const *file) {
	FILE *f = fopen(file, "r");
	if(f == NULL) {
		std::cerr << "error: cannot read '" << file << "': " << strerror(errno) << std::endl;
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *string = (char *) malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);
	string[fsize] = 0;
	return string;
}

int run_cmd(string cmd) {
	return system(cmd.c_str());
}

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>

struct MenuEntry {
	string group;
	string target;
	vector<string> desc_lines;
	bool continuation = false;
};

struct MenuGroup {
	string name;
	string display_name;
	int depth = 0;
	vector<int> entries;
	bool folded = false;
};

int run_interactive() {
	std::ifstream f(".makexx_menu");
	if(!f.is_open()) {
		std::cerr << "error: .makexx_menu not found. Run makexx first to generate it." << endl;
		return 1;
	}
	vector<MenuEntry> entries;
	vector<string> group_order;
	std::map<string, int> group_index;
	vector<MenuGroup> groups;
	string line;
	while(std::getline(f, line)) {
		std::istringstream iss(line);
		string grp, target, desc;
		if(!std::getline(iss, grp, '\t')) continue;
		if(!std::getline(iss, target, '\t')) continue;
		std::getline(iss, desc, '\t');
		bool folded = false;
		if(!grp.empty() && grp[0] == '+') {
			grp = grp.substr(1);
			folded = true;
		}
		bool cont = false;
		if(!target.empty() && target[0] == '=') {
			target = target.substr(1);
			cont = true;
		}
		vector<string> dl;
		{
			size_t start = 0;
			while(true) {
				auto p = desc.find('|', start);
				dl.push_back(desc.substr(start, p - start));
				if(p == string::npos) break;
				start = p + 1;
			}
		}
		int eidx = entries.size();
		entries.push_back({grp, target, dl, cont});
		if(group_index.find(grp) == group_index.end()) {
			string dname = grp;
			int depth = 0;
			auto slash = grp.rfind('/');
			if(slash != string::npos) {
				dname = grp.substr(slash + 1);
				for(auto c : grp) if(c == '/') depth++;
			}
			group_index[grp] = groups.size();
			groups.push_back({grp, dname, depth, {}, folded});
		}
		groups[group_index[grp]].entries.push_back(eidx);
	}
	f.close();
	if(entries.empty()) {
		std::cerr << "error: no targets found in .makexx_menu" << endl;
		return 1;
	}

	int col_width = 0;
	for(auto &e : entries)
		if((int)e.target.size() > col_width) col_width = e.target.size();
	col_width += 2;
	string fmt_sel   = "      \033[7m%-" + std::to_string(col_width) + "s\033[0m  %s\n";
	string fmt_unsel = "      \033[1m%-" + std::to_string(col_width) + "s\033[0m  %s\n";

	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	newt.c_cc[VMIN] = 1;
	newt.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	enum Key { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_SPACE, KEY_QUIT };
	auto read_key = [&]() -> Key {
		char c;
		if(read(STDIN_FILENO, &c, 1) != 1) return KEY_QUIT;
		if(c == 'q') return KEY_QUIT;
		if(c == '\n' || c == '\r') return KEY_ENTER;
		if(c == ' ') return KEY_SPACE;
		if(c == 27) {
			struct termios tmp = newt;
			tmp.c_cc[VMIN] = 0;
			tmp.c_cc[VTIME] = 1;
			tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
			char seq[2];
			int n = read(STDIN_FILENO, seq, 1);
			if(n == 0) { tcsetattr(STDIN_FILENO, TCSANOW, &newt); return KEY_QUIT; }
			if(seq[0] != '[') { tcsetattr(STDIN_FILENO, TCSANOW, &newt); return KEY_NONE; }
			n = read(STDIN_FILENO, seq + 1, 1);
			tcsetattr(STDIN_FILENO, TCSANOW, &newt);
			if(n == 0) return KEY_NONE;
			if(seq[1] == 'A') return KEY_UP;
			if(seq[1] == 'B') return KEY_DOWN;
			if(seq[1] == 'C') return KEY_RIGHT;
			if(seq[1] == 'D') return KEY_LEFT;
			return KEY_NONE;
		}
		return KEY_NONE;
	};

	auto is_ancestor_folded = [&](string const &grp) -> bool {
		string path = grp;
		while(true) {
			auto slash = path.rfind('/');
			if(slash == string::npos) break;
			path = path.substr(0, slash);
			auto it = group_index.find(path);
			if(it != group_index.end() && groups[it->second].folded)
				return true;
		}
		return false;
	};

	auto build_visible = [&]() {
		vector<std::pair<int,int>> vis;
		for(int g = 0; g < (int)groups.size(); g++) {
			if(is_ancestor_folded(groups[g].name)) continue;
			vis.push_back({g, -1});
			if(!groups[g].folded) {
				for(int ei : groups[g].entries)
					vis.push_back({g, ei});
			}
		}
		return vis;
	};

	int cursor = 0;
	bool running = true;
	while(running) {
		auto visible = build_visible();
		if(cursor >= (int)visible.size()) cursor = visible.size() - 1;
		if(cursor < 0) cursor = 0;

		printf("\033[2J\033[H");
		printf("\033[1mmakexx interactive\033[0m  \033[2m(↑↓ navigate  ←→ fold/unfold  Enter run  q quit)\033[0m\n\n");

		for(int i = 0; i < (int)visible.size(); i++) {
			auto [gidx, eidx] = visible[i];
			bool selected = (i == cursor);
			if(eidx == -1) {
				string arrow = groups[gidx].folded ? "▸" : "▾";
				string gindent(groups[gidx].depth * 3, ' ');
				if(selected)
					printf("%s   %s \033[7m%s\033[0m\n", gindent.c_str(), arrow.c_str(), groups[gidx].display_name.c_str());
				else
					printf("%s   %s \033[1m%s\033[0m\n", gindent.c_str(), arrow.c_str(), groups[gidx].display_name.c_str());
			} else {
				auto &e = entries[eidx];
				bool next_is_cont = (i + 1 < (int)visible.size() &&
					visible[i + 1].second >= 0 &&
					entries[visible[i + 1].second].continuation);
				string bracket = "";
				if(!e.continuation && next_is_cont)
					bracket = " ─┬─ ";
				else if(e.continuation && next_is_cont)
					bracket = "  │  ";
				else if(e.continuation && !next_is_cont)
					bracket = "  ┘  ";
				else
					bracket = " ─── ";
				string eindent(groups[gidx].depth * 3, ' ');
				string first_desc = e.desc_lines.empty() ? "" : e.desc_lines[0];
				if(selected)
					printf("%s", eindent.c_str()), printf(fmt_sel.c_str(), e.target.c_str(), (bracket + first_desc).c_str());
				else
					printf("%s", eindent.c_str()), printf(fmt_unsel.c_str(), e.target.c_str(), (bracket + first_desc).c_str());
				string cont_pad = eindent + "      " + string(col_width, ' ') + "       ";
				for(size_t dl = 1; dl < e.desc_lines.size(); dl++)
					printf("%s%s\n", cont_pad.c_str(), e.desc_lines[dl].c_str());
			}
		}

		Key key = read_key();
		if(key == KEY_QUIT) {
			running = false;
		} else if(key == KEY_UP) {
			if(cursor > 0) cursor--;
		} else if(key == KEY_DOWN) {
			if(cursor < (int)visible.size() - 1) cursor++;
		} else if(key == KEY_RIGHT) {
			auto [gidx, eidx] = visible[cursor];
			groups[gidx].folded = false;
		} else if(key == KEY_LEFT) {
			auto [gidx, eidx] = visible[cursor];
			groups[gidx].folded = true;
		} else if(key == KEY_SPACE) {
			auto [gidx, eidx] = visible[cursor];
			groups[gidx].folded = !groups[gidx].folded;
		} else if(key == KEY_ENTER) {
			auto [gidx, eidx] = visible[cursor];
			if(eidx == -1) {
				groups[gidx].folded = !groups[gidx].folded;
			} else {
				tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
				printf("\033[2J\033[H");
				string target = entries[eidx].target;
				printf("\033[1mRunning: make %s\033[0m\n\n", target.c_str());
				run_cmd("make " + target);
				printf("\n\033[2mPress any key to return to menu...\033[0m");
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
			}
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	printf("\033[2J\033[H");
	return 0;
}
#endif

int main(int argc, char **argv) {
	char const *hpp_path = "makefile.hpp";
	char const *cpp_path = "makefile.cpp";
	bool update_makefile_hpp = false;
	bool force_overwrite = false;
	bool compile_only = false;
	bool verbose = false;
	bool interactive = false;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-u") == 0) {
			update_makefile_hpp = true;
		} else if(strcmp(argv[i], "-v") == 0) {
			verbose = true;
		} else if(strcmp(argv[i], "-f") == 0) {
			force_overwrite = true;
		} else if(strcmp(argv[i], "-c") == 0) {
			compile_only = true;
		} else if(strcmp(argv[i], "-i") == 0) {
			interactive = true;
		}
	}
	if(!exists(hpp_path) || update_makefile_hpp) {
		write_file(hpp_path, makefile_hpp_content, sizeof(makefile_hpp_content) - 1);
		if(verbose)
			cout << "Generating makefile.hpp..." << endl;
		update_makefile_hpp = true;
	} else {
		char *local_hpp = read_file(hpp_path);
		size_t embedded_len = sizeof(makefile_hpp_content) - 1;
		if(strlen(local_hpp) != embedded_len || memcmp(local_hpp, makefile_hpp_content, embedded_len) != 0) {
			std::cerr << "warning: makefile.hpp does not match this version of makexx; run makexx -u to update it" << endl;
		}
		free(local_hpp);
	}
	if(!exists(cpp_path)) {
		if(verbose)
			cout << "Generating " << cpp_path << "." << endl;
		write_file(cpp_path, makefile_example_content, sizeof(makefile_example_content) - 1);
	}
	bool is_windows = (runsys == "win64");
	if(verbose)
		cout << "SYSTEM:" << runsys << endl;
	if(is_windows) {
		run_cmd("del tmp_makexx.cpp tmp_makexx tmp_makexx.exe err_makexx.txt makefile_gen");
	} else {
		run_cmd("rm -f tmp_makexx.cpp tmp_makexx tmp_makexx.exe err_makexx.txt makefile_gen");
	}

	// Respect CXX env var; otherwise probe candidates in order.
	vector<string> compilers;
	const char *cxx_env = std::getenv("CXX");
	if(cxx_env && cxx_env[0] != '\0') {
		compilers = {string(cxx_env)};
	} else {
		compilers = {"g++", "clang++", "icpx", "icpc"};
	}

	if(verbose)
		cout << "SEARCHING FOR A COMPILER:" << endl;
	string small_prog = "int main(){}";
	write_file("tmp_makexx.cpp", small_prog.c_str(), small_prog.size());
	int compiler_id = 0;
	while(true) {
		run_cmd(compilers[compiler_id] + " -std=c++17 tmp_makexx.cpp -o tmp_makexx 2>err_makexx.txt");
		if(exists("tmp_makexx") || exists("tmp_makexx.exe")) {
			break;
		}
		compiler_id++;
		if(compiler_id >= (int)compilers.size()) {
			std::cerr << "error: could not find a C++ compiler. Set CXX to specify one." << endl;
			return -1;
		}
	}
	if(verbose)
		cout << compilers[compiler_id] << " is used." << endl;
	run_cmd(compilers[compiler_id] + " -std=c++17 makefile.cpp -o makefile_gen");
	if(!exists("makefile_gen") && !exists("makefile_gen.exe")) {
		std::cerr << "error: failed to compile makefile.cpp" << endl;
		return -1;
	}
	if(exists("makefile")) {
		auto a = read_file("makefile");
		if(strncmp(a, makefile_header, strlen(makefile_header)) != 0) {
			if(!force_overwrite) {
				cout << "error: makefile was not generated by makexx; use -f to overwrite" << endl;
				exit(-1);
			}
		}
		free(a);
	}
	if(verbose)
		cout << "Generating makefile.." << endl;
	int gen_ret = run_cmd(is_windows ? "makefile_gen.exe" : "./makefile_gen");
	if(gen_ret != 0) {
		std::cerr << "error: makefile_gen exited with code " << gen_ret << endl;
		return -1;
	}
	if(is_windows) {
		run_cmd("del tmp_makexx.cpp tmp_makexx tmp_makexx.exe err_makexx.txt makefile_gen");
	} else {
		run_cmd("rm -f tmp_makexx.cpp tmp_makexx tmp_makexx.exe err_makexx.txt makefile_gen");
	}
#ifndef _WIN32
	if(interactive)
		return run_interactive();
#endif
	std::string a = "make ";
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-u") == 0) {
		} else if(strcmp(argv[i], "-f") == 0) {
		} else if(strcmp(argv[i], "-i") == 0) {
		} else {
			a += argv[i];
			a += " ";
		}
	}
	if(!compile_only)
		run_cmd(a);
}
