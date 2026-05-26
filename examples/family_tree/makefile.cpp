#include "makefile.hpp"
using std::string;
#define _cont " \\\n"

int main(int argc, char **argv) {
	Makefile mf;
    mf.help_title = "Family Tree";
    mf.description("Manages a family genealogy database. Generates SVG tree visualizations "
        "from a SQLite database using Graphviz, syncs data with a remote server, "
        "and generates vCard contact files.");

    string ssh_cmd = "-e 'ssh -p 22'";
    string ssh_usr = "user";
    string server = "example.com";
    string person_page_prefix = "http://example.com/show_person.py?id=";

    mf.MENU("Visualize");
    mf.add("family_network.svg", "family.db")
        <<HELP("Plot the whole relationship network without filters")
        <<"familytree2gv input=family.db output=dot.gv option=3 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("family_female.svg", "family.db")
        <<HELP("Plot the relationship network with female filter")
        <<"familytree2gv input=family.db output=dot.gv option=2 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("family_male.svg", "family.db")
        <<HELP("Plot the relationship network with male filter")
        <<"familytree2gv input=family.db output=dot.gv option=1 && dot dot.gv -Tsvg -o $@"
        ;

    mf.MENU("Subtrees");
    mf.add("full_tree.pdf", "family.db")
        <<HELP("Plot full tree including male and female branching")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=3 && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("full_tree.svg", "family.db")
        <<HELP("Plot full tree (SVG with hyperlinks)")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 branch_infamily=true input=family.db output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=3 showid=false url_prefix=\""+person_page_prefix+"\" && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("male_tree.svg", "family.db")
        <<HELP("Plot male-line tree with bottom-up layout")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db branch_dir=male output=tree2.db"
        <<"familytree2gv input=tree2.db output=dot.gv option=1 first_name=true direction=BT seq=desc edge_color=black url_prefix=\""+person_page_prefix+"\" showid=false && dot dot.gv -Tsvg -o $@"
        ;
    mf.add("male_1level.svg", "family.db")
        <<HELP("Plot male-line tree, one level of branching only")
        <<"rm -f tree2.db && extracttree url_option=id rootid=1 input=family.db branch_dir=male branch_once=true output=tree2.db"
        <<"sqlite3 tree2.db \"PRAGMA foreign_keys = 1; delete from person where is_doubted='Yes';\""
        <<"familytree2gv rootid=1 first_name=true input=tree2.db output=dot.gv option=3 ranksep=0.3 direction=BT seq=desc url_prefix=\""+person_page_prefix+"\" showid=false && dot dot.gv -Tsvg -o $@"
        ;

    mf.MENU("Batch");
    mf.add("svgs", {"family.db", "full_tree.svg", "male_tree.svg", "male_1level.svg"})
        <<HELP("Generate all family tree SVGs")
        <<"mkdir -p family_svgs"
        <<"rm -f subset.db && extracttree url_option=id rootid=1 18 input=family.db output=subset.db"
        <<"dbdump db=subset.db qry=\"select * from person\" output=tmp.t"
        <<"tforeach input_tbl=tmp.t template_file=generate_one_level_tree.sh XROOTIDX=id | sh -v"
        <<"tforeach input_tbl=tmp.t template_file=generate_desc_tree.sh XROOTIDX=id | sh -v"
        <<"tforeach input_tbl=tmp.t template_file=generate_ans_tree.sh XROOTIDX=id | sh -v"
        ;

    mf.MENU("Deploy");
    mf.add("push")
        <<HELP("Upload SVGs and database to server")
        <<"rsync "+ssh_cmd+" -rPv family_svgs "+ssh_usr+"@"+server+":~/www/html/."
        <<"rsync "+ssh_cmd+" -rPv *.svg "+ssh_usr+"@"+server+":~/www/html/family_svgs/."
        <<"rsync "+ssh_cmd+" -rPv photo "+ssh_usr+"@"+server+":~/www/html/family_svgs/."
        <<"rsync "+ssh_cmd+" -rPv family.db "+ssh_usr+"@"+server+":~/www/html/."
        <<"rsync "+ssh_cmd+" -rPv contacts "+ssh_usr+"@"+server+":~/www/html/."
        ;
    mf.add("pull")
        <<HELP("Sync database and photos from server")
        <<"rsync "+ssh_cmd+" -rPv "+ssh_usr+"@"+server+":~/www/html/family.db ."
        <<"rsync "+ssh_cmd+" -rPv "+ssh_usr+"@"+server+":~/www/html/family_svgs/photo ."
        ;

    mf.MENU("Utilities");
    mf.add("gen_contacts")
        <<HELP("Generate vCard contacts from database")
        <<"dbdump db=family.db qry=\"select title, contact from person where not contact is null and not contact=''\" output=tmp1.t"
        <<"tforeach input_tbl=tmp1.t template=\"" _cont
        "python3 make_contact.py title='XTITLEX' contact='XCONTACTX' output='contacts/XTITLEX.vcf' ;" _cont
        "\" XTITLEX=title XCONTACTX=contact | sh -v"
        ;
    mf.add("sqldiff")
        <<HELP("Compare local database against deployed version")
        <<"wget "+server+"/family.db -O ./tmp_site_version.db"
        <<"sqldiff family.db tmp_site_version.db"
        ;
    mf.add("edit")
        <<HELP("Open the database editor")
        <<"familyeditor input=family.db"
        ;
    mf.generate_with_graph();
}
