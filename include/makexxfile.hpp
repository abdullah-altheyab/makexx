// makexxfile.hpp — Makefile DSL for makexx
//
// Usage:
//   #include "makefile.hpp"
//   int main() {
//       Makefile mf;
//       mf.add("output.o", "input.cpp") << FINAL << "g++ -c $< -o $@";
//       mf.generate();
//   }
//
// Rules:
//   auto& r = mf.add(target, source)       — single target, single source
//   auto& r = mf.add(targets, sources)     — multiple targets/sources (vectors or {})
//   r << "shell command"                    — append a shell command
//   r << FINAL / OPTIONAL / INPUT           — target type (FINAL = in 'all')
//   r << TEMP({"f1","f2"})                  — mark files for cleanup
//   r << BYPRODUCT("file")                  — mark by-products for cleanup
//   r << RETAIN                             — exclude all rule targets from soft_clean
//   r << RETAIN("f") / RETAIN("a","b") / RETAIN({"a","b"})  — exclude specific files
//   r << PHONY                              — mark all of rule's targets as .PHONY (no output file produced)
//   r << PHONY("n") / PHONY("a","b") / PHONY({"a","b"})     — mark only specific targets
//   r << TARGET("file")                     — hidden/non-reproducible target
//   r << TOOL("prog") / TOOL("a","b") / TOOL({"a","b"})   — external executable(s) the
//                                                            rule depends on; mtime-tracked,
//                                                            not added to `$^`. Bare name →
//                                                            $(shell command -v ...); path
//                                                            with `/` → used literally
//   r << HELP("description")               — shown by 'make help' and makexx -i
//   r << HELP("group", "description")      — with explicit group override
//   r << DESC("file","what it is") / mf << DESC(...) — describe a file (typically an
//                                            input); shown next to it in AGENTS.md
//
// Menu groups (a rule joins a group ONLY via its own << MENU — not sticky):
//   r << MENU("Forecasting")               — put this rule in a group
//   r << MENU("Build/Tests")               — nested group via slash separator (parents auto-created)
//   mf << MENU("Archive", FOLDED)          — DECLARE a group folded by default in makexx -i
//   mf << MENU("Build", "Compile rules")   — DECLARE a group's description (make help, AGENTS.md, TUI)
//   mf << MENU("Build", "Compile rules", FOLDED)  — description + folded
//     (mf << MENU only declares description/folded; it does NOT group any rule)
//
// Makefile-level (apply by target name, independent of rules):
//   mf << PHONY("install") / PHONY("a","b") / PHONY({"a","b"})  — declare phony targets
//   mf << RETAIN("file") / RETAIN("a","b") / RETAIN({"a","b"})  — exclude files from soft_clean
//
// Settings:
//   mf.title = "My Project"           — title for 'make help'
//   mf.description = "..."                  — project description for AGENTS.md
//   mf.context = true/false                — enable/disable AGENTS.md (default: true)
//   mf.context_filename = "CLAUDE.md"      — override output filename
//   mf.silent = true                       — prefix commands with @ in makefile
//   mf.echo = false                        — suppress ### GENERATING decoration
//   mf.profile = true                      — log per-rule run time to .makexx_hits (usage/timing data)
//   mf.preamble = "CFLAGS ?= -O2\n"        — raw text injected near top of generated makefile
//                                            (for vars, include, vpath, .SUFFIXES, etc.)
//   mf.on_softclean_retain("file")         — exclude from soft_clean
//
// Output:
//   mf.generate()                          — write makefile + .makexx_menu + AGENTS.md
//   mf.generate_with_graph()               — also write makefile_graph.gv (Graphviz)
//
// Helpers:
//   stem("dir/file.cpp")                  → "file"       basename("dir/file.cpp") → "file.cpp"
//   change_ext("f.cpp", ".o")             → "f.o"              get_ext("file.cpp") → "cpp"
//   change_ext("f.cpp", {".o", ".d"})     → {"f.o","f.d"}
//   join_path("obj", "f.o")               → "obj/f.o"    replace_all(str, from, to)
//   to_upper(str)   to_lower(str)
//   open_file("report.pdf")               → shell snippet that hands the file to whichever
//                                            opener is available at make time (xdg-open /
//                                            open / wslview / start)

#include <map>
#include <set>
#include <utility>
#include <typeinfo>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <initializer_list>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

typedef std::initializer_list<std::string> StringList;

inline int get_terminal_width(int fallback = 100) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
	struct winsize w;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
		return w.ws_col;
#endif
	return fallback;
}

inline long double random_float() {
	return (long double)(std::rand()) / RAND_MAX;
}
inline std::string random_string(float min = 0, float max = 1) {
	return std::to_string(min + (max - min) * (long double)(std::rand()) / RAND_MAX);
}
inline std::string stem(std::string const a) {
	std::string base_filename = a.substr(a.find_last_of("/\\") + 1);
	std::string::size_type const p(base_filename.find_last_of('.'));
	return base_filename.substr(0, p);
}
inline std::string to_string(std::vector<std::string> const &list) {
	std::string output = "";
	if(list.size() <= 0)
		return output;
	output = list[0];
	for(int i = 1; i < (int)list.size(); i++)
		output += " " + list[i];
	return output;
}
inline std::string to_upper(std::string input) {
	for(auto &c : input) c = std::toupper(c);
	return input;
}
inline std::string to_lower(std::string input) {
	for(auto &c : input) c = std::tolower(c);
	return input;
}

inline std::string to_string(std::set<std::string> const &list) {
	std::string output = "";
	if(list.size() <= 0)
		return output;
	bool isfirst = true;
	for(auto &item : list) {
		if(isfirst) {
			isfirst = false;
		} else {
			output += " " ;
		}
		output += item;
	}
	return output;
}

enum target_type { FINAL, OPTIONAL, INPUT };
enum group_display { FOLDED };

class Rule {
  public:
	std::vector<std::string> commands;
	std::set<std::string> temp_files;
	std::set<std::string> byproducts;
	std::set<std::string> hidden_targets; // hidden_target
	std::set<std::string> retain_files;
	std::set<std::string> phony_targets;
	std::set<std::string> tools;
	std::map<std::string, std::string> file_descriptions;
	bool retain_targets;
	bool phony_all;
	target_type type;
	std::vector<std::string> help_lines;
	std::string help_group;
	int src_line = 0;            // line in makefile.cpp where mf.add(...) was called
	Rule() {
		type = OPTIONAL;
		help_group = "";
		retain_targets = false;
		phony_all = false;
	}
	std::string formatted(std::string prefix = "\t") {
		std::string a = "";
		for(auto k : commands) {
			a += prefix + k + "\n";
		}
		return a;
	}

};

inline std::string get_ext(std::string const filename) {
	auto idx = filename.rfind('.');
	if(idx != std::string::npos) {
		return filename.substr(idx + 1);
	} else {
		return "";
	}
}

inline std::string basename(std::string const &path) {
	return path.substr(path.find_last_of("/\\") + 1);
}

inline std::string change_ext(std::string const &filename, std::string const &new_ext) {
	auto dot = filename.rfind('.');
	std::string base = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
	if(new_ext.empty()) return base;
	return (new_ext[0] == '.') ? base + new_ext : base + "." + new_ext;
}

inline std::vector<std::string> change_ext(std::string const &filename, StringList exts) {
	std::vector<std::string> result;
	for(auto const &ext : exts)
		result.push_back(change_ext(filename, std::string(ext)));
	return result;
}
inline std::vector<std::string> change_ext(std::string const &filename, std::vector<std::string> const &exts) {
	std::vector<std::string> result;
	for(auto const &ext : exts)
		result.push_back(change_ext(filename, ext));
	return result;
}

inline std::string join_path(std::string dir, std::string const &file) {
	if(!dir.empty() && dir.back() != '/' && dir.back() != '\\')
		dir += '/';
	return dir + file;
}

// Shell snippet that hands a file to whichever OS opener is available on
// the host that runs `make` — picks at make time, so the same makefile.cpp
// works across WSL (wslview), Linux desktops (xdg-open), macOS (open), and
// generic Windows (start). wslview is tried first because xdg-open is also
// present on WSL but routes some types (Office formats) to broken Linux
// handlers instead of the Windows host. `path` may be a literal name or a
// make automatic like "$<" / "$@".
inline std::string open_file(std::string const &path) {
	return "{ command -v wslview  >/dev/null 2>&1 && wslview \""  + path + "\"; } || "
	       "{ command -v xdg-open >/dev/null 2>&1 && xdg-open \"" + path + "\"; } || "
	       "{ command -v open     >/dev/null 2>&1 && open \""     + path + "\"; } || "
	       "{ command -v start    >/dev/null 2>&1 && start \""    + path + "\"; }";
}

inline std::string replace_all(std::string str, const std::string &from, const std::string &to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

inline Rule &operator<<(Rule &a, std::string const command) {
#if defined(_WIN64)
	a.commands.push_back(replace_all(command, "/", "\\"));
#else
	a.commands.push_back(command);
#endif
	return a;
}

inline Rule &operator<<(Rule &a, target_type t) {
	a.type = t;
	return a;
}

class TEMP { // list of temprorary filenames
  public:
	std::set<std::string> filenames;
	TEMP(std::string filename) {
		this->filenames.insert(filename);
	};
	TEMP(StringList filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	TEMP(std::vector<std::string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};

class BYPRODUCT { // by products
  public:
	std::set<std::string> filenames;
	BYPRODUCT(std::string filename) {
		this->filenames.insert(filename);
	};
	BYPRODUCT(StringList filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	BYPRODUCT(std::vector<std::string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};
// `<< RETAIN` retains all of the rule's targets across soft_clean.
// `<< RETAIN("file")`, `<< RETAIN("f1","f2",...)`, or `<< RETAIN({"f1","f2"})`
// retain only the named files.
// RETAIN is a global instance of RETAIN_t; the operator() overloads let it act
// like a constructor while still being usable bare without parentheses.
class RETAIN_t {
  public:
	std::set<std::string> filenames;
	bool retain_targets;
	RETAIN_t() : retain_targets(true) {}
	template<typename... Rest>
	RETAIN_t operator()(std::string first, Rest&&... rest) const {
		RETAIN_t r; r.retain_targets = false;
		r.filenames.insert(first);
		(r.filenames.insert(std::string(std::forward<Rest>(rest))), ...);
		return r;
	}
	RETAIN_t operator()(StringList fs) const {
		RETAIN_t r; r.retain_targets = false; for(auto &f : fs) r.filenames.insert(f); return r;
	}
	RETAIN_t operator()(std::vector<std::string> fs) const {
		RETAIN_t r; r.retain_targets = false; for(auto &f : fs) r.filenames.insert(f); return r;
	}
};
inline const RETAIN_t RETAIN;

// `<< PHONY` marks all of the rule's targets as .PHONY.
// `<< PHONY("name")`, `<< PHONY("n1","n2",...)`, or `<< PHONY({"a","b"})`
// marks only specific targets (relevant when a single multi-target rule has
// mixed phony / real-file outputs).
class PHONY_t {
  public:
	std::set<std::string> targets;
	bool phony_all;
	PHONY_t() : phony_all(true) {}
	template<typename... Rest>
	PHONY_t operator()(std::string first, Rest&&... rest) const {
		PHONY_t r; r.phony_all = false;
		r.targets.insert(first);
		(r.targets.insert(std::string(std::forward<Rest>(rest))), ...);
		return r;
	}
	PHONY_t operator()(StringList ts) const {
		PHONY_t r; r.phony_all = false; for(auto &t : ts) r.targets.insert(t); return r;
	}
	PHONY_t operator()(std::vector<std::string> ts) const {
		PHONY_t r; r.phony_all = false; for(auto &t : ts) r.targets.insert(t); return r;
	}
};
inline const PHONY_t PHONY;

// Declare external executable(s) the rule depends on. Each entry becomes a
// prerequisite (timestamp-tracked, so editing/replacing the tool triggers
// rebuilds), but is NOT inserted into the regular rule_source list, so it
// doesn't pollute `$^`. If the argument contains '/' it's added as a literal
// path; otherwise it's wrapped in `$(shell command -v NAME 2>/dev/null)` so
// make resolves PATH at parse time, keeping the same makefile.cpp portable
// across machines with different install layouts.
class TOOL {
  public:
	std::set<std::string> tools;
	TOOL(StringList ts) { for(auto &t : ts) tools.insert(t); }
	TOOL(std::vector<std::string> ts) { for(auto &t : ts) tools.insert(t); }
	template<typename... Args>
	TOOL(std::string first, Args&&... rest) {
		tools.insert(first);
		(tools.insert(std::string(std::forward<Args>(rest))), ...);
	}
};

class TARGET { // hidden target, usually for non-reproducible (manual) targets
  public:
	std::set<std::string> filenames;
	TARGET(std::string filename) {
		this->filenames.insert(filename);
	};
	TARGET(StringList filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	TARGET(std::vector<std::string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};

class HELP { // add help to the menu
  public:
	std::string help;
	std::string group;
	HELP(std::string help) : help(help) {};
	HELP(std::string group, std::string help) : help(help), group(group) {};
};

// Describe a file (input, intermediate, or output) — what it is, where it
// comes from, how it's refreshed, etc. Shown in AGENTS.md next to the file
// name. Usable in either scope:
//   mf << DESC("input.txt", "annual price forecasts for the next 10 years");
//   rule << DESC("input.txt", "...");  // colocated with the rule that consumes it
// Multiple DESC calls for the same file ACCUMULATE — they're joined with a
// space — so you can layer annotations across scopes (e.g. an mf-level
// "what is it" + a rule-level "how this rule uses it", or schema notes
// + a contact line in separate DESCs). Order: mf-level first, then
// rule-level in command-insertion order.
class DESC {
  public:
	std::string file;
	std::string description;
	DESC(std::string file, std::string description) : file(file), description(description) {};
};

class MENU { // set menu group for a rule or for subsequent rules (on Makefile)
  public:
	std::string group;
	std::string description;
	bool has_display;
	group_display display;
	MENU(std::string group) : group(group), has_display(false), display(FOLDED) {};
	MENU(std::string group, group_display d) : group(group), has_display(true), display(d) {};
	MENU(std::string group, std::string desc) : group(group), description(desc), has_display(false), display(FOLDED) {};
	MENU(std::string group, std::string desc, group_display d) : group(group), description(desc), has_display(true), display(d) {};
};

inline Rule &operator<<(Rule &a, TEMP t) {
	for(auto &tmp : t.filenames)
		a.temp_files.insert(tmp);
	return a;
}

inline Rule &operator<<(Rule &a, BYPRODUCT t) {
	for(auto &tmp : t.filenames)
		a.byproducts.insert(tmp);
	return a;
}
inline Rule &operator<<(Rule &a, RETAIN_t r) {
	if(r.retain_targets)
		a.retain_targets = true;
	else
		for(auto &f : r.filenames)
			a.retain_files.insert(f);
	return a;
}
inline Rule &operator<<(Rule &a, PHONY_t p) {
	if(p.phony_all)
		a.phony_all = true;
	else
		for(auto &t : p.targets)
			a.phony_targets.insert(t);
	return a;
}
inline Rule &operator<<(Rule &a, TARGET t) {
	for(auto &tmp : t.filenames)
		a.hidden_targets.insert(tmp);
	return a;
}
inline Rule &operator<<(Rule &a, TOOL t) {
	for(auto &x : t.tools)
		a.tools.insert(x);
	return a;
}
inline Rule &operator<<(Rule &a, DESC d) {
	auto &existing = a.file_descriptions[d.file];
	if(existing.empty()) existing = d.description;
	else existing += " " + d.description;
	return a;
}

inline Rule &operator<<(Rule &a, HELP t) {
	a.help_lines.push_back(t.help);
	if(!t.group.empty())
		a.help_group = t.group;
	return a;
}

inline Rule &operator<<(Rule &a, MENU t) {
	a.help_group = t.group;
	return a;
}

constexpr char const* const makefile_header = "# This is an automatically generated makefile via makexx.";

class Makefile {
  private:
	std::set<std::string> nodes;
	std::vector<std::unique_ptr<Rule>> commands;
	std::map<std::string, Rule *> target_rule;
	std::multimap<Rule *, std::string> rule_source;
	std::multimap<Rule *, std::string> rule_target;
	enum BracketType { BRK_NORMAL=0, BRK_MULTI_FIRST=1, BRK_MULTI_MID=2, BRK_MULTI_END=3 };
	struct HelpEntry { std::string target; std::string description; std::string group; BracketType bracket; };
	std::vector<HelpEntry> help_menu;
	std::vector<std::string> help_group_order;
	std::set<std::string> soft_clean_retain_nodes;
	std::set<std::string> mf_phony_targets;
	std::map<std::string, std::string> mf_file_descriptions;
	std::set<std::string> folded_groups;
	std::map<std::string, std::string> group_descriptions;

	Rule &add_impl(std::vector<std::string> targets, std::vector<std::string> sources, int src_line = 0) {
		commands.push_back(std::make_unique<Rule>());
		Rule *A = commands.back().get();
		A->src_line = src_line;
		// A rule's group comes ONLY from its own `<< MENU(...)` / `<< HELP("g",...)`.
		// (There used to be a sticky `current_help_group` here that every later
		// rule silently inherited — a stateful footgun that recolored unrelated
		// rules. Removed: `mf << MENU(...)` now only declares a group.)
		for(auto &s : sources) {
			nodes.insert(s);
			rule_source.insert({A, s});
		}
		for(auto &t : targets) {
			if(target_rule.find(t) != target_rule.end()) {
				std::cerr << "# WARNING! Multiple build rules for the same target '" << t << "' only the latest rule defined will be used!" << std::endl;
			}
			nodes.insert(t);
			target_rule[t] = A;
			rule_target.insert({A, t});
		}
		return *A;
	}

	int max_width;

	void register_help_group(std::string const &group) {
		if(group.empty()) return;
		size_t start = 0;
		while(true) {
			size_t slash = group.find('/', start);
			std::string prefix = group.substr(0, slash);
			if(std::find(help_group_order.begin(), help_group_order.end(), prefix) == help_group_order.end())
				help_group_order.push_back(prefix);
			if(slash == std::string::npos) break;
			start = slash + 1;
		}
	}

	void add_to_help_menu(std::string make_rule, std::string helpstr, std::string group, BracketType bracket = BRK_NORMAL) {
		help_menu.push_back({make_rule, helpstr, group, bracket});
		register_help_group(group);
	}

	static std::string shell_escape(std::string const &s) {
		return replace_all(s, "'", "'\\''");
	}

	void emit_help_entries(std::string &script, std::string group, std::string indent = "") {
		std::string p = std::to_string(max_width);
		for(auto &itm : help_menu) {
			if(itm.group != group) continue;
			if(itm.bracket == BRK_MULTI_MID) {
				script += "printf '" + indent + "  %" + p + "s \xe2\x94\x82\\n' '" + shell_escape(itm.target) + "'; ";
			} else if(itm.bracket == BRK_MULTI_END) {
				script += "printf '" + indent + "  %" + p + "s \xe2\x94\x98\\n' '" + shell_escape(itm.target) + "'; ";
			} else {
				std::string desc = replace_all(itm.description, "\n", "\\n");
				std::string mode = (itm.bracket == BRK_MULTI_FIRST) ? "1" : "0";
				script += "printf '" + indent + "  %" + p + "s' '" + shell_escape(itm.target) + "'; "
					"printf '%b\\n' '" + shell_escape(desc) + "' | fold -s -w $$_d | "
					"awk -v p=" + p + " -v m=" + mode + " '"
					"{l[NR]=$$0}END{"
					"if(NR<=1 && m==0)printf \" \xe2\x94\x80 %s\\n\",l[1];"
					"else if(NR<=1)printf \" \xe2\x94\xac %s\\n\",l[1];"
					"else if(m==0){"
					"printf \" \xe2\x94\x8c %s\\n\",l[1];"
					"for(i=2;i<NR;i++)printf \"  %*s \xe2\x94\x82 %s\\n\",p,\"\",l[i];"
					"printf \"  %*s \xe2\x94\x94 %s\\n\",p,\"\",l[NR]"
					"}else{"
					"printf \" \xe2\x94\xac %s\\n\",l[1];"
					"for(i=2;i<NR;i++)printf \"  %*s \xe2\x94\x82 %s\\n\",p,\"\",l[i];"
					"if(NR>1)printf \"  %*s \xe2\x94\x9c %s\\n\",p,\"\",l[NR]"
					"}}'; ";
			}
		}
	}

	void add_help_rule(bool graph = false) {
		auto &f  = add("help");
		std::string p = std::to_string(max_width);
		int prefix_len = 2 + max_width + 3;
		std::string script;
		script += "_w=$$(tput cols 2>/dev/null || echo $${COLUMNS:-80}); "
				  "_d=$$((_w - " + std::to_string(prefix_len) + ")); "
				  "[ $$_d -lt 20 ] && _d=20; ";
		if(!title.empty())
			script += "echo '" + shell_escape(title) + "'; echo ''; ";
		bool has_ungrouped = false;
		for(auto &itm : help_menu)
			if(itm.group.empty()) { has_ungrouped = true; break; }
		if(has_ungrouped) {
			script += "echo 'Targets:'; ";
			emit_help_entries(script, "");
			script += "echo ''; ";
		}
		for(auto &grp : help_group_order) {
			std::string display_name = grp;
			auto slash = grp.rfind('/');
			if(slash != std::string::npos)
				display_name = grp.substr(slash + 1);
			int depth = std::count(grp.begin(), grp.end(), '/');
			std::string indent(depth * 2, ' ');
			std::string header = indent + display_name + ":";
			auto dit = group_descriptions.find(grp);
			if(dit != group_descriptions.end())
				header += " \xe2\x80\x94 " + dit->second;
			script += "echo '" + shell_escape(header) + "'; ";
			emit_help_entries(script, grp, indent);
			script += "echo ''; ";
		}
		auto builtin = [&](std::string name, std::string desc) {
			script += "printf '  %" + p + "s' '" + shell_escape(name) + "'; "
				"printf '%b\\n' '" + shell_escape(desc) + "' | fold -s -w $$_d | "
				"awk -v p=" + p + " '"
				"{l[NR]=$$0}END{"
				"if(NR<=1)printf \" \xe2\x94\x80 %s\\n\",l[1];"
				"else{"
				"printf \" \xe2\x94\x8c %s\\n\",l[1];"
				"for(i=2;i<NR;i++)printf \"  %*s \xe2\x94\x82 %s\\n\",p,\"\",l[i];"
				"printf \"  %*s \xe2\x94\x94 %s\\n\",p,\"\",l[NR]"
				"}}'; ";
		};
		script += "echo 'Built-in:'; ";
		builtin("all",          "Build all final targets");
		builtin("full_clean",   "Remove all generated files");
		builtin("soft_clean",   "Remove generated files (except retained)");
		builtin("list",         "List all tracked files");
		builtin("list_unknown", "List untracked files in directory");
		builtin("list_input",   "List input files");
		builtin("help",         "Show this help");
		if(graph) {
			builtin("makefile_graph.pdf", "Generate dependency graph PDF (requires Graphviz)");
			builtin("graph", "Open the interactive dependency graph in a browser");
		}
		f << "@" + script;
	}

	void dump_help(bool graph = false) {
		for(auto const& cmd : commands) {
			if(cmd->help_lines.empty()) continue;
			auto trange = rule_target.equal_range(cmd.get());
			std::vector<std::string> targets;
			for(auto j = trange.first; j != trange.second; j++)
				targets.push_back(j->second);
			if(targets.empty()) continue;
			for(auto &t : targets)
				if(max_width < (int)t.size())
					max_width = t.size();
			std::string help_joined = cmd->help_lines[0];
			for(size_t h = 1; h < cmd->help_lines.size(); h++)
				help_joined += "\n" + cmd->help_lines[h];
			if(targets.size() == 1) {
				add_to_help_menu(targets[0], help_joined, cmd->help_group, BRK_NORMAL);
			} else {
				add_to_help_menu(targets[0], help_joined, cmd->help_group, BRK_MULTI_FIRST);
				for(size_t i = 1; i < targets.size() - 1; i++)
					add_to_help_menu(targets[i], "", cmd->help_group, BRK_MULTI_MID);
				add_to_help_menu(targets.back(), "", cmd->help_group, BRK_MULTI_END);
			}
		}
		add_help_rule(graph);
	}

  public:
	std::set<std::string> hidden_nodes; //notes to execlude from makefile graph
	bool silent;
	bool echo;
	bool context;
	// When true, each non-built-in rule's recipe is wrapped with timing
	// instrumentation that appends a raw event (epoch, "rule", target,
	// duration_ms) to the append-only `.makexx_hits` log on every successful
	// run. The log is the source of truth for usage/timing analysis
	// (`makexx --stats`, graph heat-coloring); aggregation happens at read
	// time. Off by default — it adds two process spawns + a temp file per
	// built target.
	bool profile;
	std::string context_filename;
	std::string title;
	std::string description;
	// Raw text injected near the top of the generated makefile (after the
	// auto-generated header, before .PHONY:). Escape hatch for variable
	// defaults (`CFLAGS ?= -O2`), `include`, `vpath`, `.SUFFIXES`, etc.
	// Errors in this text surface from `make`, not `makexx`.
	std::string preamble;

	void define_menu(std::string group) {
		register_help_group(group);
	}

	void define_menu(std::string group, group_display display) {
		define_menu(group);
		if(display == FOLDED) folded_groups.insert(group);
	}

	Makefile() {
        max_width=20;
		silent = false;
		echo = true;
		context = true;
		profile = false;
		context_filename = "AGENTS.md";
		title = "";
	};

	void add_source(Rule &f, std::string node) {
		rule_source.insert(std::pair<Rule *, std::string>(&f, node));
	}
	void add_target(Rule &f, std::string node) {
		rule_target.insert(std::pair<Rule *, std::string>(&f, node));
	}

	Rule &get_rule(std::string target) {
        if(target_rule.find(target) != target_rule.end()) {
			return (*target_rule[target]);
        }else{
            return add(target);
        }
    }

	// `int _ln = __builtin_LINE()` captures the makefile.cpp line of the
	// mf.add(...) call (gcc/clang extension, works as a default argument in
	// C++17). Surfaced per node in the graph so the viewer can show where a
	// rule is defined; for loop-generated rules it points at the template line.
	Rule &add(std::string target, int _ln = __builtin_LINE()) { return add_impl({target}, {}, _ln); }
	Rule &add(StringList targets, int _ln = __builtin_LINE()) { return add_impl(targets, {}, _ln); }
	Rule &add(std::string target, std::string source, int _ln = __builtin_LINE()) { return add_impl({target}, {source}, _ln); }
	Rule &add(std::string target, StringList sources, int _ln = __builtin_LINE()) { return add_impl({target}, sources, _ln); }
	Rule &add(std::string target, std::vector<std::string> sources, int _ln = __builtin_LINE()) { return add_impl({target}, sources, _ln); }
	Rule &add(StringList targets, std::string source, int _ln = __builtin_LINE()) { return add_impl(targets, {source}, _ln); }
	Rule &add(StringList targets, StringList sources, int _ln = __builtin_LINE()) { return add_impl(targets, sources, _ln); }
	Rule &add(StringList targets, std::vector<std::string> sources, int _ln = __builtin_LINE()) { return add_impl(targets, sources, _ln); }
	Rule &add(std::vector<std::string> targets, std::string source, int _ln = __builtin_LINE()) { return add_impl(targets, {source}, _ln); }
	Rule &add(std::vector<std::string> targets, std::vector<std::string> sources, int _ln = __builtin_LINE()) { return add_impl(targets, sources, _ln); }

	void on_softclean_retain(std::string filename) {
        this->soft_clean_retain_nodes.insert(filename);
    }
	void on_softclean_retain(StringList filenames) {
		for(auto itm : filenames) {
			this->soft_clean_retain_nodes.insert(itm);
		}
    }
	void on_softclean_retain(std::vector<std::string> filenames) {
		for(auto itm : filenames) {
			this->soft_clean_retain_nodes.insert(itm);
		}
    }

	void generate_with_graph() {
		generate(true);
	}
	void generate(bool graph = false) {
		dump_help(graph);
		std::set<std::string> processed_nodes;
		std::string makefile;
		std::set<std::string> defaultmake_list;
		std::set<std::string> inputfiles_list;
		std::ofstream myfile;
		std::set<std::string> temps_list;
		std::set<std::string> byprods_list;
		std::set<std::string> hidden_targets_list;
		std::set<std::string> phony_targets = {"all", "full_clean", "soft_clean", "list", "list_unknown", "list_input", "help"};
		if(graph) phony_targets.insert("graph");
		// Collect user-marked phony targets so they appear in .PHONY and
		// skip the ### GENERATING decoration. `rule << PHONY` marks all
		// of the rule's targets; `rule << PHONY("name")` marks only named
		// ones (relevant when a single rule has mixed phony / file outputs).
		for(auto const &cmd : commands) {
			if(cmd->phony_all) {
				auto trange = rule_target.equal_range(cmd.get());
				for(auto j = trange.first; j != trange.second; j++)
					phony_targets.insert(j->second);
			}
			for(auto const &t : cmd->phony_targets)
				phony_targets.insert(t);
		}
		// Also include any names declared at the makefile level via
		// `mf << PHONY("...")`, regardless of which rule produces them.
		for(auto const &t : mf_phony_targets)
			phony_targets.insert(t);
		myfile.open("makefile");
		myfile << makefile_header << std::endl;
		myfile << "# DO NOT EDIT!" << std::endl;
		myfile << "# You can control the generation via makefile.cpp!" << std::endl;
		myfile << "SHELL=/bin/bash" << std::endl;
		// Pin the default goal explicitly. Without this, GNU make picks the
		// first non-special target — which is the `makefile: makefile.cpp
		// makefile.hpp` regeneration rule emitted below — so `make` with no
		// args would run `makexx -c` instead of building `all`. Emitted
		// before `preamble` so a user can override via `mf.preamble`.
		myfile << ".DEFAULT_GOAL := all" << std::endl;
		if(!preamble.empty()) {
			myfile << preamble;
			if(preamble.back() != '\n') myfile << '\n';
		}
		if(profile) {
			// Pick a millisecond-epoch clock once, at make parse time, that
			// works across GNU (date +%s%N) and BSD/macOS (no %N) — fall back
			// to python3, then to whole-second resolution. The chosen command
			// string becomes $(MXX_NOW), used by the per-rule timing wrappers.
			myfile << "MXX_NOW := $(shell if date +%s%N 2>/dev/null | grep -q N; then "
			          "command -v python3 >/dev/null 2>&1 && "
			          "echo \"python3 -c 'import time;print(int(time.time()*1000))'\" || "
			          "echo 'date +%s000'; else echo 'date +%s%N|cut -c1-13'; fi)" << std::endl;
		}
		myfile << ".PHONY:";
		for(auto const &p : phony_targets) myfile << " " << p;
		myfile << std::endl;
		myfile << "-include .makexx_deps" << std::endl;
		myfile << "makefile: makefile.cpp makefile.hpp" << std::endl;
		myfile << "\tmakexx -c" << std::endl;
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++) {
			if(target_rule.find(*itr) != target_rule.end()) {
				if(processed_nodes.find(*itr) == processed_nodes.end()) {
					auto command = target_rule[*itr];
					auto isfinal = (command->type == FINAL);
					auto trange = rule_target.equal_range(command);
					std::string to;
					std::set<std::string> to_list;
					std::set<std::string> from_list;
					for(auto j = trange.first; j != trange.second; j++) {
						to += j->second + " ";
						to_list.insert(j->second);
						processed_nodes.insert(j->second);
					}
					makefile += to;
					if(isfinal) {
						for(auto &itm : to_list)
							defaultmake_list.insert(itm);
					}
					makefile += ": ";
					std::string from;
					auto srange = rule_source.equal_range(command);
					for(auto j = srange.first; j != srange.second; j++) {
                        from_list.insert(j->second);
						from += j->second + " ";
					}
					makefile += from;
					// External-tool prereqs: literal path if the name contains
					// '/', otherwise let make resolve via PATH at parse time.
					for(auto const &tool : command->tools) {
						if(tool.find('/') != std::string::npos)
							makefile += tool + " ";
						else
							makefile += "$(shell command -v " + tool + " 2>/dev/null) ";
					}
					makefile += "\n";
					// Per-rule timing instrumentation (mf.profile). Wrap the
					// recipe: stamp the start to a per-target temp before the
					// recipe, append a raw event (epoch, "rule", target,
					// duration_ms) to .makexx_hits after it. Keyed by the
					// literal $@ so the log joins cleanly to graph nodes /
					// --stats. Skips built-ins and recipe-less rules. Each
					// recipe line stays its own shell (no .ONESHELL), so make's
					// fail-fast / per-line semantics are unchanged; a failed
					// recipe simply doesn't reach the trailing log line.
					static const std::set<std::string> builtin_names = {
						"all", "full_clean", "soft_clean", "list",
						"list_unknown", "list_input", "help", "graph"};
					bool instrument = profile && !command->commands.empty();
					if(instrument)
						for(auto &i : to_list)
							if(builtin_names.count(i)) { instrument = false; break; }
					if(instrument)
						makefile += "\t@mkdir -p .makexx_prof && $(MXX_NOW) > "
						            "'.makexx_prof/$(subst /,_,$@).t0'\n";
					bool is_phony = false;
					for(auto &i : to_list)
						if(phony_targets.count(i)) { is_phony = true; break; }
					if(echo && !is_phony){
						makefile += "\t@echo \"### GENERATING : \"\n";
                        for(auto & i: to_list)
                            makefile += std::string("\t@echo \"###   "+i+"\"\n");
                        if(from_list.size()>0)
                            makefile += "\t@echo \"### FROM : \"\n";
                        else
                            makefile += "\t@echo \"### FROM SCRATCH. \"\n";

                        for(auto & i: from_list)
                            makefile += std::string("\t@echo \"###   "+i+"\"\n");
                    }
					if(silent)
						makefile += command->formatted("\t@") + "\n";
					else
						makefile += command->formatted("\t") + "\n";
					if(instrument)
						makefile += "\t@_e=$$($(MXX_NOW)); "
						            "_s=$$(cat '.makexx_prof/$(subst /,_,$@).t0' 2>/dev/null||echo $$_e); "
						            "printf '%s\\trule\\t%s\\t%s\\n' \"$$(date +%s)\" '$@' \"$$((_e-_s))\" "
						            ">> .makexx_hits; rm -f '.makexx_prof/$(subst /,_,$@).t0'\n";
					for(auto tmp : command->temp_files) {
						temps_list.insert(tmp);
					}
					for(auto tmp : command->byproducts) {
						byprods_list.insert(tmp);
					}
					for(auto tmp : command->hidden_targets) {
						hidden_targets_list.insert(tmp);
					}
					if(command->retain_targets) {
						for(auto &t : to_list)
							soft_clean_retain_nodes.insert(t);
					}
					for(auto &f : command->retain_files)
						soft_clean_retain_nodes.insert(f);
				}
			} else {
				inputfiles_list.insert(*itr);
			}
		}
		myfile << "#input files:" << to_string(inputfiles_list) << std::endl;
		myfile << "all : " << to_string(defaultmake_list) << std::endl;
        {
            myfile << "full_clean: \n"<<std::endl;
            // The .makexx_prof temp dir is timing scratch; remove it. The
            // .makexx_hits log is deliberately NOT cleaned — it's accumulated
            // usage history and must survive cleans to be useful.
            if(profile)
                myfile << "\trm -rf .makexx_prof\n";
            for(auto &itm : processed_nodes){
                myfile << "\trm -f \"" << itm << "\"\n";
            }

            for(auto &itm : byprods_list)
                myfile << "\trm -f \"" << itm << "\"\n";

            for(auto &itm : temps_list)
                myfile << "\trm -f \"" << itm << "\"\n";
        }
        //soft cleaning
        {
            myfile << "soft_clean: \n"<<std::endl;
            for(auto &itm : processed_nodes){
                if(soft_clean_retain_nodes.find(itm)==soft_clean_retain_nodes.end())
                    myfile << "\trm -f \"" << itm << "\"\n";
            }

            for(auto &itm : byprods_list)
                if(soft_clean_retain_nodes.find(itm)==soft_clean_retain_nodes.end())
                    myfile << "\trm -f \"" << itm << "\"\n";

            for(auto &itm : temps_list)
                if(soft_clean_retain_nodes.find(itm)==soft_clean_retain_nodes.end())
                    myfile << "\trm -f \"" << itm << "\"\n";
        }
        //listing
		myfile << "list: \n\t@echo \"Input Files:\"\n";
		for(auto &itm : inputfiles_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << "\t@echo \"Interm Rules & Files:\"\n";
		for(auto &itm : processed_nodes) {
			if(defaultmake_list.find(itm) == defaultmake_list.end()) {
				myfile << "\t@echo \"\t" + itm + "\"\n";
			}
		}
		myfile << "\t@echo \"Byproduct Files:\"\n";
		for(auto &itm : byprods_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << "\t@echo \"Manual/Non-reproducible Target Files:\"\n";
		for(auto &itm : hidden_targets_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << "\t@echo \"Temp Files:\"\n";
		for(auto &itm : temps_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << "\t@echo \"Default Make Files:\"\n";
		for(auto &itm : defaultmake_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << "\t@echo \"Unknown Files in Directory:\"\n";
		std::string u_command = "\t@ls --ignore={\"makefile.cpp\",\"makefile.hpp\",\".makexx_gen\",makefile,makefile_graph.*"
						   ",\"*.*@\",\"*.dbf\",\"*.prj\",\"*.shx\",\"*.cpg\",\"*.aux\",\"*.lt\",\"*.acn\",\"*.bbl\",\"*.blg\",\"*.glo\",\"*ist\",\"*.log\""
						   ",\"*.toc\",\"*.sbl\",\"*.out\",\".makexx_lsdir\",\".makexx_nodes\",\".makexx_probe.cpp\",\".makexx_probe\",\".makexx_err\",\".makexx_hits\",\".makexx_prof\"}>.makexx_lsdir\n"
						   ;
		u_command += "\t@grep -xv -f .makexx_nodes .makexx_lsdir | sed 's/^/\t/'\n\trm -f .makexx_lsdir";
		std::ofstream exception_file(".makexx_nodes");
		auto write_exception = [&](std::string const &itm) { exception_file << itm << "\n"; };
		{
			for(auto &itm : inputfiles_list)
				write_exception(itm);
            for(auto &itm : processed_nodes)
                write_exception(itm);
            for(auto &itm : temps_list)
                write_exception(itm);
            for(auto &itm : byprods_list)
                write_exception(itm);
            for(auto &itm : hidden_targets_list)
                write_exception(itm);
            for(auto &itm : hidden_nodes)
                write_exception(itm);
        }
		exception_file.close();
		myfile << u_command;
		myfile << "\nlist_unknown:\n";
		myfile << "\t@echo \"Unknown Files in Directory:\"\n";
		{
			myfile << u_command;
		}
		myfile << "\nlist_input:\n";
		for(auto &itm : inputfiles_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << makefile;
		if(graph) {
			generate_graph();
			write_graph_json();
			myfile << "\nmakefile_graph.pdf : makefile_graph.gv\n";
			myfile << "\tdot -v -Tpdf makefile_graph.gv -o makefile_graph.pdf\n";
			// Standalone interactive HTML graph: `makexx --build-graph`
			// inlines the embedded Cytoscape viewer + this run's
			// .makexx_graph.json into a single self-contained file; `make
			// graph` builds it and opens it in the browser.
			myfile << "makefile_graph.html : .makexx_graph.json\n";
			myfile << "\tmakexx --build-graph\n";
			myfile << "graph : makefile_graph.html\n";
			myfile << "\t@" << open_file("makefile_graph.html") << "\n";
		}
		myfile.close();
		write_menu_file(graph);
		if(context)
			generate_context(graph);
	}

	void generate_context(bool graph = false) {
		std::ofstream cf(context_filename);
		cf << "# " << (title.empty() ? "Project" : title) << "\n\n";
		if(!description.empty())
			cf << description << "\n\n";

		cf << "## Build system\n\n";
		cf << "This project uses [makexx](https://github.com/abdullah-altheyab/makexx) — a C++ build-system generator. Build rules are written in `makefile.cpp` using a small C++ DSL; `makexx` compiles that program, runs it to produce a standard GNU `makefile`, then invokes `make`.\n\n";
		cf << "**Workflow when modifying the build:**\n\n";
		cf << "1. Edit `makefile.cpp` (and any `config.hpp` it includes).\n";
		cf << "2. Run `makexx` to recompile, regenerate the makefile, and build.\n";
		cf << "3. Run `makexx -c` to just regenerate the makefile without building.\n";
		cf << "4. Run `makexx -i` for an interactive TUI to browse, search, and run targets.\n\n";
		cf << "**DSL quick reference** (offline copy; see full reference URLs below):\n\n";
		cf << "```cpp\n";
		cf << "#include \"makefile.hpp\"\n";
		cf << "int main() {\n";
		cf << "    Makefile mf;\n";
		cf << "    auto& r = mf.add(\"output.o\", \"input.cpp\");   // or {tgt1,tgt2}, {src1,src2}\n";
		cf << "    r << \"g++ -c $< -o $@\";                      // append commands ($<, $@, $^)\n";
		cf << "    r << FINAL;                                   // included in `make all`\n";
		cf << "    r << PHONY;                                   // REQUIRED whenever the target name\n";
		cf << "                                                  //   isn't a file the recipe creates\n";
		cf << "                                                  //   (install, clean_*, list_*, etc.)\n";
		cf << "    r << HELP(\"description shown in make help\");\n";
		cf << "    r << DESC(\"input.txt\", \"what this file is\");  // also mf << DESC(...);\n";
		cf << "                                                  //   multiple DESC for the same file\n";
		cf << "                                                  //   accumulate (joined with a space)\n";
		cf << "    r << TEMP(\"scratch.tmp\");                     // cleaned by full_clean/soft_clean\n";
		cf << "    r << RETAIN;                                  // exclude rule outputs from soft_clean\n";
		cf << "    r << TOOL(\"g++\");                             // executable prereq, mtime-tracked\n";
		cf << "    r << MENU(\"Build\");                           // put this rule in a group\n";
		cf << "    r << MENU(\"Build/Tests\");                     // nested group (parents auto-created)\n";
		cf << "\n";
		cf << "    mf << MENU(\"Build/Tests\", \"unit tests\");      // DECLARE a group description/FOLDED (groups no rule)\n";
		cf << "    mf << PHONY(\"install\");                       // declare phony by name\n";
		cf << "    mf.title       = \"My Project\";\n";
		cf << "    mf.description = \"Project summary for AGENTS.md\";\n";
		cf << "    mf.preamble    = \"CFLAGS ?= -O2\\n\";           // raw make injected near top\n";
		cf << "\n";
		cf << "    // Path helpers: stem, basename, change_ext, join_path, get_ext\n";
		cf << "    // Cross-platform file opener: open_file(\"report.pdf\")\n";
		cf << "    mf.generate();                                // emit makefile + AGENTS.md\n";
		cf << "    return 0;\n";
		cf << "}\n";
		cf << "```\n\n";
		cf << "**Full DSL reference** (fetch for details / edge cases):\n\n";
		cf << "- <https://raw.githubusercontent.com/abdullah-altheyab/makexx/main/include/makexxfile.hpp>\n";
		cf << "- <https://raw.githubusercontent.com/abdullah-altheyab/makexx/main/CLAUDE.md>\n\n";

		std::set<std::string> inputfiles_list;
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++) {
			if(target_rule.find(*itr) == target_rule.end())
				inputfiles_list.insert(*itr);
		}
		// Combined file-description map. Multiple DESC calls accumulate
		// (joined with a space) across scopes too: mf-level annotations
		// come first, then rule-level descriptions in command-insertion
		// order.
		std::map<std::string, std::string> all_file_desc = combined_file_descriptions();
		if(!inputfiles_list.empty()) {
			cf << "## Input files\n\n";
			for(auto &itm : inputfiles_list) {
				cf << "- `" << itm << "`";
				auto it = all_file_desc.find(itm);
				if(it != all_file_desc.end())
					cf << " \xe2\x80\x94 " << it->second;
				cf << "\n";
			}
			cf << "\n";
		}

		// Build a flat set of phony target names (matches what's emitted in
		// the makefile's .PHONY: line), so we can annotate them per-target.
		std::set<std::string> phony_set;
		for(auto const &cmd : commands) {
			if(cmd->phony_all) {
				auto trange = rule_target.equal_range(cmd.get());
				for(auto j = trange.first; j != trange.second; j++)
					phony_set.insert(j->second);
			}
			for(auto const &t : cmd->phony_targets)
				phony_set.insert(t);
		}
		for(auto const &t : mf_phony_targets)
			phony_set.insert(t);

		bool has_ungrouped = false;
		for(auto &itm : help_menu)
			if(itm.group.empty() && itm.bracket != BRK_MULTI_MID && itm.bracket != BRK_MULTI_END) {
				has_ungrouped = true;
				break;
			}

		auto write_entries = [&](std::ofstream &out, std::string const &group) {
			for(auto &itm : help_menu) {
				if(itm.group != group) continue;
				if(itm.bracket == BRK_MULTI_MID || itm.bracket == BRK_MULTI_END) continue;
				std::string flat_desc = replace_all(itm.description, "\n", " ");
				auto srange = rule_source.equal_range(target_rule[itm.target]);
				std::string deps;
				for(auto j = srange.first; j != srange.second; j++) {
					if(!deps.empty()) deps += ", ";
					deps += "`" + j->second + "`";
				}
				std::string tool_str;
				for(auto const &t : target_rule[itm.target]->tools) {
					if(!tool_str.empty()) tool_str += ", ";
					tool_str += "`" + t + "`";
				}
				out << "- `make " << itm.target << "`";
				if(phony_set.count(itm.target))
					out << " (phony)";
				if(!deps.empty())
					out << " (from " << deps << ")";
				if(!tool_str.empty())
					out << " (uses " << tool_str << ")";
				if(!flat_desc.empty())
					out << " \xe2\x80\x94 " << flat_desc;
				out << "\n";
				// File-level description (DESC) for the target — describes
				// the artifact (contents/format), distinct from HELP which
				// describes what the rule does.
				auto desc_it = all_file_desc.find(itm.target);
				if(desc_it != all_file_desc.end())
					out << "  - File: " << desc_it->second << "\n";
			}
		};

		cf << "## Targets\n\n";
		if(has_ungrouped)
			write_entries(cf, "");
		for(auto &grp : help_group_order) {
			// Encode hierarchy two ways: heading level (more `#`s for deeper
			// groups) and the full slash-path in the heading text. The
			// earlier "indented `### Leaf`" form read as a stray indent.
			int depth = std::count(grp.begin(), grp.end(), '/');
			std::string hashes(3 + depth, '#');
			cf << hashes << " " << grp << "\n\n";
			auto dit = group_descriptions.find(grp);
			if(dit != group_descriptions.end())
				cf << "_" << dit->second << "_\n\n";
			write_entries(cf, grp);
			cf << "\n";
		}

		// Targets without HELP() — typically internal/glue rules whose names
		// appear in other rules' dep lists. Listed so the dependency graph
		// the workflow user reads is complete; otherwise a "(from
		// `bodd_forecast.t`)" annotation can look orphaned.
		std::vector<Rule *> unlabeled = collect_undocumented_rules();
		if(!unlabeled.empty()) {
			cf << "## Intermediate targets\n\n";
			cf << "_Rules without a `HELP()` description — typically internal / glue steps. Listed so the dependency graph stays complete (a `(from X)` annotation above might point to one of these)._\n\n";
			for(auto *r : unlabeled) {
				auto trange = rule_target.equal_range(r);
				std::string tlist;
				for(auto j = trange.first; j != trange.second; j++) {
					if(!tlist.empty()) tlist += ", ";
					tlist += "`" + j->second + "`";
				}
				std::string deps;
				auto srange = rule_source.equal_range(r);
				for(auto j = srange.first; j != srange.second; j++) {
					if(!deps.empty()) deps += ", ";
					deps += "`" + j->second + "`";
				}
				cf << "- " << tlist;
				if(!deps.empty()) cf << " (from " << deps << ")";
				cf << "\n";
				// If any of this rule's targets has a DESC, append it.
				for(auto j = trange.first; j != trange.second; j++) {
					auto desc_it = all_file_desc.find(j->second);
					if(desc_it != all_file_desc.end())
						cf << "  - `" << j->second << "`: " << desc_it->second << "\n";
				}
			}
			cf << "\n";
		}

		cf << "## Built-in targets\n\n";
		cf << "- `make all` — Build everything marked `FINAL`\n";
		cf << "- `make full_clean` — Remove all generated files\n";
		cf << "- `make soft_clean` — Remove generated files except those marked `RETAIN`\n";
		cf << "- `make list` — List all files makexx tracks (inputs + intermediates + final + temp)\n";
		cf << "- `make list_input` — List input files (sources not produced by any rule)\n";
		cf << "- `make list_unknown` — List files in the directory that makexx doesn't know about\n";
		cf << "- `make help` — Same target list as above with brackets / grouping\n";
		if(graph) {
			cf << "- `make makefile_graph.pdf` — Render dependency graph as PDF (requires Graphviz)\n";
			cf << "- `make graph` — Open the interactive dependency graph (standalone HTML) in a browser\n";
		}
		cf << "\n";

		// When profiling is on, hand agents the breadcrumbs to estimate build
		// time themselves: where the data is, its exact schema, the join key,
		// and the procedure. We document the data and how to use it, not just
		// the project shape — the analysis stays agent-side.
		if(profile) {
			cf << "## Usage & timing data\n\n";
			cf << "This project records how long each rule takes (`mf.profile` is on). On every "
			      "successful build, one tab-separated row is appended to the append-only "
			      "**`.makexx_hits`** log (it accumulates across runs):\n\n";
			cf << "`epoch_seconds <TAB> rule <TAB> target <TAB> duration_ms` — one row per rule execution. Columns:\n\n";
			cf << "- `epoch_seconds` — when the rule finished (Unix time)\n";
			cf << "- the literal word `rule`\n";
			cf << "- `target` — the built target. **This is the same string as the targets listed above"
			   << (graph ? " and as the node ids in `.makexx_graph.json`" : "")
			   << "** — use it to join timing data to the dependency graph.\n";
			cf << "- `duration_ms` — wall-clock run time of that rule, in milliseconds\n";
			cf << "\nOnly successful runs are logged (a failed recipe never reaches the log line).\n\n";
			cf << "### Estimating how long a target takes to build\n\n";
			cf << "1. Find the target's dependency closure (the rules that must run to produce it) from "
			      "the **Targets** section above"
			   << (graph ? " or from `.makexx_graph.json` (machine-readable nodes + edges)" : "")
			   << ".\n";
			cf << "2. For each of those rules, take the **median** `duration_ms` of its rows in "
			      "`.makexx_hits` (median, not mean — early runs are cold-cache outliers).\n";
			cf << "3. **Sum** the medians for a serial (`make` / `-j1`) build, or take the **critical "
			      "path** (longest time-weighted chain through the dependency graph) for a parallel "
			      "(`make -j`) build. Report the two as a range.\n";
			cf << "4. Rules with **no rows** in the log have no data — report them as unknown, don't guess. "
			      "Confidence ≈ the fraction of the closure that has history.\n";
			cf << "5. For an *incremental* estimate (only what's currently out of date), run "
			      "`make -n <target>` to see which rules would actually execute, and sum just those.\n";
			cf << "\nCaveat: under `make -j` rule timings overlap, so the sum overestimates parallel "
			      "wall time — the critical path is the better parallel estimate.\n\n";
		}
		cf.close();
	}

	// Combined file-description map: makefile-level DESC annotations first,
	// then per-rule DESC in command-insertion order (accumulated, space-
	// joined). Shared by the AGENTS.md and menu writers so both surface the
	// same DESC text.
	std::map<std::string, std::string> combined_file_descriptions() {
		std::map<std::string, std::string> all = mf_file_descriptions;
		for(auto const &cmd : commands)
			for(auto const &kv : cmd->file_descriptions) {
				auto &existing = all[kv.first];
				if(existing.empty()) existing = kv.second;
				else existing += " " + kv.second;
			}
		return all;
	}

	// Rules with no HELP() — typically internal / glue steps whose names show
	// up in other rules' dep lists. Excludes the built-in targets (which are
	// surfaced separately). Shared by the AGENTS.md "Intermediate targets"
	// section and the menu file's folded "Undocumented" group.
	std::vector<Rule *> collect_undocumented_rules() {
		static const std::set<std::string> builtin_names = {
			"all", "full_clean", "soft_clean", "list", "list_unknown", "list_input", "help"
		};
		std::vector<Rule *> out;
		for(auto const &cmd : commands) {
			if(!cmd->help_lines.empty()) continue;
			Rule *r = cmd.get();
			auto trange = rule_target.equal_range(r);
			if(trange.first == trange.second) continue;
			bool is_builtin = false;
			for(auto j = trange.first; j != trange.second; j++)
				if(builtin_names.count(j->second)) { is_builtin = true; break; }
			if(is_builtin) continue;
			out.push_back(r);
		}
		return out;
	}

	void write_menu_file(bool graph) {
		std::ofstream mf(".makexx_menu");
		// Declare groups in canonical order (matching `make help`) so the
		// interactive TUI doesn't fall back to entry-insertion order, which
		// diverges when per-rule MENU() registers a group later than its
		// neighbors declared via `mf << MENU(...)`.
		for(auto &grp : help_group_order)
			mf << "!group\t" << grp << "\t" << (folded_groups.count(grp) ? "+" : "") << "\n";
		for(auto &kv : group_descriptions) {
			std::string flat = replace_all(kv.second, "\n", " ");
			mf << "!desc\t" << kv.first << "\t" << flat << "\n";
		}
		auto group_prefix = [&](std::string const &g) {
			std::string name = g.empty() ? "_" : g;
			if(folded_groups.count(g)) return "+" + name;
			return name;
		};
		for(auto &itm : help_menu) {
			if(itm.bracket == BRK_MULTI_MID || itm.bracket == BRK_MULTI_END) {
				mf << group_prefix(itm.group) << "\t=" << itm.target << "\t\n";
				continue;
			}
			std::string desc = replace_all(itm.description, "\n", "|");
			mf << group_prefix(itm.group) << "\t" << itm.target << "\t" << desc << "\n";
		}
		// Targets without HELP() — collected into a folded "Undocumented"
		// group so they don't clutter the default view but stay browsable
		// (Tab to it, → to unfold) and findable via `/` search, which
		// surfaces matches even under folded parents. DESC text is shown as
		// the description when available. Declared before the Built-in
		// entries so it sorts above them in the TUI.
		auto undoc = collect_undocumented_rules();
		if(!undoc.empty()) {
			auto file_desc = combined_file_descriptions();
			mf << "!group\tUndocumented\t+\n";
			mf << "!desc\tUndocumented\tRules without a HELP() description\n";
			for(auto *r : undoc) {
				auto trange = rule_target.equal_range(r);
				for(auto j = trange.first; j != trange.second; j++) {
					std::string desc;
					auto it = file_desc.find(j->second);
					if(it != file_desc.end())
						desc = replace_all(it->second, "\n", "|");
					mf << "Undocumented\t" << j->second << "\t" << desc << "\n";
				}
			}
		}
		std::string builtin_group = "Built-in";
		auto write_builtin = [&](std::string name, std::string desc) {
			mf << builtin_group << "\t" << name << "\t" << desc << "\n";
		};
		write_builtin("all",          "Build all final targets");
		write_builtin("full_clean",   "Remove all generated files");
		write_builtin("soft_clean",   "Remove generated files (except retained)");
		write_builtin("list",         "List all tracked files");
		write_builtin("list_unknown", "List untracked files in directory");
		write_builtin("list_input",   "List input files");
		write_builtin("help",         "Show this help");
		if(graph) {
			write_builtin("makefile_graph.pdf", "Generate dependency graph PDF (requires Graphviz)");
			write_builtin("graph", "Open the interactive dependency graph in a browser");
		}
		mf.close();
	}

	// `mf << MENU("g")` / `MENU("g","desc")` / `MENU("g",FOLDED)` *declares* a
	// group — registers it (for ordering) and records its description / folded
	// state. It does NOT set a current group and does NOT affect any rule: a
	// rule joins a group only via its own `<< MENU(...)` / `<< HELP("g",...)`.
	Makefile& operator<<(MENU m) {
		if(m.has_display)
			define_menu(m.group, m.display);
		else
			define_menu(m.group);
		if(!m.description.empty())
			group_descriptions[m.group] = m.description;
		return *this;
	}

	// `mf << PHONY("install")` or `mf << PHONY("a","b")` marks named targets
	// as .PHONY at the makefile level (useful when the producing rule is
	// defined elsewhere or you want the declaration up front). The no-arg
	// `mf << PHONY` form is a no-op — there's nothing to "mark all of"
	// without a rule context.
	Makefile& operator<<(PHONY_t p) {
		if(!p.phony_all)
			for(auto const &t : p.targets)
				mf_phony_targets.insert(t);
		return *this;
	}

	// `mf << RETAIN("file")`, `RETAIN("a","b")`, or `RETAIN({"a","b"})`
	// excludes named files from soft_clean (same effect as
	// `mf.on_softclean_retain(...)`, just on the `<<` idiom). The no-arg
	// `mf << RETAIN` form is a no-op.
	Makefile& operator<<(RETAIN_t r) {
		if(!r.retain_targets)
			for(auto const &f : r.filenames)
				soft_clean_retain_nodes.insert(f);
		return *this;
	}

	// `mf << DESC("file", "description")` registers a free-text description
	// for a file (typically an input), shown next to it in AGENTS.md.
	// Multiple calls for the same file accumulate (joined with a space).
	Makefile& operator<<(DESC d) {
		auto &existing = mf_file_descriptions[d.file];
		if(existing.empty()) existing = d.description;
		else existing += " " + d.description;
		return *this;
	}

	static std::string json_escape(std::string const &s) {
		std::string o;
		o.reserve(s.size() + 8);
		for(unsigned char c : s) {
			switch(c) {
				case '"':  o += "\\\""; break;
				case '\\': o += "\\\\"; break;
				case '\n': o += "\\n";  break;
				case '\r': o += "\\r";  break;
				case '\t': o += "\\t";  break;
				default:
					if(c < 0x20) {
						std::ostringstream h;
						h << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
						o += h.str();
					} else {
						o += (char)c;
					}
			}
		}
		return o;
	}

	// Dump the dependency graph as JSON (nodes + edges with type / group /
	// HELP / DESC metadata) for the standalone interactive viewer. `makexx
	// --build-graph` inlines this into makefile_graph.html. Kept separate
	// from the Graphviz `.gv` so both the static PDF and the interactive
	// HTML stay available.
	// Extract `#hashtag` facet tags from free text (HELP / DESC): `#` at a
	// word boundary (start or after a non-alphanumeric char, so `C#` /
	// `word#x` don't match) followed by one or more of `[A-Za-z0-9_-]`.
	// Numbers-first is fine (`#42`). De-duplicated, first-seen order. The
	// hashtag stays visible in the source HELP/DESC; this just also surfaces it.
	static std::vector<std::string> extract_tags(std::string const &s) {
		auto is_alnum = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); };
		auto is_word  = [&](char c) { return is_alnum(c) || c == '_' || c == '-'; };
		std::vector<std::string> out;
		std::set<std::string> seen;
		for(size_t i = 0; i < s.size(); i++) {
			if(s[i] != '#') continue;
			if(i > 0 && is_alnum(s[i - 1])) continue;   // word boundary before #
			size_t k = i + 1;
			while(k < s.size() && is_word(s[k])) k++;
			if(k == i + 1) continue;                    // nothing after #
			std::string tag = s.substr(i + 1, k - i - 1);
			if(seen.insert(tag).second) out.push_back(tag);
			i = k - 1;
		}
		return out;
	}

	void write_graph_json() {
		std::set<std::string> phony_set;
		for(auto const &cmd : commands) {
			if(cmd->phony_all) {
				auto tr = rule_target.equal_range(cmd.get());
				for(auto j = tr.first; j != tr.second; j++) phony_set.insert(j->second);
			}
			for(auto const &t : cmd->phony_targets) phony_set.insert(t);
		}
		for(auto const &t : mf_phony_targets) phony_set.insert(t);

		std::set<std::string> final_set;
		for(auto const &cmd : commands) {
			if(cmd->type != FINAL) continue;
			auto tr = rule_target.equal_range(cmd.get());
			for(auto j = tr.first; j != tr.second; j++) final_set.insert(j->second);
		}

		// `help` is a real rule (others like all/clean are raw makefile
		// text), so filter the built-in target names out of the graph.
		static const std::set<std::string> builtin_names = {
			"all", "full_clean", "soft_clean", "list", "list_unknown", "list_input", "help"
		};

		auto file_desc = combined_file_descriptions();
		auto node_type = [&](std::string const &n) -> std::string {
			if(phony_set.count(n)) return "phony";
			if(final_set.count(n)) return "final";
			if(target_rule.find(n) != target_rule.end()) return "intermediate";
			return "input";
		};
		auto node_group = [&](std::string const &n) -> std::string {
			auto it = target_rule.find(n);
			return it != target_rule.end() ? it->second->help_group : std::string();
		};
		auto node_help = [&](std::string const &n) -> std::string {
			auto it = target_rule.find(n);
			if(it == target_rule.end()) return "";
			std::string h;
			for(auto const &l : it->second->help_lines) {
				if(!h.empty()) h += " ";
				h += l;
			}
			return h;
		};
		// Tags for a node = hashtags from its rule's HELP plus hashtags from
		// any DESC on that target/file. Cmds = the producing rule's recipe
		// lines (verbatim — $@/$< not expanded) so a traced path can show what
		// each step actually runs.
		auto node_tags = [&](std::string const &n) -> std::vector<std::string> {
			std::string text = node_help(n);
			auto dit = file_desc.find(n);
			if(dit != file_desc.end()) text += " " + dit->second;
			return extract_tags(text);
		};
		auto node_cmds = [&](std::string const &n) -> std::vector<std::string> {
			auto it = target_rule.find(n);
			if(it == target_rule.end()) return {};
			return it->second->commands;
		};
		auto node_srcline = [&](std::string const &n) -> int {
			auto it = target_rule.find(n);
			return it != target_rule.end() ? it->second->src_line : 0;
		};
		auto json_arr = [&](std::vector<std::string> const &v) -> std::string {
			std::string o;
			for(size_t i = 0; i < v.size(); i++) {
				if(i) o += ",";
				o += "\"" + json_escape(v[i]) + "\"";
			}
			return o;
		};

		std::ofstream j(".makexx_graph.json");
		j << "{\"title\":\"" << json_escape(title) << "\","
		  << "\"description\":\"" << json_escape(description) << "\",\"nodes\":[";
		bool first = true;
		std::set<std::string> emitted;
		auto excluded = [&](std::string const &n) {
			return hidden_nodes.count(n) || builtin_names.count(n);
		};
		auto emit_node = [&](std::string const &n) {
			if(!first) j << ","; first = false;
			std::string desc;
			auto dit = file_desc.find(n);
			if(dit != file_desc.end()) desc = dit->second;
			j << "{\"id\":\"" << json_escape(n) << "\","
			  << "\"label\":\"" << json_escape(n) << "\","
			  << "\"ext\":\"" << json_escape(get_ext(n)) << "\","
			  << "\"type\":\"" << node_type(n) << "\","
			  << "\"group\":\"" << json_escape(node_group(n)) << "\","
			  << "\"help\":\"" << json_escape(node_help(n)) << "\","
			  << "\"desc\":\"" << json_escape(desc) << "\","
			  << "\"tags\":[" << json_arr(node_tags(n)) << "],"
			  << "\"srcline\":" << node_srcline(n) << ","
			  << "\"cmds\":[" << json_arr(node_cmds(n)) << "]}";
			emitted.insert(n);
		};
		for(auto const &n : nodes)
			if(!excluded(n)) emit_node(n);
		// External-tool prereqs (`<< TOOL(...)`) aren't in `nodes`; surface
		// them as their own typed nodes so tool dependencies are visible.
		std::set<std::string> tool_nodes;
		for(auto const &cmd : commands)
			for(auto const &t : cmd->tools) tool_nodes.insert(t);
		for(auto const &t : tool_nodes) {
			if(emitted.count(t)) continue;
			if(!first) j << ","; first = false;
			j << "{\"id\":\"" << json_escape(t) << "\",\"label\":\"" << json_escape(t)
			  << "\",\"ext\":\"\",\"type\":\"tool\",\"group\":\"\",\"help\":\"\",\"desc\":\"\","
			  << "\"tags\":[],\"srcline\":0,\"cmds\":[]}";
			emitted.insert(t);
		}
		// Some prereqs are wired via add_source and never entered `nodes`, so
		// they show up only as edge endpoints. Emit them too — otherwise an
		// edge would reference a nonexistent node and the viewer's graph
		// library throws on load (blank page). This keeps edges and nodes
		// consistent.
		for(auto const &cmd : commands) {
			auto sr = rule_source.equal_range(cmd.get());
			auto tr = rule_target.equal_range(cmd.get());
			for(auto si = sr.first; si != sr.second; si++)
				if(!excluded(si->second) && !emitted.count(si->second)) emit_node(si->second);
			for(auto ti = tr.first; ti != tr.second; ti++)
				if(!excluded(ti->second) && !emitted.count(ti->second)) emit_node(ti->second);
		}
		j << "],\"edges\":[";
		first = true;
		for(auto const &cmd : commands) {
			auto sr = rule_source.equal_range(cmd.get());
			auto tr = rule_target.equal_range(cmd.get());
			for(auto ti = tr.first; ti != tr.second; ti++) {
				if(!emitted.count(ti->second)) continue;          // only edges between emitted nodes
				for(auto si = sr.first; si != sr.second; si++) {
					if(!emitted.count(si->second)) continue;
					if(!first) j << ","; first = false;
					j << "{\"source\":\"" << json_escape(si->second)
					  << "\",\"target\":\"" << json_escape(ti->second) << "\"}";
				}
				for(auto const &tool : cmd->tools) {
					if(!emitted.count(tool)) continue;
					if(!first) j << ","; first = false;
					j << "{\"source\":\"" << json_escape(tool)
					  << "\",\"target\":\"" << json_escape(ti->second) << "\",\"tool\":true}";
				}
			}
		}
		j << "]}";
		j.close();
	}

	void generate_graph(std::string filename = "makefile_graph.gv") {
		std::ofstream myfile;
		myfile.open("makefile_graph.gv");
		myfile << "digraph G{\n";
		myfile << "rankdir=\"LR\";\n";
		myfile << "graph[ranksep=\"40\"];\n";
		int cmd_counter = 0;
		std::map<std::string, std::string> filetype_color;
		std::set<std::string> zero_rank_nodes;
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++) {
			if(hidden_nodes.find(*itr) != hidden_nodes.end())
				continue;
			std::string const scheme = "set312"; //"pastel28"
			std::string extension = get_ext(*itr);
			if(extension == "") {
				//rule that is not a file
				myfile << "\"" + (*itr) + "\" [shape=box, style=filled, fillcolor=black, fontcolor=white];\n";
			} else {
				if(filetype_color.find(extension) == filetype_color.end()) {
					filetype_color[extension] = std::to_string((filetype_color.size() % 8 + 1));
				}
				std::string penwidth = "";
				std::string postfix = "";
				if(target_rule.find(*itr) != target_rule.end()) {
				} else {
					postfix = "*";
					penwidth = ", penwidth=10.0";
					zero_rank_nodes.insert(*itr);
				}
				myfile << "\"" + (*itr) + "\" [label=\"" + (*itr) + postfix + "\", shape=box, style=filled, colorscheme=" + scheme + ", fillcolor=" + filetype_color[extension] + ", fontcolor=black " + penwidth + "];\n";
			}
		}
		for(auto const& cmd : commands) {
			cmd_counter++;
			std::string const scheme = "set312"; //"pastel28"
			std::string randcolor = std::to_string(1 + cmd_counter % 12);
			myfile << " \"node" + std::to_string(cmd_counter) + "\" [label=\"\", shape=circle, style=filled, colorscheme=" + scheme + ", fillcolor=" + randcolor + "];\n";
			auto srange = rule_source.equal_range(cmd.get());
			int nsource = 0;
			for(auto j = srange.first; j != srange.second; j++) {
				if(hidden_nodes.find(j->second) != hidden_nodes.end()) {
					continue;
				}
				myfile << "  \"" << j->second << "\" -> \"node" << std::to_string(cmd_counter) << "\" [colorscheme=" + scheme + ", color=" + randcolor + ", penwidth=3];\n";
				nsource++;
			}
			if(nsource == 0) {
				//in this case the command has no source
				zero_rank_nodes.insert("\"node" + std::to_string(cmd_counter) + "\"");
			}
			auto trange = rule_target.equal_range(cmd.get());
			for(auto j = trange.first; j != trange.second; j++) {
				if(hidden_nodes.find(j->second) != hidden_nodes.end()) {
					continue;
				}
				myfile << "  \"node" << std::to_string(cmd_counter) << "\" -> \"" << j->second << "\" [colorscheme=" + scheme + ", color=" + randcolor + ", penwidth=3];\n";
			}
		}
		myfile << "{rank = same;";
		for(auto n : zero_rank_nodes) {
			myfile << "\"" + n + "\";";
		}
		myfile << "}\n";
		myfile << "}";
		myfile.close();
	}
};
