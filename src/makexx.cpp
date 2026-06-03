#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <chrono>
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
// Embedded interactive dependency-graph viewer assets (vendored Cytoscape
// stack + our viewer template). Assembled into a standalone HTML file by
// build_graph_html().
#include "graph_dagre_js_embed.hpp"
#include "graph_cytoscape_js_embed.hpp"
#include "graph_cytoscape_dagre_js_embed.hpp"
#include "graph_expand_collapse_js_embed.hpp"
#include "graph_viewer_html_embed.hpp"
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

// Assemble the standalone interactive dependency-graph HTML: inline the
// embedded Cytoscape stack + this project's `.makexx_graph.json` into the
// viewer template, producing a single self-contained, offline file. The
// JSON is written by mf.generate_with_graph() in the user's makefile.cpp.
int build_graph_html() {
	if(!exists(".makexx_graph.json")) {
		std::cerr << "error: .makexx_graph.json not found. Enable the graph by calling "
		             "mf.generate_with_graph() in makefile.cpp, then run makexx -c." << endl;
		return 1;
	}
	std::ifstream jf(".makexx_graph.json");
	std::stringstream jbuf;
	jbuf << jf.rdbuf();
	string data = jbuf.str();

	// Load order matters: dagre defines window.dagre, which the UMD
	// cytoscape-dagre adapter reads at evaluation time; the expand/collapse
	// extension is last. Separating semicolons guard against a lib whose
	// final statement isn't terminated.
	string libs;
	libs += graph_dagre_js;            libs += "\n;\n";
	libs += graph_cytoscape_js;        libs += "\n;\n";
	libs += graph_cytoscape_dagre_js;  libs += "\n;\n";
	libs += graph_expand_collapse_js;  libs += "\n;\n";

	// An inline <script> is terminated by the first literal "</script" the HTML
	// parser sees, regardless of JS context. The vendored libs and the JSON
	// only ever contain it inside string literals, where "<\/script" decodes
	// identically — so escape it to keep the inline blocks intact even if a
	// future lib update introduces the sequence.
	libs = replace_all(libs, "</script", "<\\/script");
	data = replace_all(data, "</script", "<\\/script");

	string html = graph_viewer_html;
	auto p = html.find("/*__MAKEXX_GRAPH_LIBS__*/");
	if(p != string::npos)
		html.replace(p, strlen("/*__MAKEXX_GRAPH_LIBS__*/"), libs);

	auto s = html.find("/*makexx-data-start*/");
	auto e = html.find("/*makexx-data-end*/");
	if(s != string::npos && e != string::npos && e >= s) {
		e += strlen("/*makexx-data-end*/");
		html.replace(s, e - s, data);
	}

	write_file("makefile_graph.html", html.c_str(), (int)html.size());
	cout << "Wrote makefile_graph.html (" << (html.size() / 1024) << " KB)" << endl;
	return 0;
}

int run_cmd(string cmd) {
	return system(cmd.c_str());
}

// Run `make`, stream its output through to the user, and on failure parse
// the error lines for the failing target name and point back to the
// corresponding location(s) in makefile.cpp (Tier-1 diagnostics).
//
// Two error shapes we look for:
//   make: *** No rule to make target 'X'         (missing rule / typo)
//   make: *** [makefile:42: X] Error N           (shell command failed)
int run_make_with_diagnostics(string cmd) {
#ifdef _WIN32
	FILE *fp = _popen((cmd + " 2>&1").c_str(), "r");
#else
	FILE *fp = popen((cmd + " 2>&1").c_str(), "r");
#endif
	if(!fp) return system(cmd.c_str());  // fallback if popen fails

	vector<string> failed_targets;
	std::set<string> seen;
	auto try_capture = [&](string const &line, string const &start, string const &stop) {
		auto a = line.find(start);
		if(a == string::npos) return;
		a += start.size();
		auto b = line.find(stop, a);
		if(b == string::npos) return;
		string t = line.substr(a, b - a);
		if(seen.insert(t).second) failed_targets.push_back(t);
	};
	char buf[4096];
	while(fgets(buf, sizeof(buf), fp)) {
		fputs(buf, stderr);
		string line(buf);
		// "make: *** No rule to make target 'X'"
		try_capture(line, "No rule to make target '", "'");
		// "make: *** [makefile:42: X] Error N"  — capture name between ": " and "]"
		auto br = line.find("*** [");
		if(br != string::npos) {
			auto colon = line.find(": ", br);
			auto rbr   = line.find("]", colon == string::npos ? br : colon);
			if(colon != string::npos && rbr != string::npos && colon < rbr) {
				string t = line.substr(colon + 2, rbr - colon - 2);
				if(seen.insert(t).second) failed_targets.push_back(t);
			}
		}
	}
#ifdef _WIN32
	int rc = _pclose(fp);
#else
	int status = pclose(fp);
	int rc = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
#endif
	if(rc == 0 || failed_targets.empty()) return rc;

	// Read makefile.cpp and grep for each failing target string.
	std::ifstream cpp("makefile.cpp");
	if(!cpp.is_open()) return rc;
	vector<string> src;
	string l;
	while(std::getline(cpp, l)) src.push_back(l);

	fprintf(stderr, "\n");
	for(auto const &t : failed_targets) {
		string needle = "\"" + t + "\"";
		bool header_printed = false;
		for(size_t i = 0; i < src.size(); i++) {
			auto col = src[i].find(needle);
			if(col == string::npos) continue;
			if(!header_printed) {
				fprintf(stderr, "\033[1mmakexx:\033[0m target \033[1m'%s'\033[0m referenced in makefile.cpp:\n", t.c_str());
				header_printed = true;
			}
			string ln = std::to_string(i + 1);
			fprintf(stderr, "  \033[2mmakefile.cpp:%s:\033[0m %s\n", ln.c_str(), src[i].c_str());
			// Underline the literal "target" inside the source line.
			string pad(2 + 13 + ln.size() + 2 + col, ' ');
			string under(needle.size(), '~');
			fprintf(stderr, "%s\033[31m%s\033[0m\n", pad.c_str(), under.c_str());
		}
	}
	return rc;
}

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <poll.h>

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
	string description;
};

vector<string> word_wrap(string const &text, int width) {
	if(width < 1) return {text};
	vector<string> lines;
	size_t start = 0;
	while(start < text.size()) {
		if(text.size() - start <= (size_t)width) {
			lines.push_back(text.substr(start));
			break;
		}
		size_t brk = text.rfind(' ', start + width);
		if(brk == string::npos || brk <= start) {
			brk = text.find(' ', start + width);
			if(brk == string::npos) {
				lines.push_back(text.substr(start));
				break;
			}
		}
		lines.push_back(text.substr(start, brk - start));
		start = brk + 1;
	}
	return lines;
}

int run_interactive() {
	// Hoisted state — populated by load_menu(), which is re-invokable so
	// the `r` shortcut can reload after `makexx -c` regenerates the menu
	// file (without exiting the TUI).
	vector<MenuEntry> entries;
	std::map<string, int> group_index;
	vector<MenuGroup> groups;
	std::map<string, int> target_to_entry;
	vector<vector<int>> child_groups;
	vector<int> root_groups;
	int recent_gidx = -1;
	int col_width = 0;
	int prefix_len = 0;
	string fmt_sel, fmt_unsel;

	const int MAX_HISTORY = 5;
	const string history_path = ".makexx_history";
	vector<string> history;
	auto load_history = [&]() {
		history.clear();
		std::ifstream hf(history_path);
		string hl;
		while(std::getline(hf, hl))
			if(!hl.empty()) history.push_back(hl);
	};
	auto save_history = [&](string const &target) {
		history.erase(std::remove(history.begin(), history.end(), target), history.end());
		history.insert(history.begin(), target);
		if((int)history.size() > MAX_HISTORY) history.resize(MAX_HISTORY);
		std::ofstream hf(history_path);
		for(auto &h : history) hf << h << "\n";
	};
	load_history();

	auto rebuild_recent_group = [&]() {
		const string rname = "\x01Recent";
		if(group_index.count(rname)) {
			groups[group_index[rname]].entries.clear();
		} else {
			group_index[rname] = groups.size();
			groups.push_back({rname, "Recent", 0, {}, false});
		}
		auto &rg = groups[group_index[rname]];
		for(auto &h : history) {
			auto it = target_to_entry.find(h);
			if(it != target_to_entry.end())
				rg.entries.push_back(it->second);
		}
	};

	auto load_menu = [&]() -> bool {
		entries.clear();
		groups.clear();
		group_index.clear();
		std::ifstream f(".makexx_menu");
		if(!f.is_open()) return false;

		auto ensure_group = [&](string const &grp, bool folded = false) {
			if(grp.empty()) {
				if(group_index.find(grp) == group_index.end()) {
					group_index[grp] = groups.size();
					groups.push_back({grp, "Targets", 0, {}, folded});
				} else if(folded) {
					groups[group_index[grp]].folded = true;
				}
				return;
			}
			size_t start = 0;
			while(true) {
				auto slash = grp.find('/', start);
				string path = grp.substr(0, slash);
				if(group_index.find(path) == group_index.end()) {
					string dname = path;
					int depth = 0;
					auto last_slash = path.rfind('/');
					if(last_slash != string::npos) {
						dname = path.substr(last_slash + 1);
						for(auto c : path) if(c == '/') depth++;
					}
					group_index[path] = groups.size();
					groups.push_back({path, dname, depth, {}, false});
				}
				if(slash == string::npos) {
					if(folded)
						groups[group_index[path]].folded = true;
					break;
				}
				start = slash + 1;
			}
		};

		std::map<string, string> pending_group_desc;
		string line;
		while(std::getline(f, line)) {
			std::istringstream iss(line);
			string grp, target, desc;
			if(!std::getline(iss, grp, '\t')) continue;
			if(!std::getline(iss, target, '\t')) continue;
			std::getline(iss, desc, '\t');
			if(grp == "!group") {
				ensure_group(target, desc == "+");
				continue;
			}
			if(grp == "!desc") {
				pending_group_desc[target] = desc;
				continue;
			}
			bool folded = false;
			if(!grp.empty() && grp[0] == '+') {
				grp = grp.substr(1);
				folded = true;
			}
			if(grp == "_")
				grp.clear();
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
			ensure_group(grp, folded);
			groups[group_index[grp]].entries.push_back(eidx);
		}
		f.close();
		if(entries.empty()) return false;
		for(auto &kv : pending_group_desc) {
			auto it = group_index.find(kv.first);
			if(it != group_index.end())
				groups[it->second].description = kv.second;
		}

		target_to_entry.clear();
		for(int i = 0; i < (int)entries.size(); i++)
			if(!entries[i].continuation)
				target_to_entry[entries[i].target] = i;

		rebuild_recent_group();

		child_groups.assign(groups.size(), {});
		root_groups.clear();
		for(int g = 0; g < (int)groups.size(); g++) {
			if(groups[g].name == "\x01Recent") continue;
			auto slash = groups[g].name.rfind('/');
			if(slash == string::npos) {
				root_groups.push_back(g);
				continue;
			}
			string parent = groups[g].name.substr(0, slash);
			auto it = group_index.find(parent);
			if(it == group_index.end())
				root_groups.push_back(g);
			else
				child_groups[it->second].push_back(g);
		}

		recent_gidx = group_index["\x01Recent"];

		col_width = 0;
		for(auto &e : entries)
			if((int)e.target.size() > col_width) col_width = e.target.size();
		col_width += 2;
		prefix_len = 6 + col_width + 2 + 3;
		fmt_sel   = "    \033[7m%-" + std::to_string(col_width) + "s\033[0m  %s\n";
		fmt_unsel = "    \033[1m%-" + std::to_string(col_width) + "s\033[0m  %s\n";

		return true;
	};

	if(!load_menu()) {
		std::cerr << "error: .makexx_menu not found or empty. Run makexx first to generate it." << endl;
		return 1;
	}

	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	// ISIG off: Ctrl+C/Z/\ stay as raw bytes instead of generating signals
	// (so Ctrl+\ no longer dumps core; Ctrl+C/Z become silent no-ops here).
	// IEXTEN off: disable line-editing extras (LNEXT, WERASE, etc.) so all
	// control bytes are delivered verbatim.
	newt.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
	newt.c_cc[VMIN] = 1;
	newt.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	enum Key { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_SPACE, KEY_QUIT, KEY_ESC, KEY_BACKSPACE, KEY_CTRL_BACKSPACE, KEY_CHAR, KEY_PGUP, KEY_PGDN, KEY_HOME, KEY_END, KEY_TAB, KEY_STAB };
	char last_char = 0;
	auto read_key = [&]() -> Key {
		char c;
		if(read(STDIN_FILENO, &c, 1) != 1) return KEY_QUIT;
		last_char = c;
		if(c == 27) {
			struct termios tmp = newt;
			tmp.c_cc[VMIN] = 0;
			tmp.c_cc[VTIME] = 1;
			tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
			char seq[2];
			int n = read(STDIN_FILENO, seq, 1);
			if(n == 0) { tcsetattr(STDIN_FILENO, TCSANOW, &newt); return KEY_ESC; }
			if(seq[0] != '[') { tcsetattr(STDIN_FILENO, TCSANOW, &newt); return KEY_ESC; }
			n = read(STDIN_FILENO, seq + 1, 1);
			tcsetattr(STDIN_FILENO, TCSANOW, &newt);
			if(n == 0) return KEY_NONE;
			if(seq[1] == 'A') return KEY_UP;
			if(seq[1] == 'B') return KEY_DOWN;
			if(seq[1] == 'C') return KEY_RIGHT;
			if(seq[1] == 'D') return KEY_LEFT;
			if(seq[1] == 'H') return KEY_HOME;
			if(seq[1] == 'F') return KEY_END;
			if(seq[1] == 'Z') return KEY_STAB;
			if(seq[1] >= '1' && seq[1] <= '6') {
				char tilde;
				struct termios tmp2 = newt;
				tmp2.c_cc[VMIN] = 0; tmp2.c_cc[VTIME] = 1;
				tcsetattr(STDIN_FILENO, TCSANOW, &tmp2);
				read(STDIN_FILENO, &tilde, 1);
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				if(seq[1] == '5') return KEY_PGUP;
				if(seq[1] == '6') return KEY_PGDN;
				if(seq[1] == '1') return KEY_HOME;
				if(seq[1] == '4') return KEY_END;
			}
			return KEY_NONE;
		}
		if(c == 127) return KEY_BACKSPACE;       // plain (and Shift+) Backspace
		if(c == 8) return KEY_CTRL_BACKSPACE;    // Ctrl+Backspace
		if(c == '\t') return KEY_TAB;
		if(c == '\n' || c == '\r') return KEY_ENTER;
		if(c >= 32 && c < 127) return KEY_CHAR;
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

	string search;
	bool searching = false;

	auto matches_search = [&](int eidx) -> bool {
		if(search.empty()) return true;
		auto &e = entries[eidx];
		string q = to_lower(search);
		if(to_lower(e.target).find(q) != string::npos) return true;
		for(auto &dl : e.desc_lines)
			if(to_lower(dl).find(q) != string::npos) return true;
		return false;
	};

	// True if the group itself or any descendant has at least one entry
	// matching the current search filter — regardless of fold state. Used
	// to decide whether a folded parent should still render its header.
	auto has_matchable_content = [&](auto &&self, int g) -> bool {
		for(int ei : groups[g].entries)
			if(matches_search(ei)) return true;
		for(int child : child_groups[g])
			if(self(self, child)) return true;
		return false;
	};

	auto add_group_to_visible = [&](auto &&self, vector<std::pair<int,int>> &vis, int g) -> bool {
		// Skip when an ancestor is folded — unless a search is active, in
		// which case matches under a folded parent should still surface.
		if(search.empty() && is_ancestor_folded(groups[g].name)) return false;
		vector<int> matched;
		for(int ei : groups[g].entries)
			if(matches_search(ei)) matched.push_back(ei);
		vector<std::pair<int,int>> child_vis;
		for(int child : child_groups[g])
			self(self, child_vis, child);
		// Show this group's header if it has any matchable content anywhere
		// underneath, even when our fold/search settings would hide every
		// descendant right now. Otherwise the user has no way to unfold us
		// to reveal the children (the previous version short-circuited via
		// has_visible_children, which double-counted the fold check).
		bool has_any = !matched.empty();
		if(!has_any)
			for(int child : child_groups[g])
				if(has_matchable_content(has_matchable_content, child)) { has_any = true; break; }
		if(!has_any) return false;
		bool show_header = !groups[g].name.empty();
		if(show_header)
			vis.push_back({g, -1});
		if(!groups[g].folded || !search.empty() || !show_header) {
			for(int ei : matched)
				vis.push_back({g, ei});
			vis.insert(vis.end(), child_vis.begin(), child_vis.end());
		}
		return true;
	};

	auto build_visible = [&]() {
		vector<std::pair<int,int>> vis;
		add_group_to_visible(add_group_to_visible, vis, recent_gidx);
		for(int g : root_groups)
			add_group_to_visible(add_group_to_visible, vis, g);
		return vis;
	};

	int cursor = 0;
	int scroll = 0;
	std::set<int> selected;
	bool running = true;
	// Double-tap-to-exit: a single Esc with no active search no longer quits
	// (too easy to fat-finger when meaning to dismiss something). Two Esc
	// presses within this window do. Uses an explicit armed flag because
	// time_point::min() would overflow when subtracted from now().
	auto esc_window = std::chrono::seconds(2);
	bool esc_armed = false;
	std::chrono::steady_clock::time_point last_esc_at;
	// Anything that shakes up the visible list — running a target (Recent
	// group grows), clearing a search filter — leaves a plain row-index
	// cursor pointing at the wrong row. Capture cursor identity *before*
	// the shake-up and re-resolve after the next build_visible().
	// Identity is (preferred group, target name) for an entry, or (group, "")
	// for a group header.  restore_gidx == -1 means no restoration pending.
	int restore_gidx = -1;
	string restore_target;
	// One-shot memory of "the entry the cursor was on when this group was
	// folded from the inside". Consumed by an immediately-following unfold
	// of the same group; cleared by any other key.
	int fold_remember_gidx = -1;
	string fold_remember_target;
	while(running) {
		auto visible = build_visible();
		auto request_cursor_restore = [&]() {
			if(visible.empty()) return;
			auto [g, e] = visible[cursor];
			restore_gidx = g;
			restore_target = (e == -1) ? string() : entries[e].target;
		};
		if(restore_gidx != -1) {
			int best = -1;
			for(int i = 0; i < (int)visible.size(); i++) {
				auto [g, e] = visible[i];
				bool is_header = (e == -1);
				if(restore_target.empty()) {
					if(is_header && g == restore_gidx) { best = i; break; }
				} else if(!is_header && entries[e].target == restore_target) {
					if(g == restore_gidx) { best = i; break; }
					if(best == -1) best = i;
				}
			}
			if(best != -1) cursor = best;
			restore_gidx = -1;
			restore_target.clear();
		}
		if(cursor >= (int)visible.size()) cursor = visible.size() - 1;
		if(cursor < 0) cursor = 0;

		struct winsize ws;
		int term_width = 80, term_height = 24;
		if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
			if(ws.ws_col > 0) term_width = ws.ws_col;
			if(ws.ws_row > 0) term_height = ws.ws_row;
		}
		struct ItemInfo { vector<string> wrapped; int height; };
		vector<ItemInfo> rinfo(visible.size());
		vector<int> line_at(visible.size());
		int total_lines = 0;
		for(int i = 0; i < (int)visible.size(); i++) {
			line_at[i] = total_lines;
			auto [gidx, eidx] = visible[i];
			if(eidx == -1) {
				rinfo[i].height = 1;
			} else {
				auto &e = entries[eidx];
				int desc_avail = term_width - prefix_len - groups[gidx].depth * 3;
				if(desc_avail < 10) desc_avail = 10;
				for(auto &dl : e.desc_lines) {
					if(dl.empty()) continue;
					auto wl = word_wrap(dl, desc_avail);
					rinfo[i].wrapped.insert(rinfo[i].wrapped.end(), wl.begin(), wl.end());
				}
				rinfo[i].height = 1 + ((int)rinfo[i].wrapped.size() > 1 ? (int)rinfo[i].wrapped.size() - 1 : 0);
			}
			total_lines += rinfo[i].height;
		}

		bool needs_scroll = total_lines > term_height - 3;
		int viewport = term_height - 3 - (needs_scroll ? 1 : 0);

		if(!visible.empty()) {
			if(line_at[cursor] < scroll)
				scroll = line_at[cursor];
			if(line_at[cursor] + rinfo[cursor].height > scroll + viewport)
				scroll = line_at[cursor] + rinfo[cursor].height - viewport;
			if(scroll < 0) scroll = 0;
		}

		int entry_count = 0;
		for(auto &v : visible) if(v.second >= 0) entry_count++;
		string sel_info = selected.empty() ? "" : "  \033[33m" + std::to_string(selected.size()) + " selected\033[0m";

		printf("\033[2J\033[H");
		if(searching)
			printf("\033[1mmakexx interactive\033[0m  / %s\033[5m▌\033[0m  \033[2m(%d, Ctrl+Backspace clear, Esc cancel)\033[0m%s\n", search.c_str(), entry_count, sel_info.c_str());
		else if(!search.empty())
			printf("\033[1mmakexx interactive\033[0m  \033[2mfilter:\033[0m %s  \033[2m(%d matches, Esc clear)\033[0m%s\n", search.c_str(), entry_count, sel_info.c_str());
		else if(esc_armed)
			printf("\033[1mmakexx interactive\033[0m  \033[33mPress Esc again to exit\033[0m%s\n", sel_info.c_str());
		else
			printf("\033[1mmakexx interactive\033[0m  \033[2m(↑↓ PgUp/Dn Home/End  Tab group  ←→ fold  / search  Space select  x clear  Enter run  d dry-run  ? deps  r refresh  q quit)\033[0m%s\n", sel_info.c_str());
		if(scroll > 0)
			printf("\033[2m    ▲\033[0m\n");
		else
			printf("\n");

		int lines_left = viewport;
		for(int i = 0; i < (int)visible.size(); i++) {
			if(line_at[i] + rinfo[i].height <= scroll) continue;
			if(line_at[i] >= scroll + viewport || lines_left <= 0) break;
			auto [gidx, eidx] = visible[i];
			bool at_cursor = (i == cursor);
			if(eidx == -1) {
				string arrow = groups[gidx].folded ? "▸" : "▾";
				string gindent(groups[gidx].depth * 3, ' ');
				string suffix = groups[gidx].description.empty()
					? ""
					: "  \033[2m" + groups[gidx].description + "\033[0m";
				if(at_cursor)
					printf("%s   %s \033[7m%s\033[0m%s\n", gindent.c_str(), arrow.c_str(), groups[gidx].display_name.c_str(), suffix.c_str());
				else
					printf("%s   %s \033[1m%s\033[0m%s\n", gindent.c_str(), arrow.c_str(), groups[gidx].display_name.c_str(), suffix.c_str());
				lines_left--;
			} else {
				auto &e = entries[eidx];
				bool next_is_cont = (i + 1 < (int)visible.size() &&
					visible[i + 1].second >= 0 &&
					entries[visible[i + 1].second].continuation);
				bool is_multi_target = e.continuation || next_is_cont;
				auto &wrapped = rinfo[i].wrapped;
				bool has_multi_desc = wrapped.size() > 1;
				string bracket;
				if(e.continuation)
					bracket = next_is_cont ? " │" : " ┘";
				else if(is_multi_target && has_multi_desc)
					bracket = " ┬";
				else if(is_multi_target)
					bracket = " ┬";
				else if(has_multi_desc)
					bracket = " ┌";
				else
					bracket = " ─";
				bool marked = selected.count(eidx);
				string mark = marked ? "\033[33m● \033[0m" : "  ";
				string eindent(groups[gidx].depth * 3, ' ');
				string first_desc = wrapped.empty() ? "" : " " + wrapped[0];
				if(at_cursor)
					printf("%s%s", eindent.c_str(), mark.c_str()), printf(fmt_sel.c_str(), e.target.c_str(), (bracket + first_desc).c_str());
				else
					printf("%s%s", eindent.c_str(), mark.c_str()), printf(fmt_unsel.c_str(), e.target.c_str(), (bracket + first_desc).c_str());
				lines_left--;
				if(has_multi_desc) {
					string cont_pad = eindent + "      " + string(col_width, ' ') + "  ";
					string last_brk = (is_multi_target && !e.continuation) ? " ├ " : " └ ";
					for(size_t dl = 1; dl < wrapped.size() - 1 && lines_left > 0; dl++) {
						printf("%s │ %s\n", cont_pad.c_str(), wrapped[dl].c_str());
						lines_left--;
					}
					if(lines_left > 0) {
						printf("%s%s%s\n", cont_pad.c_str(), last_brk.c_str(), wrapped.back().c_str());
						lines_left--;
					}
				}
			}
		}

		if(needs_scroll && total_lines > scroll + viewport)
			printf("\033[2m    ▼\033[0m\n");

		// When esc_armed, poll with the remaining window so the loop wakes
		// up after the deadline and redraws (clearing the "Press Esc again
		// to exit" hint) even if the user pressed nothing.
		Key key;
		if(esc_armed) {
			auto remaining = esc_window - (std::chrono::steady_clock::now() - last_esc_at);
			int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();
			if(ms <= 0) {
				key = KEY_NONE;
			} else {
				struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
				int r = poll(&pfd, 1, ms);
				key = (r > 0) ? read_key() : KEY_NONE;
			}
		} else {
			key = read_key();
		}
		bool keep_fold_memory = false;
		bool keep_esc_armed = false;
		if(searching) {
			if(key == KEY_ESC) {
				request_cursor_restore();
				searching = false;
				search.clear();
				scroll = 0;
			} else if(key == KEY_ENTER) {
				searching = false;
			} else if(key == KEY_BACKSPACE) {
				if(!search.empty()) search.pop_back();
				// stay in search mode even when the query is empty — the
				// user came in via "/" and only Esc or Enter should leave
				cursor = 0; scroll = 0;
			} else if(key == KEY_CTRL_BACKSPACE) {  // clear query, stay in search
				search.clear();
				cursor = 0; scroll = 0;
			} else if(key == KEY_CHAR) {
				search += last_char;
				cursor = 0; scroll = 0;
			} else if(key == KEY_UP) {
				if(cursor > 0) cursor--;
			} else if(key == KEY_DOWN) {
				if(cursor < (int)visible.size() - 1) cursor++;
			} else if(key == KEY_PGUP) {
				cursor = std::max(0, cursor - viewport);
			} else if(key == KEY_PGDN) {
				cursor = std::min((int)visible.size() - 1, cursor + viewport);
			} else if(key == KEY_HOME) {
				cursor = 0;
			} else if(key == KEY_END) {
				cursor = (int)visible.size() - 1;
			} else if(key == KEY_TAB) {
				for(int j = cursor + 1; j < (int)visible.size(); j++)
					if(visible[j].second == -1) { cursor = j; break; }
			} else if(key == KEY_STAB) {
				for(int j = cursor - 1; j >= 0; j--)
					if(visible[j].second == -1) { cursor = j; break; }
			}
		} else if(key == KEY_CHAR && last_char == 'q') {
			running = false;
		} else if(key == KEY_CHAR && last_char == 'r') {
			// Snapshot identity-by-name state to survive the reload.
			// We track both fold state and which groups existed pre-refresh
			// so we can restore an explicit unfold (overriding a FOLDED
			// default) without trampling the default of brand-new groups.
			std::set<string> saved_folded, saved_known_groups;
			for(auto &g : groups) {
				saved_known_groups.insert(g.name);
				if(g.folded) saved_folded.insert(g.name);
			}
			std::set<string> saved_selected;
			for(int si : selected) saved_selected.insert(entries[si].target);
			string saved_search = search;
			string saved_cursor_target, saved_cursor_group;
			if(!visible.empty()) {
				auto [g, e] = visible[cursor];
				saved_cursor_group = groups[g].name;
				if(e != -1) saved_cursor_target = entries[e].target;
			}

			// Hand the terminal back so compile output / errors are visible.
			tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
			printf("\033[2J\033[H\033[1mRefreshing...\033[0m\n\n");
			fflush(stdout);
			int rc = run_cmd("makexx -c");
			tcsetattr(STDIN_FILENO, TCSANOW, &newt);

			if(rc != 0) {
				printf("\n\033[31;1mRefresh failed.\033[0m \033[2mPress any key to continue...\033[0m");
				fflush(stdout);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
				// Existing in-memory state is unchanged — fall through.
			} else {
				if(!load_menu()) {
					std::cerr << "error: menu file missing or empty after refresh." << endl;
					tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
					return 1;
				}
				// Restore preserved state by name. For pre-existing groups,
				// reapply the exact saved fold state so an explicit unfold
				// survives. Brand-new groups keep their load_menu default.
				for(auto &g : groups)
					if(saved_known_groups.count(g.name))
						g.folded = saved_folded.count(g.name) > 0;
				selected.clear();
				for(int i = 0; i < (int)entries.size(); i++)
					if(saved_selected.count(entries[i].target)) selected.insert(i);
				search = saved_search;
				if(!saved_cursor_target.empty()) {
					restore_target = saved_cursor_target;
					auto it = group_index.find(saved_cursor_group);
					restore_gidx = (it != group_index.end()) ? it->second : 0;
				} else if(!saved_cursor_group.empty()) {
					restore_target.clear();
					auto it = group_index.find(saved_cursor_group);
					restore_gidx = (it != group_index.end()) ? it->second : 0;
				}
			}
		} else if(key == KEY_ESC) {
			if(!search.empty()) { request_cursor_restore(); search.clear(); scroll = 0; }
			else {
				auto now = std::chrono::steady_clock::now();
				if(esc_armed && (now - last_esc_at) < esc_window) {
					running = false;
				} else {
					esc_armed = true;
					last_esc_at = now;
					keep_esc_armed = true;  // survive end-of-iter disarm
				}
			}
		} else if(key == KEY_CHAR && last_char == '/') {
			searching = true;
			cursor = 0; scroll = 0;
		} else if(key == KEY_UP) {
			if(cursor > 0) cursor--;
		} else if(key == KEY_DOWN) {
			if(cursor < (int)visible.size() - 1) cursor++;
		} else if(key == KEY_PGUP) {
			cursor = std::max(0, cursor - viewport);
		} else if(key == KEY_PGDN) {
			cursor = std::min((int)visible.size() - 1, cursor + viewport);
		} else if(key == KEY_HOME) {
			cursor = 0;
		} else if(key == KEY_END) {
			cursor = (int)visible.size() - 1;
		} else if(key == KEY_TAB) {
			for(int j = cursor + 1; j < (int)visible.size(); j++)
				if(visible[j].second == -1) { cursor = j; break; }
		} else if(key == KEY_STAB) {
			for(int j = cursor - 1; j >= 0; j--)
				if(visible[j].second == -1) { cursor = j; break; }
		} else if(key == KEY_RIGHT) {
			auto [gidx, eidx] = visible[cursor];
			if(eidx == -1 && groups[gidx].folded
			   && fold_remember_gidx == gidx && !fold_remember_target.empty()) {
				restore_gidx = gidx;
				restore_target = fold_remember_target;
			}
			groups[gidx].folded = false;
		} else if(key == KEY_LEFT) {
			auto [gidx, eidx] = visible[cursor];
			if(eidx == -1 && groups[gidx].folded) {
				// Already folded — propagate up: fold the parent group and
				// move the cursor to its header in the rebuilt visible list.
				auto slash = groups[gidx].name.rfind('/');
				if(slash != string::npos) {
					string parent = groups[gidx].name.substr(0, slash);
					auto it = group_index.find(parent);
					if(it != group_index.end()) {
						groups[it->second].folded = true;
						restore_gidx = it->second;
						restore_target.clear();  // empty target = restore to header
					}
				}
			} else {
				if(eidx != -1) {
					fold_remember_gidx = gidx;
					fold_remember_target = entries[eidx].target;
					keep_fold_memory = true;
				}
				groups[gidx].folded = true;
				for(int j = cursor; j >= 0; j--)
					if(visible[j].first == gidx && visible[j].second == -1) { cursor = j; break; }
			}
		} else if(key == KEY_CHAR && last_char == ' ') {
			auto [gidx, eidx] = visible[cursor];
			if(eidx == -1) {
				bool folding = !groups[gidx].folded;
				if(!folding && fold_remember_gidx == gidx && !fold_remember_target.empty()) {
					restore_gidx = gidx;
					restore_target = fold_remember_target;
				}
				groups[gidx].folded = folding;
				if(folding)
					for(int j = cursor; j >= 0; j--)
						if(visible[j].first == gidx && visible[j].second == -1) { cursor = j; break; }
			} else {
				if(selected.count(eidx)) selected.erase(eidx);
				else selected.insert(eidx);
				if(cursor < (int)visible.size() - 1) cursor++;
			}
		} else if(key == KEY_CHAR && last_char == 'x') {
			selected.clear();
		} else if(key == KEY_CHAR && last_char == '?') {
			auto [gidx, eidx] = visible[cursor];
			if(eidx >= 0) {
				tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
				printf("\033[2J\033[H");
				string target = entries[eidx].target;
				printf("\033[1mDependencies: %s\033[0m\n\n", target.c_str());
				string cmd = "awk '/^" + target + "[ ]*:/{sub(/^[^:]*:[ ]*/,\"\"); gsub(/ +/,\"\\n\"); print; exit}' makefile";
				run_cmd(cmd);
				printf("\n\033[2mPress any key to return to menu...\033[0m");
				fflush(stdout);
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
			}
		} else if(key == KEY_CHAR && last_char == 'd') {
			auto [gidx, eidx] = visible[cursor];
			if(eidx >= 0) {
				tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
				printf("\033[2J\033[H");
				string target = entries[eidx].target;
				printf("\033[1mDry run: make -n %s\033[0m\n\n", target.c_str());
				run_cmd("make -n " + target);
				printf("\n\033[2mPress any key to return to menu...\033[0m");
				fflush(stdout);
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
			}
		} else if(key == KEY_ENTER) {
			auto [gidx, eidx] = visible[cursor];
			request_cursor_restore();
			if(!selected.empty()) {
				tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
				printf("\033[2J\033[H");
				int total_rc = 0;
				for(int si : selected) {
					string target = entries[si].target;
					printf("\033[1mRunning: make %s\033[0m\n\n", target.c_str());
					int rc = run_cmd("make " + target);
					if(rc != 0) total_rc = rc;
					save_history(target);
					printf("\n");
				}
				rebuild_recent_group();
				selected.clear();
				if(total_rc == 0)
					printf("\033[32;1mAll done.\033[0m \033[2mPress any key to return to menu...\033[0m");
				else
					printf("\033[31;1mSome targets failed.\033[0m \033[2mPress any key to return to menu...\033[0m");
				fflush(stdout);
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
			} else if(eidx == -1) {
				groups[gidx].folded = !groups[gidx].folded;
			} else {
				tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
				printf("\033[2J\033[H");
				string target = entries[eidx].target;
				printf("\033[1mRunning: make %s\033[0m\n\n", target.c_str());
				int rc = run_cmd("make " + target);
				save_history(target);
				rebuild_recent_group();
				if(rc == 0)
					printf("\n\033[32;1mDone.\033[0m \033[2mPress any key to return to menu...\033[0m");
				else
					printf("\n\033[31;1mFailed.\033[0m \033[2mPress any key to return to menu...\033[0m");
				fflush(stdout);
				tcsetattr(STDIN_FILENO, TCSANOW, &newt);
				char dummy;
				read(STDIN_FILENO, &dummy, 1);
			}
		}
		if(!keep_esc_armed) esc_armed = false;
		if(!keep_fold_memory) {
			fold_remember_gidx = -1;
			fold_remember_target.clear();
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
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			cout << "makexx " << MAKEXX_VERSION << " — C++ build system generator\n\n"
				<< "Usage: makexx [flags] [make-args...]\n\n"
				<< "Flags:\n"
				<< "  -u              Force update makefile.hpp\n"
				<< "  -f              Force overwrite non-makexx makefile\n"
				<< "  -c              Compile only (skip running make)\n"
				<< "  -v              Verbose output\n"
				<< "  -i              Interactive target selector (TUI)\n"
				<< "  -Dname[=value]  Define preprocessor macro for makefile.cpp\n"
				<< "  --build-graph   Assemble standalone makefile_graph.html from .makexx_graph.json\n"
				<< "  -h, --help      Show this help\n"
				<< "  --version       Show version\n\n"
				<< "All other flags are forwarded to make.\n";
			return 0;
		}
		if(strcmp(argv[i], "--version") == 0) {
			cout << "makexx " << MAKEXX_VERSION << endl;
			return 0;
		}
		if(strcmp(argv[i], "--build-graph") == 0) {
			// Standalone mode: assemble makefile_graph.html from the existing
			// .makexx_graph.json. No compile, no make. Invoked by the
			// generated `makefile_graph.html` rule.
			return build_graph_html();
		}
	}
	bool update_makefile_hpp = false;
	bool force_overwrite = false;
	bool compile_only = false;
	bool verbose = false;
	bool interactive = false;
	string define_flags;
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
		} else if(strncmp(argv[i], "-D", 2) == 0) {
			define_flags += " ";
			define_flags += argv[i];
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
			std::cerr << "error: could not find a C++ compiler.\n"
				<< "  Install one with: apt install g++  (Debian/Ubuntu)\n"
				<< "                    brew install gcc  (macOS)\n"
				<< "  Or set CXX to specify a compiler path." << endl;
			return -1;
		}
	}
	if(verbose)
		cout << compilers[compiler_id] << " is used." << endl;
	run_cmd(compilers[compiler_id] + " -std=c++17 -MMD -MF .makexx_deps -MT makefile" + define_flags + " makefile.cpp -o makefile_gen");
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
		} else if(strncmp(argv[i], "-D", 2) == 0) {
		} else {
			a += argv[i];
			a += " ";
		}
	}
	if(!compile_only)
		return run_make_with_diagnostics(a);
	return 0;
}
