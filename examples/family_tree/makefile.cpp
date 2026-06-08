#include "makefile.hpp"
using std::string;
using std::vector;
using std::pair;
#define _cont " \\\n"

int main(int argc, char **argv) {
	Makefile mf;
    mf.title = "Family Tree";
    mf.description = "Manages a family genealogy database. Generates SVG tree visualizations "
        "from a SQLite database using Graphviz, syncs data with a remote server, "
        "and generates vCard contact files.";

    string ssh_cmd = "-e 'ssh -p 22'";
    string ssh_usr = "user";
    string server = "example.com";
    string person_page_prefix = "http://example.com/show_person.py?id=";

    mf.add("family_network.svg", "family.db")
        <<MENU("Visualize")
        <<HELP("Plot the whole relationship network without filters")
        <<"familytree2gv input=family.db output=dot.gv option=3 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("family_female.svg", "family.db")
        <<MENU("Visualize")
        <<HELP("Plot the relationship network with female filter")
        <<"familytree2gv input=family.db output=dot.gv option=2 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("family_male.svg", "family.db")
        <<MENU("Visualize")
        <<HELP("Plot the relationship network with male filter")
        <<"familytree2gv input=family.db output=dot.gv option=1 && dot dot.gv -Tsvg -o $@"
        ;

    mf.add("full_tree.pdf", "family.db")
        <<MENU("Subtrees")
        <<HELP("Plot full tree including male and female branching")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=3 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("full_tree.svg", "family.db")
        <<MENU("Subtrees")
        <<HELP("Plot full tree (SVG with hyperlinks)")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 branch_infamily=true input=family.db output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=3 showid=false url_prefix=\""+person_page_prefix+"\" && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("male_tree.svg", "family.db")
        <<MENU("Subtrees")
        <<HELP("Plot male-line tree with bottom-up layout")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db branch_dir=male output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=1 first_name=true direction=BT seq=desc edge_color=black url_prefix=\""+person_page_prefix+"\" showid=false && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("male_1level.svg", "family.db")
        <<MENU("Subtrees")
        <<HELP("Plot male-line tree, one level of branching only")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db branch_dir=male branch_once=true output=tree2.db"
        <<"sqlite3 tree2.db \"PRAGMA foreign_keys = 1; delete from person where is_doubted='Yes';\""
        <<"familytree2gv rootid=1 first_name=true input=tree2.db output=dot.gv option=3 ranksep=0.3 direction=BT seq=desc url_prefix=\""+person_page_prefix+"\" showid=false && dot dot.gv -Tsvg -o $@"
        ;

    mf.add("svgs", {"family.db", "full_tree.svg", "male_tree.svg", "male_1level.svg"})
        <<MENU("Batch")
        <<HELP("Generate all family tree SVGs")
        <<"mkdir -p family_svgs"
        <<"rm -f subset.db && extracttree url_option=id rootid=1 18 input=family.db output=subset.db"
        <<"dbdump db=subset.db qry=\"select * from person\" output=tmp.t"
        <<"tforeach input_tbl=tmp.t template_file=generate_one_level_tree.sh XROOTIDX=id | sh -v"
        <<"tforeach input_tbl=tmp.t template_file=generate_desc_tree.sh XROOTIDX=id | sh -v"
        <<"tforeach input_tbl=tmp.t template_file=generate_ans_tree.sh XROOTIDX=id | sh -v"
        ;

    // Hold a reference to the rule and append one rsync command per
    // (source, destination) pair. Mixing a fixed HELP with a loop body
    // keeps the rule readable as it grows.
    vector<std::pair<string,string>> deploys = {
        {"family_svgs", ""},
        {"*.svg",       "family_svgs/"},
        {"photo",       "family_svgs/"},
        {"family.db",   ""},
        {"contacts",    ""},
    };
    auto& push = mf.add("push");
    push << MENU("Deploy") << HELP("Upload SVGs and database to server");
    for (auto& [src, subdir] : deploys)
        push << ("rsync "+ssh_cmd+" -rPv "+src+" "+ssh_usr+"@"+server+":~/www/html/"+subdir+".");
    mf.add("pull")
        <<MENU("Deploy")
        <<HELP("Sync database and photos from server")
        <<"rsync "+ssh_cmd+" -rPv "+ssh_usr+"@"+server+":~/www/html/family.db ."
        <<"rsync "+ssh_cmd+" -rPv "+ssh_usr+"@"+server+":~/www/html/family_svgs/photo ."
        ;

    mf.add("gen_contacts")
        <<MENU("Utilities")
        <<HELP("Generate vCard contacts from database")
        <<"dbdump db=family.db qry=\"select title, contact from person where not contact is null and not contact=''\" output=tmp1.t"
        <<"tforeach input_tbl=tmp1.t template=\"" _cont
        "python3 make_contact.py title='XTITLEX' contact='XCONTACTX' output='contacts/XTITLEX.vcf' ;" _cont
        "\" XTITLEX=title XCONTACTX=contact | sh -v"
        ;
    mf.add("sqldiff")
        <<MENU("Utilities")
        <<HELP("Compare local database against deployed version")
        <<"wget "+server+"/family.db -O ./tmp_site_version.db"
        <<"sqldiff family.db tmp_site_version.db"
        ;
    mf.add("edit")
        <<MENU("Utilities")
        <<HELP("Open the database editor")
        <<"familyeditor input=family.db"
        ;
    mf.generate();
}
