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

typedef std::initializer_list<std::string> StringList;

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

class Rule {
  public:
	std::vector<std::string> commands;
	std::set<std::string> temp_files;
	std::set<std::string> byproducts;
	std::set<std::string> hidden_targets; // hidden_target
	target_type type;
	std::string help;
	Rule() {
		type = OPTIONAL;
		help = "";
	}
	std::string formatted(std::string prefix = "\t") {
		std::string a = "";
		for(auto k : commands) {
			a += prefix + k + "\n";
		}
		return a;
	}

};

inline std::string get_extension(std::string const filename) {
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

inline std::string join_path(std::string dir, std::string const &file) {
	if(!dir.empty() && dir.back() != '/' && dir.back() != '\\')
		dir += '/';
	return dir + file;
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
	HELP(std::string help) {
		this->help = help;
	};
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
inline Rule &operator<<(Rule &a, TARGET t) {
	for(auto &tmp : t.filenames)
		a.hidden_targets.insert(tmp);
	return a;
}

inline Rule &operator<<(Rule &a, HELP t) {
	a.help = t.help;
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
	std::vector<std::pair<std::string, std::string>> help_menu;
	std::set<std::string> soft_clean_retain_nodes;

	Rule &add_impl(std::vector<std::string> targets, std::vector<std::string> sources) {
		commands.push_back(std::make_unique<Rule>());
		Rule *A = commands.back().get();
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

	void add_to_help_menu(std::string make_rule, std::string helpstr) {
		help_menu.push_back({make_rule, helpstr});
	}

	void add_help_rule() {
		auto &f  = add("help");
		for(auto itm : help_menu) {
			std::stringstream ss;
			ss << std::setw(max_width) << itm.first << ":  " << itm.second;
			f << "@echo \"" + ss.str() + "\"\n";
		}
	}

	void dump_help() {
		for(auto const& cmd : commands) {
			if(!(cmd->help == "")) {
				std::string targets_str = "";
				auto trange = rule_target.equal_range(cmd.get());
				bool first = true;
				for(auto j = trange.first; j != trange.second; j++) {
					if(first) {
						first = false;
					} else {
						targets_str += " ";
					}
					targets_str += j->second;
				}
				if(!first) {
					add_to_help_menu(targets_str, cmd->help);
                    if(max_width<(int)targets_str.size()){
                        max_width=targets_str.size();
                    }
				}
			}
		}
		add_help_rule();
	}

  public:
	std::set<std::string> hidden_nodes; //notes to execlude from makefile graph
	bool silent;
	bool echo;

	Makefile() {
        max_width=20;
		silent = false;
		echo = true;
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

	Rule &add(std::string target) { return add_impl({target}, {}); }
	Rule &add(StringList targets) { return add_impl(targets, {}); }
	Rule &add(std::string target, std::string source) { return add_impl({target}, {source}); }
	Rule &add(std::string target, StringList sources) { return add_impl({target}, sources); }
	Rule &add(std::string target, std::vector<std::string> sources) { return add_impl({target}, sources); }
	Rule &add(StringList targets, std::string source) { return add_impl(targets, {source}); }
	Rule &add(StringList targets, StringList sources) { return add_impl(targets, sources); }
	Rule &add(StringList targets, std::vector<std::string> sources) { return add_impl(targets, sources); }
	Rule &add(std::vector<std::string> targets, std::string source) { return add_impl(targets, {source}); }
	Rule &add(std::vector<std::string> targets, std::vector<std::string> sources) { return add_impl(targets, sources); }

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
		dump_help();
		std::set<std::string> processed_nodes;
		std::string makefile;
		std::set<std::string> defaultmake_list;
		std::set<std::string> inputfiles_list;
		std::ofstream myfile;
		std::set<std::string> temps_list;
		std::set<std::string> byprods_list;
		std::set<std::string> hidden_targets_list;
		myfile.open("makefile");
		myfile << makefile_header << std::endl;
		myfile << "# DO NOT EDIT!" << std::endl;
		myfile << "# You can control the generation via makefile.cpp!" << std::endl;
		myfile << "SHELL=/bin/bash" << std::endl;
		myfile << ".PHONY: all full_clean soft_clean list list_unknown list_input help" << std::endl;
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
					makefile += "\n";
					if(echo){
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
					for(auto tmp : command->temp_files) {
						temps_list.insert(tmp);
					}
					for(auto tmp : command->byproducts) {
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
		myfile << "#input files:" << to_string(inputfiles_list) << std::endl;
		myfile << "all : " << to_string(defaultmake_list) << std::endl;
        {
            myfile << "full_clean: \n"<<std::endl;
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
		std::string u_command = "\t@ls --ignore={\"makefile.cpp\",\"makefile.hpp\",\"makefile_gen\",makefile,makefile_graph.*"
						   ",\"*.*@\",\"*.dbf\",\"*.prj\",\"*.shx\",\"*.cpg\",\"*.aux\",\"*.lt\",\"*.acn\",\"*.bbl\",\"*.blg\",\"*.glo\",\"*ist\",\"*.log\""
						   ",\"*.toc\",\"*.sbl\",\"*.out\",\"makefile_tmp.txt\",\"makefile_nodes.txt\",\"tmp_makexx.cpp\",\"tmp_makexx\",\"err_makexx.txt\"}>makefile_tmp.txt\n"
						   ;
		u_command += "\t@grep -xv -f makefile_nodes.txt makefile_tmp.txt | sed 's/^/\t/'\n\trm -f makefile_tmp.txt";
		std::ofstream exception_file("makefile_nodes.txt");
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
			myfile << "\nmakefile_graph.pdf : makefile_graph.gv\n";
			myfile << "\tdot -v -Tpdf makefile_graph.gv -o makefile_graph.pdf\n";
		}
		myfile.close();
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
			std::string extension = get_extension(*itr);
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
