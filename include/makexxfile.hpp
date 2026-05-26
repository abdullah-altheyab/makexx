#include <map>
#include <set>
#include <utility>
#include <typeinfo>
#include <string>
using std::string;
#include <vector>
using std::vector;
using std::set;
#include <iostream>
#include <fstream>
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::to_string;
#include <initializer_list>
#include <algorithm>
#include <sstream>
#include <iomanip>
using std::initializer_list;
typedef initializer_list<string> slist;

/*
namespace atmake {
string to_string(int i) {
	return std::to_string((long long) i);
}
string to_string(float i) {
	return std::to_string((long double) i);
}
}
using namespace atmake;
*/

long double rand1() {
	return (long double)(std::rand()) / RAND_MAX;
}
string rand(float min = 0, float max = 1, bool randomize = false) {
	//if(randomize)
	//		srand(time(NULL));
	return std::to_string(min + (max - min) * (long double)(std::rand()) / RAND_MAX);
}
string basename(string const a) {
	string base_filename = a.substr(a.find_last_of("/\\") + 1);
	std::string::size_type const p(base_filename.find_last_of('.'));
	return base_filename.substr(0, p);
}
string to_string(vector<string> const &list) {
	string output = "";
	if(list.size() <= 0)
		return output;
	output = list[0];
	for(int i = 1; i < list.size(); i++)
		output += " " + list[i];
	return output;
}
inline string toupper(string input) {
	for(auto &c : input) c = std::toupper(c);
	return input;
}
inline string tolower(string input) {
	for(auto &c : input) c = std::tolower(c);
	return input;
}

string to_string(set<string> const &list) {
	string output = "";
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

class Rule {
  public:
	vector<string> cmds;
	set<string> temps; // temp files
	set<string> byprods; // byproduct files
	set<string> hidden_targets; // hidden_target
	target_type tt;
	string help;
	Rule() {
		tt = OPTIONAL; //run by default
		help = "";
	}
	string cmd(string prefix = "\t") {
		string a = "";
		for(auto k : cmds) {
			a += prefix + k + "\n";
		}
		return a;
	}

};

string get_extension(string const filename) {
	auto idx = filename.rfind('.');
	if(idx != std::string::npos) {
		return filename.substr(idx + 1);
	} else {
		return "";
	}
}

std::string replace_all(std::string str, const std::string &from, const std::string &to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

Rule &operator<<(Rule &a, string const command) {
	#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#ifdef _WIN64
	//define something for Windows (64-bit only)
	string const runsys = "win64";
	#else
	//define something for Windows (32-bit only)
	#endif
	#elif __APPLE__
#include "TargetConditionals.h"
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
	string command2;
	if(runsys == "win64") {
		command2 = replace_all(command, "/", "\\");
	} else {
		command2 = command;
	}
	a.cmds.push_back(command2);
	return a;
}

Rule &operator<<(Rule &a, target_type t) {
	a.tt = t;
	return a;
}

class TEMP { // list of temprorary filenames
  public:
	set<string> filenames;
	TEMP(string filename) {
		this->filenames.insert(filename);
	};
	TEMP(slist filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	TEMP(vector<string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};

class BYPROD { // by products
  public:
	set<string> filenames;
	BYPROD(string filename) {
		this->filenames.insert(filename);
	};
	BYPROD(slist filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	BYPROD(vector<string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};
class TARGET { // hidden target, usually for non-reproducible (manual) targets
  public:
	set<string> filenames;
	TARGET(string filename) {
		this->filenames.insert(filename);
	};
	TARGET(slist filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
	TARGET(vector<string> filenames) {
		for(auto itm : filenames) {
			this->filenames.insert(itm);
		}
	}
};

class HELP { // add help to the menue
  public:
	string help;
	HELP(string help) {
		this->help = help;
	};
};

Rule &operator<<(Rule &a, TEMP t) {
	for(auto &tmp : t.filenames)
		a.temps.insert(tmp);
	return a;
}

Rule &operator<<(Rule &a, BYPROD t) {
	for(auto &tmp : t.filenames)
		a.byprods.insert(tmp);
	return a;
}
Rule &operator<<(Rule &a, TARGET t) {
	for(auto &tmp : t.filenames)
		a.hidden_targets.insert(tmp);
	return a;
}

Rule &operator<<(Rule &a, HELP t) {
	a.help = t.help;
	return a;
}

char const *makefile_header = "# This is an automatically generated makefile via makexx.";

class BuildGraph {
  private:
	std::set<string> nodes;
	std::vector<Rule *> commands;
	std::map<string, Rule *> target_rule;
	std::multimap<Rule *, string> rule_source;
	std::multimap<Rule *, string> rule_target;
	std::vector<std::pair<string, string>> help_menu;
	std::set<string> soft_clean_retain_nodes;
  public:
	std::set<string> hidden_nodes; //notes to execlude from makefile graph

	//help functionalities
	void add_to_help_menue(string make_rule, string helpstr) {
		help_menu.push_back({make_rule, helpstr});
	};

    int max_width;
	void add_help_rule() {
		auto &f  = add("help");
		for(auto itm : help_menu) {
			std::stringstream ss;
			ss << std::setw(max_width) << itm.first << ":  " << itm.second;
			f << "@echo \"" + ss.str() + "\"\n";
		}
	}

	bool silent;
	bool echo;

	BuildGraph() {
        max_width=20;
		silent = false;
		echo = true;
	};

	void add_source(Rule &f, string node) {
		rule_source.insert(std::pair<Rule *, string>(&f, node));
	}
	void add_target(Rule &f, string node) {
		rule_target.insert(std::pair<Rule *, string>(&f, node));
	}

	Rule &get_rule(string target) {
        if(target_rule.find(target) != target_rule.end()) {
			return (*target_rule[target]);
        }else{
            return add(target);
        }
    }

	Rule &add(string target) {
		//cerr<<"$$$"<<__LINE__<<endl;
		return add({target});
	}
	Rule &add(slist targets) {
		//cerr<<"$$$"<<__LINE__<<endl;
		return add(targets, {});
	}
	Rule &add(string target, string source) {
		//cerr<<"$$$"<<__LINE__<<endl;
		return add({target}, {source});
	}
	Rule &add(string targets, slist sources) {
		//cerr<<"$$$"<<__LINE__<<endl;
		return add({targets}, sources);
	}
	Rule &add(slist targets, string source) {
		//cerr<<"$$$"<<__LINE__<<endl;
		return add(targets, {source});
	}
	Rule &add(slist targets, slist sources) {
		//cerr<<"$$$"<<__LINE__<<endl;
		Rule *A = new Rule;
		commands.push_back(A);
		for(auto i = sources.begin(); i != sources.end(); i++) {
			nodes.insert(*i);
			rule_source.insert(std::pair<Rule *, string>(A, *i));
			//cerr<<"$$$source:"<<*i<<endl;
		}
		for(auto i = targets.begin(); i != targets.end(); i++) {
			if(target_rule.find(*i) != target_rule.end()) {
				cerr << "# WARNING! Multiple build rules for the same target '" << *i << "' only the latest rule defined will be used!" << endl;
			}
			nodes.insert(*i);
			target_rule[*i] = A;
			//cerr<<"$$$targert:"<<*i<<endl;
			rule_target.insert(std::pair<Rule *, string>(A, *i));
		}
		return *A;
	}

	Rule &add(vector<string> const targets, vector<string> const sources) {
		//cerr<<"$$$"<<__LINE__<<endl;
		Rule *A = new Rule;
		commands.push_back(A);
		for(auto i = sources.begin(); i != sources.end(); i++) {
			nodes.insert(*i);
			rule_source.insert(std::pair<Rule *, string>(A, *i));
			//cerr<<"$$$source:"<<*i<<endl;
		}
		for(auto i = targets.begin(); i != targets.end(); i++) {
			if(target_rule.find(*i) != target_rule.end()) {
				cerr << "# WARNING! Multiple build rules for the same target '" << *i << "' only the latest rule defined will be used!" << endl;
			}
			nodes.insert(*i);
			target_rule[*i] = A;
			//cerr<<"$$$targert:"<<*i<<endl;
			rule_target.insert(std::pair<Rule *, string>(A, *i));
		}
		return *A;
	}

	Rule &add(slist targets, vector<string> const sources) {
		//cerr<<"$$$"<<__LINE__<<endl;
		Rule *A = new Rule;
		commands.push_back(A);
		for(auto i = sources.begin(); i != sources.end(); i++) {
			nodes.insert(*i);
			rule_source.insert(std::pair<Rule *, string>(A, *i));
			//cerr<<"$$$source:"<<*i<<endl;
		}
		for(auto i = targets.begin(); i != targets.end(); i++) {
			if(target_rule.find(*i) != target_rule.end()) {
				cerr << "# WARNING! Multiple build rules for the same target '" << *i << "' only the latest rule defined will be used!" << endl;
			}
			nodes.insert(*i);
			target_rule[*i] = A;
			//cerr<<"$$$targert:"<<*i<<endl;
			rule_target.insert(std::pair<Rule *, string>(A, *i));
		}
		return *A;
	}

	Rule &add(vector<string> const targets, string const source) {
		vector<string> a = {source};
		return add(targets, a);
	}

	Rule &add(string target, vector<string> const sources) {
		vector<string> targetvec;
		targetvec.push_back(target);
		return add(targetvec, sources);
	}

    /*
	void rename(string node_name, string new_name) {
		if(target_rule.find(node_name) != target_rule.end()) {
		} else {
		}
	}
    */
	void on_softclean_retain(string filename) {
        this->soft_clean_retain_nodes.insert(filename);
    }
	void on_softclean_retain(slist filenames) {
		for(auto itm : filenames) {
			this->soft_clean_retain_nodes.insert(itm);
		}
    }
	void on_softclean_retain(vector<string> filenames) {
		for(auto itm : filenames) {
			this->soft_clean_retain_nodes.insert(itm);
		}
    }

	void dump_help() {
		for(auto cmd : commands) {
			if(!(cmd->help == "")) {
				string targets_str = "";
				auto trange = rule_target.equal_range(cmd);
				bool first = true;
				for(auto j = trange.first; j != trange.second; j++) {
					if(first) {
						first = false;
					} else {
						targets_str += " ";
					}
					targets_str += j->second;
				}
				if(!first) { // there is at least one source
					add_to_help_menue(targets_str, cmd->help);
                    if(max_width<targets_str.size()){
                        max_width=targets_str.size();
                    }
				}
			}
		}
		add_help_rule();
	}
	void dump_makefile() {
		dump_help();
		std::set<string> processed_nodes;
		string makefile;
		set<string> defaultmake_list;
		set<string> inputfiles_list;
		ofstream myfile;
		set<string> temps_list;
		set<string> byprods_list;
		set<string> hidden_targets_list;
		myfile.open("makefile");
		myfile << makefile_header << endl;
		myfile << "# DO NOT EDIT!" << endl;
		myfile << "# You can control the generation via makefile.cpp!" << endl;
		myfile << "SHELL=/bin/bash" << endl;
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++) {
			if(target_rule.find(*itr) != target_rule.end()) {
				if(processed_nodes.find(*itr) == processed_nodes.end()) {
					auto command = target_rule[*itr];
					auto isfinal = (command->tt == FINAL);
					auto trange = rule_target.equal_range(command);
					string to;
					set<string> to_list;
					set<string> from_list;
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
					string from;
					auto srange = rule_source.equal_range(command);
					for(auto j = srange.first; j != srange.second; j++) {
                        from_list.insert(j->second);
						from += j->second + " ";
					}
					makefile += from;
					makefile += "\n";
					if(echo){
						//makefile += string("\t@echo \"      # # # # # Generating [ ") + to + "] from [ " + from + "] # # # # #\"\n";
						makefile += "\t@echo \"### GENERATING : \"\n";
                        for(auto & i: to_list)
                            makefile += string("\t@echo \"###   "+i+"\"\n");
                        if(from_list.size()>0)
                            makefile += "\t@echo \"### FROM : \"\n";
                        else
                            makefile += "\t@echo \"### FROM SCRATCH. \"\n";

                        for(auto & i: from_list)
                            makefile += string("\t@echo \"###   "+i+"\"\n");
                    }
					if(silent)
						makefile += command->cmd("\t@") + "\n";
					else
						makefile += command->cmd("\t") + "\n";
					for(auto tmp : command->temps) {
						temps_list.insert(tmp);
					}
					for(auto tmp : command->byprods) {
						byprods_list.insert(tmp);
					}
					for(auto tmp : command->hidden_targets) {
						hidden_targets_list.insert(tmp);
					}
				}
			} else {
				inputfiles_list.insert(*itr);
			}
		}
		//myfile << "#input files:" << inputfiles << endl;
		myfile << "#input files:" << to_string(inputfiles_list) << endl;
		myfile << "all : " << to_string(defaultmake_list) << endl;
        {
            myfile << "full_clean: \n"<<endl;
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
            myfile << "soft_clean: \n"<<endl;
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
		string u_command = "\t@ls --ignore={\"makefile.cpp\",\"makefile.hpp\",\"makefile_gen\",makefile,makefile_graph.*"
						   ",\"*.*@\",\"*.dbf\",\"*.prj\",\"*.shx\",\"*.cpg\",\"*.aux\",\"*.lt\",\"*.acn\",\"*.bbl\",\"*.blg\",\"*.glo\",\"*ist\",\"*.log\""
						   ",\"*.toc\",\"*.sbl\",\"*.out\",\"makefile_tmp.txt\",\"makefile_nodes.txt\",\"tmp_makexx.cpp\",\"tmp_makexx\",\"err_makexx.txt\"}>makefile_tmp.txt\n"
						   ;
		u_command += "\t@grep -xv -f makefile_nodes.txt makefile_tmp.txt | sed 's/^/\t/'\n\trm -f makefile_tmp.txt";
		ofstream exception_file("makefile_nodes.txt");
		//#define _CMD myfile << "\t@grep -v \"^"+itm+"$$\" makefile_tmp.txt >makefile_tmp2.txt && mv makefile_tmp2.txt makefile_tmp.txt\n";
#define _CMD exception_file<<""+itm+"\n";
		{
			for(auto &itm : inputfiles_list){
				_CMD;
            }
            //myfile << "| grep -v \"^"+itm+"$$\"\\\n\t";
            for(auto &itm : processed_nodes){
                _CMD;
            }
            for(auto &itm : temps_list){
                _CMD;
            }
            for(auto &itm : byprods_list){
                _CMD;
            }
            for(auto &itm : hidden_targets_list){
                _CMD;
            }
            for(auto &itm : hidden_nodes){
                _CMD;
            }
        }
		exception_file.close();
		myfile << u_command;
		myfile << "\nlist_unknown:\n";
		myfile << "\t@echo \"Unknown Files in Directory:\"\n";
		{
			myfile << u_command;
			/*
			for(auto &itm:inputfiles_list)
			    _CMD
			//    myfile << ",\""+itm+"\"";
			for(auto &itm:processed_nodes)
			    _CMD
			for(auto &itm:temps_list)
			    _CMD
			for(auto &itm:byprods_list)
			    _CMD
			myfile << "\t@sed 's/^/\t/' <makefile_tmp.txt\n";
			myfile << "\t@rm -f makefile_tmp.txt makefile_tmp2.txt"<<endl;
			//myfile << "| sed 's/^/\t/'"<<endl;
			*/
		}
		myfile << "\nlist_input:\n";
		for(auto &itm : inputfiles_list)
			myfile << "\t@echo \"\t" + itm + "\"\n";
		myfile << makefile;
		myfile.close();
	}

	void generate_graph(string filename = "makefile_graph.gv") {
		ofstream myfile;
		myfile.open("makefile_graph.gv");
		myfile << "digraph G{\n";
		myfile << "rankdir=\"LR\";\n";
		//myfile<<"rank=min;\n";
		myfile << "graph[ranksep=\"40\"];\n";
		//myfile<<"splines=ortho;\n";
		int cmd_counter = 0;
		std::map<string, string> filetype_color;
		std::set<string> zero_rank_nodes;
		for(auto itr = nodes.begin(); itr != nodes.end(); itr++) {
			if(hidden_nodes.find(*itr) != hidden_nodes.end())
				continue;
			string const scheme = "set312"; //"pastel28"
			string extension = get_extension(*itr);
			if(extension == "") {
				//rule that is not a file
				myfile << "\"" + (*itr) + "\" [shape=box, style=filled, fillcolor=black, fontcolor=white];\n";
			} else {
				if(filetype_color.find(extension) == filetype_color.end()) {
					filetype_color[extension] = to_string((filetype_color.size() % 8 + 1));
				}
				string penwidth = "";
				string postfix = "";
				if(target_rule.find(*itr) != target_rule.end()) {
				} else {
					postfix = "*";
					penwidth = ", penwidth=10.0";
					zero_rank_nodes.insert(*itr);
				}
				myfile << "\"" + (*itr) + "\" [label=\"" + (*itr) + postfix + "\", shape=box, style=filled, colorscheme=" + scheme + ", fillcolor=" + filetype_color[extension] + ", fontcolor=black " + penwidth + "];\n";
			}
		}
		for(auto cmd : commands) {
			cmd_counter++;
			string const scheme = "set312"; //"pastel28"
			string randcolor = to_string(1 + cmd_counter % 12); //"\""+rand(0, 1)+" "+rand(0.5, 1)+" "+rand(0.5, 1)+"\"";
			myfile << " \"node" + to_string(cmd_counter) + "\" [label=\"\", shape=circle, style=filled, colorscheme=" + scheme + ", fillcolor=" + randcolor + "];\n";
			auto srange = rule_source.equal_range(cmd);
			int nsource = 0;
			for(auto j = srange.first; j != srange.second; j++) {
				if(hidden_nodes.find(j->second) != hidden_nodes.end()) {
					continue;
				}
				myfile << "  \"" << j->second << "\" -> \"node" << to_string(cmd_counter) << "\" [colorscheme=" + scheme + ", color=" + randcolor + ", penwidth=3];\n";
				nsource++;
			}
			if(nsource == 0) {
				//in this case the command has no source
				zero_rank_nodes.insert("\"node" + to_string(cmd_counter) + "\"");
			}
			auto trange = rule_target.equal_range(cmd);
			for(auto j = trange.first; j != trange.second; j++) {
				if(hidden_nodes.find(j->second) != hidden_nodes.end()) {
					continue;
				}
				myfile << "  \"node" << to_string(cmd_counter) << "\" -> \"" << j->second << "\" [colorscheme=" + scheme + ", color=" + randcolor + ", penwidth=3];\n";
			}
		}
		myfile << "{rank = same;";
		for(auto n : zero_rank_nodes) {
			myfile << "\"" + n + "\";";
		}
		myfile << "}\n";
		myfile << "}";
		add("makefile_graph.pdf", "makefile.cpp")
				<< "dot -v -Tpdf makefile_graph.gv -o makefile_graph.pdf";
		myfile.close();
	}
};

