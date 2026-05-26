// Makefile containing workflows for Exploration portfolio Analytics and Planning (SPOT)
// AUTHORS: Abdullah AlTheyab
//
// WORKFLOW OPTIONS:
// Turn off or on by commenting or uncommenting the following definitions
//#define TRIAL     //for trial simulation runs (with 2 iterations) before running the simulation with alots of iterations
//#define WITH_RRAD //use RRAD input usually will take much longer to run the simulations
#define SHARED    //work on shared portfolio file
//#define TESTING   //include tests in the portfolio. Export the portfolio agian using make export_from_guwi after finishing the tests
//#define MATCHSPOT // eliminate dependencies, set nwell=1 (volume will be realized by the discovery well) , and set P_comm to 100%
//#define PETROSYS   // generate maps using petrosys workflows


#include "makefile.hpp"
#include <sstream>
#include <iomanip>

////////////////////////////////////
//Definitions:
////////////////////////////////////

// line break shorthand
#define _cont " \\\n"


//typedef for a vector of string
typedef std::vector<string> strings;

//Rig AOI class
class RigAOI{
    public:
    string title;
    string prefix;
};
// Drill Zone class
class DrillZone{
    public:
    string title;
    string prefix;
    string db_name;
    string bop_shp;
};

//Portfolio  class 
class Portfolio{
    public:
    string team;
    string file;
};

//Basin class that define the stratigraphic column and how to clip it for the report
struct Basin{
    public:
        string strat_col;
        string strat_col_top_trim;
        string strat_col_bot_trim;
        string strat_col_left_trim;
        string strat_col_right_trim;
};

//Ventures -- used for long-term planning
class Venture{
    public:
    string title;
    string prefix;
    string filter;
    float max_nrig;
};

// A class to contain play parameters
class Play{
    public:
    string prefix; // play prefix for all the files related to it
    string title; // play title as will be shown in the reports
    string drill_zone;
    string oil_rf; // oil recovery factor
    string gas_rf; // gas recovery factor
    // number used for clipping the stratigraphic column pdf file for plotting
    Basin basin;
    int strat_col_top_clip;
    int strat_col_bot_clip;
    int strat_col_top_clip2;
    int strat_col_bot_clip2;
    string strat_col_top_label;
    string strat_col_bot_label;
    //how ppd entities reference this play
    strings aed_references;
    strings pvad_references;
    strings spot_references;
    strings rrad_references;
};
////////////////////////////////////
//Functions:
////////////////////////////////////
// sql command for finding a hashtag withing a column
string hashtag_find(string tag){
    //return "regexp_like(hashtags, '#"+tag+"( |#|,|$)')";
    return 
        "(   (hashtags like '%#"+tag+" %')"
        " or (hashtags like '%#"+tag+"#%')"
        " or (hashtags like '%#"+tag+",%')"
        " or   (hashtags like '%#"+tag+"')"
        ")";
}
//extract stats from Monte-Carlo simulation results into a text file
string package_stats(string dist, string prefix, string output, string format="\%.2f"){
    string cmds;
    cmds+="atdistribution_summary input="+dist+" format=\""+format+"\" prefix=\""+prefix+"_\" output=tmp.txt; \n\t";
    cmds+="cat tmp.txt>>"+output+"\n\t";
    return cmds;
}

//short hand to convert filename.it to filename.lt and filename.t
strings itable(string base_name){
    string bname=stem(base_name);
    return {bname+".lt", bname+".t"};
}

//short hand to convert filename.it to filename.lt and filename.t and include filename.it in the list as well
strings iitable(string base_name){
    string bname=stem(base_name);
    return {bname+".it", bname+".lt", bname+".t"};
}

//

//Main Class executing portfolio analytics and planning workflows
class Main{
  public:
    Makefile mf;
#ifdef TRIAL
    bool trial_run=true; // is this a trial run
    int const nitr=2; // number of Monte-Carlo Simulation iterations
    #undef TESTING
#else
    bool trial_run=false; // is this a trial run
    int const nitr=1000;// number of Monte-Carlo Simulation iterations
#endif
#ifdef WITH_RRAD
    bool include_rrad=true; //option to include rrad portfolio
#else
    bool include_rrad=false; //option to include rrad portfolio
#endif

#ifdef SHARED
    //shared or local
    string portfolio_path = "~/spot_dev/portfolio/ppd.portfolio";
#else
    string portfolio_path = "ppd.portfolio";
#endif

    //general parameters
    int year1=2023;
    int nyear=4;
    string const oil_rf="0.3";
    string const gas_rf="0.6";
    string rigmove_min="20";
    string rigmove_max="30";
    string cost_ff="1.12";//cost fudge factor to take commulative completion cost to drilling budget to account for the cost of water wells, rigless tests, and inflation
    //Define Basins
    Basin const RedSeaBasin={"stratcol_rsed","{0 1959 0 0}", "{0 0 0 1933}", "122", "1"};
    Basin const EasternBasin={"stratcol_east","{0 3807 0 0}", "{0 0 0 3770}", "181.5", "87"};
    //
    std::map<string, Play> plays; // playmaps
    strings seq_plays; //sequenced plays
    string play_list;
    std::vector<Portfolio> portfolios;
    std::vector<RigAOI> aois;
    std::vector<DrillZone> drillzones;
    std::vector<Venture> ventures;
    strings prms_types={"2p3p", "ctgt", "pros"}; //PRMS Typs : Delineations (2P & 3P), Contingent, Prospective
    strings precompute_tables; //list of program (i.e. portfolio divisions)
    strings precompute_list; //list of pre mcrun file
    strings landscape_portfolios;
    strings gis_targets;

#ifdef TESTING
    //testing parametere
    bool generate_testing_report = true;
#else
    bool generate_testing_report = false;
#endif
    int tstf_ntest=10;
    int current_year=2025;

#ifdef PETROSYS
    bool generate_maps = true;
#else
    bool generate_maps = false;
#endif

    //
    void define_plays(){
        //when play list is changed make sure to delete all_portfolio.t
        bool show_all=true; //if false, hide play with small portfolio
        if(show_all){
            Play play; 
            play.prefix="dam";
            play.title= "Dam Play";
            play.pvad_references={};
            play.spot_references={"DAMR"};
            play.rrad_references={"TWYL-DAM"};
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_clip2=255;
            play.strat_col_bot_clip2=3667;
            play.strat_col_top_label="0.78";
            play.strat_col_bot_label="0.85";
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SHALLOW";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="hdrk";
            play.title= "Hadrukh Play";
            play.pvad_references={"UHDK","MHDK", "HSBH"};
            play.spot_references={"UHDR", "MHDR","LHDR", "HSBR"};
            play.rrad_references={"TWYL-HDRK"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.67";
            play.strat_col_bot_label="0.79";
            play.strat_col_top_clip2=282;
            play.strat_col_bot_clip2=3614;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SHALLOW";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="dmm";
            play.title= "Dammam Play";
            play.pvad_references={"DMMM"};
            play.spot_references={"DMMR"};
            play.rrad_references={"TWYL-DMMM"};
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.30";
            play.strat_col_bot_label="0.54";
            play.strat_col_top_clip2=400;
            play.strat_col_bot_clip2=3436;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SHALLOW";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="rus";
            play.title= "Rus Play";
            play.pvad_references={"RUS"};
            play.spot_references={"RUS", "RUSR", "RUSA", "RUSG"};
            play.rrad_references={"TWYL-RUS"};
            play.aed_references={"RUS"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.23";
            play.strat_col_bot_label="0.30";
            play.strat_col_top_clip2=513;
            play.strat_col_bot_clip2=3409;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SHALLOW";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="umer";
            play.title= "Umm Er Radhuma Play";
            play.pvad_references={"BSSR","UMER"};
            play.spot_references={"UERR"};
            play.rrad_references={
                "TWYL-UMER"
                "TWYL-BSSR"
            };
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.23";
            play.strat_col_bot_label="0.30";
            play.strat_col_top_clip2=540.9;
            play.strat_col_bot_clip2=3334.8;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SHALLOW";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="arum";
            play.title= "Aruma Plays";
            play.pvad_references={"LWHH","TYLR"};
            play.spot_references={"RWDR","TYLR","ARMR", "LHHR"};
            play.rrad_references={"TWYL-LWHH", "TWYL-RWYD", "TWYL-TWYL"};
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=3300;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.23";
            play.strat_col_bot_label="0.30";
            play.strat_col_top_clip2=614.5;
            play.strat_col_bot_clip2=3141.5;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="wsia";
            play.title= "Wasia Plays";
            play.pvad_references={
                "AHMD",
                "AHMS",
                "KHFJ",
                "KHFL",
                "KJSS", 
                "KSTR",
                "LMDD",
                "MDDD",
                "MKFJ",
                "MSRC",
                "MSRD",
                "MSRF", 
                "MSRL",
                "MSRU",
                "RUML", 
                "SFNY",
                "SFRA",
                "SFSS", 
                "UKST",
                "UMDD",
                "WARA",
                "WRMD", 
            };
            play.spot_references={
                "SFNR", "MRFR","KFJR",
                "AMDR",
                "AHMD",
                "KJSS", 
                "MSFA", 
                "MSFB", 
                "MSFD", 
                "UMDD","LMDR","WARR","KSTR","MDDR","MSFC","RMLR"};
            play.rrad_references={
                "SFNY\\/MSRF-AHMD",
                "SFNY\\/MSRF-MDDD",
                "SFNY\\/MSRF-SFNY",
                "SFNY\\/MSRF-WARA",
                "SFNY\\/MSRF-MSRF",
                "SFNY\\/MSRF-RUML",
                "KHFJ-KHFJ",
            };
            play.aed_references={"SFNY", "MSRF"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2800;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.45";
            play.strat_col_bot_label="0.75";
            play.strat_col_top_clip2=805;
            play.strat_col_bot_clip2=3015;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="shub";
            play.title= "Shuaiba Play";
            play.pvad_references={"SHUB","USHB"};
            play.spot_references={"SHUB","USBR","SHBR"};
            play.rrad_references={"SHUB-USBR", "SHUB-SHBR"};
            play.aed_references={"SHUB"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2800;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.45";
            play.strat_col_bot_label="0.35";
            play.strat_col_top_clip2=939;
            play.strat_col_bot_clip2=2930;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(show_all){
            Play play; 
            play.prefix="bydh";
            play.title= "Biyadh Play";
            play.pvad_references={"BYDR", "BYDH", "ZUBR"};
            play.spot_references={"BYDR"};
            play.rrad_references={"SULY-BYDH"};
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2800;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.45";
            play.strat_col_bot_label="0.35";
            play.strat_col_top_clip2=1018;
            play.strat_col_bot_clip2=2889;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="buwb";
            play.title= "Buwaib Res. and Chrysalidina Zone";
            play.pvad_references={"BUWB", "BUW6"};
            play.spot_references={"BWBR", "CRLD", "MDTM"};
            play.rrad_references={"BUWB\\/SULY-BUWB", "BUWB\\/SULY-CRLD"};
            play.aed_references={"BUWB", "BWBR", "CRLD"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2600;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.55";
            play.strat_col_bot_label="0.65";
            play.strat_col_top_clip2=1058.8;
            play.strat_col_bot_clip2=2858;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="ymsl";
            play.title= "Yamama and Sulaiy Reservoirs";
            play.aed_references={"URTR","RTWI","RBYN"};
            play.pvad_references={"LRTW","URTW","RTWI", "URUS", "URLS", "RHOZ", "RBYN","YAMM","YST1","YST50"};
            play.spot_references={"URTR","PURR","LRTR","RBNR", "RTWI"};
            play.rrad_references={
                "HNIF\\/TQMN\\/SULY-LRTR",
                "HNIF\\/TQMN\\/SULY-URTR"};
            play.aed_references={"URTR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2550;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.51";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1097;
            play.strat_col_bot_clip2=2775;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-CRETACEOUS";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(1){
            Play play; 
            play.prefix="hith";
            play.title= "Hith Plays";
            play.pvad_references= {"HITH", "LHTS","HTHS", "MNIF","RMTN","UHTS", "HSTR"};
            play.spot_references={"HITH", "HSRG","LHTH","MHTR","UHTR","MNFR","RMNR"};
            play.rrad_references={
                "HNIF\\/TQMN-UHTR",
                "HNIF\\/TQMN-MHTR",
                "HNIF\\/TQMN-RMNR",
            };
            play.aed_references={"HITH"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2450;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.64";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1170;
            play.strat_col_bot_clip2=2755;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(1){
            Play play; 
            play.prefix="abab";
            play.title= "Arab-A and B Reservoirs";
            play.pvad_references= {"ARBA","UARB","ARBB","PABS","ARAB","LARB","PAAS", "ABAB"};
            play.spot_references={"ABAR", "ABBR"};
            play.rrad_references={"HNIF\\/TQMN-ARBA","HNIF\\/TQMN-ARBB",};
            play.aed_references={"ABAR", "ABBR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2450;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.64";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1188.5;
            play.strat_col_bot_clip2=2740;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="abcr";
            play.title= "Arab-C Reservoir";
            play.pvad_references= {"ABCD","LARC", "UARC","ARBC","PACS"};
            play.spot_references={"ABCR", "PACS"};
            play.rrad_references={"HNIF\\/TQMN-ARBC"};
            play.aed_references={"ABCR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2450;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.64";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1188.5;
            play.strat_col_bot_clip2=2740;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="abdr";
            play.title= "Arab-D Reservoir";
            play.pvad_references= {"ABCD","ARBD", "ABD4", "PADS", "ABDR"};
            play.spot_references={"ABDR"};
            play.rrad_references={"HNIF\\/TQMN-ARBD",};
            play.aed_references={"ABDR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2450;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.64";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1188.5;
            play.strat_col_bot_clip2=2740;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="jubl";
            play.title= "Jubaila Reservoirs";
            play.pvad_references= {"UJBL", "JUBL","LJBL", "UJBR"};
            play.spot_references={"UJBR", "LJBR", "JUBL"};
            play.rrad_references={
                "HNIF\\/TQMN-UJUB",
                "HNIF\\/TQMN-LJUB"
            };
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2432;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.63";
            play.strat_col_bot_label="0.66";
            play.strat_col_top_clip2=1208;
            play.strat_col_bot_clip2=2725;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="hnfr";
            play.title= "Hanifa Reservoir";
            play.pvad_references={"UHNF","LHNF", "HNIF"};
            play.spot_references={"HNIF","LHNF", "UHFR", "LHFR"};
            play.rrad_references={
                "HNIF\\/TQMN-UHNF",
                "HNIF\\/TQMN-LHNF"
            };
            play.aed_references={"HNFR","HNIF"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.61";
            play.strat_col_bot_label="0.70";
            play.strat_col_top_clip2=1224;
            play.strat_col_bot_clip2=2685;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="hdrr";
            play.title= "Hadriya Reservoir";
            play.pvad_references={"HDRY","UHDR", "LHDR"};
            play.spot_references={"HDRR","BHDR", "TMLS"};
            play.rrad_references={
                "DHRM-HDRR",
                "DHRM-HDLR",
            };
            play.aed_references={"HDRR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.59";
            play.strat_col_bot_label="0.61";
            play.strat_col_top_clip2=1264;
            play.strat_col_bot_clip2=2661;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="fdhl";
            play.title= "Fadhili Reservoirs";
            play.pvad_references={"UFDL","LFDR","LFDL"};
            play.spot_references={"UFDR","MFDR","LFDR"};
            play.rrad_references={
                "DHRM-UFDR",
                "DHRM-LFDR",
            };
            play.aed_references={"UFDR", "LFDR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.55";
            play.strat_col_bot_label="0.59";
            play.strat_col_top_clip2=1270;
            play.strat_col_bot_clip2=2655;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(1){
            Play play; 
            play.prefix="dhrm";
            play.title= "Dharuma Plays";
            play.pvad_references={"LDMR","FRDA","FRDB","FRDC","FRDD","FRDE","SHRR", "FDCR", "FDDR", "FRDH", "LDRM"};
            play.spot_references={"LDMR","FDBR","FRDR","FRAR","FDCR","FDDR","FDER","SRRR"};
            play.rrad_references={
                "DHRM-SARR",
                "DHRM-FRAR",
                "DHRM-FRBR",
                "DHRM-FRCR",
                "DHRM-FRER",
                "MNJR-LDMR",
            };
            play.aed_references={"LDMR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.43";
            play.strat_col_bot_label="0.50";
            play.strat_col_top_clip2=1291;
            play.strat_col_bot_clip2=2595;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(1){
            Play play; 
            play.prefix="mrrt";
            play.title= "Marrat Plays";
            play.pvad_references={"MRRT"};
            play.spot_references={"MRTR","MRAR","MRCR","MRER"};
            play.rrad_references={
                "SUDR-MRAR",
                "SUDR-MRCR",
                "SUDR-MRER",
            };
            play.aed_references={};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.43";
            play.strat_col_bot_label="0.50";
            play.strat_col_top_clip2=1360;
            play.strat_col_bot_clip2=2490;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="mnjr";
            play.title= "Minjur";
            play.pvad_references={"MNJR","UMNJ"};
            play.spot_references={"MJRR"};
            play.rrad_references={"MNJR-MNJR"};
            play.aed_references={"MNJR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=2200;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.25";
            play.strat_col_bot_label="0.60";
            play.strat_col_top_clip2=1465;
            play.strat_col_bot_clip2=2316;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JURASSIC";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="jilh";
            play.title= "Jilh and Jilh Dolomite Plays";
            play.pvad_references={"JILH", "JLHR","JLDM"};
            play.spot_references={"JLHR","JLDM", "BJDM"};
            play.rrad_references={"BJDM-JILH"};
            play.aed_references={"JLDM", "JILH","BJDM"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1980;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.34";
            play.strat_col_bot_label="0.73";
            play.strat_col_top_clip2=1632;
            play.strat_col_bot_clip2=2140;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JILHKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        if(1){
            Play play; 
            play.prefix="sudr";
            play.title= "Sudair Play";
            play.pvad_references={};
            play.spot_references={"SDRR"};
            play.rrad_references={"SUDR-SUDR"};
            play.aed_references={"SDRR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1900;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.43";
            play.strat_col_bot_label="0.5";
            play.strat_col_top_clip2=1820;
            play.strat_col_bot_clip2=2099;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JILHKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="khff";
            play.title= "Khuff Reservoirs (A, B, C, and D)";
            play.pvad_references={"KHFF", "LKFA", "LKFB", "KHFA", "KHFB", "KHFC", "KFBC", "KHFD","KFCR"};
            play.spot_references={"KHFF", "KFAR", "KFBR", "KFCR", "KFDR", "KBCR", "BKDC"};
            play.rrad_references={"BQHS-KHFA", "BQHS-KHFB", "BQHS-KHFC", "BQHS-KHFD"};
            play.aed_references={"KHFF", "KFAR", "KFBR", "KFCR", "KFDR"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1800;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.39";
            play.strat_col_bot_label="0.64";
            play.strat_col_top_clip2=1850;
            play.strat_col_bot_clip2=1978;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-JILHKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="unza";
            play.title= "Unayzah A and B. Khuff Clastics";
            play.pvad_references={"UNSO","UNZR", "UNZA", "UZAE", "UZA2", "BKFC", "UNYZ", "UZA1"};
            play.spot_references={"UNA1","UNZA", "UNA2", "BKFR", "UNZR", "BKFC", "UNYA"};
            play.rrad_references={
                "BQHS-BKFC", 
                "BQHS-UNA1",
                "BQHS-UNA2",
            };
            play.aed_references={"BKFC", "UNYZ", "UNZA", "UNA2"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1600;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.45";
            play.strat_col_bot_label="0.81";
            play.strat_col_top_clip2=1970;
            play.strat_col_bot_clip2=1805;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-PREKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="ubcu";
            play.title= "Unayzah BC Undifferentiated";
            play.pvad_references={"UNZB","UNZC","UZBC", "UZBE", "UBCU"};
            play.spot_references={"UNZB","UNYB", "UNZC", "UNYC"};
            play.rrad_references={"BQHS-UNZB\\/UNZC"};
            play.aed_references={"UBCU"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1600;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.10";
            play.strat_col_bot_label="0.45";
            play.strat_col_top_clip2=2135;
            play.strat_col_bot_clip2=1650;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-PREKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="devo";
            play.title= "Devonian Reservoirs";
            play.pvad_references={"JAUF","TWIL", "JUBH"};
            play.spot_references={"JBHR","JUFR", "TWLR"};
            play.rrad_references={"BQHS-BRTH", "BQHS-JUBH", "BQHS-JAUF", "BQHS-TWLF"};
            play.aed_references={"JAUF", "JUFR", "TWLF"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=850;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.20";
            play.strat_col_bot_label="0.95";
            play.strat_col_top_clip2=2863;
            play.strat_col_bot_clip2=930;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-PREKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="qusb";
            play.title= "Silurian Reservoirs";
            play.pvad_references={"MQSB","SWRR"};
            play.spot_references={"LQSB","RDNS","SWRR", "MQSR", "QUSB"};
            play.rrad_references={
                "BQHS-SHRR", 
                "BQHS-MQSB", 
                "BQHS-RDNS",
            };
            play.aed_references={"SHRR", "SWRR", "MQBS", "RDNS", "QUSB"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=600;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.40";
            play.strat_col_bot_label="0.67";
            play.strat_col_top_clip2=3020;
            play.strat_col_bot_clip2=784;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-PREKHFF";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="ordv";
            play.title= "Cambrian-Ordovician Reservoirs";
            play.pvad_references={"SARH", "QWRH", "QWRR", "SAQR"};
            play.spot_references={"SRHR", "QWRR", "QSMR", "SAQR", "KRSR", "RBTR", "MRKR", "KHFH", "KFHC", "SIQR", "BURJ", "ZRQS", "SARH", "KRSB"};
            play.rrad_references={
                "QUSB\\/ORDO-SARH",
                "QUSB\\/ORDO-QWRH",
                "QUSB\\/ORDO-KHFH",
                "QUSB\\/ORDO-SAQ",
                "QUSB\\/ORDO-SIQ",
            };
            play.aed_references={"KHFH", "QWRR", "SAQ", "SARH", "ORDV"}; 
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.12";
            play.strat_col_bot_label="0.84";
            play.strat_col_top_clip2=3160;
            //play.strat_col_bot_clip2=450;
            play.strat_col_bot_clip2=633;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-SARH";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="pcam";
            play.title= "Pre-Cambrian Plays";
            play.pvad_references={};
            play.spot_references={"PCAM"};
            play.rrad_references={
                "PCAM-PCAM",
            };
            play.aed_references={}; 
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=400;
            play.strat_col_bot_clip=3500-play.strat_col_top_clip;
            play.strat_col_top_label="0.12";
            play.strat_col_bot_label="0.84";
            play.strat_col_top_clip2=3620;
            play.strat_col_bot_clip2=245;
            play.basin = EasternBasin;
            play.drill_zone = "DZ-BSMT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="ghws";
            play.title= "Ghawwas (Post-Salt) Reservoirs";
            play.pvad_references={"MNSH"};
            play.spot_references={"GWSR","MNSR"};
            play.rrad_references={"MNSY-UGHA", "MNSY-LGHA"};
            play.aed_references={"Ghawwas", "Post-Salt"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1400;
            play.strat_col_bot_clip=1600-play.strat_col_top_clip;
            play.strat_col_top_label="0.58";
            play.strat_col_bot_label="0.8";
            play.strat_col_top_clip2=282;
            play.strat_col_bot_clip2=1713;
            play.basin = RedSeaBasin;
            play.drill_zone = "DZ-POSTSALT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="mqna";
            play.title= "Maqna Group";
            play.pvad_references={"MQNA","MQNB", "MQNR","MDYS","KIAL", "JBKR", "WAQB", "WAQR", "UMLJ", "JBKA", "JBKB"};
            play.spot_references={"JBKR","KIAL", "SIRR", "WAQR", "YBAR", "UMLR"};
            play.rrad_references={"BRQN\\/WAJH-WAQB", "KIAL-SIDR"};
            play.aed_references={"Maqna Clastics", "Sidr Sands", "Yuba Sands", "Umluj Sands", "Maqna Carbonates", "Wadi Waqb", "Maqna Group", "Pre-Salt"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1200;
            play.strat_col_bot_clip=1600-play.strat_col_top_clip;
            play.strat_col_top_label="0.35";
            play.strat_col_bot_label="0.83";
            play.strat_col_top_clip2=470;
            //play.strat_col_bot_clip2=1360;
            play.strat_col_bot_clip2=1360;
            play.basin = RedSeaBasin;
            play.drill_zone = "DZ-PRESALT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="brqn";
            play.title= "Burqan Reservoirs";
            play.pvad_references={"BRQN", "BRQR"};
            play.spot_references={"BRQR"};
            play.rrad_references={"BRQN\\/WAJH-BRQN"};
            play.aed_references={"Burqan"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=1000;
            play.strat_col_bot_clip=1600-play.strat_col_top_clip;
            play.strat_col_top_label="0.45";
            play.strat_col_bot_label="0.75";
            play.strat_col_top_clip2=722;
            play.strat_col_bot_clip2=1229;
            play.basin = RedSeaBasin;
            play.drill_zone = "DZ-PRESALT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="wajh";
            play.title= "Al-Wajh Reservoirs";
            play.pvad_references={"WAJH"};
            play.spot_references={"WJHR"};
            play.rrad_references={"USFN-WAJH"};
            play.aed_references={"Al-Wajh"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=800;
            play.strat_col_bot_clip=1600-play.strat_col_top_clip;
            play.strat_col_top_label="0.2";
            play.strat_col_bot_label="0.75";
            play.strat_col_top_clip2=910;
            play.strat_col_bot_clip2=950;
            play.basin = RedSeaBasin;
            play.drill_zone = "DZ-PRESALT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        {
            Play play; 
            play.prefix="adff";
            play.title= "Adaffa Reservoir";
            play.pvad_references={"ADFR"};
            play.spot_references={"ADFR"};
            play.rrad_references={"ADFF-ADFF"};
            play.aed_references={"Adaffa Reservoir", "ADFF"};
            play.oil_rf="0.3"; // oil recovery factor
            play.gas_rf="0.6"; // gas recovery factor
            play.strat_col_top_clip=400;
            play.strat_col_bot_clip=1600-play.strat_col_top_clip;
            play.strat_col_top_label="0.35";
            play.strat_col_bot_label="0.6";
            play.strat_col_top_clip2=1270;
            play.strat_col_bot_clip2=560;
            play.basin = RedSeaBasin;
            play.drill_zone = "DZ-PRESALT";

            plays[play.prefix]=play;
            seq_plays.push_back(play.prefix);
        }
        //list the plays
        {
            auto & f = mf.add("list_plays");
            f<<HELP("to list all the defined plays");
            for(auto item: seq_plays){
                std::stringstream ss;
                ss<<std::setw(40)<<item<<":  "<<plays[item].title;
                f<<"@echo \""+ss.str()+"\"";
            }
            bool first=true;
            std::stringstream ss;
            ss<<"Listing : ";
            for(auto item: seq_plays){
                if(first){
                    first=false;
                }else{
                    ss<<",";
                }
                ss<<"'"<<item<<"'";
            }
            f<<"@echo \""+ss.str()+"\"";
        }
        //list the plays
        {
            auto & f = mf.add("list_plays_short");
            for(auto item: seq_plays){
                std::stringstream ss;
                ss<<item;
                f<<"@echo \""+ss.str()+"\"";
            }
        }
        {
            bool first = true;
            for(auto item: plays){
                if(first){
                    first=false;
                }else{
                    play_list+=", ";
                }
                play_list+="'"+item.first+"'";
            }
        }
        for(auto & item : plays){
            string play_prefix="play_"+item.first;
            auto & p = item.second;
            auto & f = mf.add(play_prefix+"_stratcol.pdf", p.basin.strat_col+".pdf");
            f<<"pdfcrop --margins '-"+p.basin.strat_col_left_trim+" -"+to_string(p.strat_col_top_clip2)+" -"+p.basin.strat_col_right_trim+" -"+to_string(p.strat_col_bot_clip2)+"' "+p.basin.strat_col+" $@";
            mf.add(play_prefix+"_stratcol.png", play_prefix+"_stratcol.pdf")
                <<"convert -density 300 $<  -quality 90 $@"
                ;
        }
    };

    void load_sde_layers(){

        StringList dbfiles={
            "sdegis_booked.it",
            "completed3d.it",
            "pvad_booked.it",
            "sdegis_fldout.it",
            "pal_eppr_dump.t",
            "pal_sde_dump_orig.it",
            "pal_vn11p_gas_dump.t",
            "pal_vn11p_oil_dump.t",
            "well_xy.t",
            "well_utmxy.t",
            "eppr_well_status.t",
            "spot_pda.t",
            "drill_schedule_orig.t",
            "drill_cost_orig.t",

        };
        {
            auto & f =mf.add("rm_db_data");
            for(auto file:dbfiles){
                f<<"rm -f "+file;
            }
            mf.add("load_db_data", dbfiles);
            mf.add("reload_db_data", {"rm_db_data", "load_db_data"})
                <<HELP("reload local copies of EPPR & SDE data");
        }
        //seismic data
        mf.add("active_seismic3d.it")
            <<"atsdeget layer='SS3D_KSA3DSEISMICAREAS' output=tmp.it login=~/atlogin"
            <<"atitsplitshapes input=tmp.it x=x y=y output=tmp2.it"
            <<"atitselect input=tmp2.it where=\"" _cont
            "blockname like '\%KUTUF SOUTH\%' " _cont
            "or blockname like '\%MAHAKIK II\%' " _cont
            "or blockname like '\%MARJAN II\%' " _cont
            "or blockname like '\%MAZALIJ-MIHWAZ\%' " _cont
            "or blockname like '\%QATIF NORTH\%' " _cont
            "or blockname like '\%RAFHA SOUTH\%' " _cont
            "or blockname like '\%RAMLAH SOUTHEAST\%' " _cont
            "or blockname like '\%SAFANIYA II\%' " _cont
            "or blockname like '\%ZIMLAH\%'\" " _cont
            "output=active_seismic3d.it"
            <<"atwgs84toutm input=active_seismic3d.t lon=x lat=y x='u39x' y='u39y'"
            <<BYPRODUCT(itable("active_seismic3d.it"))
            ;
        mf.on_softclean_retain(iitable("active_seismic3d.it"));
        mf.add("completed3d.it")
            <<"atsdeget layer='SS3D_KSA3DSEISMICAREAS' output=tmp.it login=~/atlogin"
            <<"atitsplitshapes input=tmp.it x=x y=y output=tmp2.it"
            <<"atitselect input=tmp2.it where=\"type_desc like '\%\%complet\%\%'\" output=completed3d.it"
            <<"atwgs84toutm input=completed3d.t lon=x lat=y x='u39x' y='u39y'"
            <<BYPRODUCT(itable("completed3d.it"))
            ;
        mf.on_softclean_retain(iitable("completed3d.it"));
        mf.add("completed3d.shp", "completed3d.it")
            <<"atit2shpfile input=$< output=$@ x=x y=y poly=true"
            ;
        gis_targets.push_back("completed3d.shp");
        //well status
        mf.add("eppr_well_status.t", "eppr_well_status.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@"
            ;
        mf.on_softclean_retain("eppr_well_status.t");
        mf.on_softclean_retain("eppr_well_status.t");
        //drill schedule
        mf.add("drill_schedule_orig.t", "drill_schedule.sql")
            <<"atoracle2tbl sqlfile=drill_schedule.sql login=~/atlogin output=drill_schedule_orig.t"
            ;
        mf.on_softclean_retain("drill_schedule_orig.t");
        mf.on_softclean_retain("drill_schedule_orig.t");
        //well cost
        mf.add("well_cost_history_orig.t", "well_cost_history.sql")
            <<"atoracle2tbl sqlfile=$< login=~/atlogin output=$@"
            ;
        mf.on_softclean_retain("well_cost_history_orig.t");
        mf.on_softclean_retain("well_cost_history_orig.t");
        mf.add("drill_cost_orig.t", "drill_cost.sql")
            <<"atoracle2tbl sqlfile=drill_cost.sql login=~/atlogin output=drill_cost_orig.t"
            ;
        mf.add("drill_cost.t", "drill_cost_orig.t")
            <<"attsqltool drill_cost_orig.t sql=\"select * from @1 where w_gnr_name<>'' and is_water='No' and cost_mm>1\" output=$@"
            ;
        mf.add("drill_schedule.t", {"drill_schedule_orig.t", "pal_eppr_dump.t"})
            <<"attsqltool tables=\"drill_schedule_orig.t pal_eppr_dump.t\" " _cont
            "sql=\"select " _cont
                "case when pl_w_num='' then w_gnr_name else pl_w_num end as slot_id," _cont
                " *, min(spud) as min_spud, max(compl) as max_compl," _cont
                " (select lead_name from @2 where pros_id=@1.pros_id group by pros_id) as lead_name," _cont
                " (select w_expl_name from @2 where pros_id=@1.pros_id group by pros_id) as pal_w_expl_name, " _cont
                " (select cast(w_tent_xutm_cord as int) from @2 where pros_id=@1.pros_id group by pros_id) as pal_x, " _cont
                " (select cast(w_tent_yutm_cord as int) from @2 where pros_id=@1.pros_id group by pros_id) as pal_y, " _cont
                " (select w_tent_utm_zn from @2 where pros_id=@1.pros_id group by pros_id) as pal_utm, " _cont
                " (select group_concat(rsvr_form_cd||' ('||substr(seg_pre_drill_clsf_grp, 1, 1)||', '|| clsr_types|| ')',', ') as pal_targets from @2 where pros_id=@1.pros_id group by pros_id) as lead_name" _cont
                " from @1 where rig not like '%RTC\%' group by pl_w_num order by rig, min_spud\" " _cont
            " output=$@"
            <<"attupdate $@ spud=min_spud"
            <<"attupdate $@ compl=max_compl"
            ;
        mf.add("rm_drill_schedule")
            <<"rm drill_schedule_orig.t";
        mf.add("import_last_ds_to_guwi", {"rm_drill_schedule", "drill_schedule.t", "match_ds_portfolio.t"})
            <<HELP("import the current drilling schedule as a plan in to datbase under the plan named '8888-DS-ED'")
            <<"atdialog \"Make sure the planning tool is closed and the portfolio is saved.\""
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; insert or ignore into plan(id) values('8888-DS-ED')\""
            <<"attsqltool tables=\"drill_schedule.t\" sql=\"select drlg_rqst_num from @1 limit 1\" output=tmp.t"
            <<"attforeach input_tbl=tmp.t " "template=\"sqlite3 "+portfolio_path+" \\\"PRAGMA foreign_keys=ON; update plan set comments='XCOMMENTSX' where id='8888-DS-ED';\\\"\" XCOMMENTSX=drlg_rqst_num" 
            <<"attsqltool tables=\"drill_schedule.t match_ds_portfolio.t\" sql=\"select @2.project, * from @1 inner join @2 on @1.slot_id=@2.slot_id\" output=tmp.t"
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; delete from drill_plan where plan='8888-DS-ED'\""
            <<"attforeach input_tbl=tmp.t " _cont
            "template=\"sqlite3 "+portfolio_path+" \\\"PRAGMA foreign_keys=ON; insert into drill_plan(plan, rig, project, sequence, spud_date, completion_date) values ('8888-DS-ED', 'XRIGX', 'XWELLX', XROWIDX, XSPUDDTX, XCOMPLDTX);\\\"\" " _cont
            "XRIGX=rig XWELLX=project XROWIDX=rowid  XSPUDDTX=spud XCOMPLDTX=compl | sh -v"
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; update drill_plan set duration=completion_date-spud_date where plan='8888-DS-ED'\""
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; "
                "update drill_plan set maint=(select dp2.spud_date-dp1.completion_date as maint "
                "from drill_plan dp1, drill_plan dp2 "
	            "where dp1.plan='8888-DS-ED' and dp2.plan='8888-DS-ED' and dp1.rig=dp2.rig "
                "and dp1.sequence=dp2.sequence-1 and dp2.project=drill_plan.project "
                "order by dp1.rig, maint desc)"
                "where plan='8888-DS-ED'"
            "\""
            ;
        mf.add("import_ds_to_guwi", { "script_load_bp2426.sql", "pal_eppr_dump.t"})
            <<HELP("import the a specific drilling schedule as a plan in Guwi")
            <<"atinputbox \"Schedule#\" default=\"`cat plan_id`\"> plan_id"
            <<BYPRODUCT("plan_id")
            <<"sed <$< \"s/470557/`cat plan_id`/g\" >tmp.sql"
            <<"atoracle2tbl sqlfile=tmp.sql login=~/atlogin output=tmp_plan_orig.t"
            <<"attsqltool tables=\"tmp_plan_orig.t pal_eppr_dump.t\" " _cont
            "sql=\"select " _cont
                "case when pl_w_num='' then w_gnr_name else pl_w_num end as slot_id," _cont
                " *, min(spud) as min_spud, max(compl) as max_compl," _cont
                " (select lead_name from @2 where pros_id=@1.pros_id group by pros_id) as lead_name," _cont
                " (select w_expl_name from @2 where pros_id=@1.pros_id group by pros_id) as pal_w_expl_name, " _cont
                " (select cast(w_tent_xutm_cord as int) from @2 where pros_id=@1.pros_id group by pros_id) as pal_x, " _cont
                " (select cast(w_tent_yutm_cord as int) from @2 where pros_id=@1.pros_id group by pros_id) as pal_y, " _cont
                " (select w_tent_utm_zn from @2 where pros_id=@1.pros_id group by pros_id) as pal_utm, " _cont
                " (select group_concat(rsvr_form_cd||' ('||substr(seg_pre_drill_clsf_grp, 1, 1)||', '|| clsr_types|| ')',', ') as pal_targets from @2 where pros_id=@1.pros_id group by pros_id) as lead_name" _cont
                " from @1 where rig not like '%RTC\%' and not pl_w_num='ARAK-2 ' group by pl_w_num order by rig, min_spud\" " _cont
            " output=tmp_plan.t"
            <<"attupdate tmp_plan.t spud=min_spud"
            <<"attupdate tmp_plan.t compl=max_compl"
            <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
            <<"atdbdump db=ppd.portfolio qry=\"select *, (select group_concat(play_group, ', ') as plays from drill_target where project=drill_project.project group by project) as reservoirs from drill_project\" output=tmp.t"
            <<"attsqltool tables=\"tmp_plan.t tmp.t\" sql=\"select @1.slot_id, @2.project from @1 inner join @2 on trim(replace(replace(replace(upper(@1.slot_id),' ',''),'.',''),'-',''))=trim(replace(replace(replace(upper(@2.project),' ',''),'.',''),'-',''))\" output=tmp_sugg.t"
            <<"atmatchwhich to=tmp.t to_ids=\"project\" from=tmp_plan.t from_ids=\"slot_id\" sugg=tmp_sugg.t load=match_ds_portfolio.t"
            <<"attsqltool tables=\"tmp_plan.t match_ds_portfolio.t\" sql=\"select @2.project, * from @1 inner join @2 on @1.slot_id=@2.slot_id\" output=tmp.t"
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; delete from drill_plan where plan='`cat plan_id`'\""
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; insert or ignore into plan(id) values ('`cat plan_id`')\""
            <<"attforeach input_tbl=tmp.t " _cont
            "template=\"sqlite3 "+portfolio_path+" \\\"PRAGMA foreign_keys=ON; insert into drill_plan(plan, rig, project, sequence, spud_date, completion_date) values ('`cat plan_id`', 'XRIGX', 'XWELLX', XROWIDX, XSPUDDTX, XCOMPLDTX);\\\"\" " _cont
            "XRIGX=rig XWELLX=project XROWIDX=rowid  XSPUDDTX=spud XCOMPLDTX=compl | sh -v"
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; update drill_plan set duration=completion_date-spud_date where plan='`cat plan_id`'\""
            <<"sqlite3 "+portfolio_path+" \"PRAGMA foreign_keys=ON; "
                "update drill_plan set maint=(select dp2.spud_date-dp1.completion_date as maint "
                "from drill_plan dp1, drill_plan dp2 "
	            "where dp1.plan='`cat plan_id`' and dp2.plan='`cat plan_id`' and dp1.rig=dp2.rig "
                "and dp1.sequence=dp2.sequence-1 and dp2.project=drill_plan.project "
                "order by dp1.rig, maint desc)"
                "where plan='`cat plan_id`'"
            "\""
            ;
        mf.add("import_completed_well_timing", "drill_schedule.t")
            <<HELP("import the spud and completion dates into datbase under the plan named '9999-DS'")
            <<"atdialog \"Make sure the planning tool is closed and the portfolio is saved.\""
            <<"attsqltool tables=\"drill_schedule.t match_ds_portfolio.t\" sql=\"select @2.project as id, * from @1 inner join @2 on @1.slot_id=@2.project\" output=tmp.t"
            <<"attforeach input_tbl=tmp.t template=\"sqlite3 "+portfolio_path+" \\\"PRAGMA foreign_keys=ON; update drill_plan set spud_date='XSPUDDTX', completion_date='XCOMPLDTX' where plan='9999-DS' and rig like 'XRIGX%' and project='XWELLX' and exists(select * from drill_project where project='XWELLX' and (stage like 'Completed' or stage like 'Concluded'));\\\"\" XRIGX=rig XWELLX=id XROWIDX=rowid  XSPUDDTX=spud XCOMPLDTX=compl | sh -v"
            <<"attforeach input_tbl=tmp.t " _cont
            "template=\"sqlite3 "+portfolio_path+" \\\"PRAGMA foreign_keys=ON; update drill_plan set spud_date='XSPUDDTX' where plan='9999-DS' and rig='XRIGX' and project='XWELLX' and exists(select * from drill_project where project='XWELLX' and stage like 'InProgress');\\\"\" " _cont
            "XRIGX=rig XWELLX=id XROWIDX=rowid  XSPUDDTX=spud XCOMPLDTX=compl | sh -v"
            ;

        //pvp
        mf.add("rrad_pvp.t", "rrad_pvp.txt")
            <<"atreadascii2tbl input=$< output=$@";
        mf.add("ordered_plays_w_rrad_status.t", {"ordered_plays.t", "rrad_pvp.t"})
            <<"attsqltool tables=\"ordered_plays.t rrad_pvp.t\" sql=\"select @1.*, " _cont
            " (select assessment_type from @2 where @2.play=@1.play) as rrad_assessment_type," _cont
            " (select pvp_date from @2 where @2.play=@1.play) as pvp_date" _cont
            " from @1\" output=$@"
            ;
        //booked limits
        mf.add("sdegis_booked.it")
            <<"atsdeget layer='RSVO_ReservoirLimits' output=$@ login=~/atlogin"
            <<BYPRODUCT(itable("sdegis_booked.it"))
            ;
        mf.on_softclean_retain(iitable("sdegis_booked.it"));
        mf.add("pvad_booked.it", "sdegis_booked.it")
            <<"atitsplitshapes input=$< x=x y=y output=$@"
            <<"atwgs84toutm input=pvad_booked.t lon=x lat=y x='u39x' y='u39y'"
            <<BYPRODUCT(itable("pvad_booked.it"))
            ;
        mf.add("pvad_far.t", "pvad_far.txt")
            <<"atreadascii2tbl input=pvad_far.txt output=pvad_far.t";


        mf.add("pvad_booked_with_far.it", {"pvad_booked.it", "pvad_far.t"}) //with far attributes
            <<"atittee input=pvad_booked.it output=tmp.it"
            <<"attrmcol input=tmp.lt list='shape'"
            <<"attsqltool tables=\"tmp.lt pvad_far.t\"" _cont 
            " sql=\"select *, " _cont 
            "         (select is2dor3d from @2 where @1.w_prim_hid like field_code and far_code like '%'||@1.rsvr_cd||'%') as is2dor3d, " _cont
            "         (select is1mmblor1bscf from @2 where @1.w_prim_hid like field_code and far_code like '%'||@1.rsvr_cd||'%') as prod_rate, " _cont
            "         (select over5pct_prod from @2 where @1.w_prim_hid like field_code and far_code like '%'||@1.rsvr_cd||'%') as over5pct_prod " _cont
            "       from @1\" output=tmp.lt"
            <<"atittee input=tmp.it output=$@" _cont 
            ;
        mf.add("pvad_booked_with_far.shp", {"pvad_booked_with_far.it"})
            <<"atit2shpfile input=$< output=$@ x=x y=y poly=true";

        mf.add("pvad_booked.shp", "pvad_booked.it")
            <<"atit2shpfile input=$< output=$@ x=x y=y poly=true";
        gis_targets.push_back("pvad_booked.shp");

        mf.add("sdegis_fldout.it")
            <<"atsdeget layer='FLDO_KSAHCFIELDANDAREABNDS_VW' output=tmp.it login=~/atlogin"
            <<"atitsplitshapes input=tmp.it x=x y=y output=$@"
            <<"atwgs84toutm input=sdegis_fldout.t lon=x lat=y x='u39x' y='u39y'"
            <<BYPRODUCT(itable("sdegis_fldout.it"))
            ;
        mf.on_softclean_retain(iitable("sdegis_fldout.it"));
        mf.add("sdegis_fldout_oil.it","sdegis_fldout.it")
            <<"atitselect input=sdegis_fldout.it where=\"hybrid_hc_class='OIL'\" output=sdegis_fldout_oil.it"
            <<BYPRODUCT(itable("sdegis_fldout_oil.it"))
            ;
        mf.add("sdegis_fldout_oil.shp", "sdegis_fldout_oil.it")
            <<"atit2shpfile input=$< output=$@ x=x y=y poly=true" ;
        gis_targets.push_back("sdegis_fldout_oil.shp");

        mf.add("sdegis_fldout_gas.it","sdegis_fldout.it")
            <<"atitselect input=sdegis_fldout.it where=\"hybrid_hc_class='GAS'\" output=sdegis_fldout_gas.it"
            <<BYPRODUCT(itable("sdegis_fldout_gas.it"))
            ;
        mf.add("sdegis_fldout_gas.shp", "sdegis_fldout_gas.it")
            <<"atit2shpfile input=$< output=$@ x=x y=y poly=true";
        gis_targets.push_back("sdegis_fldout_gas.shp");

        //SPOT input
        mf.add("pal_eppr_dump.t")
            <<"atoracle2tbl db=EPPR login=~/atlogin sql=\"select * from PAL_SUMMARY_VW where pre_drill_ver_flg='Y' and ver_pros_lead_type in ('PROSPECT', 'LEAD')\" output=$@";
        mf.on_softclean_retain("pal_eppr_dump.t");

        mf.add({"pal_opr.t", "pal_opr.it"}, {"pal_eppr_dump.t", "spot_play_lookup.t","pap_traptype_spot_lookup.t"})
            <<"attsqltool tables=\"pal_eppr_dump.t spot_play_lookup.t pap_traptype_spot_lookup.t\" sql="
            "\"select case when w_expl_name not null and not w_expl_name='' then replace(w_expl_name, '_','-') else @1.name end as name, "
            " case when name=lead_name then '' when name=pros_name then lead_name else (case when pros_name<>lead_name then pros_name ||', ' || lead_name else lead_name end) end as old_names, "
            " @1.pros_status_type as status, group_concat(rsvr_form_cd, ', ') reservoirs, w_tent_xutm_cord, w_tent_yutm_cord, w_tent_utm_zn, "
            " (select play from @2 where @1.rsvr_form_cd=@2.rescd) as play_group "
            " , (select max(pap_traptype) from @3 where spot_clsr_class like @1.clsr_types) as clsr_type"
            " , group_concat(distinct seg_pre_drill_clsf) as prms_class"
            " from @1 where rsvr_form_cd<>'' and"
            " ((pre_drill_ver_flg='Y' and pros_status_type='INACTIVE') or (ver_cur_flg='Y' and pros_status_type='ACTIVE')) "
            "group by name, play_group order by name\" output=tmp.t"
            <<"atindextable input=tmp.t key=name keep=true output=pal_opr.it";
            ;

        mf.add("pal_sde_dump_orig.it")
            <<"atsdeget layer='LDPR_PROSPECTANDLEADSEXPL_VW' output=$@ login=~/atlogin"
            <<BYPRODUCT(itable("pal_sde_dump_orig.it"))
            ;
        mf.on_softclean_retain(iitable("pal_sde_dump_orig.it"));
        mf.add("pal_sde_dump.it", "pal_sde_dump_orig.it")
            <<"atittee input=$< output=tmp.it"
            <<"atwgs84toutm input=tmp.t lon=x lat=y x='u39x' y='u39y'"
            //removing duplicate polygons (same seg_id)!
            <<"attsqltool tmp.lt sql=\"select seg_id, max(index_col) as idxcol from @1 where pros_lead_type in ('PROSPECT', 'LEAD') group by seg_id\" output=tmp_filtlist.t"
            <<"atitsubset input=tmp.it output=tmp2.it subset=tmp_filtlist.t subsetkey=idxcol"
            //index by seg_id now
            <<"attupdate input=tmp2.lt index_col=\"cast(seg_id as int)\""
            <<"atittee input=tmp2.it output=$@"
            <<BYPRODUCT(itable("pal_sde_dump.it"))
            ;
        mf.add("spot_pda.t", "spot_pda.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@" ;
        mf.on_softclean_retain("spot_pda.t");

        mf.add("spot_pda_simple.t", {"spot_pda.t", "well_utmxy.t", "historical_wellvolumeadd_withxy.t", "spot_play_lookup.t"})
            <<"attsqltool tables=\"spot_pda.t historical_wellvolumeadd_withxy.t spot_play_lookup.t\" sql=\"select @1.*, play "
                ", case when exists(select * from @2 where well=@1.w_gnr_name and @2.play=@3.play and (oil>0 or gas>0)) then 'Yes' else 'No' end as commercial"
                ", case when post_drill_clsf_typ_cd like 'dry' then 'No' else 'Yes' end as technical"
                ", case when pre_drill_clsf like '\%delineation\%' then 'Delineation' else 'Prospective' end as prms_class"
                " from @1 inner join @3 on @1.rsvr_form_cd=@3.rescd \" output=tmp.t"
            <<"attsqltool tables=\"well_utmxy.t tmp.t\" sql=\"select @2.*, @1.u39x, @1.u39y from @2 inner join @1 on @2.w_gnr_name=@1.w_gnr_name\" output=$@";

        //grab geox assessments
        mf.add("pal_vn11p_gas_dump.t")
            <<"atoracle2tbl database=VN11P login=~/atlogin sql=\"select * from geox.cu_spot_seg_inplace_non_gas\" output=$@"
            ;
        mf.on_softclean_retain("pal_vn11p_gas_dump.t");
        mf.add("pal_vn11p_oil_dump.t")
            <<"atoracle2tbl database=VN11P login=~/atlogin sql=\"select * from geox.cu_spot_seg_inplace_oil\" output=$@"
            ;
        mf.on_softclean_retain("pal_vn11p_oil_dump.t");
        //all geox
        mf.add("pal_geox_all_gas.t", {"pal_eppr_dump.t", "pal_vn11p_gas_dump.t"})
            //<<"attsqltool tables=\"pal_eppr_dump.t pal_vn11p_gas_dump.t\" sql=\"select @1.pos_segment, cast(@1.seg_id as int) as seg_id, cast(@1.geox_anal_id as int) as geox_anal_id, @1.seg_desc, @1.gas_rsk_rsvrs_mean, @1.pros_status_type, @2.* from @2 inner join @1 on @2.seg_ana_id = @1.geox_anal_id \" output=$@"
            <<"attsqltool tables=\"pal_eppr_dump.t pal_vn11p_gas_dump.t\" sql=\"select @1.pos_segment, cast(@1.seg_id as int) as seg_id, cast(@1.geox_anal_id as int) as geox_anal_id, @1.seg_desc, @1.gas_rsk_rsvrs_mean, @1.pros_status_type, @2.* from @2 inner join @1 on @2.seg_ana_id = @1.geox_anal_id where p00<10000\" output=$@"
            ;
        mf.add("pal_geox_all_oil.t", {"pal_eppr_dump.t", "pal_vn11p_oil_dump.t"})
            <<"attsqltool tables=\"pal_eppr_dump.t pal_vn11p_oil_dump.t\" sql=\"select @1.pos_segment, cast(@1.seg_id as int) as seg_id, cast(@1.geox_anal_id as int) as geox_anal_id, @1.seg_desc, @1.oil_rsk_rsvrs_mean, @1.pros_status_type, @2.* from @2 inner join @1 on @2.seg_ana_id = @1.geox_anal_id\" output=$@"
            ;
        //active geox
        mf.add("pal_geox_active_gas.t", {"pal_eppr_dump.t", "pal_vn11p_gas_dump.t"})
            <<"attsqltool tables=\"pal_eppr_dump.t pal_vn11p_gas_dump.t\" sql=\"select @1.pos_segment, cast(@1.seg_id as int) as seg_id, cast(@1.geox_anal_id as int) as geox_anal_id, @1.seg_desc, @1.gas_rsk_rsvrs_mean, @2.* from @2 inner join @1 on @2.seg_ana_id = @1.geox_anal_id where @1.pros_status_type='ACTIVE' and p00<10000\" output=$@"
            ;
        mf.add("pal_geox_active_oil.t", {"pal_eppr_dump.t", "pal_vn11p_oil_dump.t"})
            <<"attsqltool tables=\"pal_eppr_dump.t pal_vn11p_oil_dump.t\" sql=\"select @1.pos_segment, cast(@1.seg_id as int) as seg_id, cast(@1.geox_anal_id as int) as geox_anal_id, @1.seg_desc, @1.oil_rsk_rsvrs_mean, @2.* from @2 inner join @1 on @2.seg_ana_id = @1.geox_anal_id where @1.pros_status_type='ACTIVE'and p00<10000\" output=$@"
            ;
        mf.add("pal_geox_all_gas.g", "pal_geox_all_gas.t")
            <<"atgeox2array input=$< o1=0 d1=10 n1=1001 output=$@"
            ;
        mf.add("pal_geox_all_oil.g", "pal_geox_all_oil.t")
            <<"atgeox2array input=$< o1=0 d1=10 n1=1001 output=$@"
            ;
        mf.add("pal_geox_active_gas.g", "pal_geox_active_gas.t")
            <<"atgeox2array input=$< o1=0 d1=10 n1=1001 output=$@"
            ;
        mf.add("pal_geox_active_oil.g", "pal_geox_active_oil.t")
            <<"atgeox2array input=$< o1=0 d1=10 n1=1001 output=$@"
            ;
        mf.add("pal_geox_active_gas_agr.g", {"pal_geox_active_gas.g", "pal_geox_active_gas.t"})
            <<"atspotaggregate target_table=pal_geox_active_gas.t target_gasvol=pal_geox_active_gas.g output=$@ n1=10000"
            ;

        mf.add("pal_geox_active_oil_agr.g", {"pal_geox_active_oil.g", "pal_geox_active_oil.t"})
            <<"atspotaggregate target_table=pal_geox_active_oil.t target_gasvol=pal_geox_active_oil.g output=$@ n1=10000"
            ;
        mf.add("pal_geox_active_gas_stats.dat", "pal_geox_active_gas_agr.g")
            <<"rm -f $@"
            <<package_stats("$<", "gas", "$@")
            ;
        mf.add("pal_geox_active_oil_stats.dat", "pal_geox_active_oil_agr.g")
            <<"rm -f $@"
            <<package_stats("$<", "oil", "$@")
            ;

        mf.add("spot_active_portfolio_gas.it", "spot_active_portfolio.it") 
            <<"atitselect input=spot_active_portfolio.it where=\"bscf_mean>0\" output=$@"
            <<BYPRODUCT(itable("spot_active_portfolio_gas.it"))
            ;
        mf.add("spot_active_portfolio_oil.it", "spot_active_portfolio.it") 
            <<"atitselect input=spot_active_portfolio.it where=\"mmbo_mean>0\" output=$@"
            <<BYPRODUCT(itable("spot_active_portfolio_oil.it"))
            ;

        mf.add("spot_active_portfolio_gas.shp", "spot_active_portfolio_gas.it") 
            <<"atit2shpfile input=$< x=u39x y=u39y poly=true output=$@"
            ;
        gis_targets.push_back("spot_active_portfolio_gas.shp");

        mf.add("spot_active_portfolio_oil.shp", "spot_active_portfolio_oil.it") 
            <<"atit2shpfile input=$< x=u39x y=u39y poly=true output=$@"
            ;
        gis_targets.push_back("spot_active_portfolio_oil.shp");

        //other input
        mf.add("strat_formations_vw.t")
            <<"atoracle2tbl login=~/atlogin sql=\"select * from strat_formations_vw\" output=$@";
        mf.on_softclean_retain("strat_formations_vw.t");
        mf.add("strat_info.t")
            <<"atoracle2tbl login=~/atlogin sql=\"select * from strat_info\" output=$@";
        mf.on_softclean_retain("strat_info.t");
    }
    void qc_spot_inputs(){
        int nqc=1;
        mf.add("spot_stats", 
                {"pal_eppr_dump.t", 
                "pal_geox_active_gas.t", "pal_geox_active_gas_stats.dat",
                "pal_geox_active_oil.t", "pal_geox_active_oil_stats.dat",
                })
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select * from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" output=shaply.t"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets assessed in SPOT portfolio (GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_unrsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets assessed in SPOT portfolio (GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_unrsk_rsvrs_mean))-round(sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to risks' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(1.0*seg_seal_risk*seg_closure_risk*seg_charge_risk*gas_unrsk_rsvrs_mean)-sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to reservoir risk' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(seg_rsvr_risk*1.0*seg_closure_risk*seg_charge_risk*gas_unrsk_rsvrs_mean)-sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to seal risks' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(seg_rsvr_risk*seg_seal_risk*1.0*seg_charge_risk*gas_unrsk_rsvrs_mean)-sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to closure risks' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(seg_rsvr_risk*seg_seal_risk*seg_closure_risk*1.0*gas_unrsk_rsvrs_mean)-sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to charge risks' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(seg_rsvr_risk*seg_seal_risk*seg_closure_risk*seg_charge_risk*gas_unrsk_rsvrs_mean)-sum(gas_rsk_rsvrs_mean)) || ' BSCF lost to other risks' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
        ;
        mf.add("spot_qc"+to_string(nqc++), 
                {"pal_eppr_dump.t", 
                "pal_geox_active_gas.t", "pal_geox_active_gas_stats.dat",
                "pal_geox_active_oil.t", "pal_geox_active_oil_stats.dat",
                })
            //<<HELP("display basic SPOT-portfolio information")
            <<"@echo '##############################################################################################'"
            <<"@echo '## Summary of SPOT's gas portfolio                                                          ##'"
            <<"@echo '##############################################################################################'"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets assessed in SPOT portfolio (GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets assessed in SPOT portfolio (non-GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id not in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets assessed in SPOT portfolio (Total)' from @1 where pros_status_type='ACTIVE'\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## A break-down of GEOX assessments by pros/lead types                                      ##'"
            <<"@echo '##############################################################################################'"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select pros_lead_type, round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets ' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2) group by pros_lead_type\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## A break-down of non-GEOX assessments by pros/lead types                                  ##'"
            <<"@echo '##############################################################################################'"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_gas.t\" sql=\"select pros_lead_type, round(sum(gas_rsk_rsvrs_mean)) || ' BSCF in ' || count(*) || ' Targets ' from @1 where pros_status_type='ACTIVE' and seg_id not in (select seg_id from @2) group by pros_lead_type\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## This is the statistical summery of my implemntation of aggregates of GEOX assessments    ##'"
            <<"@echo '##############################################################################################'"
            <<"@cat pal_geox_active_gas_stats.dat"
            <<"@echo '##############################################################################################'"
            <<"@echo '## Summary of SPOT's oil portfolio                                                          ##'"
            <<"@echo '##############################################################################################'"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_oil.t\" sql=\"select round(sum(oil_rsk_rsvrs_mean)) || ' MMBO in ' || count(*) || ' Targets assessed in SPOT portfolio (GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_oil.t\" sql=\"select round(sum(oil_rsk_rsvrs_mean)) || ' MMBO in ' || count(*) || ' Targets assessed in SPOT portfolio (non-GEOX)' from @1 where pros_status_type='ACTIVE' and seg_id not in (select seg_id from @2)\" headers=false"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_oil.t\" sql=\"select round(sum(oil_rsk_rsvrs_mean)) || ' MMBO in ' || count(*) || ' Targets assessed in SPOT portfolio (Total)' from @1 where pros_status_type='ACTIVE'\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## A break-down of GEOX assessments by pros/lead types                                      ##'"
            <<"@echo '##############################################################################################'"
            <<"attsqltool tables=\"pal_eppr_dump.t pal_geox_active_oil.t\" sql=\"select pros_lead_type, round(sum(oil_rsk_rsvrs_mean)) || ' MMBO in ' || count(*) || ' Targets ' from @1 where pros_status_type='ACTIVE' and seg_id in (select seg_id from @2) group by pros_lead_type\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## A break-down of non-GEOX assessments by pros/lead types                                  ##'"
            <<"@echo '##############################################################################################'"
            <<"@attsqltool tables=\"pal_eppr_dump.t pal_geox_active_oil.t\" sql=\"select pros_lead_type, round(sum(oil_rsk_rsvrs_mean)) || ' MMBO in ' || count(*) || ' Targets ' from @1 where pros_status_type='ACTIVE' and seg_id not in (select seg_id from @2) group by pros_lead_type\" headers=false"
            <<"@echo '##############################################################################################'"
            <<"@echo '## This is the statistical summery of my implemntation of aggregates of GEOX assessments    ##'"
            <<"@echo '##############################################################################################'"
            <<"@cat pal_geox_active_oil_stats.dat"
            <<"@echo '##############################################################################################'"
            ;

        //check SPOT tables before using
        mf.add("spot_qc"+to_string(nqc++), 
            {"pal_eppr_dump.t", "pal_sde_dump.it"})
            //<<HELP("cross-check the portfolio data in EPPR and SDE")
            <<"@echo '################################################################'"
            <<"@echo '## P&L in EPPR but not in SDE (i.e. no corrosponding polygon) ##'"
            <<"@echo '################################################################'"
            <<"attsqltool tables=\"pal_eppr_dump.t pal_sde_dump.lt\" sql=\"select  name, rsvr_form_cd, seg_name, cast(seg_id as int) as seg_id, pros_status_type, status_dt from @1 where @1.pros_status_type='ACTIVE' and @1.seg_id>0 and pros_lead_type in ('PROSPECT', 'LEAD') and seg_id not in (select seg_id from @2 where pros_status_type='ACTIVE') order by name\" headers=false"
            <<"@echo '################################################################'"
            <<"@echo '## P&L in SDE but not in EPPR (i.e. dengling polygons)        ##'"
            <<"@echo '################################################################'"
            <<"attsqltool tables=\"pal_eppr_dump.t pal_sde_dump.lt\" sql=\"select cast(seg_id as int) as seg_id, name, seg_name from @2 where @2.pros_status_type='ACTIVE' and pros_id not in (select pros_id from @1 where pros_status_type='ACTIVE') order by name\""
            ;
        //
        mf.add("spot_active_portfolio.it", {"pal_eppr_dump.t", "pal_sde_dump.it"})
            <<"attsqltool tables=\"pal_sde_dump.lt pal_eppr_dump.t\" sql=\"select objectid, name, seg_name, seg_id from @1 where seg_id>0 and pros_status_type='ACTIVE' and seg_id in (select seg_id from @2 where pros_status_type='ACTIVE')\" output=tmp_pal_common.t"
            <<"atitsubset input=pal_sde_dump.it output=tmp.it subset=tmp_pal_common.t subsetkey=\"seg_id\""
            <<"attsqltool tables=\"tmp.lt pal_eppr_dump.t\" sql=\""
            "select index_col, beg, nel, cast(@1.seg_id as int) as seg_id, @1.name,"
            " (select pros_name || ', ' || lead_name || ', ' || w_expl_name from @2 where @2.seg_id=@1.seg_id) as possible_names,"
            " (select rsvr_form_cd from @2 where @2.seg_id=@1.seg_id) as rsvr_cd,"
            " @1.seg_name,"
            " (select seg_desc from @2 where @2.seg_id=@1.seg_id) as seg_desc, "
            " pros_lead_type, "
            " (select case seg_hc_typ_cd when 'G' then 'GAS' else 'OIL' end from @2 where @2.seg_id=@1.seg_id) as hc_type,"
            " (select div_orgn_shrt_name from @2 where @2.seg_id=@1.seg_id) as division,"
            " (select pos_segment from @2 where @2.seg_id=@1.seg_id) as pg, "
            " (select gas_unrsk_rsvrs_p10 from @2 where @2.seg_id=@1.seg_id) as bscf_p10, "
            " (select gas_unrsk_rsvrs_p50 from @2 where @2.seg_id=@1.seg_id) as bscf_p50, "
            " (select gas_unrsk_rsvrs_p90 from @2 where @2.seg_id=@1.seg_id) as bscf_p90, "
            " (select gas_unrsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as bscf_mean, "
            " (select gas_rsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as pg_bscf, "
            " (select oil_unrsk_rsvrs_p10 from @2 where @2.seg_id=@1.seg_id) as mmbo_p10, "
            " (select oil_unrsk_rsvrs_p50 from @2 where @2.seg_id=@1.seg_id) as mmbo_p50, "
            " (select oil_unrsk_rsvrs_p90 from @2 where @2.seg_id=@1.seg_id) as mmbo_p90, "
            " (select oil_unrsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as mmbo_mean, "
            " (select oil_rsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as pg_mmbo, "
            " (select geox_anal_id from @2 where @2.seg_id=@1.seg_id) as geox_anal_id, "
            " (select w_tent_xutm_cord from @2 where @2.seg_id=@1.seg_id) as w_tent_xutm_cord, "
            " (select w_tent_yutm_cord from @2 where @2.seg_id=@1.seg_id) as w_tent_yutm_cord, "
            " (select w_tent_utm_zn from @2 where @2.seg_id=@1.seg_id) as w_tent_utm_zn, "
            " @1.pros_status_type "
            "from @1 order by rowid\" output=tmp.lt"
            <<"atwgs84toutm input=tmp.t lon=x lat=y x='u39x' y='u39y'"
            <<"atunifyutm input=tmp.lt x=w_tent_xutm_cord y=w_tent_yutm_cord zone=w_tent_utm_zn newx=u39x newy=u39y output=tmp.lt"
            <<"atittee input=tmp.it output=$@"
            <<TEMP("tmp_pal_common.t")
            <<BYPRODUCT(itable("spot_active_portfolio.it"))
            ;
        mf.add("spot_all_portfolio.it", {"pal_eppr_dump.t", "pal_sde_dump.it"})
            <<"attsqltool tables=\"pal_sde_dump.lt pal_eppr_dump.t\" sql=\"select objectid, name, seg_name, seg_id from @1 where seg_id>0 \" output=tmp_pal_common.t"
            <<"atitsubset input=pal_sde_dump.it output=tmp.it subset=tmp_pal_common.t subsetkey=\"seg_id\""
            <<"attsqltool tables=\"tmp.lt pal_eppr_dump.t\" sql=\""
            "select index_col, beg, nel, cast(@1.seg_id as int) as seg_id, @1.name,"
            " (select seg_desc ||', ' || pros_name || ', ' || lead_name || ', ' || w_expl_name from @2 where @2.seg_id=@1.seg_id) as possible_names,"
            " (select rsvr_form_cd from @2 where @2.seg_id=@1.seg_id) as rsvr_cd,"
            " @1.seg_name,"
            " (select seg_desc from @2 where @2.seg_id=@1.seg_id) as seg_desc, "
            " pros_lead_type, "
            " (select case seg_hc_typ_cd when 'G' then 'GAS' else 'OIL' end from @2 where @2.seg_id=@1.seg_id) as hc_type,"
            " (select div_orgn_shrt_name from @2 where @2.seg_id=@1.seg_id) as division,"
            " (select pos_segment from @2 where @2.seg_id=@1.seg_id) as pg, "
            " (select gas_unrsk_rsvrs_p10 from @2 where @2.seg_id=@1.seg_id) as bscf_p10, "
            " (select gas_unrsk_rsvrs_p50 from @2 where @2.seg_id=@1.seg_id) as bscf_p50, "
            " (select gas_unrsk_rsvrs_p90 from @2 where @2.seg_id=@1.seg_id) as bscf_p90, "
            " (select gas_unrsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as bscf_mean, "
            " (select gas_rsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as pg_bscf, "
            " (select oil_unrsk_rsvrs_p10 from @2 where @2.seg_id=@1.seg_id) as mmbo_p10, "
            " (select oil_unrsk_rsvrs_p50 from @2 where @2.seg_id=@1.seg_id) as mmbo_p50, "
            " (select oil_unrsk_rsvrs_p90 from @2 where @2.seg_id=@1.seg_id) as mmbo_p90, "
            " (select oil_unrsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as mmbo_mean, "
            " (select oil_rsk_rsvrs_mean from @2 where @2.seg_id=@1.seg_id) as pg_mmbo, "
            " (select geox_anal_id from @2 where @2.seg_id=@1.seg_id) as geox_anal_id, "
            " (select w_tent_xutm_cord from @2 where @2.seg_id=@1.seg_id) as w_tent_xutm_cord, "
            " (select w_tent_yutm_cord from @2 where @2.seg_id=@1.seg_id) as w_tent_yutm_cord, "
            " (select w_tent_utm_zn from @2 where @2.seg_id=@1.seg_id) as w_tent_utm_zn, "
            " @1.pros_status_type "
            "from @1 order by rowid\" output=tmp.lt"
            <<"atwgs84toutm input=tmp.t lon=x lat=y x='u39x' y='u39y'"
            <<"atunifyutm input=tmp.lt x=w_tent_xutm_cord y=w_tent_yutm_cord zone=w_tent_utm_zn newx=u39x newy=u39y output=tmp.lt"
            <<"atittee input=tmp.it output=$@"
            <<TEMP("tmp_pal_common.t")
            <<BYPRODUCT(itable("spot_all_portfolio.it"))
            ;
        //
        mf.add("spot_qc"+to_string(nqc++), 
            "spot_all_portfolio.it")
            //<<HELP("check for duplicate P&L in SDE")
            <<"@echo '#############################################################'"
            <<"@echo '## Duplicate P&L in SDE (i.e. with the same seg_id)!       ##'"
            <<"@echo '#############################################################'"
            <<"@attsqltool spot_all_portfolio.lt sql=\"select * from (select seg_id, name, count(*) as c from @1 group by seg_id) where c>1\" headers=false";

        mf.add("spot_active_portfolio.shp", "spot_active_portfolio.it")
            <<"atit2shpfile input=$< x=u39x y=u39y poly=true output=$@";
        gis_targets.push_back("spot_active_portfolio.shp");

        mf.add("spot_qc"+to_string(nqc++), 
                {"spot_all_portfolio.it","papgis_sa.t"})
            //<<HELP("check for P&L polygons outside kingdom")
            <<"@echo '##########################################'"
            <<"@echo '## SPOT polygons outside kingdom!       ##'"
            <<"@echo '##########################################'"
            <<"atitwind   input=spot_all_portfolio.it expr=\"(u39x<-1220000)+(u39x>1030000)+(u39y<1800000)+(u39y>3670000)\" output=tmp.it"
            //<<"atittee input=spot_all_portfolio.it output=tmp2.it"
            //<<"attinpolygon input=tmp2.t xexpr=u39x yexpr=u39y newcol=inkingdom ply=papgis_sa.t plyx=x plyy=y"
            //<<"atdistance_to_polygons loc=tmp2.t locx=u39x locy=u39y newcol=inkingdom ply=papgis_sa.t plyx=x plyy=y"
            //<<"atitwind   input=tmp2.it expr=\"inkingdom=0\" output=tmp.it"
            <<"attsqltool tmp.lt sql=\"select name from @1\" headers=false"
                ;
        mf.add("spot_qc"+to_string(nqc++), 
            {"spot_all_portfolio.it", 
                "pal_vn11p_gas_dump.t",
                "pal_vn11p_oil_dump.t"
                } )
            //<<HELP("cross check EPPR P&L entries with VN11P")
            <<"@echo '#########################################################'"
            <<"@echo '## GOEX references from EPPR, not present in VN11P!    ##'"
            <<"@echo '#########################################################'"
            <<"@attsqltool tables=\"spot_all_portfolio.lt pal_vn11p_gas_dump.t pal_vn11p_oil_dump.t\" headers=true sql=\"select seg_desc, geox_anal_id, seg_id from @1 where geox_anal_id>0 and geox_anal_id not in (select seg_ana_id from @2) union select seg_desc, geox_anal_id, seg_id from @1 where geox_anal_id>0 and geox_anal_id not in (select seg_ana_id from @3)\""
                ;
        mf.add("spot_qc"+to_string(nqc++), 
            "spot_all_portfolio.it")
            //<<HELP("check for zero-gas & zero-oil prospects")
            <<"@echo '######################################'"
            <<"@echo '## Zero Gas, Zero Oil Prospects!    ##'"
            <<"@echo '######################################'"
            <<"attsqltool tables=\"spot_all_portfolio.lt\" sql=\"select seg_desc, geox_anal_id, seg_id, bscf_mean, mmbo_mean from @1 where bscf_mean=0 and mmbo_mean=0 and geox_anal_id>0\""
            <<"attsqltool tables=\"pal_eppr_dump.t\" sql=\"select seg_desc, gas_rsk_rsvrs_mean, oil_rsk_rsvrs_mean, tot_oileqv_rsk_rsvrs_mean, gas_unrsk_rsvrs_mean, oil_unrsk_rsvrs_mean, geox_anal_id from @1 where gas_rsk_rsvrs_mean=0 and oil_rsk_rsvrs_mean=0 and seg_id>0 and pros_status_type='ACTIVE' and geox_anal_id>0\" headers=false"
                ;
        mf.add("spot_qc"+to_string(nqc++), 
                {"spot_all_portfolio.it"})
            <<"@echo '###########################################################################'"
            <<"@echo '## Prospects with the tentative location not in prospect/segment polygon  ##'"
            <<"@echo '###########################################################################'"
            <<"atspot_check_tent_location locx=u39x locy=u39y loc_cols=\"seg_desc\" ply=spot_all_portfolio.it plyx=u39x plyy=u39y ply_cols=\"seg_id\" output=tmp.t "
            <<"attsqltool tmp.t sql=\"select * from @1 where match<>'inpolygon' order by distance_from_polygon desc\" headers=true output=tmp2.t"
            <<"attedit tmp2.t"
            ;
        mf.add("spot_qc"+to_string(nqc++), 
                {"spot_pda.t"
                })
            <<"@echo '###################################################'"
            <<"@echo '## PDAs with missing post-drill classifications  ##'"
            <<"@echo '###################################################'"
            <<"attsqltool spot_pda.t sql=\"select * from @1 where post_drill_clsf_typ_cd='' order by completion_year\" output=tmp.t"
            <<"attedit tmp.t"
            ;

        {
            strings dep;
            for(int i=1; i<nqc; i++){
                dep.push_back("spot_qc"+to_string(i));
            }
            mf.add("spot_qc{1-"+to_string(nqc-1)+"}", dep)
                <<HELP("QC SPOT data");
        }
    }
    void load_pap_text_files(){
        //investment plan targets
        mf.add("pap_ip_targets.t", "pap_ip_targets.txt")
            <<"atreadascii2tbl input=$< output=$@"
            ;
        //loading trap_types
        mf.add("trap_types.t", "trap_types.txt")
            <<"atreadascii2tbl input=$< output=tmp.t"
            <<"attsqltool tmp.t sql=\"select * from @1 where show='Yes'\" output=$@"
            ;
        mf.add("pap_traptype_spot_lookup.t", "pap_traptype_translation.txt")
            <<"atreadascii2tbl input=$< output=$@";
        //historical cost data
        mf.add("historical_wellcost.t", "historical_wellcost.txt")
            <<"atreadascii2tbl input=$< output=$@";
        //historical volume add data
        mf.add("historical_wellvolumeadd.t", "historical_wellvolumeadd.txt")
            <<"atreadascii2tbl input=$< output=$@";
        mf.add("historical_wellvolumeadd_withxy.t", {"historical_wellvolumeadd.t","well_utmxy.t", "pvad_play_lookup.t"})
            <<"attsqltool tables=\"well_utmxy.t historical_wellvolumeadd.t\" sql=\"select @2.*, @1.u39x, @1.u39y from @2 inner join @1 on @2.Well=@1.w_gnr_name where is_simple like 'yes' and use_for_fsd='yes'\" output=tmp.t"
            <<"attsqltool tables=\"tmp.t pvad_play_lookup.t\" sql=\"select @1.*, play from @1 inner join @2 on @1.Reservoir=@2.rescd\" output=$@";
        //mapping of plays to formation naming in wellsite reports
        mf.add("input_target2td_formations.t", "input_target2td_formations.txt")
            <<"atreadascii2tbl input=input_target2td_formations.txt output=input_target2td_formations.t";
        //forecasting tables
        {
            mf.add("forecast_rigdays.t", {
                    "analysis_wellcluster_cahwth_all_rigdays.g",
                    "analysis_wellcluster_canyym_all_rigdays.g",
                    "analysis_wellcluster_eghwr_prekhff_rigdays.g",
                    "analysis_wellcluster_erak_cretaceous_rigdays.g",
                    "analysis_wellcluster_erak_deep_rigdays.g",
                    "analysis_wellcluster_erak_jurassic_rigdays.g",
                    "analysis_wellcluster_gulf_midsection_rigdays.g",
                    "analysis_wellcluster_gulf_khff_rigdays.g",
                    "analysis_wellcluster_gulf_prekhff_rigdays.g",
                    "analysis_wellcluster_narabia_deep_rigdays.g",
                    "analysis_wellcluster_nghwr_deep_rigdays.g",
                    "analysis_wellcluster_nghwr_shallow_rigdays.g",
                    "analysis_wellcluster_nghwr_jurassic_rigdays.g",
                    "analysis_wellcluster_sghwr_jurassic_rigdays.g",
                    "analysis_wellcluster_wgsat_prekhff_rigdays.g",
                    "analysis_wellcluster_wrak_jurassic_rigdays.g",
                    "analysis_wellcluster_wrak_prekhff_rigdays.g",
                    "analysis_wellcluster_wrak_triassic_rigdays.g",
                    "analysis_wellcluster_rsed_onshore_rigdays.g",
                    "analysis_wellcluster_rsed_shalloww_rigdays.g",
                    "analysis_wellcluster_rsed_deepw_postsalt_rigdays.g",
                    "analysis_wellcluster_rsed_deepw_presalt_rigdays.g",
                    "forecast_rigdays.txt"
                    })
                <<"atreadascii2tbl input=forecast_rigdays.txt output=$@";
            mf.add("forecast_cost.t", {
                    "analysis_wellcluster_cahwth_all_cost.g",
                    "analysis_wellcluster_canyym_all_cost.g",
                    "analysis_wellcluster_eghwr_prekhff_cost.g",
                    "analysis_wellcluster_erak_cretaceous_cost.g",
                    "analysis_wellcluster_erak_deep_cost.g",
                    "analysis_wellcluster_erak_jurassic_cost.g",
                    "analysis_wellcluster_gulf_midsection_cost.g",
                    "analysis_wellcluster_narabia_deep_cost.g",
                    "analysis_wellcluster_nghwr_deep_cost.g",
                    "analysis_wellcluster_nghwr_jurassic_cost.g",
                    "analysis_wellcluster_sghwr_jurassic_cost.g",
                    "analysis_wellcluster_wgsat_prekhff_cost.g",
                    "analysis_wellcluster_wrak_jurassic_cost.g",
                    "analysis_wellcluster_wrak_prekhff_cost.g",
                    "analysis_wellcluster_wrak_triassic_cost.g",
                    "analysis_wellcluster_rsed_onshore_cost.g",
                    "analysis_wellcluster_rsed_shalloww_cost.g",
                    "analysis_wellcluster_rsed_deepw_presalt_cost.g",
                    "analysis_wellcluster_rsed_deepw_postsalt_cost.g",
                    "forecast_cost.txt"
                    })
                <<"atreadascii2tbl input=forecast_cost.txt output=$@";
            mf.add("forecast_gas_wad.t", {
                    "forecast_gas_wad.txt",
                    "analysis_play_abcr_gas_wad.g",
                    "analysis_play_abdr_gas_wad.g",
                    "analysis_play_adff_gas_wad.g",
                    "analysis_play_brqn_gas_wad.g",
                    "analysis_play_buwb_gas_wad.g",
                    "analysis_play_devo_gas_wad.g",
                    "analysis_play_dhrm_gas_wad.g",
                    "analysis_play_fdhl_gas_wad.g",
                    "analysis_play_ghws_gas_wad.g",
                    "analysis_play_hdrr_gas_wad.g",
                    "analysis_play_hith_gas_wad.g",
                    "analysis_play_hnfr_gas_wad.g",
                    "analysis_play_jilh_gas_wad.g",
                    "analysis_play_jubl_gas_wad.g",
                    "analysis_play_khff_gas_wad.g",
                    "analysis_play_mnjr_gas_wad.g",
                    "analysis_play_mqna_gas_wad.g",
                    "analysis_play_mrrt_gas_wad.g",
                    "analysis_play_ordv_gas_wad.g",
                    "analysis_play_qusb_gas_wad.g",
                    "analysis_play_ymsl_gas_wad.g",
                    "analysis_play_rus_gas_wad.g",
                    "analysis_play_shub_gas_wad.g",
                    "analysis_play_ubcu_gas_wad.g",
                    "analysis_play_unza_gas_wad.g",
                    "analysis_play_wajh_gas_wad.g",
                    "analysis_play_wsia_gas_wad.g",
                    "analysis_play_ymsl_gas_wad.g",
                    //"analysis_play_*_gas_wad.g",
                    })
                <<"atreadascii2tbl input=$< output=$@";
            mf.add("forecast_oil_wad.t", 
                    {"forecast_oil_wad.txt",
                    "analysis_play_abab_oil_wad.g",
                    "analysis_play_abcr_oil_wad.g",
                    "analysis_play_abdr_oil_wad.g",
                    "analysis_play_arum_oil_wad.g",
                    "analysis_play_brqn_oil_wad.g",
                    "analysis_play_buwb_oil_wad.g",
                    "analysis_play_bydh_oil_wad.g",
                    "analysis_play_devo_oil_wad.g",
                    "analysis_play_dhrm_oil_wad.g",
                    "analysis_play_dam_oil_wad.g",
                    "analysis_play_dmm_oil_wad.g",
                    "analysis_play_fdhl_oil_wad.g",
                    "analysis_play_ghws_oil_wad.g",
                    "analysis_play_hdrk_oil_wad.g",
                    "analysis_play_hdrr_oil_wad.g",
                    "analysis_play_hith_oil_wad.g",
                    "analysis_play_hnfr_oil_wad.g",
                    "analysis_play_jilh_oil_wad.g",
                    "analysis_play_jubl_oil_wad.g",
                    "analysis_play_khff_oil_wad.g",
                    "analysis_play_mnjr_oil_wad.g",
                    "analysis_play_mqna_oil_wad.g",
                    "analysis_play_mrrt_oil_wad.g",
                    "analysis_play_ordv_oil_wad.g",
                    "analysis_play_qusb_oil_wad.g",
                    "analysis_play_rus_oil_wad.g",
                    "analysis_play_shub_oil_wad.g",
                    "analysis_play_sudr_oil_wad.g",
                    "analysis_play_ubcu_oil_wad.g",
                    "analysis_play_umer_oil_wad.g",
                    "analysis_play_unza_oil_wad.g",
                    "analysis_play_wsia_oil_wad.g",
                    "analysis_play_ymsl_oil_wad.g",
                    })
                <<"atreadascii2tbl input=$< output=$@";
            mf.add("forecast_gas_fsd.t", {
                    "forecast_gas_fsd.txt",
                    "analysis_play_abcr_gas_fsd.g",
                    "analysis_play_abdr_gas_fsd.g",
                    "analysis_play_adff_gas_fsd.g",
                    "analysis_play_brqn_gas_fsd.g",
                    "analysis_play_buwb_gas_fsd.g",
                    "analysis_play_devo_gas_fsd.g",
                    "analysis_play_dhrm_gas_fsd.g",
                    "analysis_play_fdhl_gas_fsd.g",
                    "analysis_play_ghws_gas_fsd.g",
                    "analysis_play_hdrr_gas_fsd.g",
                    "analysis_play_hith_gas_fsd.g",
                    "analysis_play_hnfr_gas_fsd.g",
                    "analysis_play_jilh_gas_fsd.g",
                    "analysis_play_jubl_gas_fsd.g",
                    "analysis_play_khff_gas_fsd.g",
                    "analysis_play_mnjr_gas_fsd.g",
                    "analysis_play_mqna_gas_fsd.g",
                    "analysis_play_mrrt_gas_fsd.g",
                    "analysis_play_ordv_gas_fsd.g",
                    "analysis_play_qusb_gas_fsd.g",
                    "analysis_play_ymsl_gas_fsd.g",
                    "analysis_play_rus_gas_fsd.g",
                    "analysis_play_ubcu_gas_fsd.g",
                    "analysis_play_unza_gas_fsd.g",
                    "analysis_play_wajh_gas_fsd.g",
                    "analysis_play_wsia_gas_fsd.g",
                    "analysis_play_ymsl_gas_fsd.g",
                    })
                <<"atreadascii2tbl input=$< output=$@";
            mf.add("forecast_oil_fsd.t", 
                    {"forecast_oil_fsd.txt",
                    "analysis_play_abab_oil_fsd.g",
                    "analysis_play_abcr_oil_fsd.g",
                    "analysis_play_abdr_oil_fsd.g",
                    "analysis_play_arum_oil_fsd.g",
                    "analysis_play_brqn_oil_fsd.g",
                    "analysis_play_buwb_oil_fsd.g",
                    "analysis_play_bydh_oil_fsd.g",
                    "analysis_play_devo_oil_fsd.g",
                    "analysis_play_dhrm_oil_fsd.g",
                    "analysis_play_fdhl_oil_fsd.g",
                    "analysis_play_ghws_oil_fsd.g",
                    "analysis_play_hdrk_oil_fsd.g",
                    "analysis_play_hdrr_oil_fsd.g",
                    "analysis_play_hith_oil_fsd.g",
                    "analysis_play_hnfr_oil_fsd.g",
                    "analysis_play_jilh_oil_fsd.g",
                    "analysis_play_jubl_oil_fsd.g",
                    "analysis_play_khff_oil_fsd.g",
                    "analysis_play_mnjr_oil_fsd.g",
                    "analysis_play_mqna_oil_fsd.g",
                    "analysis_play_mrrt_oil_fsd.g",
                    "analysis_play_ordv_oil_fsd.g",
                    "analysis_play_qusb_oil_fsd.g",
                    "analysis_play_rus_oil_fsd.g",
                    "analysis_play_shub_oil_fsd.g",
                    "analysis_play_ubcu_oil_fsd.g",
                    "analysis_play_umer_oil_fsd.g",
                    "analysis_play_unza_oil_fsd.g",
                    "analysis_play_wsia_oil_fsd.g",
                    "analysis_play_ymsl_oil_fsd.g",
                    })
                <<"atreadascii2tbl input=$< output=$@";
            mf.add("forecast_pos.t", {
                    "forecast_pos.txt",
                    "analysis_play_abab_del_pos.g",
                    "analysis_play_abab_wc_pos.g",
                    "analysis_play_abcr_del_pos.g",
                    "analysis_play_abcr_wc_pos.g",
                    "analysis_play_abdr_del_pos.g",
                    "analysis_play_abdr_wc_pos.g",
                    "analysis_play_adff_wc_pos.g",
                    "analysis_play_arum_del_pos.g",
                    "analysis_play_arum_wc_pos.g",
                    "analysis_play_brqn_wc_pos.g",
                    "analysis_play_buwb_del_pos.g",
                    "analysis_play_buwb_wc_pos.g",
                    "analysis_play_bydh_wc_pos.g",
                    "analysis_play_devo_del_pos.g",
                    "analysis_play_devo_wc_pos.g",
                    "analysis_play_dhrm_del_pos.g",
                    "analysis_play_dhrm_wc_pos.g",
                    "analysis_play_dmm_wc_pos.g",
                    "analysis_play_fdhl_del_pos.g",
                    "analysis_play_fdhl_wc_pos.g",
                    "analysis_play_ghws_wc_pos.g",
                    "analysis_play_hdrk_del_pos.g",
                    "analysis_play_hdrk_wc_pos.g",
                    "analysis_play_hdrr_del_pos.g",
                    "analysis_play_hdrr_wc_pos.g",
                    "analysis_play_hith_del_pos.g",
                    "analysis_play_hith_wc_pos.g",
                    "analysis_play_hnfr_del_pos.g",
                    "analysis_play_hnfr_wc_pos.g",
                    "analysis_play_jilh_del_pos.g",
                    "analysis_play_jilh_wc_pos.g",
                    "analysis_play_jubl_del_pos.g",
                    "analysis_play_jubl_wc_pos.g",
                    "analysis_play_khff_del_pos.g",
                    "analysis_play_khff_wc_pos.g",
                    "analysis_play_dhrm_del_pos.g",
                    "analysis_play_dhrm_wc_pos.g",
                    "analysis_play_mnjr_del_pos.g",
                    "analysis_play_mnjr_wc_pos.g",
                    "analysis_play_mqna_del_pos.g",
                    "analysis_play_mqna_wc_pos.g",
                    "analysis_play_mrrt_del_pos.g",
                    "analysis_play_mrrt_wc_pos.g",
                    "analysis_play_ordv_del_pos.g",
                    "analysis_play_ordv_wc_pos.g",
                    "analysis_play_qusb_wc_pos.g",
                    "analysis_play_ymsl_wc_pos.g",
                    "analysis_play_rus_del_pos.g",
                    "analysis_play_rus_wc_pos.g",
                    "analysis_play_shub_del_pos.g",
                    "analysis_play_shub_wc_pos.g",
                    "analysis_play_ubcu_del_pos.g",
                    "analysis_play_ubcu_wc_pos.g",
                    "analysis_play_umer_wc_pos.g",
                    "analysis_play_unza_del_pos.g",
                    "analysis_play_unza_wc_pos.g",
                    "analysis_play_wajh_wc_pos.g",
                    "analysis_play_wsia_del_pos.g",
                    "analysis_play_wsia_wc_pos.g",
                    "analysis_play_ymsl_del_pos.g",
                    "analysis_play_ymsl_wc_pos.g",
                    })
                <<"atreadascii2tbl input=$< output=$@";
            mf.add("forecast_pcom.t", "forecast_pcom.txt")
                <<"atreadascii2tbl input=$< output=$@";
        }
    }
    void load_pap_shape_files(){
        mf.add("papgis_sa_onshore.t", "papgis_sa_onshore_simple.shp")
            <<"atshp2itbl input=$< output=tmp.it"
            <<"atitdumptable input=tmp.it index=0-part1 output=$@"
            ;
        mf.add("papgis_sa.t", "sa_boundary_onshore_offshore_aat.shp")
            <<"atshp2itbl input=$< output=tmp.it"
            <<"atlargestpolygon input=tmp.it xcol=x ycol=y output=$@"
            ;

        mf.add("rigaoi.it", "papgis_rigaoi.shp")
            <<"atshp2itbl input=$< output=$@"
            <<BYPRODUCT(itable("rigaoi.it"))
            ;
        mf.add("input_rsed_area.t", "input_rsed_area.shp")
            <<"atshp2tbl input=$< output=$@"
            ;
        mf.add("papgis_rigdays_spatial_clusters.it", "papgis_rigdays_spatial_clusters.shp")
            <<"atshp2itbl input=$< output=$@"
            <<BYPRODUCT(itable("papgis_rigdays_spatial_clusters.it"))
            ;
        mf.add("papgis_area_maturity.it", "papgis_area_maturity.shp")
            <<"atshp2itbl input=$< output=$@"
            <<BYPRODUCT(itable("papgis_area_maturity.it"))
            ;
        {

            strings dep;
            for(auto item: plays){
                string play=item.first;
                string basename="papgis_play_"+play+"_phasemap";
               mf.add(basename+".it", {
                        "pap_play_"+play+"_phasemap.shp",
                        "pap_play_"+play+"_phasemap.cpg",
                        })
                    <<"atshp2itbl input=$< output=$@ cleanup=true"
                    <<"attaddstringcol input="+basename+".lt col=play value="+play
                    <<BYPRODUCT(itable("papgis_play_"+play+"_phasemap.it"))
                    ;
                dep.push_back(basename+".it");
            }
            bool first=true;
            auto & f = mf.add("hcphase_byplay.it", dep);
            for(auto item: plays){
                string play=item.first;
                string basename="papgis_play_"+play+"_phasemap";
                if(first){
                    first=false;
                    f<<"atittee input="+basename+".it output=tmp.it";
                }else{
                    f<<"atitmerge l=tmp.it r="+basename+".it output=tmp.it rename=true";
                }
            }
            f<<"atittee input=tmp.it output=$@";
            f<<BYPRODUCT(itable("hcphase_byplay.it"));
        }
        {
            strings dep;
            for(auto item: drillzones){
               string prefix=item.prefix;
               string basename="papgis_drillzone_"+prefix+"_bop";
               mf.add(basename+".it", item.bop_shp)
                    <<"atshp2itbl input=$< output=$@ cleanup=true"
                    <<"attaddstringcol input="+basename+".lt col=drillzone value=\"DZ-"+to_upper(prefix)+"\""
                    <<BYPRODUCT(itable("papgis_drillzone_"+prefix+"_bop.it"))
                    ;
                dep.push_back(basename+".it");
            }
            bool first=true;
            auto & f = mf.add("drillzone_bop.it", dep);
            for(auto item: drillzones){
                string prefix=item.prefix;
                string basename="papgis_drillzone_"+prefix+"_bop";
                if(first){
                    first=false;
                    f<<"atittee input="+basename+".it output=tmp.it";
                }else{
                    f<<"atitmerge l=tmp.it r="+basename+".it output=tmp.it rename=true";
                }
            }
            f<<"atittee input=tmp.it output=$@";
            f<<BYPRODUCT(itable("drillzone_bop.it"));
        }
    }
    void create_testing_portfolios(){
        //create a test portfolio  of a single prospect
        {
            string test_name = "tsta";
            auto & f = mf.add(test_name+"_portfolio.txt");
            f<<"@echo \""
                " string:play"
                " string:id"
                " string:old_names"
                " string:stage"
                " string:dep"
                " string:maturity"
                " string:prmstype"
                " string:comments"
                " string:hashtags"
                " int:utm"
                " float:x"
                " float:y"
                " string:account_as"
                " string:minbop"
                " string:is_nonhc"
                " string:variables"
                " \" >$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTA-1" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            Portfolio p;
            p.team = test_name;
            p.file = test_name+"_portfolio.txt";
            portfolios.push_back(p);
        }
        //create a test portfolio  of a single prospect
        {
            string test_name = "tstb";
            auto & f = mf.add(test_name+"_portfolio.txt");
            f<<"@echo \""
                " string:play"
                " string:id"
                " string:old_names"
                " string:stage"
                " string:dep"
                " string:maturity"
                " string:prmstype"
                " string:comments"
                " string:hashtags"
                " int:utm"
                " float:x"
                " float:y"
                " string:account_as"
                " string:minbop"
                " string:is_nonhc"
                " string:variables"
                " \" >$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTB-1" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            Portfolio p;
            p.team = test_name;
            p.file = test_name+"_portfolio.txt";
            portfolios.push_back(p);
        }
        //create a test portfolio  of two prospects
        {
            string test_name = "tstc";
            auto & f = mf.add(test_name+"_portfolio.txt");
            f<<"@echo \""
                " string:play"
                " string:id"
                " string:old_names"
                " string:stage"
                " string:dep"
                " string:maturity"
                " string:prmstype"
                " string:comments"
                " string:hashtags"
                " int:utm"
                " float:x"
                " float:y"
                " string:account_as"
                " string:minbop"
                " string:is_nonhc"
                " string:variables"
                " \" >$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTC-1" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTC-2" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            Portfolio p;
            p.team = test_name;
            p.file = test_name+"_portfolio.txt";
            portfolios.push_back(p);
        }
        //create a test portfolio  of two prospects
        {
            int nprospect=20;
            mf.add("tstd_fsd.g")
                //<<"atmath tmp.g=\"d1()*shift*(amp^(n))\"   n=1                            mean=\"(minoil+maxoil)/2\" minoil=200 maxoil=400 shift=\"exp(-2.0*i()*x1()*pi()*(0.5*tau+minoil+n*mean))\" amp=\"0*i()+ifnan(sin(aa)/max(0.0001,aa),1)\" aa=\"x1()*pi()*tau\" tau=\"(maxoil-minoil)\" n1=4000 d1=0.00001 && atifft1 input=tmp.g output=$@";
                <<"atmath expr=\"(x1()>200)-(x1()>400)\" n1=4000 d1=1 output=$@";
            mf.add("tstd_ideal_agr.g")
                <<"atmath tmp.g=\"d1()*shift*(amp^(n))\" n=\""+to_string(nprospect)+"-1\" mean=\"(minoil+maxoil)/2\" minoil=200 maxoil=400 shift=\"exp(-2.0*i()*x1()*pi()*(0.5*tau+minoil+n*mean))\" amp=\"0*i()+ifnan(sin(aa)/max(0.0001,aa),1)\" aa=\"x1()*pi()*tau\" tau=\"(maxoil-minoil)\" n1=4000 d1=0.00001 && atifft1 input=tmp.g output=$@";
            string test_name = "tstd";
            auto & f = mf.add(test_name+"_portfolio.txt", {"tstd_fsd.g"});
            f<<"@echo \""
                " string:play"
                " string:id"
                " string:old_names"
                " string:stage"
                " string:dep"
                " string:maturity"
                " string:prmstype"
                " string:comments"
                " string:hashtags"
                " int:utm"
                " float:x"
                " float:y"
                " string:account_as"
                " string:minbop"
                " string:is_nonhc"
                " string:variables"
                " \" >$@";
            for(int i=0; i<nprospect; i++){
                f<<"@echo \""
                    " abdr" //string:play"
                    " TSTD-"+to_string(i+1)+ //string:id"
                    " NONE"//string:old_names"
                    " Scheduled"//string:stage"
                    " NONE"//string:dep"
                    " IdentifiedDefinit" //string:maturity"
                    " Prospective" //string:prmstype"
                    " ''" //"string:comments"
                    " \\\"\\\\#test\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                    " 39" //int:utm"
                    " 350476 3013490"//float:x" " float:y"
                    " AUTO-HC"
                    " AUTO"
                    " No"
                    " ''"
                    " \" >>$@";
            }
            Portfolio p;
            p.team = test_name;
            p.file = test_name+"_portfolio.txt";
            portfolios.push_back(p);
        }
        //create a test portfolio  of two prospects
        {
            string test_name = "tste";
            mf.add("tste_ideal_agr.g")
                <<"atmath $@=\"if(i1()==0, 0.25)+if((x1()>=100) && (x1()<=200), 0.25/100)+if((x1()>=300) && (x1()<=400), 0.25/100)+if((x1()>400) &&(x1()<=500), (x1()-400)*0.25/100/100)+if((x1()>500) &&(x1()<=600), (600-x1())*0.25/100/100)\" n1=10000 d1=1";
            auto & f = mf.add(test_name+"_portfolio.txt");
            f<<"@echo \""
                " string:play"
                " string:id"
                " string:old_names"
                " string:stage"
                " string:dep"
                " string:maturity"
                " string:prmstype"
                " string:comments"
                " string:hashtags"
                " int:utm"
                " float:x"
                " float:y"
                " string:account_as"
                " string:minbop"
                " string:is_nonhc"
                " string:variables"
                " \" >$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTE-1" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test \\\\#loc1\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            f<<"@echo \""
                " abdr" //string:play"
                " TSTE-2" //string:id"
                " NONE"//string:old_names"
                " Scheduled"//string:stage"
                " NONE"//string:dep"
                " IdentifiedDefinit" //string:maturity"
                " Prospective" //string:prmstype"
                " ''" //"string:comments"
                " \\\"\\\\#test \\\\#loc2\\\\#"+test_name+"\\\\#\\\"" //string:hashtags"
                " 39" //int:utm"
                " 350476 3013490"//float:x" " float:y"
                " AUTO-HC"
                " AUTO"
                " No"
                " ''"
                " \" >>$@";
            Portfolio p;
            p.team = test_name;
            p.file = test_name+"_portfolio.txt";
            portfolios.push_back(p);
        }
        //create a test portfolio from PVAD data
        {
            for(int i=0; i<tstf_ntest; i++){
                string test_name = "tstf"+to_string(i);
                auto & f = mf.add(test_name+"_portfolio.txt", {"historical_wellvolumeadd_withxy.t"});
                f<<"attsqltool tables=\"historical_wellvolumeadd_withxy.t\" sql=\""
                    "select "
                    "play, "
                    "@1.Well||'-"+test_name+"' as id, "
                    "'' as old_names, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.well_class like '\%DEL\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #volume_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.oil as booked_oil, "
                    "@1.gas as booked_gas "
                    "from @1 where use_for_fsd like 'yes' and multiwell_booking like 'no' and abs(random()%100)<20\" output=tmp.t";
                f<<"att2ascii input=tmp.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        {
            for(int i=2005; i<current_year; i++){
                string test_name = "tstg"+to_string(i);
                auto & f = mf.add(test_name+"_portfolio.txt", {"historical_wellvolumeadd_withxy.t", "pvad_play_lookup.t"});
                f<<"attsqltool tables=\"historical_wellvolumeadd_withxy.t pvad_play_lookup.t\" sql=\""
                    "select "
                    "play, "
                    "@1.Well||'-"+test_name+"' as id, "
                    "'' as old_names, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.well_class like '\%DEL\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #volume_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.oil as booked_oil, "
                    "@1.gas as booked_gas "
                    "from @1 where use_for_fsd like 'yes' and year like '"+to_string(i)+"'\" output=tmp.t";
                f<<"att2ascii input=tmp.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        // volume addition by play
        {
            for(auto play:seq_plays){
                string test_name = "tstm_"+play;
                auto & f = mf.add(test_name+"_portfolio.txt", {"historical_wellvolumeadd_withxy.t"});
                f<<"attsqltool tables=\"historical_wellvolumeadd_withxy.t pvad_play_lookup.t\" sql=\""
                    "select "
                    "play, "
                    "@1.Well||'-"+test_name+"' as id, "
                    "'' as old_names, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.well_class like '\%DEL\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #volume_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.oil as booked_oil, "
                    "@1.gas as booked_gas "
                    "from @1 where use_for_fsd like 'yes' and play like '"+play+"'\" output=tmp.t";
                f<<"att2ascii input=tmp.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        //create a test from SPOT PDAs to test POS models
        {
            for(int i=0; i<tstf_ntest; i++){
                string test_name = "tsth"+to_string(i);
                auto & f = mf.add(test_name+"_portfolio.txt", {"spot_pda_simple.t", "spot_play_lookup.t"});
                f<<"attsqltool tables=\"spot_pda_simple.t spot_play_lookup.t\" sql=\""
                    "select "
                    "@2.play as play, "
                    "@1.w_gnr_name||'-"+test_name+"' as id, "
                    "'' as old_names, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.pre_drill_clsf like '\%Delineation\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #pos_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.commercial, "
                    "@1.technical "
                    "from @1 inner join @2 on @1.rsvr_form_cd=@2.rescd where abs(random()%100)<20\" output=tmp.t";
                f<<"att2ascii input=tmp.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        //create a test from SPOT PDAs to test POS models
        {
            for(int i=2013; i<current_year; i++){
                string test_name = "tsti"+to_string(i);
                auto & f = mf.add(test_name+"_portfolio.txt", {"spot_pda_simple.t", "spot_play_lookup.t"});
                f<<"attsqltool tables=\"spot_pda_simple.t spot_play_lookup.t\" sql=\""
                    "select "
                    "@2.play as play, "
                    "@1.w_gnr_name||'-"+test_name+"' as id, "
                    "'' as old_names, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.pre_drill_clsf like '\%Delineation\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #pos_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.commercial, "
                    "@1.technical "
                    "from @1 inner join @2 on @1.rsvr_form_cd=@2.rescd where completion_year like '"+to_string(i)+"'\" output=tmp.t";
                f<<"att2ascii input=tmp.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        //test J: create a test from SPOT PDAs and PVAD's booking to test POS*VOl models
        {
            for(int i=2013; i<current_year; i++){
                string test_name = "tstj"+to_string(i);
                auto & f = mf.add(test_name+"_portfolio.txt", {"spot_pda_simple.t", "spot_play_lookup.t","historical_wellvolumeadd_withxy.t", "pvad_play_lookup.t"});
                f<<"attsqltool tables=\"spot_pda_simple.t historical_wellvolumeadd_withxy.t\" sql=\""
                    "select "
                    "'' as old_names, "
                    "@1.play as play, "
                    "@1.w_gnr_name||'-"+test_name+"' as id, "
                    "'Scheduled' as stage, "
                    "'NONE' as dep, "
                    "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                    "case when @1.pre_drill_clsf like '\%Delineation\%' then 'Delineation' else 'Prospective' end as prmstype, "
                    "'' as comments, "
                    "'#test #pos_test #"+test_name+"#' as hashtags, "
                    "'#test #posvol_test ' as hashtags, "
                    "'#test #posvol_test #"+test_name+"#' as hashtags, "
                    "cast(39 as int) as utm, "
                    "@1.u39x as x, @1.u39y as y, "
                    "@1.commercial as commercial, "
                    "@1.technical as technical, "
                    "(select oil from @2 where use_for_fsd like 'yes' and year like '"+to_string(i)+"' and lower(@2.Well)=lower(@1.w_gnr_name) and @2.play=@1.play) as booked_oil, " 
                    "(select gas from @2 where use_for_fsd like 'yes' and year like '"+to_string(i)+"' and lower(@2.Well)=lower(@1.w_gnr_name) and @2.play=@1.play) as booked_gas " 
                    "from @1 where completion_year like '"+to_string(i)+"' group by play, id\" output=tmp.t";
                f<<"attsqltool tmp.t sql=\"select * from @1 where not (commercial='Yes' and booked_oil=0 and booked_gas=0)\" output=tmp2.t";
                f<<"att2ascii input=tmp2.t output=$@";
                Portfolio p;
                p.team = test_name;
                p.file = test_name+"_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        //testing prediction performance by play
        {
            for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                for(auto play: seq_plays){
                    string test_name = "tstk_"+play+"_"+wcdel;
                    auto & f = mf.add(test_name+"_portfolio.txt", {"spot_pda_simple.t", "spot_play_lookup.t", "historical_wellvolumeadd_withxy.t", "pvad_play_lookup.t"});
                    f<<"attsqltool tables=\"spot_pda_simple.t historical_wellvolumeadd_withxy.t\" sql=\""
                        "select "
                        "@1.play as play, "
                        "@1.w_gnr_name||'-"+test_name+"' as id, "
                        "'' as old_names, "
                        "'Scheduled' as stage, "
                        "'NONE' as dep, "
                        "'IdentifiedDefinit' as maturity, "
                    "'AUTO-HC' as account_as, "
                    "'AUTO' as minbop, "
                    "'No' as is_nonhc, "
                    " '' as variables,"
                        "case when @1.pre_drill_clsf like '\%Delineation\%' then 'Delineation' else 'Prospective' end as prmstype, "
                        "'' as comments, "
                        "'#test #posvol_test #"+test_name+"#' as hashtags, "
                        "cast(39 as int) as utm, "
                        "@1.u39x as x, @1.u39y as y, "
                        "@1.commercial, "
                        "@1.technical, "
                        "(select oil from @2 where use_for_fsd like 'yes' and lower(@2.Well)=lower(@1.w_gnr_name) and @2.play=@2.play) as booked_oil, " 
                        "(select gas from @2 where use_for_fsd like 'yes' and lower(@2.Well)=lower(@1.w_gnr_name) and @2.play=@2.play) as booked_gas " 
                        "from @1 where play like '"+play+"' and prms_class='"+wcdel+"'\" output=tmp.t";
                    f<<"attsqltool tmp.t sql=\"select * from @1 where not (commercial='Yes' and booked_oil=0 and booked_gas=0)\" output=tmp2.t";
                    f<<"att2ascii input=tmp2.t output=$@";
                    Portfolio p;
                    p.team = test_name;
                    p.file = test_name+"_portfolio.txt";
                    portfolios.push_back(p);
                }
            }
        }
        //testing FSD
        {
        }
    }
    void load_team_portfolios(){
        mf.add("guwi")
            <<HELP("launch the Guwi application to edite the local copy of the portfolio")
            <<"ataedplan canvas=curr.canvas portfolio="+portfolio_path+" shared=true";
        mf.on_softclean_retain("curr.canvas");

        if(generate_testing_report){
            create_testing_portfolios();
        }

        //load portfolio from portfolio database
        if(!false){
            mf.add({"xxx_portfolio.txt", "seismic_projects.t"})
                <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
                <<"atdbdump db=ppd.portfolio qry=\""
                "select drill_project.project as id, old_names, x, y,"
                " account_as,"
                " minbop,"
                " cast(case coord when 'UTM36' then 36  when 'UTM37' then 37 when 'UTM38' then 38 when 'UTM39' then 39 when 'UTM40' then 40  else 0 end as int) as utm1,"
                " stage as stage,"
                " drill_target.play_group as play,"
                " maturity as maturity,"
                " prms_class as prmstype,"
#ifdef MATCHSPOT
                " 'NONE' as dep,"
#else
                " depends_on as dep,"
#endif
                " is_nonhc,"
                " drill_target.hashtags as hashtags,"
                " drill_target.variables as variables,"
                " drill_target.comments as comments,"
                " drill_project.comments as project_comments,"
                " drill_project.edited_on as project_editedon"
                " from drill_project"
                //" inner join drill_target on drill_project.project=drill_target.project where drill_project.stage in ('Proposed', 'Scheduled', 'InProgress')\" output=tmp1.t "
                " inner join drill_target on drill_project.project=drill_target.project\" output=tmp1.t "
                <<"attsqltool tmp1.t sql=\"select *, cast(utm1 as int) as utm from @1\" output=tmp2.t"
                <<"attrmcol tmp2.t list=\"utm1\""
                <<"att2ascii input=tmp2.t output=xxx_portfolio.txt"
                <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
                <<"atdbdump db=ppd.portfolio qry=\""
                "select seismic_project.project as project, "
                " stage as stage,"
                " hashtags,"
                " comments"
                " from seismic_project"
                " \" output=seismic_projects.t "
                ;

            mf.add("rm_db_exports")
                <<"rm -f xxx_portfolio.txt seismic_projects.t";

            mf.add("export_from_guwi", {"rm_db_exports", "xxx_portfolio.txt", "seismic_projects.t"})
                <<HELP("export data from the Guwi application file (ppd.portfolio) for qc and simulation")
                ;
            {
                Portfolio p;
                p.team = "xxx";
                p.file = "xxx_portfolio.txt";
                portfolios.push_back(p);
            }
        }
        //include RRAD
        if(include_rrad){
            Portfolio p;
            p.team = "rrad";
            p.file = "rrad_portfolio.txt";
            portfolios.push_back(p);
        }
        {
            mf.add("pap_rrad_translation.t", "pap_rrad_translation.txt")
                <<"atreadascii2tbl input=$< output=$@"
            ;
            mf.add("rrad_portfolio_concepts_table.t", "rrad_portfolio_concepts.txt")
                <<"atreadascii2tbl input=rrad_portfolio_concepts.txt output=$@"
                //<<"attmath tmp.t ndel=0 int=true output=$@"
                ;

            mf.add("rrad_portfolio_concepts.t", {"rrad_portfolio_concepts_table.t", "pap_rrad_translation.t"})
                <<"attsqltool tables=\"rrad_portfolio_concepts_table.t pap_rrad_translation.t\" sql=\"select @1.*, @2.pap_play_group as play from @1 inner join @2 on lower(@1.play_orig)=lower(@2.reservoir)\" output=$@"
                ;
        }
        if(include_rrad){
            strings dep;
            dep.push_back("template_wf_top.tsk");
            dep.push_back("template_wf_convert_shape_to_utm39.tsk");
            dep.push_back("template_wf_bot.tsk");
            dep.push_back("rrad_portfolio_concepts.t");
            dep.push_back("papgis_sa.t");
            auto &f = mf.add({"import_rrad_shapefiles", "rrad_concepts.it"}, dep);
            f<<HELP("Load RRAD shapefiles for play-concept polygons.");
            f<<"cat template_wf_top.tsk >interm_task_import_rrad_shapefiles.tsk";
            f<<"ls Play_Concepts/{0,1,2,3,4,5,6,7,8,9}*/*.shp 2>/dev/null | awk 'BEGIN{print \"file\"}{print \"\\\"\" $$0 \"\\\"\"}'>tmp.txt";
            f<<"atreadascii2tbl input=tmp.txt output=tmp.t";
            f<<"attsqltool tmp.t sql=\"select file, 'rrad_'||lower(replace(substr(file, instr(replace(file,'Play_Concepts/','              '),'/')+1),' ','_')) as target from @1\" output=tmp2.t";
            f<<"attforeach input_tbl=tmp2.t template_file=template_wf_convert_shape_to_utm39.tsk XXX=file rrad_concept_001.shp=target >>interm_task_import_rrad_shapefiles.tsk"; 
            f<<"cat template_wf_bot.tsk >>interm_task_import_rrad_shapefiles.tsk";
            f<<"atdialog \"Can you run interm_task_import_rrad_shapefiles.tsk from petrosys surface modeling. Click 'OK' when finished!\"";
            f<<"atloadshapefiles input=rrad_portfolio_concepts.t keycol=concept output=rrad_concepts.it";
            f<<"attinpolygon input=rrad_concepts.t xexpr=x yexpr=y newcol=inkingdom ply=papgis_sa.t plyx=x plyy=y";
            f<<"atutm2wgs84 input=rrad_concepts.t x=x y=y zone=39 newx=wgs84x newy=wgs84y";
            f<<"attforeach input_tbl=tmp2.t template=\"rm FILE;\" FILE=target | sh -v"; 
            f<<BYPRODUCT(itable("rrad_concepts.it"));
            f<<TEMP("interm_task_import_rrad_shapefiles.tsk");

            mf.add("rrad_concepts.shp", "rrad_concepts.it")
                <<"atit2shpfile input=$< output=$@ x=wgs84x y=wgs84y"
                ;
            gis_targets.push_back("rrad_concepts.shp");
        }
        //aed portfolio without RRAD's
        {
            strings dep;
            for(auto portfolio: portfolios){
                if(!(portfolio.team=="rrad"))
                    dep.push_back(portfolio.team+"_portfolio.t");
            }
            auto & f = mf.add({"aed_portfolio_without_rrad.t"}, dep);
            bool first=true;
            for(auto portfolio: portfolios){
                if(!(portfolio.team=="rrad")){
                    if(first){
                        f<<"attsqltool "+portfolio.team+"_portfolio.t sql=\"select id, stage, lower(play) as play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags, variables, comments, project_comments, project_editedon from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"attsqltool tables='"+portfolio.team+"_portfolio.t tmp.t' sql=\"select * from (select id, stage, lower(play) as play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags, variables, comments, project_comments, project_editedon from @1) union select * from @2\" output=tmp.t";
                    }
                }
            }
            f<<"attstrsort input=tmp.t key=play output=$@";
            ;
        }
        // Load input data from each team
        for(auto portfolio: portfolios){
            if(portfolio.team=="rrad"){
                {
                    strings dep;
                    for(auto portfolio: portfolios){
                        if(!(portfolio.team=="rrad"))
                            dep.push_back(portfolio.team+"_portfolio.t");
                    }
                    dep.push_back("rrad_concepts.it");
                    auto & f = mf.add({"rrad_portfolio.t"}, dep);
                    bool first=true;
                    for(auto portfolio: portfolios){
                        if(!(portfolio.team=="rrad")){
                            if(first){
                                f<<"attsqltool "+portfolio.team+"_portfolio.t sql=\"select id, stage, lower(play) as play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags, variables, comments from @1\" output=tmp.t";
                                first=false;
                            }else{
                                f<<"attsqltool tables='"+portfolio.team+"_portfolio.t tmp.t' sql=\"select * from (select id, stage, lower(play) as play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags,variables,  comments from @1) union select * from @2\" output=tmp.t";
                            }
                        }
                    }
                    f<<"attstrsort input=tmp.t key=play";
                    f<<"atindextable input=tmp.t key=play";
                    f<<"atconcepts2features input=rrad_concepts.it output=tmp2.t fixed=tmp.it fixedx=u39x fixedy=u39y";
                    f<<"atnudge pnt=tmp2.t pntx=x pnty=y newx=u39x newy=u39y ply=papgis_sa.t plyx=x plyy=y output=tmp3.t";
                    f<<"atutm2wgs84 input=tmp3.t output=$@ x=u39x y=u39y zone=39 newx=wgs84x newy=wgs84y";
                    f<<"attaddcol $@ old_names=\"''\""
                    ;
                }
                mf.add("rrad_portfolio.shp", "rrad_portfolio.t")
                    <<"att2shpfile input=$< output=$@ x=wgs84x y=wgs84y"
                    ;
            }else{
                auto & f = mf.add(portfolio.team+"_portfolio.t", portfolio.file);
                    f<<"atreadascii2tbl input=$< output=tmp.t";
                    f<<"atunifyutm input=tmp.t zone=utm x=x y=y newx=u39x newy=u39y output=$@" ;
            }
        }
        //combining portfolios from all teams 
        {
            strings dep;
            for(auto portfolio: portfolios){
                dep.push_back(portfolio.team+"_portfolio.t");
            }
            {
                auto & f = mf.add("aed_portfolio_allteams.t",dep);
                bool first=true;
                for(auto portfolio: portfolios){
                    if(first){
                        f<<"attsqltool "+portfolio.team+"_portfolio.t sql=\"select id, old_names, stage, play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags, variables, comments from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"attsqltool tables='"+portfolio.team+"_portfolio.t tmp.t' sql=\"select * from (select id, old_names, stage, play, dep, account_as, minbop, is_nonhc, maturity, prmstype, u39x, u39y, hashtags, variables, comments from @1) union select * from @2\" output=tmp.t";
                    }
                }
                //extract hc phase attributes
                f<<"attsqltool tmp.t sql=\"select * from @1\" output=$@";

                mf.add("aed_portfolio.t", {"aed_portfolio_allteams.t", "hcphase_byplay.it", "rigaoi.it", "papgis_area_maturity.it"})
                    <<"attsqltool tables=\"aed_portfolio_allteams.t\" sql=\"select * from @1\" output=tmp.t"
                    <<"attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=hcphase_byplay.it plyx=x plyy=y newcol=hc_phase plyattribute=phase matchcol=play match_attrib=play"
                    <<"attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=rigaoi.it plyx=x plyy=y plyattribute=rigaoi newcol=rigaoi output=tmp2.t"
                    <<"attinpolygons input=tmp2.t xexpr=u39x yexpr=u39y ply=papgis_area_maturity.it plyx=x plyy=y plyattribute=AMaturity newcol=area_maturity"
                    <<"attsqltool tmp2.t sql=\"select id from @1 where hc_phase like 'GAS' group by id\" output=tmp_gas_wells.t"
                    <<"attsqltool tables=\"tmp2.t tmp_gas_wells.t\" sql=\"select *, exists(select id from @2 where id=@1.id) as well_has_gas from @1\" output=tmp3.t"
                    <<"attsqltool tables=\"tmp3.t \" sql=\"select *, case when account_as='AUTO-HC'  and well_has_gas=1 then 'GAS' when account_as='AUTO-HC' and well_has_gas=0 then 'OIL' else account_as end as project_phase from @1\" output=$@"
                    ;

                mf.add("aed_portfolio.shp", "aed_portfolio.t")
                    <<"att2shpfile input=aed_portfolio.t output=aed_portfolio.shp x=u39x y=u39y"
                    ;
                gis_targets.push_back("aed_portfolio.shp");
                mf.add("aed_portfolio_overlaps.t", "aed_portfolio.t")
                    <<"attsqltool aed_portfolio.t sql=\"select a.play a, b.play b, count(*) as cnt from @1 a, @1 b where a.id=b.id and a.play<>b.play group by a.play, b.play\" output=$@";
            }
            //portfolio of all the considered plays; filtering out plays that PAP decide to ignore (due to lack of enough opr or for other reasons!).
            mf.add("all_portfolio.t", {"aed_portfolio.t"})
                <<"attsqltool $< headers=false sql=\"select * from @1 where lower(play) in ("+play_list+") and hashtags not like '\%#tst\%'"
                //" and stage in ('Proposed', 'Scheduled', 'InProgress') "
                "\" output=$@"
                ;
            mf.add("test_portfolio.t", "aed_portfolio.t")
                <<"attsqltool $< headers=false sql=\"select * from @1 where lower(play) in ("+play_list+") and hashtags like '\%#TST\%'\" output=$@";
        }
    }
    void define_aois(){
        {
            RigAOI aoi;
            aoi.title="North Arabia";
            aoi.prefix="narb";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="North Ghawar";
            aoi.prefix="ngwr";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Arabian Gulf";
            aoi.prefix="gulf";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Greater Ghawar";
            aoi.prefix="ggwr";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Central Arabia";
            aoi.prefix="carb";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="West Rub AlKhali";
            aoi.prefix="wrak";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="East Rub AlKhali";
            aoi.prefix="erak";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Onshore Red Sea";
            aoi.prefix="onrs";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Shallow-Water Red Sea";
            aoi.prefix="swrs";
            aois.push_back(aoi);
        }
        {
            RigAOI aoi;
            aoi.title="Deep-Water Red Sea";
            aoi.prefix="dwrs";
            aois.push_back(aoi);
        }
        {
            auto & f = mf.add("list_rigaois");
            f<<HELP("to list all the rig areas");
            for(auto item: aois){
                std::stringstream ss;
                ss<<std::setw(40)<<item.prefix<<":  "<<item.title;
                f<<"@echo \""+ss.str()+"\"";
            }
        }
    }
    void define_drill_zones(){
        {
            DrillZone dz;
            dz.title="Shallow";
            dz.prefix="shallow";
            dz.bop_shp="papgis_drillzone_shallow_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Cretaceous";
            dz.prefix="cretaceous";
            dz.bop_shp="papgis_drillzone_cretaceous_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Jurassic";
            dz.prefix="jurassic";
            dz.bop_shp="papgis_drillzone_jurassic_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Jilh and Khuff";
            dz.prefix="jilhkhff";
            dz.bop_shp="papgis_drillzone_jilhkhff_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Pre-Khuff"; // Unayzah, Devonian, and Silurian
            dz.prefix="prekhff";
            dz.bop_shp="papgis_drillzone_prekhff_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Ordovician Section";
            dz.prefix="sarh";
            dz.bop_shp="papgis_drillzone_sarh_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Basement";
            dz.prefix="bsmt";
            dz.bop_shp="papgis_drillzone_sarh_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Post-Salt Red Sea";
            dz.prefix="postsalt";
            dz.bop_shp="papgis_drillzone_rsed_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Pre-Salt Red Sea";
            dz.prefix="presalt";
            dz.bop_shp="papgis_drillzone_rsed_bop.shp";
            drillzones.push_back(dz);
        }
        {
            DrillZone dz;
            dz.title="Mansyah Salt";
            dz.prefix="mnsysalt";
            dz.bop_shp="papgis_drillzone_rsed_bop.shp";
            drillzones.push_back(dz);
        }
    }
    void define_ventures(){
        {
            Venture v;
            v.title="East-RAK Gas";
            v.prefix="erakgas";
            v.filter="rigaoi like 'erak' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="East-RAK Oil";
            v.prefix="erakoil";
            v.filter="rigaoi like 'erak' and project_phase like 'oil' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="West-RAK Gas";
            v.prefix="wrakgas";
            v.filter="rigaoi like 'wrak' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Ghawar Area Gas";
            v.prefix="gggas";
            v.filter="rigaoi like 'ggwr' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Ghawar Area Oil";
            v.prefix="ggoil";
            v.filter="rigaoi like 'ggwr' and project_phase like 'oil' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Centeral Arabia Gas";
            v.prefix="catgas";
            v.filter="rigaoi like 'carb' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Centeral Arabia Oil";
            v.prefix="catoil";
            v.filter="rigaoi like 'carb' and project_phase like 'oil' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="North Arabia Gas";
            v.prefix="nagas";
            v.filter="rigaoi like 'narb' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="North Ghawar Gas";
            v.prefix="nggas";
            v.filter="rigaoi like 'ngwr' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="North Ghawar Oil";
            v.prefix="ngoil";
            v.filter="rigaoi like 'ngwr' and project_phase like 'oil' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Arabian Gulf Gas";
            v.prefix="gulfgas";
            v.filter="rigaoi like 'gulf' and project_phase like 'gas' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Arabian Gulf Oil";
            v.prefix="gulfoil";
            v.filter="rigaoi like 'gulf' and project_phase like 'oil' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 0.5;
            ventures.push_back(v);
        }
        {
            Venture v;
            v.title="Red-Sea Deep Water Gas";
            v.prefix="srdwgas";
            v.filter="rigaoi like 'dwrs' and (lower(maturity) like '%POM' or lower(maturity) in ('assessed', 'scheduled')) ";
            v.max_nrig = 1;
            ventures.push_back(v);
        }
        if(0){
            Venture v;
            v.title="Unidentified";
            v.prefix="uniden";
            v.filter="hc_phase like 'gas' and (lower(maturity) like 'unidentified') ";
            v.max_nrig = 2;
            ventures.push_back(v);
        }
        {
            auto & f = mf.add("list_ventures");
            f<<HELP("to list all the ventures");
            for(auto item: ventures){
                std::stringstream ss;
                ss<<std::setw(40)<<item.prefix<<":  "<<item.title<<" = "<<item.filter;
                f<<"@echo \""+ss.str()+"\"";
            }
        }
    }
    void extract_point_gis_attributes(string tbl, Rule & f, bool with_plays){
        mf.add_source(f, "papgis_sa_onshore.t");
        mf.add_source(f, "input_rsed_area.t");
        mf.add_source(f, "papgis_area_maturity.t");
        mf.add_source(f, "papgis_rigdays_spatial_clusters.it");
        mf.add_source(f, "drillzone_bop.it");
        f<<"attinpolygon    input="+tbl+" xexpr=u39x yexpr=u39y ply=papgis_sa_onshore.t                   plyx=x plyy=y newcol=onshore_or_offshore inlabel=onshore outlabel=offshore";
        f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=rigaoi.it                              plyx=x plyy=y plyattribute=rigaoi newcol=rigaoi";
        f<<"attinpolygon    input="+tbl+" xexpr=u39x yexpr=u39y ply=input_rsed_area.t                     plyx=x plyy=y newcol=aed_department inlabel=RSED outlabel=EAED";
        f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=papgis_rigdays_spatial_clusters.it     plyx=x plyy=y plyattribute=cluster newcol=wellcost_cluster";
        //f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=papgis_area_maturity.it                plyx=x plyy=y plyattribute=AMaturity newcol=area_maturity";
        //f<<"attinpolygons input="+tbl+" xexpr=u39x yexpr=u39y ply=drillzone_bop.it plyx=x plyy=y newcol=bop_pressure plyattribute=pressure matchcol=drillzone match_attrib=drillzone";
        if(with_plays){
            for(auto item: plays){
                string play=item.first;
                mf.add_source(f, "play_"+play+"_spot_active_portfolio.it");
                mf.add_source(f, "play_"+play+"_booked.it");
                f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_desc newcol="+play+"_segment overwrite=true";
                f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=play_"+play+"_booked.it         plyx=x plyy=y plyattribute=rsvr_name newcol="+play+"_booked_rsvr overwrite=true";
                f<<"attinpolygons  input="+tbl+" xexpr=u39x yexpr=u39y ply=play_"+play+"_booked.it         plyx=x plyy=y plyattribute=type newcol="+play+"_booked_type overwrite=true";
            }
        }
    }
    void rigdays_analysis(){
        //areas

        mf.add("historical_rigdays2.t", "eppr_spud_to_rigrelease.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@"
            <<"atunifyutm input=$@ zone=utm_zn newzone=39 x=sv_xutm_cord y=sv_yutm_cord newx=u39x newy=u39y"
            ;
        mf.on_softclean_retain("historical_rigdays2.t");

        mf.add("historical_rigdays2.shp", "historical_rigdays2.t")
            <<"att2shpfile input=$< x=u39x y=u39y output=$@"
            ;

        mf.add("historical_rigdays.t", "eppr_bhlist.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@"
            <<"atunifyutm input=$@ zone=utm_zn newzone=39 x=sv_xutm_cord y=sv_yutm_cord newx=u39x newy=u39y"
                ;
        mf.on_softclean_retain("historical_rigdays.t");
        mf.add("tops_eppr.t")
                <<"atoracle2tbl login=~/atlogin sql=\"select WB_EXPL_NAME, ST_LONG_CD, W_ST_SS_DPTH, W_ST_DMRK_DPTH from wellbore_stt_vw where W_ST_VER_CD='AAP' and st_long_cd not in ('TDPT', 'SRFD') and st_long_cd not like '\%0\%' and st_long_cd not like '\%1\%' and st_long_cd not like '\%2\%' and st_long_cd not like'\%3\%' and st_long_cd not like'\%4\%' and st_long_cd not like'\%5\%' and st_long_cd not like'\%6\%' and st_long_cd not like'\%7\%' and st_long_cd not like'\%8\%' and st_long_cd not like'\%9\%' and length(st_long_cd)=4\" output=$@"
                <<"attsqltool $@ sql=\"select * from @1 where rowid in (select max(rowid) from @1 group by WB_EXPL_NAME, ST_LONG_CD) order by wb_expl_name, st_long_cd\" output=$@"
                <<"attmath input=$@ expr=\"w_st_dmrk_dpth\" newcol=\"md\""
                <<"attmath input=$@ expr='w_st_ss_dpth' newcol='z'"
                ;
        mf.on_softclean_retain("tops_eppr.t");
    }
    void wellcost_analysis(){
        //preparing the data for distributions
        mf.add("well_xy.t", "eppr_well_xy.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@"
            <<"atunifyutm input=$@ zone=utm_zn newzone=39 x=sv_xutm_cord y=sv_yutm_cord newx=u39x newy=u39y"
                ;
        mf.on_softclean_retain("well_xy.t");
        mf.add("well_utmxy.t", "eppr_well_utmxy.sql")
            <<"atoracle2tbl login=~/atlogin sqlfile=$< output=$@"
            <<"atunifyutm input=$@ zone=utm_zn newzone=39 x=sv_xutm_cord y=sv_yutm_cord newx=u39x newy=u39y"
                ;
        mf.on_softclean_retain("well_utmxy.t");
        mf.add("pap_formation_ranker.t", "pap_formation_ranker.txt")
            <<"atreadascii2tbl input=$< output=tmp.t"
            <<"attsqltool tmp.t sql=\"select *, rowid as rankid from @1\" output=$@"
            ;
        mf.add("well_xy_drillzone.t",{"well_xy.t", "pap_formation_ranker.t"})
            <<"attsqltool \"well_xy.t pap_formation_ranker.t\" sql=\"select @1.*, @2.* from @1 inner join @2 on @1.cur_formation=@2.top\" output=tmp.t"
            <<"attsqltool tmp.t sql=\"select w_gnr_name, u39x, u39y, cast(max(rigdays) as float) as maxrigdays, cast(max(rankid) as int) as maxrankid, top, formation, drill_zone, area from @1 group by w_gnr_name\" output=well_xy_drillzone.t"
            ;
        mf.add("well_xy_drillzone_bop.t", {"well_xy_drillzone.t","rig_bop.t"})
            <<"attsqltool tables=\"well_xy_drillzone.t rig_bop.t\" sql=\"" _cont
            " select @1.*, rig_shrt_name, max_bop_press as bop_press from @1 inner join @2 on @1.w_gnr_name like replace(@2.w_expl_name, '_', '-')\" output=$@"
            ;
        mf.add("list_drill_zones", "pap_formation_ranker.t")
            <<HELP("list drill zones to be used for cluster analysis and forecasting.")
            <<"@attsqltool $< sql=\"select drill_zone as rank from @1 group by drill_zone order by min(rankid)\"" ;

        {
            auto & f = mf.add("historical_wellcost_with_attrib.t", {"well_xy_drillzone.t", "historical_wellcost.t"});
                f<<"attsqltool tables=\"historical_wellcost.t well_xy_drillzone.t\" sql=\"select @1.*, @2.* from @1 inner join @2 on @1.well=@2.w_gnr_name\" output=tmp.t";
                f<<"attsqltool tmp.t sql=\"select *, cast(sap_cost/1000000 as float) as mcost from @1\" output=tmp.t";
                extract_point_gis_attributes("tmp.t", f, false);
                f<<"attcategory2num input=tmp.t columns=\"wellcost_cluster drill_zone\" overwrite=true numcol=ncat catcol=cat output=$@" ;
                f<<"attstrsort input=$@ key=cat";
        }
        {
            auto & f = mf.add("drillcost_with_attrib.t", {"well_xy_drillzone.t", "drill_cost.t"});
                f<<"attsqltool tables=\"drill_cost.t well_xy_drillzone.t\" sql=\"select @1.*, @2.* from @1 inner join @2 on @1.w_gnr_name=@2.w_gnr_name\" output=tmp.t";
                f<<"attsqltool tmp.t sql=\"select *, cast(cost_mm as float) as mcost from @1\" output=tmp.t";
                extract_point_gis_attributes("tmp.t", f, false);
                f<<"attcategory2num input=tmp.t columns=\"wellcost_cluster drill_zone\" overwrite=true numcol=ncat catcol=cat output=$@" ;
                f<<"attstrsort input=$@ key=cat";
        }
        mf.add("historical_wellcost_with_attrib.shp", "historical_wellcost_with_attrib.t")
            <<"att2shpfile input=$< x=u39x y=u39y output=$@";
        mf.add("drillcost_with_attrib.shp", "drillcost_with_attrib.t")
            <<"att2shpfile input=$< x=u39x y=u39y output=$@";
        {
            auto & f = mf.add("historical_rigdays_with_attrib.t", {"well_xy_drillzone.t"});
                f<<"attsqltool tables=\"well_xy_drillzone.t\" sql=\"select * from @1\" output=tmp.t";
                extract_point_gis_attributes("tmp.t", f, false);
                f<<"attcategory2num input=tmp.t columns=\"wellcost_cluster drill_zone\" overwrite=true numcol=ncat catcol=cat output=$@" ;
                f<<"attstrsort input=$@ key=cat";
        }
        //now building pdfs
        vector<std::pair<string,string>> cluster_cond_list={
            //{file prefix, sql-condition for filtering out the data}
            {"narabia_deep", "wellcost_cluster='NAED_NORTH_ARABIA' and drill_zone in ('DZ-SARH', 'DZ-BSMT')"},
            {"nghwr_shallow", "wellcost_cluster='NAED_NORTH_GHAWAR'   and drill_zone in ('DZ-SHALLOW', 'DZ-CRETACEOUS')"},
            {"nghwr_jurassic", "wellcost_cluster='NAED_NORTH_GHAWAR'   and drill_zone in ('DZ-JURASSIC')"},
            {"nghwr_deep", "wellcost_cluster='NAED_NORTH_GHAWAR'   and drill_zone in ('DZ-PREKHFF', 'DZ-SARH', 'DZ-BSMT')"},
            {"gulf_prekhff", "wellcost_cluster='NAED_ARABIAN_GULF'    and drill_zone in ('DZ-PREKHFF','DZ-SARH', 'DZ-BSMT')"},
            {"gulf_khff", "wellcost_cluster='NAED_ARABIAN_GULF'    and drill_zone in ('DZ-JILHKHFF')"},
            {"gulf_midsection", "wellcost_cluster='NAED_ARABIAN_GULF'    and drill_zone in ('DZ-CRETACEOUS', 'DZ-JURASSIC')"},
            {"cahwth_all", "wellcost_cluster='CA_HWTH_TREND' and drill_zone in ('DZ-PREKHFF','DZ-SARH')"},
            {"canyym_all", "wellcost_cluster='CA_NYYM_TREND' and drill_zone in ('DZ-PREKHFF','DZ-SARH')"},
            {"cawrak_pcamp", "wellcost_cluster in ('CA_NYYM_TREND', 'CA_NYYM_TREND', 'SAED_WRAK')  and drill_zone in ('DZ-BSMT')"},
            {"twqi_jurassic", "wellcost_cluster in ('GW_TWQI') and drill_zone in ('DZ-JURASSIC')"},
            {"sghwr_cretaceous", "wellcost_cluster in ('GE_GHAWAR_JAFURAH', 'GE_NIBN_JAWB_QTR_ARCH', 'GW_SAT_FIELD') and drill_zone in ('DZ-CRETACEOUS')"},
            {"sghwr_jurassic", "wellcost_cluster in ('GE_GHAWAR_JAFURAH', 'GE_NIBN_JAWB_QTR_ARCH', 'GW_SAT_FIELD') and drill_zone in ('DZ-JURASSIC')"},
            {"eghwr_prekhff", "wellcost_cluster in ('GE_GHAWAR_JAFURAH', 'GE_NIBN_JAWB_QTR_ARCH', 'GE_UTMN_HWYH_HIGH_PRESSURE') and drill_zone in ('DZ-JILHKHFF', 'DZ-PREKHFF')"},
            {"wgsat_prekhff", "wellcost_cluster in ('GW_SAT_FIELD') and drill_zone in ('DZ-PREKHFF', 'DZ-SARH')"},
            {"erak_cretaceous", "wellcost_cluster in ('SAED_ERAK') and drill_zone in ('DZ-CRETACEOUS')"},
            {"erak_jurassic", "wellcost_cluster in ('SAED_ERAK') and drill_zone in ('DZ-JURASSIC')"},
            {"erak_deep", "wellcost_cluster in ('SAED_ERAK') and drill_zone in ('DZ-JILHKHFF', 'DZ-SARH')"},
            {"wrak_cretaceous", "wellcost_cluster in ('SAED_WRAK') and drill_zone in ('DZ-CRETACEOUS')"},
            {"wrak_jurassic", "wellcost_cluster in ('SAED_WRAK') and drill_zone in ('DZ-JURASSIC')"},
            {"wrak_triassic", "wellcost_cluster in ('SAED_WRAK') and drill_zone in ('DZ-JILHKHFF')"},
            {"wrak_prekhff", "wellcost_cluster in ('SAED_WRAK') and drill_zone in ('DZ-PREKHFF', 'DZ-SARH')"},
            {"rsed_onshore", "wellcost_cluster in ('RSED_ONSHORE')"},
            {"rsed_shalloww", "wellcost_cluster in ('RSED_SHALLOW_WATER')"},
            {"rsed_deepw_postsalt", "wellcost_cluster in ('RSED_DEEP_WATER') and drill_zone in ('DZ-POSTSALT')"},
            {"rsed_deepw_presalt", "wellcost_cluster in ('RSED_DEEP_WATER') and drill_zone in ('DZ-PRESALT')"},
        };
        {
            auto &f = mf.add("list_well_clusters");
            f<<HELP("list geo-spatial well clusters (with TD zonation) used for forecasting cost and rigdays.");
            for(auto &item: cluster_cond_list){
                f<<"@echo '"+item.first+"   :   "+item.second+"'";
            }

        }
        mf.add("analysis_wellcluster_{XXXX}_cost")<<HELP("model the drill-cost distributon for the {XXXX} well cluster.");
        for(auto c: cluster_cond_list){
            auto prefix = c.first;
            auto cond = c.second;
            //mf.add("analysis_wellcluster_"+prefix+"_cost", {"historical_wellcost_with_attrib.t"})
            mf.add("analysis_wellcluster_"+prefix+"_cost", {"drillcost_with_attrib.t"})
                //<<HELP("model drill-cost distributon for the "+prefix+" geospatial cluster.")
                <<"attsqltool $< sql=\"select * from @1 where "+cond+"\" output=tmp.t"
                <<"atdrawdist data=tmp.t datacol=mcost maxval=200 proj=analysis_wellcluster_"+prefix+"_cost.proj"
                <<TARGET({
                        "analysis_wellcluster_"+prefix+"_cost.proj", 
                        "analysis_wellcluster_"+prefix+"_cost.g", 
                        "analysis_wellcluster_"+prefix+"_cost.t"})
                ;
        }

        mf.add("analysis_wellcluster_{XXXX}_rigdays")<<HELP("model the rigdays distributon for the {XXXX} well cluster.");
        for(auto c: cluster_cond_list){
            auto prefix = c.first;
            auto cond = c.second;
            mf.add("analysis_wellcluster_"+prefix+"_rigdays", {"historical_rigdays_with_attrib.t"})
                //<<HELP("model drill-rig-days distributon for the "+prefix+" geospatial cluster.")
                <<"attsqltool $< sql=\"select * from @1 where "+cond+"\" output=tmp.t"
                <<"atdrawdist data=tmp.t datacol=maxrigdays maxval=600 proj=analysis_wellcluster_"+prefix+"_rigdays.proj"
                <<TARGET({
                        "analysis_wellcluster_"+prefix+"_rigdays.proj",
                        "analysis_wellcluster_"+prefix+"_rigdays.g",
                        "analysis_wellcluster_"+prefix+"_rigdays.t"
                        })
                ;
        }

    }
    void portfolio2program(string input_portfolio, string output_projects, string output_targets, string output_dep, bool nodel, string zero_filter="", strings carry_over_cols={}){
            auto & f =mf.add(
                    {output_projects, output_targets}, 
                    {input_portfolio, "seismic_projects.t", "forecast_rigdays.t", "forecast_cost.t", "aed_spot_rel.t",
                    "forecast_pos.t", "forecast_pcom.t",
                    "papgis_sa_onshore.t", "papgis_rigdays_spatial_clusters.it", "rigaoi.it", "ordered_plays.t", "ordered_stages.t", 
                    "play_drillzone_lookup.t", "drillzone_bop.it",
                    "conv3d.it", "active_seismic3d.it", "proposed3d.it",
                    "ordered_maturity.t", "papgis_area_maturity.it",
                    "forecast_gas_wad.t", "forecast_oil_wad.t", 
                    "forecast_gas_fsd.t", "forecast_oil_fsd.t", 
                    "fsd_gas_factorized.g", "fsd_oil_factorized.g", 
                    "wad_posterior_gas.g", "wad_posterior_oil.g",
                    "pos.g", "nwell.t"
                    });
            string carry_over_option="";
            if(carry_over_cols.size()>0){
                carry_over_option+="carry_over_columns=\""+to_string(carry_over_cols)+"\"";
            }
            if(nodel){
                f<<"attaddcol input="+input_portfolio+" ndel=\"cast (0 as int)\" output=tmp_portfolio.t ";
            }else{
                f<<"attsqltool tables=\""+input_portfolio+" nwell.t\" sql=\"select @1.*, cast((select nwell-1 from @2 where @2.id=@1.id) as int) as ndel  from @1\"  output=tmp_portfolio.t";
            }
            f<<"atportfolio2program portfolio=tmp_portfolio.t seismic=seismic_projects.t output=tmp.t "+carry_over_option;
            //adding sequence to the table to see which target is the deepest in the project
            f<<"attsqltool tables=\"tmp.t ordered_plays.t ordered_stages.t aed_spot_rel.t ordered_maturity.t\" sql=\"select @1.*,"
                " (select seq from @2 where lower(@1.play)=lower(@2.play)) as play_seq,"
                " (select seq from @3 where lower(@1.stage)=lower(@3.stage)) as stage_seq,"
                " (select seq from @5 where lower(@1.maturity)=lower(@5.maturity)) as maturity_seq,"
                " (select rowid-1 from @4 where lower(@1.prospect)=lower(@4.id) and lower(@1.play)=lower(@4.play)) as prospect_rowid"
                " from @1\" output=tmp_tgts.t";
            string carry_over_cols_sql="";
            if(carry_over_cols.size()>0){
                for(auto col:carry_over_cols){
                    carry_over_cols_sql+=col+", ";
                }
            }
            f<<"attsqltool tmp_tgts.t sql=\"select project, u39x, u39y, account_as, project_phase, minbop, is_nonhc, type,"+carry_over_cols_sql+
                " max(play_seq) as deepest_play_seq,"
                " min(stage_seq) as min_stage,"
                " min(maturity_seq) as min_maturity,"
                " hashtags,"
                " variables,"
                " cast(max(mincampaign) as int) as mincampaign from @1 group by project\" output=tmp_projs.t";
            f<<"attsqltool tables=\"tmp_projs.t ordered_plays.t ordered_stages.t ordered_maturity.t\" sql=\"select @1.*,"
                " (select play from @2 where @1.deepest_play_seq=@2.seq) as deepest_play,"
                " (select stage from @3 where @1.min_stage=@3.seq) as stage,"
                " (select maturity from @4 where @1.min_maturity=@4.seq) as maturity"
                " from @1\" output=tmp_projs.t";
            f<<"attsqltool tables=\"tmp_projs.t play_drillzone_lookup.t\" sql=\"select @1.*, @2.drillzone from @1 inner join @2 on @1.deepest_play=@2.play\" output=tmp_projs.t";
            //f<<"attsqltool tmp_tgts.t sql=\"select * from @1 where type='Drilling'\" output=tmp_tgts.t";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=papgis_rigdays_spatial_clusters.it plyx=x plyy=y plyattribute=cluster newcol=wellcost_cluster";
            f<<"attinpolygon  input=tmp_projs.t xexpr=u39x yexpr=u39y ply=papgis_sa_onshore.t plyx=x plyy=y newcol=on_or_offshore inlabel=onshore outlabel=offshore";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=rigaoi.it plyx=x plyy=y plyattribute=rigaoi newcol=rigaoi";
            f<<"attinpolygons input=tmp_tgts.t xexpr=u39x yexpr=u39y ply=rigaoi.it plyx=x plyy=y plyattribute=rigaoi newcol=rigaoi";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=papgis_area_maturity.it plyx=x plyy=y plyattribute=AMaturity newcol=area_maturity";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=drillzone_bop.it plyx=x plyy=y newcol=bop_pressure plyattribute=pressure matchcol=drillzone match_attrib=drillzone";
            f<<"attaddcol input=tmp_projs.t bop_req=\"minbop\" where=\"minbop!='AUTO'\"";
            f<<"attupdate input=tmp_projs.t bop_req=\"case when bop_pressure=5000 then '5K' when bop_pressure=10000 then '10K' else '15K' end\" where=\"minbop='AUTO'\"";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=conv3d.it plyx=u39x plyy=u39y newcol=survey";
            f<<"attaddcol input=tmp_projs.t in_completed3d=\"'Yes'\" where=\"survey!=''\"";
            f<<"attupdate input=tmp_projs.t in_completed3d=\"'No'\" where=\"survey=''\"";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=active_seismic3d.it plyx=u39x plyy=u39y newcol=act_survey";
            f<<"attaddcol input=tmp_projs.t in_active3d=\"'Yes'\" where=\"act_survey!=''\"";
            f<<"attupdate input=tmp_projs.t in_active3d=\"'No'\" where=\"act_survey=''\"";
            f<<"attinpolygons input=tmp_projs.t xexpr=u39x yexpr=u39y ply=proposed3d.it plyx=x plyy=y newcol=prop_survey";
            f<<"attaddcol input=tmp_projs.t in_proposed3d=\"'Yes'\" where=\"prop_survey!=''\"";
            f<<"attupdate input=tmp_projs.t in_proposed3d=\"'No'\" where=\"prop_survey=''\"";
            f<<"attrmcol input=tmp_projs.t list=\"act_survey survey prop_survey\"";
            f<<"attsetwhere cond=forecast_rigdays.t condcol=cond valuecol=title newcol='rigdays_cond' input=tmp_projs.t output=tmp_projs.t";
            f<<"attsetwhere cond=forecast_rigdays.t condcol=cond valuecol=rigdays_dist input=tmp_projs.t output=tmp_projs.t";
            f<<"attsetwhere cond=forecast_cost.t condcol=cond valuecol=title newcol='cost_cond' input=tmp_projs.t output=tmp_projs.t";
            f<<"attsetwhere cond=forecast_cost.t condcol=cond valuecol=cost_dist input=tmp_projs.t output=tmp_projs.t";
            f<<"attupdate input=tmp_projs.t rigdays_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"type!='Drilling'\"";
            f<<"attupdate input=tmp_projs.t cost_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"type!='Drilling'\"";
            //QCs
            f<<"attsqltool tmp_projs.t sql=\"select * from @1 where rigdays_dist='' or cost_dist=''\" output=tmp.t";
            f<<"attassert_nrow input=tmp.t max=0";
            //get well volume after  posterior calculations
            f<<"attsetwhere cond=forecast_gas_wad.t condcol=cond valuecol=title newcol='well_gasvolume_cond' input=tmp_tgts.t output=tmp_tgts.t";
            f<<"attsetwhere cond=forecast_oil_wad.t condcol=cond valuecol=title newcol='well_oilvolume_cond' input=tmp_tgts.t output=tmp_tgts.t";
            f<<"attaddcol input=tmp_tgts.t well_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"wad_posterior_gas.g\\\", \\\"i2\\\":'||prospect_rowid||'}'\"";
            f<<"attaddcol input=tmp_tgts.t well_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"wad_posterior_oil.g\\\", \\\"i2\\\":'||prospect_rowid||'}'\"";
            f<<"attupdate input=tmp_tgts.t well_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"type!='Drilling'\"";
            f<<"attupdate input=tmp_tgts.t well_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"type!='Drilling'\"";

            //get volumes from factorized fsd 
            f<<"attaddcol input=tmp_tgts.t upside_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\"";
            f<<"attaddcol input=tmp_tgts.t upside_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\"";
            f<<"attupdate input=tmp_tgts.t upside_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"fsd_gas_factorized.g\\\", \\\"i2\\\":'||prospect_rowid||'}'\" where=\"type='Drilling'\"";
            f<<"attupdate input=tmp_tgts.t upside_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"fsd_oil_factorized.g\\\", \\\"i2\\\":'||prospect_rowid||'}'\" where=\"type='Drilling'\"";
            f<<"attsetwhere cond=forecast_pos.t condcol=cond valuecol=title newcol='pos_cond' input=tmp_tgts.t output=tmp_tgts.t";
            f<<"attsetwhere cond=forecast_pos.t condcol=cond valuecol=pos_dist input=tmp_tgts.t output=tmp_tgts.t";
            f<<"attupdate input=tmp_tgts.t pos_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":1}'\" where=\"type!='Drilling'\"";
            f<<"attupdate input=tmp_tgts.t pos_dist=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"pos.g\\\", \\\"i2\\\":'||prospect_rowid||'}'\" where=\"is_first_well='Yes'\"";
            f<<"attsetwhere cond=forecast_pcom.t condcol=cond valuecol=value newcol='pcom' input=tmp_tgts.t output=tmp_tgts.t";
            //QCs
            f<<"attsqltool tmp_tgts.t sql=\"select * from @1 where well_gas_bscf_dist='' or well_oil_mmbo_dist='' or upside_gas_bscf_dist='' or upside_oil_mmbo_dist='' or pos_dist='' or pcom=0\" output=tmp.t";
            f<<"attassert_nrow input=tmp.t max=0";
            if(!(zero_filter=="")){
                //in this case set costs and volumes to zeros and model the pos only;
                //f<<"attupdate tmp_projs.t cost_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";
                //f<<"attupdate tmp_projs.t rigdays_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";
                //f<<"attupdate tmp_tgts.t well_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";
                //f<<"attupdate tmp_tgts.t well_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";
                //f<<"attupdate tmp_tgts.t upside_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";
                //f<<"attupdate tmp_tgts.t upside_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\"rigaoi<>'"+rigaoi+"'\"";

                f<<"attupdate tmp_projs.t cost_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
                f<<"attupdate tmp_projs.t rigdays_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
                f<<"attupdate tmp_tgts.t well_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
                f<<"attupdate tmp_tgts.t well_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
                f<<"attupdate tmp_tgts.t upside_gas_bscf_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
                f<<"attupdate tmp_tgts.t upside_oil_mmbo_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":0}'\" where=\""+zero_filter+"\"";
            }
            f<<"atttee input=tmp_tgts.t output="+output_targets;
            f<<"atttee input=tmp_projs.t output="+output_projects;
            ;
            mf.add(output_dep, output_targets)
                <<"attsqltool $< sql=\"select project, dep, play from @1 where dep<>'NONE' group by project, dep, play\" output=$@";
    }
    void add_mcrun(string input_prefix, string output_prefix, int nitr, bool nodel, string runparams="", string zero_filter="", strings carry_over_columns={}){
            portfolio2program(input_prefix+"_portfolio.t", input_prefix+"_program_projects.t", input_prefix+"_program_targets.t", input_prefix+"_program_dep.t", nodel, zero_filter, carry_over_columns);
            //generating mcrun input distributions
            mf.add(input_prefix+"_program_project_costs.g", input_prefix+"_program_projects.t")
                //<<"atgendistarray input=$< jsoncol=cost_dist o1=0 d1=10 n1=1000 output=$@" ;
                <<"atgendistarray input=$< jsoncol=cost_dist o1=0 d1=4 n1=100 output=tmp.g" 
                <<"atrescaleaxis input=tmp.g s1="+cost_ff+" output=$@"
                ;
            mf.hidden_nodes.insert(input_prefix+"_program_project_costs.g");

            mf.add(input_prefix+"_program_project_rigyears.g", input_prefix+"_program_projects.t")
                    //<<"atgendistarray input=$< jsoncol=rigdays_dist o1=0 d1=14.6 n1=100 output=tmp.g" 
                    //<<"atgendistarray input=$< jsoncol=rigdays_dist o1=0 d1=7.3 n1=100 output=tmp.g" 
                    <<"atgendistarray input=$< jsoncol=rigdays_dist o1=0 d1=10 n1=100 output=tmp.g" 
                    <<"atrescaleaxis input=tmp.g s1=0.0027397260273973 output=$@"
                        ;
            mf.hidden_nodes.insert(input_prefix+"_program_project_rigyears.g");
            
            mf.add(input_prefix+"_program_target_wellgasvol.g", input_prefix+"_program_targets.t")
                <<"atgendistarray input=$< jsoncol=well_gas_bscf_dist o1=0 d1=20 n1=100 output=$@" ;
            mf.hidden_nodes.insert(input_prefix+"_program_target_wellgasvol.g");

            mf.add(input_prefix+"_program_target_welloilvol.g", input_prefix+"_program_targets.t")
                <<"atgendistarray input=$< jsoncol=well_oil_mmbo_dist o1=0 d1=12 n1=100 output=$@" ;
            mf.hidden_nodes.insert(input_prefix+"_program_target_welloilvol.g");

            mf.add(input_prefix+"_program_target_upsidegasvol.g", input_prefix+"_program_targets.t")
                <<"atgendistarray input=$< jsoncol=upside_gas_bscf_dist o1=0 d1=30 n1=100 output=$@" ;
            mf.hidden_nodes.insert(input_prefix+"_program_target_upsidegasvol.g");

            mf.add(input_prefix+"_program_target_upsideoilvol.g", input_prefix+"_program_targets.t")
                <<"atgendistarray input=$< jsoncol=upside_oil_mmbo_dist o1=0 d1=15 n1=100 output=$@" ;
            mf.hidden_nodes.insert(input_prefix+"_program_target_upsideoilvol.g");

            mf.add(input_prefix+"_program_target_pos.g", input_prefix+"_program_targets.t")
                <<"atgendistarray input=$< jsoncol=pos_dist o1=0 d1=0.005 n1=202 output=$@" ;
            mf.hidden_nodes.insert(input_prefix+"_program_target_pos.g");

            precompute_list.push_back( input_prefix+"_program_projects.t"); 
            precompute_list.push_back( input_prefix+"_program_targets.t");
            precompute_list.push_back( input_prefix+"_program_dep.t");
            precompute_list.push_back( input_prefix+"_program_project_costs.g");
            precompute_list.push_back( input_prefix+"_program_project_rigyears.g");
            precompute_list.push_back( input_prefix+"_program_target_wellgasvol.g");
            precompute_list.push_back( input_prefix+"_program_target_welloilvol.g");
            precompute_list.push_back( input_prefix+"_program_target_upsidegasvol.g");
            precompute_list.push_back( input_prefix+"_program_target_upsideoilvol.g");
            precompute_list.push_back( input_prefix+"_program_target_pos.g");
            precompute_tables.push_back( input_prefix+"_program_projects.t"); 
            precompute_tables.push_back( input_prefix+"_program_targets.t");
            precompute_tables.push_back( input_prefix+"_program_dep.t");

            StringList tgts = {
                    output_prefix+"_results.p",
                    output_prefix+"_nprospect_dist.g",
                    output_prefix+"_ncomp_dist.g",
                    output_prefix+"_wellsuccessrate_dist.g",
                    output_prefix+"_nwellsuccess_dist.g",
                    output_prefix+"_ntargetsuccess_dist.g",
                    output_prefix+"_ntargettest_dist.g",
                    output_prefix+"_targetsuccessrate_dist.g",
                    output_prefix+"_nseismic_dist.g",
                    output_prefix+"_nstudy_dist.g",
                    output_prefix+"_identified_welloil_dist.g",
                    output_prefix+"_identified_wellgas_dist.g",
                    output_prefix+"_identified_wellboe_dist.g",
                    output_prefix+"_unidentified_welloil_dist.g",
                    output_prefix+"_unidentified_wellgas_dist.g",
                    output_prefix+"_unidentified_wellboe_dist.g",
                    output_prefix+"_assessed_welloil_dist.g",
                    output_prefix+"_assessed_wellgas_dist.g",
                    output_prefix+"_assessed_wellboe_dist.g",
                    output_prefix+"_discovered_welloil_dist.g",
                    output_prefix+"_discovered_wellgas_dist.g",
                    output_prefix+"_discovered_wellboe_dist.g",
                    output_prefix+"_ytf_welloil_dist.g",
                    output_prefix+"_ytf_wellgas_dist.g",
                    output_prefix+"_ytf_wellboe_dist.g",
                    output_prefix+"_identified_upsideoil_dist.g",
                    output_prefix+"_identified_upsidegas_dist.g",
                    output_prefix+"_identified_upsideboe_dist.g",
                    output_prefix+"_unidentified_upsideoil_dist.g",
                    output_prefix+"_unidentified_upsidegas_dist.g",
                    output_prefix+"_unidentified_upsideboe_dist.g",
                    output_prefix+"_assessed_upsideoil_dist.g",
                    output_prefix+"_assessed_upsidegas_dist.g",
                    output_prefix+"_assessed_upsideboe_dist.g",
                    output_prefix+"_discovered_upsideoil_dist.g",
                    output_prefix+"_discovered_upsidegas_dist.g",
                    output_prefix+"_discovered_upsideboe_dist.g",
                    output_prefix+"_ytf_upsideoil_dist.g",
                    output_prefix+"_ytf_upsidegas_dist.g",
                    output_prefix+"_ytf_upsideboe_dist.g",
                    output_prefix+"_findingcost_dist.g",
                    output_prefix+"_cost_dist.g",
                    output_prefix+"_rig_oilfinding_rate_dist.g",
                    output_prefix+"_rig_gasfinding_rate_dist.g",
                    output_prefix+"_rig_boefinding_rate_dist.g",
                    output_prefix+"_rig_years_dist.g",
                    output_prefix+"_onshore_oil_rig_years_dist.g",
                    output_prefix+"_onshore_gas_rig_years_dist.g",
                    output_prefix+"_offshore_oil_rig_years_dist.g",
                    output_prefix+"_offshore_gas_rig_years_dist.g",
                    output_prefix+"_nonhc_rig_years_dist.g",
                    output_prefix+"_campaign_identified_welloilvol.g",
                    output_prefix+"_campaign_identified_wellgasvol.g",
                    output_prefix+"_campaign_identified_wellboevol.g",
                    output_prefix+"_campaign_unidentified_welloilvol.g",
                    output_prefix+"_campaign_unidentified_wellgasvol.g",
                    output_prefix+"_campaign_unidentified_wellboevol.g",
                    output_prefix+"_campaign_assessed_welloilvol.g",
                    output_prefix+"_campaign_assessed_wellgasvol.g",
                    output_prefix+"_campaign_assessed_wellboevol.g",
                    output_prefix+"_campaign_ytf_welloilvol.g",
                    output_prefix+"_campaign_ytf_wellgasvol.g",
                    output_prefix+"_campaign_ytf_wellboevol.g",
                    output_prefix+"_campaign_discovered_welloilvol.g",
                    output_prefix+"_campaign_discovered_wellgasvol.g",
                    output_prefix+"_campaign_discovered_wellboevol.g",
                    output_prefix+"_campaign_identified_upsideoilvol.g",
                    output_prefix+"_campaign_identified_upsidegasvol.g",
                    output_prefix+"_campaign_identified_upsideboevol.g",
                    output_prefix+"_campaign_unidentified_upsideoilvol.g",
                    output_prefix+"_campaign_unidentified_upsidegasvol.g",
                    output_prefix+"_campaign_unidentified_upsideboevol.g",
                    output_prefix+"_campaign_assessed_upsideoilvol.g",
                    output_prefix+"_campaign_assessed_upsidegasvol.g",
                    output_prefix+"_campaign_assessed_upsideboevol.g",
                    output_prefix+"_campaign_ytf_upsideoilvol.g",
                    output_prefix+"_campaign_ytf_upsidegasvol.g",
                    output_prefix+"_campaign_ytf_upsideboevol.g",
                    output_prefix+"_campaign_discovered_upsideoilvol.g",
                    output_prefix+"_campaign_discovered_upsidegasvol.g",
                    output_prefix+"_campaign_discovered_upsideboevol.g",
                    output_prefix+"_campaign_findingcost.g",
                    output_prefix+"_campaign_cost.g",
                    output_prefix+"_campaign_rig_oilfinding_rate.g",
                    output_prefix+"_campaign_rig_gasfinding_rate.g",
                    output_prefix+"_campaign_rig_boefinding_rate.g",
                    output_prefix+"_campaign_rig_years.g",
                    output_prefix+"_campaign_onshore_oil_rig_years.g",
                    output_prefix+"_campaign_onshore_gas_rig_years.g",
                    output_prefix+"_campaign_offshore_oil_rig_years.g",
                    output_prefix+"_campaign_offshore_gas_rig_years.g",
                    output_prefix+"_campaign_nonhc_rig_years.g",
                    output_prefix+"_campaign_ncompletion.g",
                    output_prefix+"_rig_level.it",
            };
            mf.add(tgts, {
                    input_prefix+"_program_projects.t", 
                    input_prefix+"_program_targets.t", 
                    input_prefix+"_program_dep.t",
                    input_prefix+"_program_project_costs.g", 
                    input_prefix+"_program_project_rigyears.g",
                    input_prefix+"_program_target_wellgasvol.g", 
                    input_prefix+"_program_target_welloilvol.g", 
                    input_prefix+"_program_target_upsidegasvol.g", 
                    input_prefix+"_program_target_upsideoilvol.g", 
                    input_prefix+"_program_target_pos.g", 
                    })
                <<"atprogram_eval " _cont
                " projects="+input_prefix+"_program_projects.t " _cont
                " targets="+input_prefix+"_program_targets.t " _cont
                " deps="+input_prefix+"_program_dep.t " _cont
                " remove_first=false" _cont
                " maxnrig=10000" //number of rigs
                " project_cost="+input_prefix+"_program_project_costs.g" _cont
                " project_rigyears="+input_prefix+"_program_project_rigyears.g" _cont
                " target_wellgasvol="+input_prefix+"_program_target_wellgasvol.g" _cont
                " target_welloilvol="+input_prefix+"_program_target_welloilvol.g" _cont
                " target_upsidegasvol="+input_prefix+"_program_target_upsidegasvol.g" _cont
                " target_upsideoilvol="+input_prefix+"_program_target_upsideoilvol.g" _cont
                " target_pos="+input_prefix+"_program_target_pos.g" _cont
                " oil_rf="+oil_rf+"" _cont
                " gas_rf="+gas_rf+"" _cont
                //outputs
                " output="+output_prefix+"_results.p" _cont
                " nprospect_dist="+output_prefix+"_nprospect_dist.g" _cont
                " ncomple_dist="+output_prefix+"_ncomp_dist.g" _cont
                " wellsuccessrate_dist="+output_prefix+"_wellsuccessrate_dist.g" _cont
                " nwellsuccess_dist="+output_prefix+"_nwellsuccess_dist.g" _cont
                " ntargetsuccess_dist="+output_prefix+"_ntargetsuccess_dist.g" _cont
                " ntargettest_dist="+output_prefix+"_ntargettest_dist.g" _cont
                " targetsuccessrate_dist="+output_prefix+"_targetsuccessrate_dist.g" _cont
                " nseismic_dist="+output_prefix+"_nseismic_dist.g" _cont
                " nstudy_dist="+output_prefix+"_nstudy_dist.g" _cont
                " identified_welloil_dist="+output_prefix+"_identified_welloil_dist.g" _cont
                " identified_wellgas_dist="+output_prefix+"_identified_wellgas_dist.g" _cont
                " identified_wellboe_dist="+output_prefix+"_identified_wellboe_dist.g" _cont
                " unidentified_welloil_dist="+output_prefix+"_unidentified_welloil_dist.g" _cont
                " unidentified_wellgas_dist="+output_prefix+"_unidentified_wellgas_dist.g" _cont
                " unidentified_wellboe_dist="+output_prefix+"_unidentified_wellboe_dist.g" _cont
                " assessed_welloil_dist="+output_prefix+"_assessed_welloil_dist.g" _cont
                " assessed_wellgas_dist="+output_prefix+"_assessed_wellgas_dist.g" _cont
                " assessed_wellboe_dist="+output_prefix+"_assessed_wellboe_dist.g" _cont
                " discovered_welloil_dist="+output_prefix+"_discovered_welloil_dist.g" _cont
                " discovered_wellgas_dist="+output_prefix+"_discovered_wellgas_dist.g" _cont
                " discovered_wellboe_dist="+output_prefix+"_discovered_wellboe_dist.g" _cont
                " ytf_welloil_dist="+output_prefix+"_ytf_welloil_dist.g" _cont
                " ytf_wellgas_dist="+output_prefix+"_ytf_wellgas_dist.g" _cont
                " ytf_wellboe_dist="+output_prefix+"_ytf_wellboe_dist.g" _cont
                " identified_upsideoil_dist="+output_prefix+"_identified_upsideoil_dist.g" _cont
                " identified_upsidegas_dist="+output_prefix+"_identified_upsidegas_dist.g" _cont
                " identified_upsideboe_dist="+output_prefix+"_identified_upsideboe_dist.g" _cont
                " unidentified_upsideoil_dist="+output_prefix+"_unidentified_upsideoil_dist.g" _cont
                " unidentified_upsidegas_dist="+output_prefix+"_unidentified_upsidegas_dist.g" _cont
                " unidentified_upsideboe_dist="+output_prefix+"_unidentified_upsideboe_dist.g" _cont
                " assessed_upsideoil_dist="+output_prefix+"_assessed_upsideoil_dist.g" _cont
                " assessed_upsidegas_dist="+output_prefix+"_assessed_upsidegas_dist.g" _cont
                " assessed_upsideboe_dist="+output_prefix+"_assessed_upsideboe_dist.g" _cont
                " discovered_upsideoil_dist="+output_prefix+"_discovered_upsideoil_dist.g" _cont
                " discovered_upsidegas_dist="+output_prefix+"_discovered_upsidegas_dist.g" _cont
                " discovered_upsideboe_dist="+output_prefix+"_discovered_upsideboe_dist.g" _cont
                " ytf_upsideoil_dist="+output_prefix+"_ytf_upsideoil_dist.g" _cont
                " ytf_upsidegas_dist="+output_prefix+"_ytf_upsidegas_dist.g" _cont
                " ytf_upsideboe_dist="+output_prefix+"_ytf_upsideboe_dist.g" _cont
                " findingcost_dist="+output_prefix+"_findingcost_dist.g" _cont
                " cost_dist="+output_prefix+"_cost_dist.g" _cont
                " rig_oilfinding_rate_dist="+output_prefix+"_rig_oilfinding_rate_dist.g" _cont
                " rig_gasfinding_rate_dist="+output_prefix+"_rig_gasfinding_rate_dist.g" _cont
                " rig_boefinding_rate_dist="+output_prefix+"_rig_boefinding_rate_dist.g" _cont
                " rig_years_dist="+output_prefix+"_rig_years_dist.g" _cont
                " onshore_oil_rig_years_dist="+output_prefix+"_onshore_oil_rig_years_dist.g" _cont
                " onshore_gas_rig_years_dist="+output_prefix+"_onshore_gas_rig_years_dist.g" _cont
                " offshore_oil_rig_years_dist="+output_prefix+"_offshore_oil_rig_years_dist.g" _cont
                " offshore_gas_rig_years_dist="+output_prefix+"_offshore_gas_rig_years_dist.g" _cont
                " nonhc_rig_years_dist="+output_prefix+"_nonhc_rig_years_dist.g" _cont
                " campaign_identified_welloilvol="+output_prefix+"_campaign_identified_welloilvol.g" _cont
                " campaign_identified_wellgasvol="+output_prefix+"_campaign_identified_wellgasvol.g" _cont
                " campaign_identified_wellboevol="+output_prefix+"_campaign_identified_wellboevol.g" _cont
                " campaign_unidentified_welloilvol="+output_prefix+"_campaign_unidentified_welloilvol.g" _cont
                " campaign_unidentified_wellgasvol="+output_prefix+"_campaign_unidentified_wellgasvol.g" _cont
                " campaign_unidentified_wellboevol="+output_prefix+"_campaign_unidentified_wellboevol.g" _cont
                " campaign_assessed_welloilvol="+output_prefix+"_campaign_assessed_welloilvol.g" _cont
                " campaign_assessed_wellgasvol="+output_prefix+"_campaign_assessed_wellgasvol.g" _cont
                " campaign_assessed_wellboevol="+output_prefix+"_campaign_assessed_wellboevol.g" _cont
                " campaign_ytf_welloilvol="+output_prefix+"_campaign_ytf_welloilvol.g" _cont
                " campaign_ytf_wellgasvol="+output_prefix+"_campaign_ytf_wellgasvol.g" _cont
                " campaign_ytf_wellboevol="+output_prefix+"_campaign_ytf_wellboevol.g" _cont
                " campaign_discovered_welloilvol="+output_prefix+"_campaign_discovered_welloilvol.g" _cont
                " campaign_discovered_wellgasvol="+output_prefix+"_campaign_discovered_wellgasvol.g" _cont
                " campaign_discovered_wellboevol="+output_prefix+"_campaign_discovered_wellboevol.g" _cont
                " campaign_identified_upsideoilvol="+output_prefix+"_campaign_identified_upsideoilvol.g" _cont
                " campaign_identified_upsidegasvol="+output_prefix+"_campaign_identified_upsidegasvol.g" _cont
                " campaign_identified_upsideboevol="+output_prefix+"_campaign_identified_upsideboevol.g" _cont
                " campaign_unidentified_upsideoilvol="+output_prefix+"_campaign_unidentified_upsideoilvol.g" _cont
                " campaign_unidentified_upsidegasvol="+output_prefix+"_campaign_unidentified_upsidegasvol.g" _cont
                " campaign_unidentified_upsideboevol="+output_prefix+"_campaign_unidentified_upsideboevol.g" _cont
                " campaign_assessed_upsideoilvol="+output_prefix+"_campaign_assessed_upsideoilvol.g" _cont
                " campaign_assessed_upsidegasvol="+output_prefix+"_campaign_assessed_upsidegasvol.g" _cont
                " campaign_assessed_upsideboevol="+output_prefix+"_campaign_assessed_upsideboevol.g" _cont
                " campaign_ytf_upsideoilvol="+output_prefix+"_campaign_ytf_upsideoilvol.g" _cont
                " campaign_ytf_upsidegasvol="+output_prefix+"_campaign_ytf_upsidegasvol.g" _cont
                " campaign_ytf_upsideboevol="+output_prefix+"_campaign_ytf_upsideboevol.g" _cont
                " campaign_discovered_upsideoilvol="+output_prefix+"_campaign_discovered_upsideoilvol.g" _cont
                " campaign_discovered_upsidegasvol="+output_prefix+"_campaign_discovered_upsidegasvol.g" _cont
                " campaign_discovered_upsideboevol="+output_prefix+"_campaign_discovered_upsideboevol.g" _cont
                " campaign_findingcost="+output_prefix+"_campaign_findingcost.g" _cont
                " campaign_cost="+output_prefix+"_campaign_cost.g" _cont
                " campaign_rig_oilfinding_rate="+output_prefix+"_campaign_rig_oilfinding_rate.g" _cont
                " campaign_rig_gasfinding_rate="+output_prefix+"_campaign_rig_gasfinding_rate.g" _cont
                " campaign_rig_boefinding_rate="+output_prefix+"_campaign_rig_boefinding_rate.g" _cont
                " campaign_rig_years="+output_prefix+"_campaign_rig_years.g" _cont
                " campaign_onshore_oil_rig_years="+output_prefix+"_campaign_onshore_oil_rig_years.g" _cont
                " campaign_onshore_gas_rig_years="+output_prefix+"_campaign_onshore_gas_rig_years.g" _cont
                " campaign_offshore_oil_rig_years="+output_prefix+"_campaign_offshore_oil_rig_years.g" _cont
                " campaign_offshore_gas_rig_years="+output_prefix+"_campaign_offshore_gas_rig_years.g" _cont
                " campaign_nonhc_rig_years="+output_prefix+"_campaign_nonhc_rig_years.g" _cont
                " campaign_ncompletion="+output_prefix+"_campaign_ncompletion.g" _cont
                " riglevel="+output_prefix+"_rig_level.it" _cont
                " ns="+to_string(nitr)+
                " trial="+(trial_run?"true":"false")+
                ""+runparams+""
                <<BYPRODUCT(itable(output_prefix+"_rig_level.it"));
                ;
                //hide targets from build graph plot
                for(auto tgt:tgts){
                    mf.hidden_nodes.insert(tgt);
                }
                mf.add(output_prefix+"_stat_sum.dat", { 
                            output_prefix+"_nprospect_dist.g",
                            output_prefix+"_ncomp_dist.g",
                            output_prefix+"_nwellsuccess_dist.g",
                            output_prefix+"_ntargetsuccess_dist.g",
                            output_prefix+"_targetsuccessrate_dist.g",
                            output_prefix+"_wellsuccessrate_dist.g",
                            output_prefix+"_ntargettest_dist.g",
                            output_prefix+"_identified_welloil_dist.g",
                            output_prefix+"_identified_wellgas_dist.g",
                            output_prefix+"_identified_wellboe_dist.g",
                            output_prefix+"_unidentified_welloil_dist.g",
                            output_prefix+"_unidentified_wellgas_dist.g",
                            output_prefix+"_unidentified_wellboe_dist.g",
                            output_prefix+"_assessed_welloil_dist.g",
                            output_prefix+"_assessed_wellgas_dist.g",
                            output_prefix+"_assessed_wellboe_dist.g",
                            output_prefix+"_discovered_welloil_dist.g",
                            output_prefix+"_discovered_wellgas_dist.g",
                            output_prefix+"_discovered_wellboe_dist.g",
                            output_prefix+"_ytf_welloil_dist.g",
                            output_prefix+"_ytf_wellgas_dist.g",
                            output_prefix+"_ytf_wellboe_dist.g",
                            output_prefix+"_identified_upsideoil_dist.g",
                            output_prefix+"_identified_upsidegas_dist.g",
                            output_prefix+"_identified_upsideboe_dist.g",
                            output_prefix+"_unidentified_upsideoil_dist.g",
                            output_prefix+"_unidentified_upsidegas_dist.g",
                            output_prefix+"_unidentified_upsideboe_dist.g",
                            output_prefix+"_assessed_upsideoil_dist.g",
                            output_prefix+"_assessed_upsidegas_dist.g",
                            output_prefix+"_assessed_upsideboe_dist.g",
                            output_prefix+"_discovered_upsideoil_dist.g",
                            output_prefix+"_discovered_upsidegas_dist.g",
                            output_prefix+"_discovered_upsideboe_dist.g",
                            output_prefix+"_ytf_upsideoil_dist.g",
                            output_prefix+"_ytf_upsidegas_dist.g",
                            output_prefix+"_ytf_upsideboe_dist.g",
                            output_prefix+"_findingcost_dist.g",
                            output_prefix+"_cost_dist.g",
                            output_prefix+"_rig_oilfinding_rate_dist.g",
                            output_prefix+"_rig_gasfinding_rate_dist.g",
                            output_prefix+"_rig_boefinding_rate_dist.g",
                            output_prefix+"_rig_years_dist.g",
                        })
                    <<"rm -f $@"
                    <<"echo \"title='"+input_prefix+"' \" >>$@"
                    <<package_stats(output_prefix+"_nprospect_dist.g", "nprospect",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ncomp_dist.g", "ncomp",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_nwellsuccess_dist.g", "nwellsuccess",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ntargetsuccess_dist.g", "ntargetsuccess",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_targetsuccessrate_dist.g", "targetsuccessrate",  "$@", "\%.2f")
                    <<package_stats(output_prefix+"_wellsuccessrate_dist.g", "wellsuccessrate",  "$@", "\%.2f")
                    <<package_stats(output_prefix+"_nseismic_dist.g", "nseismic",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_nstudy_dist.g", "nstudy",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ntargettest_dist.g", "ntargettest",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_welloil_dist.g", "identified_welloil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_wellgas_dist.g", "identified_wellgas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_wellboe_dist.g", "identified_wellboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_welloil_dist.g", "unidentified_welloil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_wellgas_dist.g", "unidentified_wellgas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_wellboe_dist.g", "unidentified_wellboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_welloil_dist.g", "assessed_welloil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_wellgas_dist.g", "assessed_wellgas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_wellboe_dist.g", "assessed_wellboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_welloil_dist.g", "discovered_welloil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_wellgas_dist.g", "discovered_wellgas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_wellboe_dist.g", "discovered_wellboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_welloil_dist.g", "ytf_welloil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_wellgas_dist.g", "ytf_wellgas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_wellboe_dist.g", "ytf_wellboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_upsideoil_dist.g", "identified_upsideoil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_upsidegas_dist.g", "identified_upsidegas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_identified_upsideboe_dist.g", "identified_upsideboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_upsideoil_dist.g", "unidentified_upsideoil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_upsidegas_dist.g", "unidentified_upsidegas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_unidentified_upsideboe_dist.g", "unidentified_upsideboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_upsideoil_dist.g", "assessed_upsideoil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_upsidegas_dist.g", "assessed_upsidegas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_assessed_upsideboe_dist.g", "assessed_upsideboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_upsideoil_dist.g", "discovered_upsideoil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_upsidegas_dist.g", "discovered_upsidegas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_discovered_upsideboe_dist.g", "discovered_upsideboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_upsideoil_dist.g", "ytf_upsideoil",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_upsidegas_dist.g", "ytf_upsidegas",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_ytf_upsideboe_dist.g", "ytf_upsideboe",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_findingcost_dist.g", "findingcost",  "$@")
                    <<package_stats(output_prefix+"_cost_dist.g", "cost",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_rig_oilfinding_rate_dist.g", "rig_oilfinding_rate",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_rig_gasfinding_rate_dist.g", "rig_gasfinding_rate",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_rig_boefinding_rate_dist.g", "rig_boefinding_rate",  "$@", "\%.0f")
                    <<package_stats(output_prefix+"_rig_years_dist.g", "rig_years",  "$@")
                    ;
                //number of completions figure
                mf.add(output_prefix+"_nprospect_dist.pdf", output_prefix+"_nprospect_dist.g")
                    <<"atgraph "+output_prefix+"_nprospect_dist.g show_yaxis=false grid=false title='Prospect-Count Distribution' xlabel='Number of Prospects' ylabel='' max1=100 saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_nprospect_dist.pdf");

                //number of completions figure
                mf.add(output_prefix+"_ncomp_dist.pdf", output_prefix+"_ncomp_dist.g")
                    <<"atgraph "+output_prefix+"_ncomp_dist.g show_yaxis=false grid=false min1=0 max1=50 autoexpand=true wheight=400 wwidth=1200 title='Well-Count Distribution' xlabel='Number of Completions' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_ncomp_dist.pdf");

                //costs
                mf.add(output_prefix+"_cost_dist.pdf", output_prefix+"_cost_dist.g")
                    <<"atgraph "+output_prefix+"_cost_dist.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Cost Distribution' xlabel='Cost [$$MM]' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_cost_dist.pdf");

                //rig years
                mf.add(output_prefix+"_rig_years_dist.pdf", output_prefix+"_rig_years_dist.g")
                    <<"atgraph "+output_prefix+"_rig_years_dist.g show_yaxis=false max1=10 autoexpand=true grid=false wheight=400 wwidth=1200 title='Rig-Years Distribution' xlabel='Rig-Years' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_rig_years_dist.pdf");

                //finding cost
                mf.add(output_prefix+"_findingcost_dist.pdf", output_prefix+"_findingcost_dist.g")
                    <<"atgraph "+output_prefix+"_findingcost_dist.g show_yaxis=false grid=false min1=0 max1=2 autoexpand=true wheight=400 wwidth=1200 title='Baseline Finding-Cost Distribution' xlabel='Finding Cost [$$/BOE]' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_findingcost_dist.pdf");

                //rig finding rate
                mf.add(output_prefix+"_rig_gasfinding_rate_dist.pdf", output_prefix+"_rig_level.it")
                    <<"atgraph "+output_prefix+"_rig_gasfinding_rate_dist.g show_yaxis=false grid=false max1=200 autoexpand=true wheight=400 wwidth=1200 title='Baseline Gas-Finding Rate Distribution'  xlabel='Finding Rate [BSCF/Rig-Year]' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_rig_gasfinding_rate_dist.pdf");

                //rig finding rate
                mf.add(output_prefix+"_rig_oilfinding_rate_dist.pdf", output_prefix+"_rig_level.it")
                    <<"atgraph "+output_prefix+"_rig_oilfinding_rate_dist.g show_yaxis=false grid=false max1=200 autoexpand=true wheight=400 wwidth=1200 title='Baseline Oil-Finding Rate Distribution'  xlabel='Finding Rate [MMBO/Rig-Year]' saveto=$@" ;
                mf.hidden_nodes.insert(output_prefix+"_rig_oilfinding_rate_dist.pdf");

                //ceating plots
                std::map<string, string> unit_map={{"oil", "MMBO"}, {"gas", "BSCF"}, {"boe", "BOE"}};
                std::map<string, string> title_map={{"oil", "OIP"}, {"gas", "GIP"}, {"boe", "Reserves"}, {"discovered", "Discovered"}, {"assessed", "Assessed"},{"identified","Identified"},{"unidentified","Unidentified"}, {"ytf","YTF"}};
                for(auto maturity:std::vector<string>({"discovered", "assessed","identified","unidentified","ytf"})){
                    for(auto phase:std::vector<string>({"oil","gas","boe"})){
                        mf.add(output_prefix+"_"+maturity+"_well"+phase+"_dist.pdf", output_prefix+"_"+maturity+"_well"+phase+"_dist.g")
                            <<"atmath tmp.g=\"(i1()>0)*"+output_prefix+"_"+maturity+"_well"+phase+"_dist.g\""
                            <<"atgraph tmp.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='"+title_map[maturity]+" "+title_map[phase]+" Addition Distribution'  xlabel='Volume ["+unit_map[phase]+"]' saveto=$@" ;
                        mf.hidden_nodes.insert(output_prefix+"_"+maturity+"_well"+phase+"_dist.pdf");
                        mf.add(output_prefix+"_"+maturity+"_upside"+phase+"_dist.pdf", output_prefix+"_"+maturity+"_upside"+phase+"_dist.g")
                            <<"atmath tmp.g=\"(i1()>0)*"+output_prefix+"_"+maturity+"_upside"+phase+"_dist.g\""
                            <<"atgraph tmp.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='"+title_map[maturity]+" "+title_map[phase]+" Upside Distribution'  xlabel='Volume ["+unit_map[phase]+"]' saveto=$@" ;
                        mf.hidden_nodes.insert(output_prefix+"_"+maturity+"_upside"+phase+"_dist.pdf");
                    }
                }

    }
    void generate_pap_tables(){
        //prepare table to help in sql queries
        {
            auto & f = mf.add({"update_ordered_plays", "ordered_plays.t"});
            f<<"@echo \"string:seq string:play string:title\" >tmp.txt";
            int seq=1;
            for(auto item: seq_plays){
                string base="00";
                string num=to_string(seq);
                string zero_led_seq = base.substr(0, 2-num.size())+num;
                f<<"echo "+zero_led_seq+" \"'"+item+"'\" \"'"+plays[item].title+"'\">>tmp.txt";
                seq++;
            }
            f<<"atreadascii2tbl input=tmp.txt output=ordered_plays.t";
        }
        {
            auto & f = mf.add({"pvad_play_lookup.t", "update_pvad_play_lookup"});
            f<<"@echo \"string:rescd string:play \" >tmp.txt";
            for(auto item: plays){
                string play=item.first;
                auto ref_list = item.second.pvad_references;
                for(auto res: ref_list){
                    f<<"@echo \""+res+" "+play+"\" >>tmp.txt";
                }
            }
            f<<"atreadascii2tbl input=tmp.txt output=pvad_play_lookup.t";
        }
        {
            auto & f = mf.add({"spot_play_lookup.t", "update_spot_play_lookup"});
            f<<"@echo \"string:rescd string:play \" >tmp.txt";
            string all_spot_references;
            bool first=true;
            for(auto item: plays){
                string play=item.first;
                for(auto res: item.second.spot_references){
                    f<<"@echo \""+res+" "+play+"\" >>tmp.txt";
                }
            }
            f<<"atreadascii2tbl input=tmp.txt output=spot_play_lookup.t";
        }
        {
            auto & f = mf.add({"play_drillzone_lookup.t", "update_play_drillzone_lookup"});
            f<<"@echo \"string:play string:drillzone \" >tmp.txt";
            bool first=true;
            for(auto item: plays){
                string play=item.first;
                string drillzone=item.second.drill_zone;
                f<<"@echo \""+play+" "+drillzone+"\" >>tmp.txt";
            }
            f<<"atreadascii2tbl input=tmp.txt output=play_drillzone_lookup.t";
        }
        {
            auto & f = mf.add({"update_ordered_stages", "ordered_stages.t"});
            f<<"@echo \"int:seq string:stage \" >tmp.txt";
            f<<"@echo \"1 Proposed\" >>tmp.txt";
            f<<"@echo \"2 Cancelled\" >>tmp.txt";
            f<<"@echo \"3 Scheduled\" >>tmp.txt";
            f<<"@echo \"4 InProgress\" >>tmp.txt";
            f<<"@echo \"5 Completed\" >>tmp.txt";
            f<<"@echo \"6 Concluded\" >>tmp.txt";
            f<<"atreadascii2tbl input=tmp.txt output=ordered_stages.t";
        }
        {
            auto & f = mf.add({"update_ordered_maturity", "ordered_maturity.t"});
            f<<"@echo \"int:seq string:maturity \" >tmp.txt";
            f<<"@echo \"1 Unidentified\" >>tmp.txt";
            f<<"@echo \"2 IdentifiedLowPOM\" >>tmp.txt";
            f<<"@echo \"3 IdentifiedMidPOM\" >>tmp.txt";
            f<<"@echo \"4 IdentifiedHighPOM\" >>tmp.txt";
            f<<"@echo \"5 IdentifiedDifinitPOM\" >>tmp.txt";
            f<<"@echo \"6 Dropped\" >>tmp.txt";
            f<<"@echo \"7 Assessed\" >>tmp.txt";
            f<<"@echo \"8 Failed\" >>tmp.txt";
            f<<"@echo \"9 Evaluating\" >>tmp.txt";
            f<<"@echo \"10 EvaluatingRigless\" >>tmp.txt";
            f<<"@echo \"11 TechnicalSuccess\" >>tmp.txt";
            f<<"@echo \"12 UncapturedResource\" >>tmp.txt";
            f<<"@echo \"13 GeologicalSuccess\" >>tmp.txt";
            f<<"@echo \"14 SubcommercialResource\" >>tmp.txt";
            f<<"@echo \"15 CommercialSuccess\" >>tmp.txt";
            f<<"@echo \"16 UnbookedResource\" >>tmp.txt";
            f<<"@echo \"17 BookedResource\" >>tmp.txt";
            f<<"atreadascii2tbl input=tmp.txt output=ordered_maturity.t";
        }
        {
            auto & f = mf.add("stage_maturity_matrix.t",{"ordered_maturity.t", "ordered_stages.t"});
            f<<"attsqltool tables=\"ordered_stages.t ordered_maturity.t\" sql=\"select @1.stage, @1.seq as stage_seq, @2.maturity, @2.seq as maturity_seq, 'INVALID' as validity from @1,@2\" output=tmp.t" ;
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='Completed' and maturity_seq in (6, 8, 10, 11, 12, 13, 14, 15, 16, 17)\"";
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='Concluded' and maturity_seq in (6, 8, 11, 12, 14, 15, 16, 17)\"";
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='InProgress' \"";
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='Scheduled' and maturity_seq in (1, 2, 3, 4, 5, 6, 7)\"";
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='Proposed' and maturity_seq in (1, 2, 3, 4, 5, 6, 7)\"";
            f<<"attupdate tmp.t validity=\"'VALID'\" where=\"stage='Cancelled' and maturity_seq in (1, 2, 3, 4, 5, 6, 7)\"";
            f<<"atttee input=tmp.t output=$@";
        }
        {
            auto & f = mf.add({"update_ordered_prmstypes", "ordered_prmstypes.t"});
            f<<"@echo \"string:seq string:prmstype\" >tmp.txt";
            f<<"@echo \"1 Prospective\" >>tmp.txt";
            f<<"@echo \"2 Delineation\" >>tmp.txt";
            f<<"@echo \"3 Contingent\" >>tmp.txt";
            f<<"atreadascii2tbl input=tmp.txt output=ordered_prmstypes.t";
        }
        mf.add("update_framework", {"update_ordered_plays","update_ordered_maturity","update_ordered_stages", "update_ordered_prmstypes", "update_pvad_play_lookup", "update_spot_play_lookup", "update_play_drillzone_lookup"})
            <<HELP("update the workflows after editing plays, rig-aois, etc. in the file makefile.cpp");
            ;
    }
    void split_input_data_by_play(){
        //////////////////////////////////////////////////////
        // windowing data for each play
        for(auto item: plays){
            string play=item.first;
            auto ref_list = item.second.pvad_references;
            string bname = "play_"+play+"_booked"; // basename
            auto &f = mf.add({bname+".it", bname+".t"}, "pvad_booked_with_far.it");
            string where_clause;
            bool first=true;
            for(auto res: ref_list){
                if(!first){
                    where_clause+=" or ";
                }else{
                    first=false;
                }
                where_clause+="RSVR_CD='"+res+"'";
            }
            if(where_clause==""){
                where_clause="0";
            }
            f<<"atitselect input=$< where=\""+where_clause+"\" output="+bname+".it";
            mf.add("play_"+play+"_booked.shp", "play_"+play+"_booked.it")
                 <<"atit2shpfile input=$< output=$@ x=x y=y poly=true";
            gis_targets.push_back("play_"+play+"_booked.shp");
            mf.add("play_"+play+"_booked_table.pdf", "play_"+play+"_booked.it")
                <<"attsqltool play_"+play+"_booked.lt sql=\"select w_prim_hid, rsvr_cd from @1 group by w_prim_hid, rsvr_cd order by w_prim_hid\" output=tmp.t "
                    <<"att2latex input=tmp.t output=tmp2.tex headers=\"Field, Reservoir\" super=false caption=\"Booked Reservoirs:\" isdoc=true"
                <<"pdflatex tmp2.tex"
                <<"cp tmp2.pdf $@"
                ;
        }
        for(auto item: plays){
            string play=item.first;
            auto ref_list = item.second.pvad_references;
            string bname = "play_"+play+"_booked_oil"; // basename
            auto &f = mf.add({bname+".it", bname+".t"}, "pvad_booked_with_far.it");
            string where_clause="(";
            bool first=true;
            for(auto res: ref_list){
                if(!first){
                    where_clause+=" or ";
                }else{
                    first=false;
                }
                where_clause+="RSVR_CD='"+res+"'";
            }
            if(where_clause=="("){
                where_clause="0";
            }else{
                where_clause+=") and type_desc like '\%OIL\%'";
            }
            f<<"atitselect input=$< where=\""+where_clause+"\" output="+bname+".it";
        }
        for(auto item: plays){
            string play=item.first;
            auto ref_list = item.second.pvad_references;
            string bname = "play_"+play+"_booked_gas"; // basename
            auto &f = mf.add({bname+".it", bname+".t"}, "pvad_booked_with_far.it");
            string where_clause="(";
            bool first=true;
            for(auto res: ref_list){
                if(!first){
                    where_clause+=" or ";
                }else{
                    first=false;
                }
                where_clause+="RSVR_CD='"+res+"'";
            }
            if(where_clause=="("){
                where_clause="0";
            }else{
                where_clause+=") and type_desc like '\%GAS\%'";
            }
            f<<"atitselect input=$< where=\""+where_clause+"\" output="+bname+".it";
        }
        for(auto item: plays){
            string play=item.first;
            auto ref_list = item.second.pvad_references;
            string where_clause;
            bool first=true;
            for(auto res: ref_list){
                if(!first){
                    where_clause+=" or ";
                }else{
                    first=false;
                }
                where_clause+="reservoir='"+res+"'";
            }
            if(where_clause==""){
                where_clause="0";
            }
            mf.add("play_"+play+"_historical_gaswad.t", "historical_wellvolumeadd_withxy.t")
                <<"attsqltool historical_wellvolumeadd_withxy.t sql=\"select year, well, reservoir, cast(oil as float) as oil_wad_mmbo, cast(gas as float) as gas_wad_bscf, u39x, u39y from @1 where is_simple like 'yes'  and use_for_fsd='yes' and gas>0 and ("+where_clause+") order by year desc\" output=$@"
                ;
            mf.add("play_"+play+"_historical_oilwad.t", "historical_wellvolumeadd_withxy.t")
                <<"attsqltool historical_wellvolumeadd_withxy.t sql=\"select year, well, reservoir, cast(oil as float) as oil_wad_mmbo, cast(gas as float) as gas_wad_bscf, u39x, u39y from @1 where is_simple like 'yes' and use_for_fsd='yes'  and oil>0 and ("+where_clause+") order by year desc\" output=$@"
                ;
            mf.add("play_"+play+"_booked_wad_table.pdf", "play_"+play+"_historical_gaswad.t")
                <<"attsqltool $< sql=\"select year, well, reservoir, oil_wad_mmbo, gas_wad_bscf from @1\" output=tmp.t"
                <<"att2latex input=tmp.t output=tmp2.tex isdoc=true headers=\"Year,Well,Reservoir, OIP Booked, GIP Booked\" headers2=\",,,[MMBO],[BSCF]\" super=false caption=\"Historical Well Booking since 2005:\""
                <<"pdflatex tmp2.tex"
                <<"cp tmp2.pdf $@"
                ;
        }
        //generate PAL data (legacy & active) for analysis
        {
            for(auto & item : plays){
                string play=item.first;
                auto ref_list = item.second.spot_references;
                {
                    auto &f = mf.add("play_"+play+"_pal.t", "pal_eppr_dump.t");
                    string where_clause;
                    bool first=true;
                    for(auto res: ref_list){ if(!first){
                            where_clause+=" or ";
                        }else{
                            first=false;
                        }
                        where_clause+="rsvr_form_cd='"+res+"'";
                    }
                    //f<<"attsqltool $< sql=\"select * from @1 where seg_hc_type='Gas' and ("+where_clause+") \" output=$@";
                    f<<"attsqltool $< sql=\"select * from @1 where ("+where_clause+") \" output=$@";
                    //f<<"attsqltool $< sql=\"select * from @1 where geox_anal_id>0 and seg_hc_type='Gas' and ("+where_clause+") \" output=$@";
                    f<<"attrmcol $@ list=\"petro_sys_assess_seal petro_sys_assess_rsvr petro_sys_assess_clsr petro_sys_assess_chr exec_smry_vol exec_smry_rcmnd exec_smry_intro spot_smry_url lead_stat_reason drlg_schedule_name undrilled_well_bi drilled_well_bi pda_exec_smry pda_rslt pda_impct_disc pda_future_work seg_post_drill_clsf_cd seg_post_drill_clsf seg_post_drill_clsf_grp\" ";
                }
            }
        }

        //split active prospects by play
        for(auto item: plays){
            string play=item.first;
            auto ref_list = item.second.spot_references;
            {
                string bname = "play_"+play+"_spot_all_portfolio";
                auto &f = mf.add({bname+".it", bname+".t"}, "spot_all_portfolio.it");
                string where_clause;
                bool first=true;
                for(auto res: ref_list){
                    if(!first){
                        where_clause+=" or ";
                    }else{
                        first=false;
                    }
                    where_clause+="RSVR_CD='"+res+"' ";
                }
                if(where_clause==""){
                    where_clause="0";
                }else{
                    where_clause+=" ";
                }
                //where_clause+=" and hc_type='GAS'";
                f<<"atitselect input=$< where=\""+where_clause+"\" output="+bname+".it";
            }
            {
                string bname = "play_"+play+"_spot_active_portfolio";
                auto &f = mf.add({bname+".it", bname+".t"}, "spot_active_portfolio.it");
                string where_clause;
                bool first=true;
                for(auto res: ref_list){
                    if(!first){
                        where_clause+=" or ";
                    }else{
                        first=false;
                    }
                    where_clause+="RSVR_CD='"+res+"' ";
                }
                if(where_clause==""){
                    where_clause="0";
                }else{
                    where_clause+=" ";
                }
                //where_clause+=" and hc_type='GAS'";
                f<<"atitselect input=$< where=\""+where_clause+"\" output="+bname+".it";
            }

            mf.add("play_"+play+"_spot_all_portfolio.shp", "play_"+play+"_spot_all_portfolio.it")
                <<"atit2shpfile input=$< output=$@ x=u39x y=u39y poly=true";

            mf.add("play_"+play+"_spot_all_gas_portfolio.it", "play_"+play+"_spot_all_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean>0\" output=$@"
                <<BYPRODUCT(itable("play_"+play+"_spot_all_gas_portfolio.it"))
                ;

            mf.add("play_"+play+"_spot_all_oil_portfolio.it", "play_"+play+"_spot_all_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean=0\" output=$@"
                <<BYPRODUCT(itable("play_"+play+"_spot_all_oil_portfolio.it"))
                ;

            mf.add("play_"+play+"_spot_active_gas_portfolio.it", "play_"+play+"_spot_active_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean>0\" output=$@"
                <<BYPRODUCT(itable("play_"+play+"_spot_active_gas_portfolio.it"))
                ;

            mf.add("play_"+play+"_spot_active_oil_portfolio.it", "play_"+play+"_spot_active_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean=0\" output=$@"
                <<BYPRODUCT(itable("play_"+play+"_spot_active_oil_portfolio.it"))
                ;

            mf.add("play_"+play+"_spot_active_portfolio_gas.shp", "play_"+play+"_spot_active_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean>0\" output=tmp.it"
                <<"atit2shpfile input=tmp.it output=$@ x=u39x y=u39y poly=true";
            gis_targets.push_back("play_"+play+"_spot_active_portfolio_gas.shp");


            mf.add("play_"+play+"_spot_active_portfolio_oil.shp", "play_"+play+"_spot_active_portfolio.it")
                <<"atitselect input=$< where=\"bscf_mean=0\" output=tmp.it"
                <<"atit2shpfile input=tmp.it output=$@ x=u39x y=u39y poly=true";
            gis_targets.push_back("play_"+play+"_spot_active_portfolio_oil.shp");
        }
        //split play concepts by play
        {
            for(auto item: plays){
                string play=item.first;
                string bname = "play_"+play+"_rrad_concepts";
                auto &f = mf.add({bname+".it", bname+".t"}, "rrad_concepts.it");
                f<<"atitselect input=$< where=\"play='"+play+"'\" output="+bname+".it";
            }
        }
        //generate play-level portfolios from AED inputs
        {
            strings graphs_dep;
            for(auto item: plays){
                string play = item.first;
                {
                    mf.add("play_"+play+"_portfolio.t", "all_portfolio.t")
                    <<"attsqltool all_portfolio.t sql=\"select id, old_names, stage, play, account_as, minbop, is_nonhc, maturity, dep, prmstype, hc_phase, project_phase, u39x, u39y, hashtags, variables, comments from @1 where upper(play)=upper('"+play+"')\" output=$@"
                    ;
                }
                // plot play-level dependencies and generate dependency shaps (arrows)
                {
                    string tgt="play_"+play+"_graph.pdf";
                    graphs_dep.push_back(tgt);
                    mf.add( tgt , {"play_"+play+"_program_targets.t", "play_"+play+"_program_projects.t"})
                        <<"attsqltool tables=\"play_"+play+"_program_targets.t play_"+play+"_program_projects.t\" "
                            " sql=\"select * from @1 where maturity not in ('Unidentified', 'Dropped', 'Failed', 'BookedResource', 'UnbookedResource', 'UncapturedResource') and project in (select project from @2 where stage in ('InProgress', 'Proposed', 'Scheduled', 'Completed'))\" output=tmp.t"
                        <<"attsqltool tables=\"play_"+play+"_program_targets.t tmp.t\" "
                            " sql=\"select * from @1 where project in (select project from @2) or project in (select dep from @2)\" output=tmp2.t"
                        <<"atcontingency_graph input=tmp2.t output=tmp.txt title='Dependency Graph for "+item.second.title+"'"
                        <<"dot -Tpdf tmp.txt -o play_"+play+"_graph.pdf"
                        ;
                }
                // plot play-level dependencies and generate dependency shaps (arrows)
                {
                    string tgt="play_"+play+"_rrad_graph.pdf";
                    graphs_dep.push_back(tgt);
                    mf.add( tgt , {"play_"+play+"_program_targets.t"})
                        <<"attsqltool $< sql=\"select * from @1 where maturity like 'Unidentified'\" output=tmp.t"
                        <<"atcontingency_graph input=tmp.t output=tmp.txt title='Dependency Graph for "+item.second.title+"'"
                        <<"dot -Tpdf tmp.txt -o play_"+play+"_rrad_graph.pdf"
                        ;
                }
                {
                    string tgt="play_"+play+"_unidentified_graph.pdf";
                    graphs_dep.push_back(tgt);
                    mf.add( tgt , {"play_"+play+"_program_targets.t"})
                        <<"attsqltool $< sql=\"select * from @1 where maturity like 'Unidentified'\" output=tmp.t"
                        <<"atcontingency_graph input=tmp.t output=tmp.txt title='Dependency Graph for Unideinifed "+item.second.title+" Locations'"
                        <<"dot -Tpdf tmp.txt -o play_"+play+"_unidentified_graph.pdf"
                        ;
                }
                {
                    mf.add("play_"+play+"_locations.shp", "play_"+play+"_portfolio.t")
                        <<"attsqltool $< sql=\"select * from @1 where maturity<>'Unidentified'\" output=tmp.t"
                        <<"att2shpfile input=tmp.t x=u39x y=u39y output=$@";
                        ;
                    gis_targets.push_back("play_"+play+"_locations.shp");
                    mf.add("play_"+play+"_rrad_locations.shp", "play_"+play+"_portfolio.t")
                        <<"attsqltool $< sql=\"select * from @1 where maturity like 'Unidentified'\" output=tmp.t"
                        <<"att2shpfile input=tmp.t x=u39x y=u39y output=$@";
                        ;
                    gis_targets.push_back("play_"+play+"_rrad_locations.shp");
                    mf.add("play_"+play+"_arrows.shp", "play_"+play+"_portfolio.t")
                        <<"attsqltool $< sql=\"select id, u39x, u39y, id as arrow_to, dep, (select u39x from @1 as t2 where trim(t2.id)=trim(t1.dep)) as u39xx, (select u39y from @1 as t2 where trim(t2.id)=trim(t1.dep)) as u39yy, (select id from @1 as t2 where trim(t2.id)=trim(t1.dep)) as arrow_from, exists(select id from @1 as t2 where trim(t2.id)=trim(t1.dep)) as from_location from @1 as t1 where dep<>'NONE' and maturity<>'Unidentified'\" output=tmp.t"
                        <<"attsqltool tmp.t sql=\"select * from @1 where from_location>0\" output=tmp2.t"
                        <<"atdrawline input=tmp2.t xmin=u39xx ymin=u39yy xmax=u39x ymax=u39y output=tmp3.it arrow=true asize=7000"
                        <<"atitselect input=tmp3.it where=\"((u39x-u39xx)*(u39x-u39xx)+(u39y-u39yy)*(u39y-u39yy))>10*10\" output=tmp4.it"
                        <<"atit2shpfile input=tmp4.it x=x y=y output=$@ poly=true";
                    gis_targets.push_back("play_"+play+"_arrows.shp");
                }
            }
            //mf.add("graphs", graphs_dep);
        }
        //checking if dependencies for each play all present in the portfolio
        {
            strings dep;
            dep.push_back("seismic_projects.t");
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_portfolio.t");
            }
            //play level checks
            {
                auto & f = mf.add("seis_qc1", dep);
                f<<HELP("QC dependencies with non-drilling/seimsic projects");
                f<<"@echo '##########################################################################################'";
                f<<"@echo '## the following are dependencies not present in the plan or non-drilling project list! ##'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@attsqltool \"play_"+play+"_portfolio.t seismic_projects.t\" headers=false sql=\"select play, dep, count(*) from @1 where dep not in (select id from @1 union select project from @2 union select 'NONE') group by dep\"";
                }
                f<<"@echo '##########################################################################################'";
            }
        }
    }
    void qc_pap_framework(){
        //check that all plays in the input files are considers
        int nqc=1;
        {
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"aed_portfolio.t"});
            //f<<HELP("check for plays with new prospects with no workflow to handle them");
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays with new prospects with no workflow to handle them!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool $< headers=false sql=\"select play, count(*) from @1 where lower(play) not in ("+play_list+") group by play\"";

        }
        //check that no hardwired play is left without any target
        {
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"aed_portfolio.t"});
            //f<<HELP("check for plays with no features attached to them");
            f<<"@echo \"string:play\">tmp_plays.txt";
            for(auto item: plays){
                f<<"@echo \""+item.first+"\">>tmp_plays.txt";
            }
            f<<"@atreadascii2tbl input=tmp_plays.txt output=tmp_plays.t";
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays with no features attached to them!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool tables=\"aed_portfolio.t tmp_plays.t\" headers=false sql=\"select * from @2 where upper(play) not in (select upper(play) from @1)\"";
        }
        //check if we have not considered a play with gas booking
        {
            string all_pvad_references;
            bool first=true;
            for(auto play: plays){
                for(auto ref: play.second.pvad_references){
                    if(first){
                        first=false;
                    }else{
                        all_pvad_references+=", ";
                    }
                    all_pvad_references+="'"+ref+"'";
                }
            }
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"pvad_booked.it"});
            //f<<HELP("check for plays not considered although they had booked reservoirs");
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays not considered although they had booked reservoirs!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool tables=\"aed_portfolio.t pvad_booked.lt\" headers=false sql=\"select count(*) as cc, rsvr_cd from @2 where upper(rsvr_cd) not in ("+all_pvad_references+") group by rsvr_cd order by rsvr_cd\"";
            f<<"@atitselect input=\"pvad_booked.it\" where=\"upper(rsvr_cd) not in ("+all_pvad_references+")\" output=tmp_remfields.it";
        }
        //check if we have not considered a play with gas P&L
        {
            string all_spot_references;
            bool first=true;
            for(auto play: plays){
                for(auto ref: play.second.spot_references){
                    if(first){
                        first=false;
                    }else{
                        all_spot_references+=", ";
                    }
                    all_spot_references+="'"+ref+"'";
                }
            }
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"aed_portfolio.t", "spot_all_portfolio.it"});
            //f<<HELP("check for plays not considered although they had gas prospects in SPOT portfolio");
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays not considered although they had gas prospects in SPOT portfolio!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool tables=\"aed_portfolio.t spot_all_portfolio.lt\" headers=false sql=\"select count(*) as cc, rsvr_cd, division, seg_name from @2 where upper(rsvr_cd) not in ("+all_spot_references+") and hc_type='GAS' group by rsvr_cd, division order by division, cc desc\"";
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays not considered although they had oil prospects in SPOT portfolio!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool tables=\"aed_portfolio.t spot_all_portfolio.lt\" headers=false sql=\"select count(*) as cc, rsvr_cd, division, seg_name from @2 where upper(rsvr_cd) not in ("+all_spot_references+") and hc_type='OIL' group by rsvr_cd, division order by division, cc desc\"";
        }
        {
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"fsd_oil.t", "fsd_gas.t", "pos.t", "wellgasvol.t", "welloilvol.t"});
            //f<<HELP("check for plays without POS dist, oil and gas FSD, or oil and gas WAD");
            f<<"@echo '##########################'";
            f<<"@echo '!!plays without POS Dist!!'";
            f<<"@echo '##########################'";
            f<<"@attsqltool pos.t sql=\"select lower(play) from @1 where pos_dist='' group by lower(play)\"";
            f<<"@echo '##########################'";
            f<<"@echo '!!plays without oil FSD !!'";
            f<<"@echo '##########################'";
            f<<"@attsqltool fsd_oil.t sql=\"select lower(play) from @1 where fsd_oil='' group by lower(play)\"";
            f<<"@echo '##########################'";
            f<<"@echo '!!plays without gas FSD !!'";
            f<<"@echo '##########################'";
            f<<"@attsqltool fsd_gas.t sql=\"select lower(play) from @1 where fsd_gas='' group by lower(play)\"";
            f<<"@echo '##########################'";
            f<<"@echo '!!plays without oil WAD !!'";
            f<<"@echo '##########################'";
            f<<"@attsqltool welloilvol.t sql=\"select lower(play) from @1 where wellvol_oil='' group by lower(play)\"";
            f<<"@echo '##########################'";
            f<<"@echo '!!plays without gas WAD !!'";
            f<<"@echo '##########################'";
            f<<"@attsqltool wellgasvol.t sql=\"select lower(play) from @1 where wellvol_gas='' group by lower(play)\"";
            f<<"@echo '##########################'";
        }
        {
            auto &f = mf.add("pap_qc"+to_string(nqc++), {"all_program_projects.t"});
            //f<<HELP("check for wells without a cost or a rig-days distributions");
            f<<"@echo '######################################'";
            f<<"@echo '## Proposed wells without Cost Dist ##'";
            f<<"@echo '######################################'";
            f<<"@attsqltool all_program_projects.t sql=\"select project, deepest_play, wellcost_cluster from @1 where cost_dist='' and type='Drilling' group by deepest_play, wellcost_cluster order by wellcost_cluster\"";
            f<<"@echo '########################################'";
            f<<"@echo '## Proposed wells without Ridays Dist ##'";
            f<<"@echo '########################################'";
            f<<"@attsqltool all_program_projects.t sql=\"select project, deepest_play, wellcost_cluster from @1 where rigdays_dist='' and type='Drilling' group by deepest_play, wellcost_cluster order by wellcost_cluster\"";
        }
        if(1){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_booked.it");
                dep.push_back("papgis_play_"+play+"_phasemap.it");
            }
            {
                auto & f=mf.add("pap_qc"+to_string(nqc++), dep);
                //f<<HELP("check for HC-phase consistency between overlapping PVAD polygons and RRAD Concepts extent polygons");
                f<<"@echo '######################################################'";
                f<<"@echo '## PAP phasemap mismatch with booked limits!        ##'";
                f<<"@echo '######################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_booked.it ply1x=u39x ply1y=u39y ply1carry_attribs=\"fld_name rsvr_cd rsvr_name type_desc type\" ply2=papgis_play_"+play+"_phasemap.it ply2x=x ply2y=y ply2carry_attribs=\"play phase\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select relation, '"+play+":', phase, '<-->', type_desc, fld_name, rsvr_name from @1 where (phase like 'OIL' and type=2) or (phase like 'GAS' and type=1)\" headers=false nice=true";
                }
            }
        }
        if(1){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
                dep.push_back("papgis_play_"+play+"_phasemap.it");
            }
            {
                auto & f=mf.add("pap_qc"+to_string(nqc++), dep);
                //f<<HELP("check for HC-phase consistency between overlapping PVAD polygons and RRAD Concepts extent polygons");
                f<<"@echo '######################################################'";
                f<<"@echo '## PAP phasemap mismatch with SPOT prospect phase!  ##'";
                f<<"@echo '######################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_spot_active_portfolio.it ply1x=u39x ply1y=u39y ply1carry_attribs=\" seg_desc hc_type\" ply2=papgis_play_"+play+"_phasemap.it ply2x=x ply2y=y ply2carry_attribs=\"play phase\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select relation, '"+play+":', phase, '<-->', hc_type, seg_desc from @1 where (phase like 'OIL' and hc_type like 'GAS') or (phase like 'GAS' and hc_type like 'OIL')\" headers=false nice=true";
                }
            }
        }
        {
            strings dep;
            for(int i=1; i<nqc; i++){
                dep.push_back("pap_qc"+to_string(i));
            }
            mf.add("pap_qc{1-"+to_string(nqc-1)+"}", dep) <<HELP("QC Portfolio Anaytics and Planning framework");
        }
    }
    void qc_aed_inputs_stage1(){
        //check AED inputs
        int nqc=1;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "ordered_maturity.t"})
                //<<HELP("Check for non-complient maturity values")
                <<"@echo '###################################################'"
                <<"@echo '## Checking for non-complient maturity values !! ##'"
                <<"@echo '###################################################'"
                <<"attsqltool tables=\"aed_portfolio_without_rrad.t ordered_maturity.t\" headers=false sql=\"select id, play, maturity from @1 where maturity not in (select maturity from @2) order by id\""
                <<"@echo '###################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "ordered_stages.t"})
                //<<HELP("Check for non-complient stage values")
                <<"@echo '###################################################'"
                <<"@echo '## Checking for non-complient stage values !!    ##'"
                <<"@echo '###################################################'"
                <<"attsqltool tables=\"aed_portfolio_without_rrad.t ordered_stages.t\" headers=false sql=\"select id, play, stage from @1 where stage not in (select stage from @2) order by id\""
                <<"@echo '###################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "stage_maturity_matrix.t"})
                //<<HELP("Check for non-complient stage-maturity pair values")
                <<"@echo '###################################################'"
                <<"@echo '## Checking for non-complient stage values !!    ##'"
                <<"@echo '###################################################'"
                <<"attsqltool tables=\"aed_portfolio_without_rrad.t stage_maturity_matrix.t\" headers=false sql=\"select id, play, @1.stage, @1.maturity, @2.validity from @1 inner join @2 on @1.stage=@2.stage and @1.maturity=@2.maturity where validity like 'INVALID'\""
                <<"@echo '###################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "ordered_prmstypes.t"})
                //<<HELP("Check for non-complient PRMS class values")
                <<"@echo '######################################################'"
                <<"@echo '## Checking for non-complient PRMS classs values !! ##'"
                <<"@echo '######################################################'"
                <<"attsqltool tables=\"aed_portfolio_without_rrad.t ordered_prmstypes.t\" headers=false sql=\"select id, play, prmstype from @1 where prmstype not in (select prmstype from @2) order by id\""
                <<"@echo '######################################################'"
                ;
            //check if the coordinates are close to kingdom
            //TODO: use gispap_sa.t polygon
            mf.add("aed_qc"+to_string(nqc++), "aed_portfolio_without_rrad.t")
                //<<HELP("Check for AED locations outside the Kingdom")
                <<"@echo '##############################################################'"
                <<"@echo '## Those are locations that are too far to be in Kingdom !! ##'"
                <<"@echo '##############################################################'"
                <<"@attsqltool aed_portfolio_without_rrad.t headers=false sql=\"select play, id from @1 where u39x<-1220000 or u39x>1030000 or u39y<1800000 or u39y>3670000\""
                <<"@echo '##############################################################'"
                ;
            //check there are references that are not in the aed portfolios are in the seismic projects
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "seismic_projects.t"})
                //<<HELP("Check dependencies that are not in the project list")
                <<"@echo '#####################################################################################'"
                <<"@echo '## Those are projectes referenced as dependecies but are not in the project list!! ##'"
                <<"@echo '#####################################################################################'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t seismic_projects.t\" headers=false sql=\"select dep, count(*) from @1 where dep not in (select id from @1 union select project from @2 union select 'NONE') group by dep \""
                <<"@echo '#####################################################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t", "eppr_well_status.t"})
                //<<HELP("Check for drilling and completed wells status in the portfolio matches the status in EPPR")
                <<"@echo '################################################################################################'"
                <<"@echo '## Those are wells that are completed in D&WO database but have different status in portfolio ##'"
                <<"@echo '################################################################################################'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t eppr_well_status.t\" headers=false sql=\"select id, stage, project_comments, project_editedon from @1 where not stage in ('Completed', 'Concluded') and lower(id) in (select lower(w_gnr_name) from @2 where status like 'Completed') group by id order by id\""
                <<"@echo '##############################################################################################'"
                <<"@echo '## Those are wells that are drilling in D&WO database but the protfolio status doesnt match ##'"
                <<"@echo '##############################################################################################'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t eppr_well_status.t\" headers=false sql=\"select id, stage, project_comments, project_editedon from @1 where stage not in ('InProgress', 'Completed', 'Concluded') and lower(id) in (select lower(w_gnr_name) from @2 where status like 'Drilling') group by id order by id, stage\""
                <<"@echo '#############################################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t"})
                //<<HELP("Check for locations targeting the same play with different well names, but within less than 1 km from each other")
                <<"@echo '#################################################################################'"
                <<"@echo '## The following are locations targeting the same play with different well names, but within less than 1 km from each other! ##'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t\" headers=false sql=\"select r1.id id1, r1.comments comments1,  r2.id id2, r2.comments comments2 from @1 r1, @1 r2 where r1.stage in ('Proposed', 'InPrgoress') and r2.stage in ('Proposed', 'InProgress') and r1.hashtags not like '\%#tst\%' and r2.hashtags not like '\%#tst\%' and r1.id<>r2.id and r1.play=r2.play and 1000*1000>((r1.u39x-r2.u39x)*(r1.u39x-r2.u39x)+(r1.u39y-r2.u39y)*(r1.u39y-r2.u39y)) and r1.id<r2.id group by r1.id, r2.id\""
                <<"@echo '#################################################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t"})
                //<<HELP("Check for locations targeting different plays with different well names, but within less than 1 km from each other")
                <<"@echo '#################################################################################################################################'"
                <<"@echo '## The following are locations targeting different plays with different well names, but within less than 1 km from each other! ##'"
                <<"@echo '#################################################################################################################################'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t\" headers=false sql=\"select r1.id, r2.id, r1.comments, r2.comments from @1 r1, @1 r2 where r1.stage in ('Proposed', 'InProgress') and r2.stage in ('Proposed', 'InProgress') and r1.id<>r2.id and r1.play<>r2.play and 1000*1000>((r1.u39x-r2.u39x)*(r1.u39x-r2.u39x)+(r1.u39y-r2.u39y)*(r1.u39y-r2.u39y)) and r1.hashtags not like '\%#tst\%' and r2.hashtags not like '\%#tst\%' and r1.id<r2.id group by r1.id, r2.id \""
                <<"@echo '#################################################################################################################################'"
                ;
            mf.add("aed_qc"+to_string(nqc++), {"aed_portfolio_without_rrad.t"})
                //<<HELP("Check for locations with the same names, but in different locations")
                <<"@echo '##################################################################################'"
                <<"@echo '## The following are locations with the same names, but in different locations! ##'"
                <<"@echo '##################################################################################'"
                <<"@attsqltool \"aed_portfolio_without_rrad.t\" headers=false sql=\"select r1.id, r1.play, r2.id, r2.play from @1 r1, @1 r2 where r1.id=r2.id and 100*100<((r1.u39x-r2.u39x)*(r1.u39x-r2.u39x)+(r1.u39y-r2.u39y)*(r1.u39y-r2.u39y))\""
                <<"@echo '##################################################################################'"
                ;
        {
            strings dep;
            for(int i=1; i<nqc; i++){
                dep.push_back("aed_qc"+to_string(i));
            }
            mf.add("aed_qc{1-"+to_string(nqc-1)+"}", dep)<<HELP("QC AED Portfolio/Data");
        }
    }
    void qc_rrad_inputs_stage1(){
        int nqc=0;
        {
            auto &f = mf.add("rrad_qc"+to_string(++nqc), {"pap_rrad_translation.t","rrad_portfolio_concepts.t"});
            //f<<HELP("Check for plays in RRAD tables that are not in the PAP RRAD Translation tables");
            f<<"@echo '###########################################################################################'";
            f<<"@echo '!!Those are plays in RRAD tables that are not in the PAP RRAD Translation Table!'";
            f<<"@echo '###########################################################################################'";
            f<<"@attsqltool tables=\"rrad_portfolio_concepts.t pap_rrad_translation.t\" sql=\"select play_orig from @1 where play_orig not in (select reservoir from @2) group by play_orig\""
            ;
        }
        {
            strings dep;
            for(auto item: plays){
                string play = item.first;
                //dep.push_back("play_"+play+"_portfolio.t");
                dep.push_back("play_"+play+"_rrad_concepts.it");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
                dep.push_back("play_"+play+"_booked.it");
                dep.push_back("play_"+play+"_historical_gaswad.t");
                dep.push_back("play_"+play+"_historical_oilwad.t");
                dep.push_back("well_xy.t");
            }
            {
                auto &f = mf.add("rrad_qc"+to_string(++nqc), dep);
                //f<<HELP("Check for RRAD gas-concepts that tested oil and vice-versa");
                f<<"@echo '###########################################################################################'";
                f<<"@echo '!!RRAD Gas Concepts that tested oil!'";
                f<<"@echo '###########################################################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitselect input=play_"+play+"_rrad_concepts.it where=\"hc_type like 'Gas'\" output=tmp.it" ;
                    f<<"@attinpolygons input=play_"+play+"_historical_oilwad.t xexpr=u39x yexpr=u39y ply=tmp.it plyx=x plyy=y plyattribute=hashtags output=tmp2.t";
                    f<<"@attsqltool tmp2.t sql=\"select '"+play+"', well, year, oil_wad_mmbo || ' MMBO', gas_wad_bscf || 'BSCF', hashtags from @1 where hashtags<>''\" headers=false nice=true";
                }
                f<<"@echo '###########################################################################################'";
                f<<"@echo '!!RRAD Oil Concepts that tested gas!'";
                f<<"@echo '###########################################################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitselect input=play_"+play+"_rrad_concepts.it where=\"hc_type like 'Oil'\" output=tmp.it" ;
                    f<<"@attinpolygons input=play_"+play+"_historical_gaswad.t xexpr=u39x yexpr=u39y ply=tmp.it plyx=x plyy=y plyattribute=hashtags output=tmp2.t";
                    f<<"@attsqltool tmp2.t sql=\"select '"+play+"', well, year, oil_wad_mmbo || ' MMBO', gas_wad_bscf || ' BSCF', hashtags from @1 where hashtags<>''\" headers=false nice=true";
                }
            }
            {
                auto &f = mf.add("rrad_qc"+to_string(++nqc), dep);
                //f<<HELP("Check for RRAD gas-concepts that overlap with oil booking and vice-versa");
                f<<"@echo '###########################################################################################'";
                f<<"@echo '!!RRAD Oil Concepts that overlap gas booking!'";
                f<<"@echo '###########################################################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitselect input=play_"+play+"_rrad_concepts.it where=\"hc_type like 'Gas'\" output=tmp1.it" ;
                    f<<"@atitselect input=play_"+play+"_booked.it where=\"type=1\" output=tmp2.it" ;
                    f<<"@atitinpolygons ply1=tmp2.it ply1x=u39x ply1y=u39y ply1carry_attribs=\"fld_name fld_code rsvr_name rsvr_cd type_desc\" ply2=tmp1.it ply2x=x ply2y=y ply2carry_attribs=\"concept hc_type\" output=tmp3.t";
                    f<<"@attsqltool tmp3.t sql=\"select fld_name, rsvr_name, type_desc, '<--->', concept, hc_type from @1\" headers=false nice=true";
                }
                f<<"@echo '###########################################################################################'";
                f<<"@echo '!!RRAD Gas Concepts that overlap oil booking!'";
                f<<"@echo '###########################################################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitselect input=play_"+play+"_rrad_concepts.it where=\"hc_type like 'Oil'\" output=tmp1.it" ;
                    f<<"@atitselect input=play_"+play+"_booked.it where=\"type=2\" output=tmp2.it" ;
                    f<<"@atitinpolygons ply1=tmp2.it ply1x=u39x ply1y=u39y ply1carry_attribs=\"fld_name fld_code rsvr_name rsvr_cd type_desc\" ply2=tmp1.it ply2x=x ply2y=y ply2carry_attribs=\"concept hc_type\" output=tmp3.t";
                    f<<"@attsqltool tmp3.t sql=\"select fld_name, rsvr_name, type_desc, '<--->', concept, hc_type from @1\" headers=false nice=true";
                }
            }
            {
                strings dep;
                for(int i=0; i<nqc; i++){
                    dep.push_back("rrad_qc"+to_string(i+1));
                }
                mf.add("rrad_qc{1-"+to_string(nqc)+"}", dep)<<HELP("QC RRAD Portfolio/Data");
            }
        }
        {
            strings dep;
            dep.push_back("rrad_portfolio_concepts.t");
            dep.push_back("rrad_portfolio.t");
            dep.push_back("template_concept_view.dbm");
            dep.push_back("template_wf_top.tsk");
            dep.push_back("template_wf_bot.tsk");
            dep.push_back("template_wf_plotmap2.tsk");
            dep.push_back("template_latexdoc_top.tex");
            dep.push_back("template_latexdoc_bot.tex");
            dep.push_back("all_program_targets.t");
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_locations.shp");
                //dep.push_back("play_"+play+"_concept_locations.shp");

                //dep.push_back("play_"+play+"_rrad_concepts.it");
                //dep.push_back("play_"+play+"_spot_active_portfolio.it");
                //dep.push_back("play_"+play+"_booked.it");
                //dep.push_back("play_"+play+"_historical_gaswad.t");
                //dep.push_back("play_"+play+"_historical_oilwad.t");
                //dep.push_back("well_xy.t");
            }
            //generating javed's plots
            mf.add("javeds_plots.pdf", dep)
                <<HELP("generate Javed's plots for RRAD's play concepts")
                <<"attforeach input_tbl=rrad_portfolio_concepts.t template=\""
                    "attsqltool all_program_targets.t sql=\\\"select * from @1 where project like '\%XCONCEPTX\%'\\\" output=tmp.t; "
                    "atcontingency_graph input=tmp.t output=tmp.txt;"
                    "dot -Tpdf tmp.txt -o tmp_XCONCEPTX_rrad_graph.pdf;"
                    "attsqltool rrad_portfolio.t sql=\\\"select * from @1 where concept like 'XCONCEPTX'\\\" output=tmp.t; "
                    "att2shpfile input=tmp.t x=u39x y=u39y output=tmp_XCONCEPTX_locations.shp; "
                    "cat template_concept_view.dbm |"
                    //" sed 's/play_ordv_arrows/play_PLAY_arrows/g' |"
                    " sed 's/play_ordv_locations/play_PLAY_locations/g' |"
                    " sed 's/rrad_sarah_play_concept_polygon_3_locations/tmp_XCONCEPTX_locations/g' |"
                    " sed 's/play_abcr_booked/play_PLAY_booked/g' |"
                    " sed 's/spot_active_portfolio/play_PLAY_spot_active_portfolio/g' |"
                    " sed 's/TEMPLATE_TITLE/XCONCEPT_ORIGX/g' |"
                    " sed 's/Sarah_Play Concept_Polygon_5/FILE/g' |"
                    " sed 's/Sarah_Play Concept_Polygon_4/FILE/g' |"
                    " cat >tmp_XCONCEPTX_map.dbm;"
                    "\" FILE=polygon_file XCONCEPT_ORIGX=concept XCONCEPTX=concept PLAY=play | sh -v"
                <<"cat template_wf_top.tsk >interm_task_create_javed_maps.tsk"
                <<"attforeach input_tbl=rrad_portfolio_concepts.t template_file=template_wf_plotmap2.tsk  XFILEX=concept>>interm_task_create_javed_maps.tsk"
                <<"cat template_wf_bot.tsk >>interm_task_create_javed_maps.tsk"
                <<"atdialog \"Can you run interm_task_create_javed_maps.tsk from petrosys surface modeling. Click 'OK' when finished!\""
                <<"cat template_latexdoc_top.tex >javeds_plots.tex;"
                <<"attforeach input_tbl=rrad_portfolio_concepts.t template='"
                    "\\begin{tikzpicture}[remember picture,overlay]"
                    "  \\node[anchor=south west,inner sep=0pt] at ($$(current page.south west)+(0.1\\textwidth,0.95\\textheight)$$) {"
                    "     \\includegraphics[width=0.15\\textwidth]{trap_types/XTRAPTYPEX.png}"
                    "  };"
                    "  \\node[anchor=south west,inner sep=0pt] at ($$(current page.south west)+(0.25\\textwidth,0.92\\textheight)$$) {"
                    "      \\begin{tabular}{p{0.8\\textwidth}}"
                    "      \\verb!XCONCEPT_ORIGX!"
                    "      \\newline"
                    "      \\verb!prefix=XCONCEPTX!"
                    "      \\newline"
                    "      \\verb!play=XPLAYX!"
                    "      \\newline"
                    "      \\verb!play_group=XPLAYGROUPX!"
                    "      \\newline"
                    "      \\verb!nfeature=XNFEATUREX phase=XPHASEX!"
                    "       \\\\\\\\ "
                    //"      \\hline"
                    "      \\end{tabular}"
                    "  };"
                    "  \\node[anchor=south west,inner sep=0pt] at ($$(current page.south west)+(0.02\\textwidth,0.10\\textheight)$$) {"
                    "     \\includegraphics[width=0.60\\textwidth]{tmp_XCONCEPTX_map.pdf}"
                    "  };"
                    "  \\node[anchor=south west,inner sep=0pt] at ($$(current page.south west)+(0.70\\textwidth,0.10\\textheight)$$) {"
                    "     \\includegraphics[width=0.4\\textwidth]{tmp_XCONCEPTX_rrad_graph.pdf}"
                    "  };"
                    "\\end{tikzpicture}"
                    "\\newpage"
                    "'"
                    " XCONCEPTX=concept"
                    " XCONCEPT_ORIGX=concept_orig"
                    " XPLAYX=play_orig"
                    " XPLAYGROUPX=play"
                    " XNFEATUREX=nfeature"
                    " XPHASEX=hc_type"
                    " XTRAPTYPEX=ccm_trap_type"
                    ">>javeds_plots.tex"
                <<"cat template_latexdoc_bot.tex >>javeds_plots.tex;"
                <<"pdflatex javeds_plots.tex"
                <<"pdflatex javeds_plots.tex"
                <<"pdflatex javeds_plots.tex"
                <<TEMP({"interm_task_create_javed_maps.tsk", "javeds_plots.tex"})
                ;
        }
    }
    void join_spot_tables_with_aed_portfolio(){
        //matching realized P&L from area explration to seg id from SPOT. 
        //create mapping table if not already
        mf.add("reset_realized_segids")
            //<<HELP("reset the links between portfolio and SPOT P&L Portfolios")
            <<"echo \"string:play string:id int:seg_id\">tmp.txt"
            <<"atreadascii2tbl input=tmp.txt output=realized_segids.t"
            ;
        //first creating candidates for matching
        {
            strings dep;
            dep.push_back("realized_segids.t");
            dep.push_back("ordered_maturity.t");
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_portfolio.t");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
                dep.push_back("play_"+play+"_spot_all_portfolio.it");
            }
            auto & f = mf.add({"assign_segid", "prop_match.t"}, dep);
            f<<HELP("link AED plans to SPOT portfolio");
            bool isfirst=true;
            for(auto item: plays){
                string play = item.first;
                if(isfirst){
                    f<<"atdistance_to_polygons loc=play_"+play+"_portfolio.t locx=u39x locy=u39y loc_cols=\"id play stage maturity old_names\" ply=play_"+play+"_spot_all_portfolio.it plyx=u39x plyy=u39y ply_cols=\"seg_id pros_status_type\" output=tmp.t ";
                    f<<"attsqltool tmp.t sql=\"select * from @1 where (stage in('Proposed', 'InProgress') and pros_status_type like 'active') or (stage in ('Completed', 'Concluded') and pros_status_type like 'inactive') \" output=tmp2.t";
                    f<<"atttee input=tmp2.t output=prop_match.t";
                    isfirst=false;
                }else{
                    f<<"atdistance_to_polygons loc=play_"+play+"_portfolio.t locx=u39x locy=u39y loc_cols=\"id play stage maturity old_names\" ply=play_"+play+"_spot_all_portfolio.it plyx=u39x plyy=u39y ply_cols=\"seg_id pros_status_type\" output=tmp.t ";
                    f<<"attsqltool tmp.t sql=\"select * from @1 where (stage in('Proposed', 'InProgress') and pros_status_type like 'active') or (stage in ('Completed', 'Concluded') and pros_status_type like 'inactive') \" output=tmp2.t";
                    f<<"attcat list=\"prop_match.t tmp2.t\" output=prop_match.t";
                }
            }
            f<<"attsqltool tables=\"aed_portfolio.t ordered_maturity.t\" sql=\"select @1.* from @1 inner join @2 on @2.maturity=@1.maturity where @2.seq >6 order by id, play\" output=tmp_realized.t";
            f<<"attsqltool spot_all_portfolio.lt sql=\"select seg_id, name, possible_names, rsvr_cd, seg_desc, pros_lead_type, hc_type, division, pros_status_type, w_tent_xutm_cord, w_tent_yutm_cord, w_tent_utm_zn as zn, u39x, u39y from @1 order by name\" output=tmp_spot_portfolio.t";
            f<<"atmatchwhich from=tmp_realized.t from_ids=\"play id\" to=tmp_spot_portfolio.t to_ids=\"seg_id\" load=realized_segids.t sugg=prop_match.t auto=false";
        }

        //sum multiple targets in the same play
        mf.add({"assigned_pal_gas.t", "assigned_pal_gas.g"}, {"realized_segids.t", "pal_geox_all_gas.t", "pal_geox_all_gas.g"})
            <<"atspot_segment_stack realized_segids=realized_segids.t pal_table=pal_geox_all_gas.t pal_dist=pal_geox_all_gas.g output_dist=assigned_pal_gas.g output_table=assigned_pal_gas.t";
        mf.add({"assigned_pal_oil.t", "assigned_pal_oil.g"}, {"realized_segids.t", "pal_geox_all_oil.t", "pal_geox_all_oil.g"})
            <<"atspot_segment_stack realized_segids=realized_segids.t pal_table=pal_geox_all_oil.t pal_dist=pal_geox_all_oil.g output_dist=assigned_pal_oil.g output_table=assigned_pal_oil.t";
        //related assessed P&Ls to SPOT tables
        mf.add("aed_spot_rel.t", {"aed_portfolio.t", "realized_segids.t", "spot_all_portfolio.it", "assigned_pal_gas.t", "assigned_pal_oil.t"})
            <<"attsqltool tables=\"aed_portfolio.t realized_segids.t assigned_pal_gas.t spot_all_portfolio.lt assigned_pal_oil.t\" sql=\"" _cont
            "select @1.id, @1.account_as, @1.minbop,  @1.is_nonhc, @1.stage, @1.play, @1.maturity, @1.prmstype, @1.hashtags" _cont
            ", (case " _cont 
            " when not exists(select * from @3 inner join @2 on @3.id=@2.id and @3.play=@2.play where @1.id=@3.id and @1.play=@3.play) then @1.hc_phase " _cont
            " when (select urmean from @3 where @1.id=@3.id and @1.play=@3.play)>0 and (select urmean from @5 where @1.id=@5.id and @1.play=@5.play)>0  then @1.hc_phase  " _cont
            " when (select urmean from @3 where @1.id=@3.id and @1.play=@3.play)>0 then 'GAS' " _cont //over-rule forecasted phase
            " when (select urmean from @5 where @1.id=@5.id and @1.play=@5.play)>0 then 'OIL' " _cont //over-rule forecasted phase
            " else @1.hc_phase " _cont
            "end) as hc_phase" _cont
            ", exists(select seg_id from @2 where @1.id=@2.id and @1.play=@2.play) as spot_assessed " _cont
            //!
            ", (select seg_id from @2 where @1.id=@2.id and @1.play=@2.play) as seg_id " _cont
            ", cast(exists(select * from @3 inner join @2 where @1.id=@3.id and @1.play=@3.play) as int) as geox_assessed " _cont
            ", (select distid from @3 where @1.id=@3.id and @1.play=@3.play) as gas_distid" _cont
            ", (select distid from @5 where @1.id=@5.id and @1.play=@5.play) as oil_distid" _cont
            ", (select pos from @3 where @1.id=@3.id and @1.play=@3.play) as pg" _cont
            //!
            //", (select pg from @2 inner join @4 on @2.seg_id=@4.seg_id where @1.id=@2.id and @1.play=@2.play) as pg "
            ////!
            ", (select seg_desc from @2 inner join @4 on @2.seg_id=@4.seg_id where @1.id=@2.id and @1.play=@2.play) as seg_desc " _cont
            " from @1 order by @1.rowid\" output=$@"
            ;
    }
    void join_pvad_tables_with_aed_portfolio(){
        mf.add("reset_rel_delineation_booking")
            //<<HELP("reset the links between portfolio and SPOT P&L Portfolios")
            <<"echo \"string:play string:id int:objectid\">tmp.txt"
            <<"atreadascii2tbl input=tmp.txt output=rel_delineation_booking.t"
            ;
            strings dep;
            dep.push_back("rel_delineation_booking.t");
            dep.push_back("aed_portfolio.t");
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_booked.it");
            }
            auto & f = mf.add({"assign_bookedid", "del_match.t"}, dep);
            f<<HELP("link AED delineation wells to PVAD booking");
            bool isfirst=true;
            for(auto item: plays){
                string play = item.first;
                if(isfirst){
                    isfirst=false;
                    f<<"atdistance_to_polygons loc=play_"+play+"_portfolio.t locx=u39x locy=u39y loc_cols=\"id play stage maturity old_names\" ply=play_"+play+"_booked.it plyx=u39x plyy=u39y ply_cols=\"objectid\" maxdist=4000 output=tmp.t ";
                    f<<"atttee input=tmp.t output=del_match.t";
                }else{
                    f<<"atdistance_to_polygons loc=play_"+play+"_portfolio.t locx=u39x locy=u39y loc_cols=\"id play stage maturity old_names\" ply=play_"+play+"_booked.it plyx=u39x plyy=u39y ply_cols=\"objectid\" maxdist=4000 output=tmp.t ";
                    f<<"attcat list=\"del_match.t tmp.t\" output=del_match.t";
                }
            }
            f<<"attsqltool tables=\"aed_portfolio.t\" sql=\"select @1.* from @1 where prmstype like 'Delineation' and stage in ('Proposed', 'InProgress') order by id, play\" output=tmp_delineation.t";
            f<<"attsqltool tables=\"del_match.t\" sql=\"select @1.* from @1 where not match like 'inpolygon'\" output=del_match.t";
            f<<"atmatchwhich to=tmp_delineation.t to_ids=\"play id\" from=pvad_booked_with_far.lt from_ids=\"objectid\" load=rel_delineation_booking.t sugg=del_match.t auto=false";
    }
    void qc_ppd_inputs_consistency(){
        int nqc=1;
        //aed & spot
        {
            strings dep;
            dep.push_back("all_portfolio.t");
            dep.push_back("realized_segids.t");
            dep.push_back("spot_pda_simple.t");
            dep.push_back("spot_all_portfolio.it");
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_portfolio.t");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for zero-volume propsects used in planning");
                f<<"@echo '####################################################################################################'";
                f<<"@echo '## Check locations in AED Portfolio against x,y-locations in SPOT portfolio for linked prospects! ##'";
                f<<"@echo '####################################################################################################'";
                f<<"attsqltool tables=\"realized_segids.t spot_all_portfolio.lt all_portfolio.t\"" _cont
                    " sql=\"select @3.id as id, @2.possible_names as names, " _cont 
                    //" abs(@3.u39x-@2.u39x)+abs(@3.u39y-@2.u39y) as dloc, " _cont
                    " @3.u39x as guwi_x, @2.u39x as spot_x, " _cont
                    " @3.u39y as guwi_y, @2.u39y as spot_y " _cont
                    " from @3 inner join @1 on  @1.id=@3.id and @1.play=@3.play " _cont 
                    " inner join @2 on @2.seg_id=@1.seg_id " _cont
                    //" where abs(@3.u39x-@2.u39x)+abs(@3.u39y-@2.u39y)>100 " _cont
                    " group by @3.id " _cont
                    //" order by abs(@3.u39x-@2.u39x)+abs(@3.u39y-@2.u39y) desc " _cont
                    " \" output=tmp.t";
                f<<"attmath tmp.t dist=\"sqrt(pow(guwi_x-spot_x,2)+pow(guwi_y-spot_y, 2))\"";
                f<<"attsqltool tmp.t sql=\"select * from @1 where dist>1000 order by dist desc\" output=tmp2.t";
                f<<"attedit tmp2.t title=\"Check locations in Guwi Portfolio against x,y-locations in SPOT EPPR records for linked prospects!\"";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("Check for un-realized targets in a play that fall within a SPOT polygon within the same play");
                f<<"@echo '####################################################################################################'";
                f<<"@echo '## un-realized/unassessed targets in a play that fall within a SPOT polygon within the same play! ##'";
                f<<"@echo '####################################################################################################'";
                bool first=true;
                for(auto item: plays){
                    string play = item.first;
                    f<<"@attsqltool tables=\"play_"+play+"_portfolio.t\" sql=\"select * from @1 where maturity like '\%dentified\%'\" output=tmp.t";
                    f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=possible_names newcol=seg overwrite=true";
                    f<<"@attsqltool tmp.t sql=\"select play, id, maturity, seg from @1 where seg<>'' and dep='NONE' and not "+hashtag_find("RRAD")+" group by  play, id order by play, id\" output=tmp.t";
                    if(first){
                        f<<"@atttee input=tmp.t output=tmp2.t";
                        first=false;
                    }else{
                        f<<"@attcat list=\"tmp.t tmp2.t\" output=tmp2.t";
                    }
                }
                f<<"@attsqltool tmp2.t sql=\"select * from @1 where maturity like '\%identified\%' order by id\" headers=false";
            }
            if(0){
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("Check for un-realized targets in a play that fall within a SPOT polygon within the same play");
                f<<"@echo '#########################################################################################'";
                f<<"@echo '## un-realized targets in a play that fall within a SPOT polygon from another play! ##'";
                f<<"@echo '#########################################################################################'";
                bool first=true;
                for(auto item: plays){
                    string play = item.first;
                    f<<"@attsqltool tables=\"play_"+play+"_portfolio.t all_portfolio.t\" sql=\"select * from @1 where maturity like '\%dentified\%' and not id in (select id from @2 where maturity like 'Assessed')\" output=tmp.t";
                    for(auto item2: plays){
                        string play2 = item2.first;
                        if(!(play==play2)){
                            f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=play_"+play2+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_desc possible_names newcol=seg overwrite=true output=tmp3.t";
                            f<<"@attsqltool tmp3.t sql=\"select play, id, maturity, seg from @1 where seg<>'' and dep='NONE' and not "+hashtag_find("RRAD")+" group by play, id order by play, id\" output=tmp4.t";
                            if(first){
                                f<<"@atttee input=tmp4.t output=tmp2.t";
                                first=false;
                            }else{
                                f<<"@attcat list=\"tmp4.t tmp2.t\" output=tmp2.t";
                            }
                        }
                    }
                }
                f<<"@attsqltool tmp2.t sql=\"select * from @1 where maturity like '\%identified\%' order by id\" headers=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("Check for realized targets in a play that do not fall in any SPOT polygon within the same play");
                f<<"@echo '###########################################################################################'";
                f<<"@echo '## Realized targets in a play that do not fall in any SPOT polygon within the same play! ##'";
                f<<"@echo '###########################################################################################'";
                bool first=true;
                for(auto item: plays){
                    string play = item.first;
                    f<<"@attsqltool tables=\"play_"+play+"_portfolio.t\" sql=\"select * from @1 where maturity='Assessed' and is_nonhc='No'\" output=tmp.t";
                    f<<"@atclosest_polygon loc=tmp.t locx=u39x locy=u39y loc_cols=\"id play maturity\" ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y ply_cols=\"seg_desc\" output=tmp_dist.t ";
                    //f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_desc newcol=seg overwrite=true";
                    f<<"@attsqltool tmp_dist.t sql=\"select play, id, stage, maturity, match, seg_desc, match_distance from @1 where match<>'inpolygon' group by play, id order by play, id\" output=tmp.t";
                    if(first){
                        f<<"@atttee input=tmp.t output=tmp2.t";
                        first=false;
                    }else{
                        f<<"@attcat list=\"tmp.t tmp2.t\" output=tmp2.t";
                    }
                }
                f<<"@attsqltool tmp2.t sql=\"select * from @1 order by match_distance desc\" headers=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("Check for potential target additions to existing P&L in SPOT Portfolio");
                f<<"@echo '###################################################################'";
                //f<<"@echo '## Unused SPOT targets, that are below/above used targets! ##'";
                f<<"@echo '## Potential target additions to existing P&L in SPOT Portfolio! ##'";
                f<<"@echo '###################################################################'";
                bool first=true;
                for(auto item: plays){
                    string play = item.first;
                    f<<"@attsqltool tables=\"play_"+play+"_portfolio.t\" sql=\"select * from @1 where maturity<>'Assessed'\" output=tmp.t";
                    f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_desc newcol=seg1 overwrite=true";
                    f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_desc newcol=seg2 overwrite=true";
                    f<<"@attsqltool tmp.t sql=\"select id, stage, play, maturity, seg2 from @1 where seg1='' and seg2<>'' group by play, id order by play, id\" output=tmp.t";
                    if(first){
                        f<<"@atttee input=tmp.t output=tmp2.t";
                        first=false;
                    }else{
                        f<<"@attcat list=\"tmp.t tmp2.t\" output=tmp2.t";
                    }
                }
                f<<"@attsqltool tmp2.t sql=\"select * from @1 where maturity like '\%dentified' order by id\" headers=false";
            }
            // check if there are realized targets that do not have segment ID from spot table
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), {"aed_portfolio.t", "realized_segids.t"});
                //f<<HELP("cross-check realized targets in the plans with the SPOT portfolio");
                f<<"@echo '##########################################################'";
                f<<"@echo '## assessed targets that are not assigned a segment id! ##'";
                f<<"@echo '##########################################################'";
                f<<"attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select play, id, stage, comments from @1 where maturity='Assessed' and id||play not in (select id|| play from @2)\" headers=false";
                f<<"@echo '#############################################################################'";
                f<<"@echo '## seg_ids in relaized_segids table that are no longer in plans portfolio! ##'";
                f<<"@echo '#############################################################################'";
                f<<"attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play not in (select id|| play from @1)\" aders=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), {"realized_segids.t", "pal_geox_all_gas.t", "pal_geox_all_oil.t"});
                //f<<HELP("check for seg_ids in relaized_segids table that are no longer in GOEX portfolio");
                f<<"@echo '#############################################################################'";
                f<<"@echo '## seg_ids in relaized_segids table that are no longer in GOEX portfolio! ##'";
                f<<"@echo '#############################################################################'";
                f<<"attsqltool tables=\"realized_segids.t pal_geox_all_gas.t pal_geox_all_oil.t\" sql=\"select * from @1 where @1.seg_id not in (select seg_id from @2) and @1.seg_id not in (select seg_id from @3)\" ";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for unused SPOT targets, that are below/above used targets");
                f<<"@echo '###################################################################'";
                f<<"@echo '## Unused SPOT targets, that are below/above used targets!       ##'";
                f<<"@echo '###################################################################'";
                bool first=true;
                for(auto item: plays){
                    string play = item.first;
                    //list prospects not mentioned in the plan for this play but part of theplans for other plays
                    f<<"@attsqltool tables=\"play_"+play+"_portfolio.t aed_portfolio.t\" sql=\"select * from @2 where id not in (select id from @1)\" output=tmp.t";
                    f<<"@attinpolygons input=tmp.t xexpr=u39x yexpr=u39y ply=play_"+play+"_spot_active_portfolio.it plyx=u39x plyy=u39y plyattribute=seg_id newcol=seg_id overwrite=true";
                    f<<"@attsqltool tables=\"tmp.t realized_segids.t spot_active_portfolio.lt\" sql=\"select id, '"+play+"' as play, @3.seg_desc, hc_type, hashtags from @1 inner join @3 on @1.seg_id=@3.seg_id where @1.seg_id<>'' and @1.seg_id not in (select seg_id from @2) order by id, play\" output=tmp.t";
                    //see if the location in the list fall with any polygon
                    if(first){
                        f<<"@atttee input=tmp.t output=tmp2.t";
                        first=false;
                    }else{
                        f<<"@attcat list=\"tmp.t tmp2.t\" output=tmp2.t";
                    }
                }
                f<<"@attsqltool tmp2.t sql=\"select * from @1 where not "+hashtag_find("RRAD")+" and not "+hashtag_find("tst")+" group by id, play order by id, play\" headers=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), {"aed_portfolio.t", "spot_active_portfolio.t", "realized_segids.t"});
                //f<<HELP("check for gas in SPOT portfolio is not used in planning");
                f<<"@echo '##############################################################'";
                f<<"@echo '## volume check on how much of risked mean gas is not used! ##'";
                f<<"@echo '##############################################################'";
                f<<"@attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play in (select id|| play from @1)\" output=tmp.t headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_bscf))||' bscf from '|| count(*)|| ' GEOX-based P&Ls are used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_bscf))||' bscf from '|| count(*)|| ' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_bscf))||' bscf from '|| count(*)|| ' Non-GEOX P&Ls are used in AED plans ' from @1 where pg_bscf>0 and geox_anal_id=0 and seg_id in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_bscf))||' bscf from '|| count(*)|| ' Non-GEOX P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id=0 and seg_id not in (select seg_id from @2)\" headers=false";
                f<<"@echo '#############################################################'";
                f<<"@echo '## Gas P&Ls not used in the AED's plans and their volumes! ##'";
                f<<"@echo '#############################################################'";
                //f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select pg_bscf, seg_desc, division from @1 where pg_bscf>100 and seg_id not in (select seg_id from @2) order by division, pg_bscf desc\" headers=false";
                f<<"@atitstat input=spot_active_portfolio.it expr=u39y output=tmp_y.t keepattrib=true";
                f<<"@atitstat input=spot_active_portfolio.it expr=u39x output=tmp_x.t keepattrib=true";
                f<<"@attsqltool tables=\"tmp_x.t tmp_y.t\" sql=\"select @1.index_col, @1.seg_desc, @1.p50 as x, @2.p50 as y from @1 inner join @2 on @1.index_col=@2.index_col\" output=tmp_loc.t";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t tmp_loc.t\" sql=\"select pg_bscf, @1.seg_desc, division, geox_anal_id, @3.x, @3.y from @1 inner join @3 on @1.index_col=@3.index_col where pg_bscf>50 and geox_anal_id>0 and seg_id not in (select seg_id from @2) order by pg_bscf desc\" headersX=false";
                //f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select pg_bscf, seg_desc, division, geox_anal_id from @1 where pg_bscf>50 and geox_anal_id=0 and seg_id not in (select seg_id from @2) order by pg_bscf desc\" headersX=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), {"aed_portfolio.t", "spot_active_portfolio.t", "realized_segids.t"});
                //f<<HELP("check for oil in SPOT portfolio is not used in planning");
                f<<"@echo '##############################################################'";
                f<<"@echo '## volume check on how much of risked mean oil is not used! ##'";
                f<<"@echo '##############################################################'";
                f<<"@attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play in (select id|| play from @1)\" output=tmp.t headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_mmbo))||' mmbo from '|| count(*)|| ' GEOX-based P&Ls are used in AED plans' from @1 where pg_mmbo>0 and geox_anal_id>0 and seg_id in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_mmbo))||' mmbo from '|| count(*)|| ' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_mmbo>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_mmbo))||' mmbo from '|| count(*)|| ' Non-GEOX P&Ls are used in AED plans ' from @1 where pg_mmbo>0 and geox_anal_id=0 and seg_id in (select seg_id from @2)\" headers=false";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select round(sum(pg_mmbo))||' mmbo from '|| count(*)|| ' Non-GEOX P&Ls are NOT used in AED plans' from @1 where pg_mmbo>0 and geox_anal_id=0 and seg_id not in (select seg_id from @2)\" headers=false";
                f<<"@echo '#############################################################'";
                f<<"@echo '## Oil P&Ls not used in the AED's plans and their volumes! ##'";
                f<<"@echo '#############################################################'";
                //f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select pg_bscf, seg_desc, division from @1 where pg_bscf>100 and seg_id not in (select seg_id from @2) order by division, pg_bscf desc\" headers=false";
                f<<"@atitstat input=spot_active_portfolio.it expr=u39y output=tmp_y.t keepattrib=true";
                f<<"@atitstat input=spot_active_portfolio.it expr=u39x output=tmp_x.t keepattrib=true";
                f<<"@attsqltool tables=\"tmp_x.t tmp_y.t\" sql=\"select @1.index_col, @1.seg_desc, @1.p50 as x, @2.p50 as y from @1 inner join @2 on @1.index_col=@2.index_col\" output=tmp_loc.t";
                f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t tmp_loc.t\" sql=\"select pg_mmbo, @1.seg_desc, division, geox_anal_id, @3.x, @3.y from @1 inner join @3 on @1.index_col=@3.index_col where pg_mmbo>50 and geox_anal_id>0 and seg_id not in (select seg_id from @2) order by pg_mmbo desc\" headersX=false";
                //f<<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\"select pg_mmbo, seg_desc, division, geox_anal_id from @1 where pg_mmbo>50 and geox_anal_id=0 and seg_id not in (select seg_id from @2) order by pg_mmbo desc\" headersX=false";
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), {"aed_spot_rel.t", "spot_active_portfolio.it"});
                //f<<HELP("check for zero-volume propsects used in planning");
                f<<"@echo '############################'";
                f<<"@echo '## zero volume prospects! ##'";
                f<<"@echo '############################'";
                f<<"attsqltool tables=\"aed_spot_rel.t spot_active_portfolio.lt\" sql=\"select play, id, bscf_mean, mmbo_mean, @2.seg_desc from @1 inner join @2 on @1.seg_id=@2.seg_id where maturity='Assessed' and bscf_mean=0  and mmbo_mean=0\"";
            }
            //check for cyclic dependencies
            mf.add("ppd_qc"+to_string(nqc++), "all_program_dep.t")
                //<<HELP("check for cyclic dependencies (graphical)")
                <<"@echo '##############################################################################################'"
                <<"@echo '##look at the plot and make sure it is ADG (Acyclic directional graph; no loops)           ##'"
                <<"atrelationgraph relt=$< fromcol=dep tocol=project output=tmp_graph.gv &&  dot <tmp_graph.gv -Tpdf -o tmp_graph.pdf && evince tmp_graph.pdf"
                <<"@echo '##############################################################################################'"
                ;
        }
        //aed
        {
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_portfolio.t");
            }
        }
        //aed & pvad
        {
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_booked.it");
                dep.push_back("play_"+play+"_portfolio.t");
            }
            auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
            //f<<HELP("check for SPOT polygons overalping with PVAD booked limits");
            f<<"@echo '################################################################'";
            f<<"@echo '## Check if proposed delineations have booking next to them!  ##'";
            f<<"@echo '################################################################'";
            bool first=true;
            for(auto item: plays){
                string play = item.first;
                f<<"@attsqltool tables=\"play_"+play+"_portfolio.t\" sql=\"select * from @1 where prmstype='Delineation' and (maturity like 'Assessed' or maturity like 'Identified%') and dep like 'NONE'\" output=tmp.t";
                f<<"@atclosest_polygon loc=tmp.t locx=u39x locy=u39y loc_cols=\"id play prmstype depends_on\" ply=play_"+play+"_booked.it plyx=u39x plyy=u39y ply_cols=\"objectid fld_name fld_code rsvr_name rsvr_cd type_desc\" output=tmp_dist.t ";
                f<<"@attsqltool tmp_dist.t sql=\"select * from @1 where match_distance>3000\" output=tmp.t";
                if(first){
                    f<<"@atttee input=tmp.t output=tmp2.t";
                    first=false;
                }else{
                    f<<"@attcat list=\"tmp.t tmp2.t\" output=tmp2.t";
                }
            }
            f<<"@attloosesearch input=tmp2.t find=fld_code incol=id output=tmp2.t";
            f<<"@attsqltool tmp2.t sql=\"select * from @1 where match_result=0 order by match_distance\" headers=true output=tmp3.t && attedit tmp3.t";
        }
        //spot & pvad
        {
            auto & f=mf.add("ppd_qc"+to_string(nqc++), {"spot_pda_simple.t"});
                f<<"@echo '########################################################################'";
                f<<"@echo '## Commercial successes (PVAD) that are not technical success (SPOT)! ##'";
                f<<"@echo '########################################################################'";
                f<<"attsqltool spot_pda_simple.t sql=\"select w_gnr_name, play, exists(select * from @1 as b where a.w_gnr_name=b.w_gnr_name and a.play=b.play and post_drill_clsf_typ_cd like 'suc') as has_tech_suc, exists(select * from @1 as b where a.w_gnr_name=b.w_gnr_name and a.play=b.play and commercial like 'yes') as has_comm_suc from @1 as a group by w_gnr_name, play\" output=tmp.t";
                f<<"attsqltool tmp.t sql=\"select * from @1 where has_tech_suc=0 and has_comm_suc=1\"";

        }
        if(0){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_booked.it");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for SPOT polygons overalping with PVAD booked limits");
                f<<"@echo '##############################################################'";
                f<<"@echo '## SPOT polygons overalping with PVAD booked limits!        ##'";
                f<<"@echo '##############################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_booked.it ply1x=u39x ply1y=u39y ply1carry_attribs=\"fld_name rsvr_cd rsvr_name type_desc\" ply2=play_"+play+"_spot_active_portfolio.it ply2x=u39x ply2y=u39y ply2carry_attribs=\"name rsvr_cd hc_type\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select '"+play+":', relation, fld_name, '<-->', name, case when (name like '%'|| fld_name|| '%' ) then 'OK' else '#CHECK#' end, ply1_rsvr_cd, '<-->', ply2_rsvr_cd, type_desc,'<-->', hc_type , case when (type_desc like '%'||hc_type||'%') then 'OK' else '#CHECK#' end from @1\" headers=false nice=true";
                }
            }
        }
        //rrad & pvad
        if(0){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_booked.it");
                dep.push_back("play_"+play+"_rrad_concepts.it");
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for HC-phase consistency between overlapping PVAD polygons and RRAD Concepts extent polygons");
                f<<"@echo '##############################################################'";
                f<<"@echo '## RRAD Concepts overalping with PVAD booked limits!        ##'";
                f<<"@echo '##############################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_booked.it ply1x=u39x ply1y=u39y ply1carry_attribs=\"fld_name rsvr_cd rsvr_name type_desc\" ply2=play_"+play+"_rrad_concepts.it ply2x=x ply2y=y ply2carry_attribs=\"concept hc_type\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select '"+play+":', concept, hc_type, '<-->', type_desc , count(*), case when (type_desc like '%'||hc_type||'%') then 'OK' else '#CHECK#' end from @1 group by concept, hc_type\" headers=false nice=true";
                }
            }
        }
        //spot & rrad
        if(0){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_rrad_concepts.it");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for HC-phase consistency between overlapping SPOT polygons and RRAD Concepts extent polygons");
                f<<"@echo '##############################################################'";
                f<<"@echo '## SPOT polygons overalping with RRAD Concepts limits!      ##'";
                f<<"@echo '##############################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_rrad_concepts.it ply1x=x ply1y=y ply1carry_attribs=\"concept hc_type\" ply2=play_"+play+"_spot_active_portfolio.it ply2x=u39x ply2y=u39y ply2carry_attribs=\"name rsvr_cd hc_type\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select '"+play+":', concept, ply1_hc_type,'<-->', ply2_hc_type , count(*), case when (ply1_hc_type like ply2_hc_type) then 'OK' else '#CHECK#' end  from @1 group by concept, ply2_hc_type \" headers=false nice=true";
                }
            }
        }
        if(0){
            strings dep;
            for(auto item: plays){
                string play = item.first;
                dep.push_back("play_"+play+"_rrad_concepts.it");
                dep.push_back("play_"+play+"_spot_active_portfolio.it");
            }
            {
                auto & f=mf.add("ppd_qc"+to_string(nqc++), dep);
                //f<<HELP("check for HC-phase consistency between overlapping SPOT polygons and RRAD Concepts extent polygons with more details");
                f<<"@echo '##############################################################'";
                f<<"@echo '## SPOT polygons overalping with RRAD Concepts limits!      ##'";
                f<<"@echo '##############################################################'";
                for(auto item: plays){
                    string play = item.first;
                    f<<"@atitinpolygons ply1=play_"+play+"_rrad_concepts.it ply1x=x ply1y=y ply1carry_attribs=\"concept hc_type\" ply2=play_"+play+"_spot_active_portfolio.it ply2x=u39x ply2y=u39y ply2carry_attribs=\"name rsvr_cd hc_type\" output=tmp.t";
                    f<<"@attsqltool tmp.t sql=\"select '"+play+":', relation, concept, name, rsvr_cd, ply1_hc_type,'<-->', ply2_hc_type , case when (ply1_hc_type like ply2_hc_type) then 'OK' else '#CHECK#' end  from @1 where ply1_hc_type not like ply2_hc_type \" headers=false nice=true";
                }
            }
        }
        {
            strings dep;
            for(int i=1; i<nqc; i++){
                dep.push_back("ppd_qc"+to_string(i));
            }
            mf.add("ppd_qc{1-"+to_string(nqc-1)+"}", dep)<<HELP("Check consistancy between inputs from PPD departments");
        }
    }
    void assign_fsd_and_wad_to_targets(){
        //assign field size for targets
        mf.add("fsd_gas.t", {"aed_spot_rel.t", "forecast_gas_fsd.t"})
            <<"attsetwhere cond=forecast_gas_fsd.t condcol=cond valuecol=title newcol='fsd_gas_cond' input=aed_spot_rel.t output=tmp.t"
            <<"attsetwhere cond=forecast_gas_fsd.t condcol=cond valuecol=upside_gas_bscf_dist newcol='fsd_gas' input=tmp.t output=tmp.t"
            <<"attupdate input=tmp.t fsd_gas=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"assigned_pal_gas.g\\\", \\\"i2\\\":'||gas_distid||'}'\" where=\"geox_assessed>0\""
            <<"attupdate input=tmp.t fsd_gas_cond=\"'SPOT Assigned'\" where=\"geox_assessed>0\""
            <<"atttee input=tmp.t output=$@"
            ;
        //
        mf.add("fsd_oil.t", {"aed_spot_rel.t", "forecast_oil_fsd.t"})
            <<"attsetwhere cond=forecast_oil_fsd.t condcol=cond valuecol=title newcol='fsd_oil_cond' input=aed_spot_rel.t output=tmp.t"
            <<"attsetwhere cond=forecast_oil_fsd.t condcol=cond valuecol=upside_oil_mmbo_dist newcol='fsd_oil' input=tmp.t output=tmp.t"
            <<"attupdate input=tmp.t fsd_oil=\"'{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"assigned_pal_oil.g\\\", \\\"i2\\\":'||oil_distid||'}'\" where=\"geox_assessed>0\""
            <<"attupdate input=tmp.t fsd_oil_cond=\"'SPOT Assigned'\" where=\"geox_assessed>0\""
            <<"atttee input=tmp.t output=$@"
            ;
        mf.add("wellgasvol.t", {"aed_spot_rel.t", "forecast_gas_wad.t"})
            <<"attsetwhere cond=forecast_gas_wad.t condcol=cond valuecol=title newcol='wellvol_gas_cond' input=aed_spot_rel.t output=tmp.t"
            <<"attsetwhere cond=forecast_gas_wad.t condcol=cond valuecol=well_gas_bscf_dist newcol='wellvol_gas' input=tmp.t output=tmp.t"
            <<"atttee input=tmp.t output=$@"
            ;
        mf.add("welloilvol.t", {"aed_spot_rel.t", "forecast_oil_wad.t"})
            <<"attsetwhere cond=forecast_oil_wad.t condcol=cond valuecol=title newcol='wellvol_oil_cond' input=aed_spot_rel.t output=tmp.t"
            <<"attsetwhere cond=forecast_oil_wad.t condcol=cond valuecol=well_oil_mmbo_dist newcol='wellvol_oil' input=tmp.t output=tmp.t"
            <<"atttee input=tmp.t output=$@"
            ;

        mf.add("pos.t", {"aed_spot_rel.t", "forecast_pos.t"})
            <<"attsetwhere cond=forecast_pos.t condcol=cond valuecol=title newcol='pos_cond' input=aed_spot_rel.t output=tmp.t"
            <<"attsetwhere cond=forecast_pos.t condcol=cond valuecol=pos_dist input=tmp.t output=tmp.t"
            <<"attaddcol input=tmp.t p1p=\"cast(pg as float)\""
            <<"attupdate input=tmp.t pos_dist=\"'{\\\"distribution\\\":\\\"ConstantValue\\\", \\\"value\\\":'||p1p||'}'\" where=\"geox_assessed>0\""
            <<"attupdate input=tmp.t pos_cond=\"'SPOT Assigned'\" where=\"geox_assessed>0\""
            <<"atttee input=tmp.t output=$@"
            ;

        string array_params = "o1=0 d1=10 n1=350";
        mf.add("fsd_gas.g", {"fsd_gas.t", "assigned_pal_gas.g"})
            <<"atgendistarray input=$< jsoncol=fsd_gas "+array_params+" output=$@" ;
            ;
        mf.add("fsd_oil.g", {"fsd_oil.t", "assigned_pal_oil.g"})
            <<"atgendistarray input=$< jsoncol=fsd_oil "+array_params+" output=$@" ;
            ;
        mf.add("wad_prior_gas.g", "wellgasvol.t")
            <<"atgendistarray input=$< jsoncol=wellvol_gas "+array_params+" output=$@" ;
            ;
        mf.add("wad_prior_oil.g", "welloilvol.t")
            <<"atgendistarray input=$< jsoncol=wellvol_oil "+array_params+" output=$@" ;
            ;
        //clip WAD to honor FSD.
        mf.add("wad_posterior_gas.g", {"wad_prior_gas.g", "fsd_gas.g"})
            <<"atposteriorwellvolumes fsd=fsd_gas.g wad=wad_prior_gas.g output=$@"
            ;
        mf.add("wad_posterior_oil.g", {"wad_prior_oil.g", "fsd_oil.g"})
            <<"atposteriorwellvolumes fsd=fsd_oil.g wad=wad_prior_oil.g output=$@"
            ;
        mf.add("nwell_gas.t", {"fsd_gas.g", "wad_posterior_gas.g"})
            <<"atnwell fsd=fsd_gas.g wad=wad_posterior_gas.g output=$@";
        mf.add("nwell_oil.t", {"fsd_oil.g", "wad_posterior_oil.g"})
            <<"atnwell fsd=fsd_oil.g wad=wad_posterior_oil.g output=$@";

        mf.add("nwell.t", {"nwell_gas.t", "nwell_oil.t", "aed_spot_rel.t"})
#ifdef MATCHSPOT
            <<"attsqltool tables=\"nwell_gas.t nwell_oil.t aed_spot_rel.t\" sql=\"select @3.*, @1.nwell as gas_nwell, @2.nwell as oil_nwell, cast(min(3, max(1, @1.nwell, @2.nwell)) as int) as nwell_1, cast(1 as int) as nwell from @1, @2, @3 where @1.rowid=@2.rowid and @1.rowid=@3.rowid\" output=$@"
#else
            <<"attsqltool tables=\"nwell_gas.t nwell_oil.t aed_spot_rel.t\" sql=\"select @3.*, @1.nwell as gas_nwell, @2.nwell as oil_nwell, cast(min(3, max(1, @1.nwell, @2.nwell)) as int) as nwell, cast(1 as int) as nwell_1 from @1, @2, @3 where @1.rowid=@2.rowid and @1.rowid=@3.rowid\" output=$@"
#endif
            <<"attupdate input=$@ nwell=1 where=\"hashtags like '\%#test\%'\""
            <<"attupdate input=$@ nwell=1 where=\"maturity like 'Failed'\""
            <<"attupdate input=$@ nwell=1 where=\"maturity like 'Dropped'\""
            <<"attupdate input=$@ nwell=1 where=\"maturity like 'UncapturedResource'\""
            ;

        mf.add("fsd_gas_factorized.g", {"nwell.t", "fsd_gas.t", "fsd_gas.g"})
            <<"attsqltool tables=\"nwell.t fsd_gas.t\" sql=\"select @2.*, @1.nwell from @1, @2 where @1.rowid=@2.rowid \" output=tmp.t"
            <<"atsplitdistributions input=fsd_gas.g nfacttbl=tmp.t distid=fsd_gas nfactcol=nwell output=$@"
            ;
        mf.add("fsd_oil_factorized.g", {"nwell.t", "fsd_oil.t", "fsd_oil.g"})
            <<"attsqltool tables=\"nwell.t fsd_oil.t\" sql=\"select @2.*, @1.nwell from @1, @2 where @1.rowid=@2.rowid \" output=tmp.t"
            <<"atsplitdistributions input=fsd_oil.g nfacttbl=tmp.t distid=fsd_oil  nfactcol=nwell output=$@"
            ;

        mf.add("pos.g", "pos.t")
            <<"atgendistarray input=$< jsoncol=pos_dist o1=0 d1=0.01 n1=102 output=$@" ;
            ;
    }
    void create_maps_for_plays_rigaois(){
        //create play maps (using data from all the teams);
        strings maps_dep;
        maps_dep.push_back("template_wf_top.tsk");
        maps_dep.push_back("template_wf_plotmap.tsk");
        maps_dep.push_back("template_wf_bot.tsk");
        strings maps_tgt;
        for(auto item: plays){
            string play = item.first;
            string target= "map_play_"+play+".dbm";
            mf.add(target, {"template_teamreview.dbm", 
                    "play_"+play+"_arrows.shp", 
                    "play_"+play+"_locations.shp", 
                    "play_"+play+"_rrad_locations.shp", 
                    "play_"+play+"_booked.shp", 
                    "play_"+play+"_spot_active_portfolio_gas.shp", 
                    "play_"+play+"_spot_active_portfolio_oil.shp", 
                    })
                //<<FINAL
                <<"cat template_teamreview.dbm | \\"
                <<"sed \"s/placeholder_play_arrows/play_"+play+"_arrows/g\" |\\"
                <<"sed \"s/placeholder_play_locations/play_"+play+"_locations/g\" |\\"
                <<"sed \"s/placeholder_play_rrad_locations/play_"+play+"_rrad_locations/g\" |\\"
                <<"sed \"s/play_abcr_booked/play_"+play+"_booked/g\" |\\"
                <<"sed \"s/spot_active_portfolio/play_"+play+"_spot_active_portfolio/g\" |\\"
                <<"sed \"s/TEMPLATE_TITLE/"+item.second.title+"/g\" |\\"
                <<" cat >$@"
                ;
            maps_dep.push_back(target);
            maps_tgt.push_back("map_play_"+play+".pdf");
        }
        for(auto aoi: aois){
            string target= "map_rigaoi_"+aoi.prefix+".dbm";
            mf.add(target, {"template_rigaoi_view.dbm", 
                    "rigaoi_"+aoi.prefix+"_area.shp", 
                    "sa_boundary_onshore_offshore_aat.shp",
                    "sa_boundary_onshore_offshore_negative_aat.shp",
                    //"rigaoi_"+aoi+"_arrows.shp", 
                    //"rigaoi_"+aoi+"_locations.shp", 
                    })
                //<<FINAL
                <<"cat template_rigaoi_view.dbm | \\"
                <<"sed \"s/placeholder/rigaoi_"+aoi.prefix+"_area/g\" |\\"
                <<"sed \"s/TEMPLATE_TITLE/"+aoi.title+"/g\" |\\"
                <<" cat >$@"
                ;
            maps_dep.push_back(target);
            maps_tgt.push_back("map_rigaoi_"+aoi.prefix+".pdf");
        }
        //disable when the maps do not need to be updated or in test mode
        if(generate_maps){
            //TODO: fix the path
            auto &f = mf.add(maps_tgt, maps_dep);
            //f<<FINAL;
            f<<"cat template_wf_top.tsk >interm_task_plot_play_maps.tsk";
            for(auto item: plays){
                string play = item.first;
                f<<"sed <template_wf_plotmap.tsk \"s?\\/work0\\/macromodel\\/maps_master_arum?`pwd`\\/map_play_"+play+"?g\" >>interm_task_plot_play_maps.tsk";
            }
            for(auto item: aois){
                f<<"sed <template_wf_plotmap.tsk \"s?\\/work0\\/macromodel\\/maps_master_arum?`pwd`/map_rigaoi_"+item.prefix+"?g\" >>interm_task_plot_play_maps.tsk";
            }
            f<<"cat template_wf_bot.tsk >>interm_task_plot_play_maps.tsk";
            f<<"atdialog \"Can you run interm_task_plot_play_maps.tsk from petrosys surface modeling. Click 'OK' when finished!\" ";
            f<<TEMP("interm_task_plot_play_maps.tsk");
        }
    }
    void play_level_analysis(){
        //play level analysis
        mf.add("analysis_play_{XXXX}_wc_pos")<<HELP( "model wildcat POS distributon for the XXXX play.");
        mf.add("analysis_play_{XXXX}_del_pos")<<HELP("model delineation POS distributon for the XXXX play.");
        mf.add("analysis_play_{XXXX}_{oil|gas}_wad")<<HELP("model (prior) well-add distributon for the XXXX play.");
        mf.add("analysis_play_{XXXX}_{oil|gas}_fsd")<<HELP("model FSD for the {XXXX} play. This is updated by FSD and well count later.");
        for(auto & item : plays){
            string play=item.first;
            {
                mf.add( "play_"+play+"_geox_gas_historical.t", {"play_"+play+"_pal.t", "pal_geox_all_gas.t"})
                    <<"attsqltool tables=\"pal_geox_all_gas.t play_"+play+"_pal.t\" sql=\"select * from @1 inner join @2 on @1.geox_anal_id=@2.geox_anal_id and seg_pre_drill_clsf_grp like 'wildcat'\" output=$@" ;
                mf.add( "play_"+play+"_geox_oil_historical.t", {"play_"+play+"_pal.t", "pal_geox_all_oil.t"})
                    <<"attsqltool tables=\"pal_geox_all_oil.t play_"+play+"_pal.t\" sql=\"select * from @1 inner join @2 on @1.geox_anal_id=@2.geox_anal_id and seg_pre_drill_clsf_grp like 'wildcat'\" output=$@" ;
                mf.add( "play_"+play+"_geox_gas_historical.g", "play_"+play+"_geox_gas_historical.t")
                    <<"atgeox2array input=$< o1=0 d1=5 n1=1701 output=$@" ;
                mf.add( "play_"+play+"_geox_oil_historical.g", "play_"+play+"_geox_oil_historical.t")
                    <<"atgeox2array input=$< o1=0 d1=5 n1=1701 output=$@" ;
            }

            //play-level portfolio analytics
            mf.add("play_"+play+"_geox_gas_fsd_stack.g", {"play_"+play+"_geox_gas_historical.g", "play_"+play+"_geox_gas_historical.t" })
                <<"attwind din=play_"+play+"_geox_gas_historical.g tin=play_"+play+"_geox_gas_historical.t tout=tmp1.t dout=tmp1.g expr=\"gas_unrsk_rsvrs_mean>10\""
                <<"atmath tmp2.g=\"(i1()>0)*tmp1.g\""
                <<"atspot_prospect_stack pal_table=tmp1.t pal_dist=tmp2.g output_dist=tmp3.g output_table=tmp3.t"
                <<"attmath tmp3.t pos1=1.0"
                <<"atspotaggregate target_table=tmp3.t target_gasvol=tmp3.g poscol=pos1 output=tmp4.g"
                <<"attsqltool tmp3.t sql=\"select cast(count(*) as int) nwell from @1\" output=tmp4.t"
                <<"attsqltool tmp4.t sql=\"select * from @1 where nwell>0\" output=tmp4b.t"
                <<"atbestlognormalfit input=tmp4.g nfacttbl=tmp4b.t nfactcol=nwell drvw=100 nitr=1000 output=$@"
                //<<"atspot_prospect_stack pal_table=play_"+play+"_geox_gas_historical.t pal_dist=play_"+play+"_geox_gas_historical.g output_dist=tmp.g output_table=tmp.t"
                //<<"atstack2 input=tmp.g output=$@"
                ;
            mf.add("play_"+play+"_geox_oil_fsd_stack.g", {"play_"+play+"_geox_oil_historical.g", "play_"+play+"_geox_oil_historical.t" })
                <<"attwind din=play_"+play+"_geox_oil_historical.g tin=play_"+play+"_geox_oil_historical.t tout=tmp1.t dout=tmp1.g expr=\"oil_unrsk_rsvrs_mean>10\""
                <<"atmath tmp2.g=\"(i1()>0)*tmp1.g\""
                <<"atspot_prospect_stack pal_table=tmp1.t pal_dist=tmp2.g output_dist=tmp3.g output_table=tmp3.t"
                <<"attmath tmp3.t pos1=1.0"
                <<"atspotaggregate target_table=tmp3.t target_gasvol=tmp3.g poscol=pos1 output=tmp4.g"
                <<"attsqltool tmp3.t sql=\"select cast(count(*) as int) nwell from @1\" output=tmp4.t"
                <<"attsqltool tmp4.t sql=\"select * from @1 where nwell>0\" output=tmp4b.t"
                <<"atbestlognormalfit input=tmp4.g nfacttbl=tmp4b.t nfactcol=nwell drvw=100 nitr=1000 output=$@"
                //<<"atsplitdistributions input=tmp4.g nfacttbl=tmp4.t nfactcol=nwell drvw=100 nitr=1000 output=$@"
                //<<"atstack2 input=tmp.g output=$@"
                ;
            mf.add("analysis_play_"+play+"_geox_gas_fsd", 
                    {"play_"+play+"_geox_gas_historical.g", "play_"+play+"_geox_gas_historical.t" })
                <<"atspot_prospect_stack pal_table=play_"+play+"_geox_gas_historical.t pal_dist=play_"+play+"_geox_gas_historical.g output_dist=tmp.g output_table=tmp.t"
                //<<"attsort input=tmp.t key1=gas_unrsk_rsvrs_mean output=tmp1.t keep_index=true"
                //<<"attracesort din=tmp.g tin=tmp1.t skey=new_index dout=tmp2.g tout=tmp2.t"
                <<"atwigplot tmp.g attrib=tmp.t plotcol=pos label1=\"Volume [bscf]\" tbt_scale=true"
                    ;
            mf.add("analysis_play_"+play+"_geox_oil_fsd", 
                    {"play_"+play+"_geox_oil_historical.g", "play_"+play+"_geox_oil_historical.t" })
                <<"atspot_prospect_stack pal_table=play_"+play+"_geox_oil_historical.t pal_dist=play_"+play+"_geox_oil_historical.g output_dist=tmp.g output_table=tmp.t"
                //<<"attsort input=tmp.t key1=oil_unrsk_rsvrs_mean output=tmp1.t keep_index=true"
                //<<"attracesort din=tmp.g tin=tmp1.t skey=new_index dout=tmp2.g tout=tmp2.t"
                <<"atwigplot tmp.g attrib=tmp.t plotcol=pos label1=\"Volume [bscf]\" tbt_scale=true"
                    ;
            mf.add("analysis_play_"+play+"_gas_cdf", "play_"+play+"_pal.t")
                <<"atthistogram input=$< expr='gas_unrsk_rsvrs_p90' output=tmp.t"
                <<"attcategory2num input=tmp.t columns=\"seg_pre_drill_clsf_grp\" overwrite=true numcol=ncat catcol=cat"
                <<"attxycplot tmp.t x=gas_unrsk_rsvrs_p90 y=cdf psize=20  minx=0 miny=0 maxy=1.1 z=ncat"
                ;

            mf.add("analysis_play_"+play+"_oil_cdf", "play_"+play+"_pal.t")
                <<"atthistogram input=$< expr='oil_unrsk_rsvrs_p90' output=tmp.t"
                <<"attcategory2num input=tmp.t columns=\"seg_pre_drill_clsf_grp\" overwrite=true numcol=ncat catcol=cat"
                <<"attxycplot tmp.t x=oil_unrsk_rsvrs_p90 y=cdf psize=20 minx=0 miny=0 maxx=100 maxy=1.1 z=ncat"
                ;

            mf.add("analysis_play_"+play+"_pos_cdf", "play_"+play+"_pal.t")
                <<"atthistogram input=$< expr=pos_segment output=tmp.t"
                <<"attcategory2num input=tmp.t columns=\"seg_pre_drill_clsf_grp\" overwrite=true numcol=ncat catcol=cat"
                <<"attxycplot tmp.t x=pos_segment y=cdf psize=20  minx=0 miny=0 maxx=1.1 maxy=1.1 z=ncat"
                ;

            mf.add("analysis_play_"+play+"_wc_pos", "play_"+play+"_pal.t")
                //<<HELP("model POS for wildcat targets in the "+play+" play.")
                <<"attsqltool $< sql=\"select * from @1 where seg_pre_drill_clsf_grp like 'WILDCAT'\" output=tmp.t"
                <<"atdrawdist data=tmp.t datacol=pos_segment maxval=1 proj=analysis_play_"+play+"_wc_pos.proj"
                <<TARGET({"analysis_play_"+play+"_wc_pos.proj", "analysis_play_"+play+"_wc_pos.t"})
                ;
            
            mf.add("analysis_play_"+play+"_del_pos", "play_"+play+"_pal.t")
                //<<HELP("model POS for delineation targets in the "+play+" play.")
                <<"attsqltool $< sql=\"select * from @1 where seg_pre_drill_clsf_grp like 'DELINEATION'\" output=tmp.t"
                <<"atdrawdist data=tmp.t datacol=pos_segment maxval=1 proj=analysis_play_"+play+"_del_pos.proj"
                <<TARGET({"analysis_play_"+play+"_del_pos.proj", "analysis_play_"+play+"_del_pos.t", "analysis_play_"+play+"_del_pos.g" })
                ;


            mf.add("analysis_play_"+play+"_gas_fsd", {"play_"+play+"_historical_gaswad.t", "play_"+play+"_geox_gas_fsd_stack.g"})
                //<<HELP("model gas field-size distribution (FSD) for the "+play+" play.")
                <<"atdrawdist data=$< datacol=gas_wad_bscf maxval=1000 proj=analysis_play_"+play+"_gas_fsd.proj pdf_overlay=play_"+play+"_geox_gas_fsd_stack.g"
                //<<"atdrawdist data=$< datacol=gas_wad_bscf maxval=1000 proj=analysis_play_"+play+"_gas_fsd.proj pdf_overlay=analysis_play_"+play+"_gas_wad.g"
                <<TARGET({"analysis_play_"+play+"_gas_fsd.proj", "analysis_play_"+play+"_gas_fsd.t"})
                ;
            mf.add("analysis_play_"+play+"_oil_fsd", {"play_"+play+"_historical_oilwad.t", "play_"+play+"_geox_oil_fsd_stack.g"})
                //<<HELP("model oil field-size distribution (FSD) for the "+play+" play.")
                <<"atdrawdist data=$< datacol=oil_wad_mmbo maxval=1000 proj=analysis_play_"+play+"_oil_fsd.proj pdf_overlay=play_"+play+"_geox_oil_fsd_stack.g"
                <<TARGET({"analysis_play_"+play+"_oil_fsd.proj", "analysis_play_"+play+"_oil_fsd.t"})
                ;
            mf.add("analysis_play_"+play+"_gas_wad", {"play_"+play+"_historical_gaswad.t", "play_"+play+"_geox_gas_fsd_stack.g"})
                //<<HELP("model gas well-add distribution (WAD) for the "+play+" play.")
                <<"atdrawdist data=$< datacol=gas_wad_bscf maxval=1000 proj=analysis_play_"+play+"_gas_wad.proj pdf_overlay=analysis_play_"+play+"_gas_fsd.g"
                <<TARGET({"analysis_play_"+play+"_gas_wad.proj", "analysis_play_"+play+"_gas_wad.t"})
                ;
            mf.add("analysis_play_"+play+"_oil_wad", {"play_"+play+"_historical_oilwad.t", "play_"+play+"_geox_oil_fsd_stack.g"})
                //<<HELP("model oil well-add distribution (WAD) for the "+play+" play.")
                <<"atdrawdist data=$< datacol=oil_wad_mmbo maxval=1000 proj=analysis_play_"+play+"_oil_wad.proj pdf_overlay=analysis_play_"+play+"_oil_fsd.g"
                <<TARGET({"analysis_play_"+play+"_oil_wad.proj", "analysis_play_"+play+"_oil_wad.t"})
                ;
        }
    }
    void monte_carlo_simulations(){
        // assessing plays via mc simulations
        strings compute_dep;
        //create a compelete porgram
        {
            add_mcrun("all", "mcrun_all", nitr, false, 
                    " max_cost=60000 mmdollers," _cont
                    " max_rigyears=600 years," _cont
                    "   max_wellgas=120000 bscf," _cont
                    " max_upsidegas=200000 bscf," _cont
                    "   max_welloil=80000 mmbo," _cont
                    " max_upsideoil=100000 mmbo"
                    );
            compute_dep.push_back("mcrun_all_rig_level.it");
            landscape_portfolios.push_back("all");
            { 
                string input_prefix="all";
                mf.add( input_prefix+"_graph.pdf" , {input_prefix+"_program_targets.t"})
                        <<"attsqltool tables=\"all_program_targets.t all_program_projects.t\" "
                            " sql=\"select * from @1 where maturity not in ('Unidentified', 'Dropped', 'Failed', 'BookedResource', 'UnbookedResource', 'UncapturedResource') and project in (select project from @2 where stage in ('InProgress', 'Proposed', 'Scheduled', 'Completed'))\" output=tmp.t"
                        <<"attsqltool tables=\"all_program_targets.t tmp.t\" "
                            " sql=\"select * from @1 where project in (select project from @2) or project in (select dep from @2)\" output=tmp2.t"
                        <<"atcontingency_graph input=tmp2.t output=tmp.txt title='Dependency Graph for all Plans'"
                        <<"dot -Tpdf tmp.txt -o $@"
                        ;
            }
        }
        for(auto & item : plays){
            string input_prefix="play_"+item.first;
            landscape_portfolios.push_back(input_prefix);
            auto & p = item.second;
            string output_prefix="mcrun_play_"+item.first;
            //simulation codes
            add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=012000 mmdollers," _cont
                    " max_rigyears=200 years," _cont
                    "   max_wellgas=030000 bscf," _cont
                    " max_upsidegas=040000 bscf," _cont
                    "   max_welloil=020000 mmbo," _cont
                    " max_upsideoil=030000 mmbo"
                    );
            // preparing the report
            compute_dep.push_back(output_prefix+"_rig_level.it");
            strings report_dep = {output_prefix+"_stat_sum.dat", "template_play_report.tex"};
            //reporting
            //dependency graph
            report_dep.push_back(output_prefix+"_rig_level.it");
            report_dep.push_back(input_prefix+"_graph.pdf");
            report_dep.push_back(input_prefix+"_unidentified_graph.pdf");
            report_dep.push_back(output_prefix+"_nprospect_dist.pdf");
            report_dep.push_back(output_prefix+"_ncomp_dist.pdf");
            report_dep.push_back(output_prefix+"_cost_dist.pdf");
            report_dep.push_back(output_prefix+"_rig_years_dist.pdf");
            report_dep.push_back(output_prefix+"_identified_welloil_dist.pdf");
            report_dep.push_back(output_prefix+"_identified_wellgas_dist.pdf");
            report_dep.push_back(output_prefix+"_unidentified_welloil_dist.pdf");
            report_dep.push_back(output_prefix+"_unidentified_wellgas_dist.pdf");
            report_dep.push_back(output_prefix+"_assessed_welloil_dist.pdf");
            report_dep.push_back(output_prefix+"_assessed_wellgas_dist.pdf");
            report_dep.push_back(output_prefix+"_discovered_welloil_dist.pdf");
            report_dep.push_back(output_prefix+"_discovered_wellgas_dist.pdf");
            report_dep.push_back(output_prefix+"_identified_upsideoil_dist.pdf");
            report_dep.push_back(output_prefix+"_identified_upsidegas_dist.pdf");
            report_dep.push_back(output_prefix+"_unidentified_upsideoil_dist.pdf");
            report_dep.push_back(output_prefix+"_unidentified_upsidegas_dist.pdf");
            report_dep.push_back(output_prefix+"_assessed_upsideoil_dist.pdf");
            report_dep.push_back(output_prefix+"_assessed_upsidegas_dist.pdf");
            report_dep.push_back(output_prefix+"_discovered_upsideoil_dist.pdf");
            report_dep.push_back(output_prefix+"_discovered_upsidegas_dist.pdf");
            //report_dep.push_back(output_prefix+"_boe_dist.pdf");
            report_dep.push_back(output_prefix+"_findingcost_dist.pdf");
            report_dep.push_back(output_prefix+"_rig_gasfinding_rate_dist.pdf");
            report_dep.push_back(output_prefix+"_rig_oilfinding_rate_dist.pdf");
            /*
            report_dep.push_back(output_prefix+"_rig_oilfinding_rate_dist.pdf");
            report_dep.push_back(output_prefix+"_rig_boefinding_rate_dist.pdf");
            report_dep.push_back(output_prefix+"_riglevel_avgop_uncapped.png");
            report_dep.push_back(output_prefix+"_riglevel_avgop_capped.png");
            report_dep.push_back(output_prefix+"_campaign_oilvol_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_oilvol_capped.png");
            report_dep.push_back(output_prefix+"_campaign_gasvol_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_gasvol_capped.png");
            report_dep.push_back(output_prefix+"_campaign_findingcost_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_findingcost_capped.png");
            report_dep.push_back(output_prefix+"_campaign_cost_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_cost_capped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_years_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_years_capped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_oilfinding_rate_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_oilfinding_rate_capped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_gasfinding_rate_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_gasfinding_rate_capped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_boefinding_rate_uncapped.png");
            report_dep.push_back(output_prefix+"_campaign_rig_boefinding_rate_capped.png");
            */

            report_dep.push_back("map_"+input_prefix+".pdf");
            mf.add(input_prefix+"_spot_table.pdf", input_prefix+"_pal.t")
                //<<"attsqltool $< sql=\"select upper(name) as Name, rsvr_form_cd, pos_segment as POS, cast(gas_unrsk_rsvrs_p90 as int) as Gas_P90, cast(oil_unrsk_rsvrs_p90 as int), seg_pre_drill_clsf_grp as Class from @1 where geox_anal_id>0 and seg_hc_type='Gas' order by class desc \" output=tmp.t"
                <<"attsqltool $< sql=\"select upper(name) as Name, rsvr_form_cd, pos_segment as POS, cast(gas_unrsk_rsvrs_p90 as int) as Gas_P90, cast(oil_unrsk_rsvrs_p90 as int) as Oil_P90, seg_pre_drill_clsf_grp as Class from @1 where geox_anal_id>0 order by class desc \" output=tmp.t"
                //<<"attsqltool $< sql=\"select upper(name) as name, pos_segment as POS, status  from @1 where pros_lead_type!='PLANNING'\" output=tmp.t"
                //<<"att2latex input=tmp.t output=$@ headers=\"Name, Reservoir, POS, Gas \\$$P_{90}$$, Class\" headers2=\",,,[BSCF],\""
                <<"att2latex input=tmp.t output=tmp2.tex align=\"L{6.0cm}|C{1.3cm}| R{0.7cm}| R{1.2cm} |R{1.2cm}|C{2.3cm}\" headers=\"Name, \\small{Reservoir}, POS, \\small{Gas \\$$P_{90}$$}, \\small{Oil \\$$P_{90}$$}, Class\" headers2=\",,,\\small{[BSCF]},\\small{[MMBO]},\" super=false isdoc=true docwidth=16cm caption=\"Historical SPOT GeoX-based Assessments:\""
                <<"pdflatex tmp2.tex"
                <<"cp tmp2.pdf $@"
                ;
            report_dep.push_back(input_prefix+"_spot_table.pdf");
            report_dep.push_back(input_prefix+"_booked_table.pdf");
            report_dep.push_back(input_prefix+"_booked_wad_table.pdf");

            mf.add({input_prefix+"_cost_forecast_table.tex", input_prefix+"_cost_forecast_plots.pdf"}, {"forecast_cost.t", input_prefix+"_program_projects.t"})
                <<"attsqltool "+input_prefix+"_program_projects.t sql=\"select cost_cond, cost_dist from @1 where cost_cond not like '\%No\%Assigned\%' and type not like 'Seismic' group by cost_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=cost_dist o1=0 d1=0.1 n1=2500 output=tmp_cost_dist.g" 
                <<"atgraph tmp_cost_dist.g single=true title="" grid=false max1=50 autoexpand=true xlabel=\"Cost [\\$$MM]\" wheight=350 show_yaxis=false saveto="+input_prefix+"_cost_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select cost_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_cost_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_cost_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_cost_forecast_table.tex");
            report_dep.push_back(input_prefix+"_cost_forecast_plots.pdf");

            mf.add({input_prefix+"_rigdays_forecast_table.tex", input_prefix+"_rigdays_forecast_plots.pdf"}, {"forecast_rigdays.t", input_prefix+"_program_projects.t"})
                <<"attsqltool "+input_prefix+"_program_projects.t sql=\"select rigdays_cond, rigdays_dist from @1 where type like '\%Drilling\%' group by rigdays_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=rigdays_dist o1=0 d1=2 n1=1000 output=tmp_rigdays_dist.g" 
                <<"atgraph tmp_rigdays_dist.g single=true title="" grid=false max1=120 autoexpand=true xlabel=\"Rig Days\" wheight=350 show_yaxis=false saveto="+input_prefix+"_rigdays_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select rigdays_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_rigdays_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_rigdays_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_rigdays_forecast_table.tex");
            report_dep.push_back(input_prefix+"_rigdays_forecast_plots.pdf");

            mf.add({input_prefix+"_wellgasvol_forecast_table.tex", input_prefix+"_wellgasvol_forecast_plots.pdf"}, {"forecast_gas_wad.t", input_prefix+"_program_targets.t"})
                <<"attsetwhere cond=forecast_gas_wad.t condcol=cond valuecol=well_gas_bscf_dist newcol=tmp_wellgas_bscf_dist input="+input_prefix+"_program_targets.t output=tmp.t"
                <<"attsqltool tmp.t sql=\"select well_gasvolume_cond, tmp_wellgas_bscf_dist from @1 where is_nonhc='No' and well_gasvolume_cond not like '\%seismic\%' and (maturity like 'Identified\%' or maturity like 'Unidentified') and well_gasvolume_cond not like 'Oil Targets' group by well_gasvolume_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=tmp_wellgas_bscf_dist o1=0 d1=1 n1=1000 output=tmp.g" 
                <<"atgraph tmp.g single=true title="" grid=false xlabel=\"Gas Volume [BSCF]\" wheight=350 show_yaxis=false max1=2000 autoshrink=true saveto="+input_prefix+"_wellgasvol_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select well_gasvolume_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_wellgasvol_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_wellgasvol_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_wellgasvol_forecast_table.tex");
            report_dep.push_back(input_prefix+"_wellgasvol_forecast_plots.pdf");

            mf.add({input_prefix+"_welloilvol_forecast_table.tex", input_prefix+"_welloilvol_forecast_plots.pdf"}, {"forecast_oil_wad.t", input_prefix+"_program_targets.t"})
                <<"attsetwhere cond=forecast_oil_wad.t condcol=cond valuecol=well_oil_mmbo_dist newcol=tmp_welloil_mmbo_dist input="+input_prefix+"_program_targets.t output=tmp.t"
                <<"attsqltool tmp.t sql=\"select well_oilvolume_cond, tmp_welloil_mmbo_dist from @1 where is_nonhc='No' and well_oilvolume_cond not like '\%seismic\%' and (maturity like 'Identified\%' or maturity like 'Unidentified') and well_oilvolume_cond not like 'Gas Targets' group by well_oilvolume_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=tmp_welloil_mmbo_dist o1=0 d1=1 n1=1000 output=tmp.g" 
                <<"atgraph tmp.g single=true title="" grid=false xlabel=\"Oil Volume [MMBO]\" wheight=350 show_yaxis=false max1=2000 autoshrink=true saveto="+input_prefix+"_welloilvol_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select well_oilvolume_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_welloilvol_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_welloilvol_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_welloilvol_forecast_table.tex");
            report_dep.push_back(input_prefix+"_welloilvol_forecast_plots.pdf");

            mf.add({input_prefix+"_upsidegasvol_forecast_table.tex", input_prefix+"_upsidegasvol_forecast_plots.pdf"}, {"forecast_gas_fsd.t", input_prefix+"_program_targets.t"})
                <<"attsetwhere cond=forecast_gas_fsd.t condcol=cond valuecol=upside_gas_bscf_dist newcol=tmp_upsidegas_bscf_dist input="+input_prefix+"_program_targets.t output=tmp.t"
                <<"attsetwhere cond=forecast_gas_fsd.t condcol=cond valuecol=title newcol=upside_gasvolume_cond input=tmp.t output=tmp.t"
                <<"attsqltool tmp.t sql=\"select upside_gasvolume_cond, tmp_upsidegas_bscf_dist from @1 where is_nonhc='No' and upside_gasvolume_cond not like '\%seismic\%' and (maturity like 'Identified\%' or maturity like 'Unidentified') and upside_gasvolume_cond not like 'Oil Targets' group by upside_gasvolume_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=tmp_upsidegas_bscf_dist o1=0 d1=5 n1=1000 output=tmp.g" 
                <<"atgraph tmp.g single=true title="" grid=false xlabel=\"Gas Volume [BSCF]\" wheight=350 show_yaxis=false max1=2000  autoshrink=true saveto="+input_prefix+"_upsidegasvol_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select upside_gasvolume_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_upsidegasvol_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_upsidegasvol_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_upsidegasvol_forecast_table.tex");
            report_dep.push_back(input_prefix+"_upsidegasvol_forecast_plots.pdf");

            mf.add({input_prefix+"_upsideoilvol_forecast_table.tex", input_prefix+"_upsideoilvol_forecast_plots.pdf"}, {"forecast_oil_fsd.t", input_prefix+"_program_targets.t"})
                <<"attsetwhere cond=forecast_oil_fsd.t condcol=cond valuecol=upside_oil_mmbo_dist newcol=tmp_upsideoil_mmbo_dist input="+input_prefix+"_program_targets.t output=tmp.t"
                <<"attsetwhere cond=forecast_oil_fsd.t condcol=cond valuecol=title newcol=upside_oilvolume_cond input=tmp.t output=tmp.t"
                <<"attsqltool tmp.t sql=\"select upside_oilvolume_cond, tmp_upsideoil_mmbo_dist from @1 where is_nonhc='No' and upside_oilvolume_cond not like '\%seismic\%' and (maturity like 'Identified\%' or maturity like 'Unidentified') and upside_oilvolume_cond not like 'Gas Targets'  group by upside_oilvolume_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=tmp_upsideoil_mmbo_dist o1=0 d1=5 n1=1000 output=tmp.g" 
                <<"atgraph tmp.g single=true title="" grid=false xlabel=\"Oil Volume [MMBO]\" wheight=350 show_yaxis=false max1=2000  autoshrink=true saveto="+input_prefix+"_upsideoilvol_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select upside_oilvolume_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_upsideoilvol_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_upsideoilvol_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_upsideoilvol_forecast_table.tex");
            report_dep.push_back(input_prefix+"_upsideoilvol_forecast_plots.pdf");


            mf.add({input_prefix+"_pos_forecast_table.tex", input_prefix+"_pos_forecast_plots.pdf"}, {"forecast_pos.t", input_prefix+"_program_targets.t"})
                <<"attsqltool "+input_prefix+"_program_targets.t sql=\"select pos_cond, pos_dist from @1 where pos_cond not like '\%No\%Assigned\%' and (maturity like 'Identified\%' or maturity like 'Unidentified') group by pos_cond\" output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=pos_dist o1=0 d1=0.005 n1=202 output=tmp.g" 
                <<"atgraph tmp.g single=true title="" grid=false xlabel=\"POS\" wheight=350 show_yaxis=false max1=1.0 saveto="+input_prefix+"_pos_forecast_plots.pdf"
                <<"attsqltool tmp.t sql=\"select pos_cond as Condition, '\\includegraphics[width=0.18\\textwidth, page='|| rowid ||']{"+input_prefix+"_pos_forecast_plots}' as Distribution from @1\" output=tmp2.t"
                <<"att2latex input=tmp2.t align=\"R{5cm}|C{4.0cm}\" preprocess=\"yn\" headers=\"Condition, Probability Distribution\" output="+input_prefix+"_pos_forecast_table.tex"
                ;
            report_dep.push_back(input_prefix+"_pos_forecast_table.tex");
            report_dep.push_back(input_prefix+"_pos_forecast_plots.pdf");

            mf.add(input_prefix+"_sumfigures_table.tex", {input_prefix+"_portfolio.t", input_prefix+"_spot_active_portfolio.it"})
                <<"attsqltool tables=\""+input_prefix+"_portfolio.t "+input_prefix+"_spot_active_portfolio.lt\" sql=\""
                //"select 1 as ord, 'P&L' as title, (select count(*) from @2 where hc_type='GAS') as c"
                "select 1 as ord, 'P&L' as title, (select count(*) from @2 ) as c"
                " union "
                "select 2 as ord, 'Features', (select count(*) from @1 where maturity like 'Identified%') as c "
                "\" output=tmp.t"
                <<"attsqltool tmp.t sql=\"select title, c from @1 order by ord\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\",Count\" output=$@"
                ;
            report_dep.push_back(input_prefix+"_sumfigures_table.tex");

            mf.add(input_prefix+"_sumfeatures_pom_table.tex", {input_prefix+"_portfolio.t"})
                <<"attsqltool tables=\""+input_prefix+"_portfolio.t\" sql=\""
                "select 1 as ord, 'High' as POM, count(*) as c from @1 where maturity like 'IdentifiedHighPOM' "
                " union "
                "select 2 as ord, 'Mid' as POM, count(*) as c from @1 where maturity like 'IdentifiedMidPOM'  "
                " union "
                "select 3 as ord, 'Low' as POM, count(*) as c from @1 where maturity like 'IdentifiedLowPOM'  "
                "\" output=tmp.t"
                <<"attsqltool tmp.t sql=\"select POM, c from @1 order by ord\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\"POM,Count\" output=$@"
                ;
            report_dep.push_back(input_prefix+"_sumfeatures_pom_table.tex");

            mf.add(input_prefix+"_overlap_table.tex", {"aed_portfolio_overlaps.t", "ordered_plays.t"})
                <<"attsqltool tables=\"aed_portfolio_overlaps.t ordered_plays.t\" sql=\"select @2.title, @1.cnt from @1 inner join @2 on lower(@1.b)=lower(@2.play) where lower(a)=lower('"+item.first+"') order by seq\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\"Play,Count\" output=$@"
                ;
            report_dep.push_back(input_prefix+"_overlap_table.tex");
            report_dep.push_back(p.basin.strat_col+".pdf");

            //compiling the report
            auto & f = mf.add(input_prefix+"_interm_report.tex", report_dep);
                f<<"sed <template_play_report.tex 's/TEMPLATE_TITLE/"+p.title+"/g' | \\";
                f<<"sed 's/TEMPLATE_PLAYPREFIX/"+input_prefix+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_PDF/"+p.basin.strat_col+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_TOP_TRIM/"+p.basin.strat_col_top_trim+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_BOT_TRIM/"+p.basin.strat_col_bot_trim+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_TOP_CLIP/"+to_string(p.strat_col_top_clip)+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_BOT_CLIP/"+to_string(p.strat_col_bot_clip)+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_TOP_LABEL/"+p.strat_col_top_label+"/g' | \\";
                f<< "sed 's/TEMPLATE_STRATCOL_BOT_LABEL/"+p.strat_col_bot_label+"/g' | \\";
                f<< "sed 's/TEMPLATE_MAP/map_"+input_prefix+"/g' | \\";
                f<< "sed 's/TEMPLATE_PLAN/"+input_prefix+"_graph/g' | \\";
                f<< "sed 's/TEMPLATE_OILRF/"+p.oil_rf+"/g' | \\";
                f<< "sed 's/TEMPLATE_GASRF/"+p.gas_rf+"/g' | \\";
                
                f<< "sed 's/TEMPLATE_NPROSP_DIST/"+output_prefix+"_nprospect_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_NPROSP_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=nprospect_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NPROSP_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=nprospect_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NPROSP_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=nprospect_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NPROSP_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=nprospect_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_NCOMP_DIST/"+output_prefix+"_ncomp_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_NCOMP_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=ncomp_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NCOMP_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=ncomp_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NCOMP_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=ncomp_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_NCOMP_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=ncomp_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_cost_DIST/"+output_prefix+"_cost_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_cost_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=cost_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_cost_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=cost_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_cost_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=cost_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_cost_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=cost_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_RIGYEARS_DIST/"+output_prefix+"_rig_years_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_RIGYEARS_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_years_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_RIGYEARS_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_years_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_RIGYEARS_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_years_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_RIGYEARS_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_years_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_identified_welloil_DIST/"+output_prefix+"_identified_welloil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_identified_wellgas_DIST/"+output_prefix+"_identified_wellgas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_identified_welloil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_welloil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_welloil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_welloil_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_welloil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_welloil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_welloil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_welloil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_welloil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_welloil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_identified_wellboe_DIST/"+output_prefix+"_identified_wellboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_identified_wellboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_wellboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellboe_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_unidentified_welloil_DIST/"+output_prefix+"_unidentified_welloil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_unidentified_wellgas_DIST/"+output_prefix+"_unidentified_wellgas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_welloil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_welloil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_welloil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_welloil_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_welloil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_welloil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_welloil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_welloil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_welloil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_welloil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p90`/g\" | \\";
                
                
                f<< "sed 's/TEMPLATE_unidentified_wellboe_DIST/"+output_prefix+"_unidentified_wellboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_wellboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellboe_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_assessed_welloil_DIST/"+output_prefix+"_assessed_welloil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_assessed_wellgas_DIST/"+output_prefix+"_assessed_wellgas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_assessed_welloil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_welloil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_welloil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_welloil_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_welloil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_welloil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_welloil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_welloil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_welloil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_welloil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_assessed_wellboe_DIST/"+output_prefix+"_assessed_wellboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_wellboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellboe_p90`/g\" | \\";

                f<< "sed 's/TEMPLATE_identified_upsideoil_DIST/"+output_prefix+"_identified_upsideoil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_identified_upsidegas_DIST/"+output_prefix+"_identified_upsidegas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideoil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideoil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideoil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideoil_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideoil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideoil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideoil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideoil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideoil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideoil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_identified_upsideboe_DIST/"+output_prefix+"_identified_upsideboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_identified_upsideboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsideboe_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_unidentified_upsideoil_DIST/"+output_prefix+"_unidentified_upsideoil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_unidentified_upsidegas_DIST/"+output_prefix+"_unidentified_upsidegas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideoil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideoil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideoil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideoil_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideoil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideoil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideoil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideoil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideoil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideoil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_unidentified_upsideboe_DIST/"+output_prefix+"_unidentified_upsideboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_unidentified_upsideboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsideboe_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_assessed_upsideoil_DIST/"+output_prefix+"_assessed_upsideoil_dist/g' | \\";
                f<< "sed 's/TEMPLATE_assessed_upsidegas_DIST/"+output_prefix+"_assessed_upsidegas_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideoil_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideoil_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideoil_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideoil_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideoil_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideoil_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideoil_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideoil_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideoil_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideoil_sc_p90`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_assessed_upsideboe_DIST/"+output_prefix+"_assessed_upsideboe_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideboe_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideboe_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideboe_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideboe_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideboe_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideboe_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_assessed_upsideboe_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsideboe_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_findingcost_DIST/"+output_prefix+"_findingcost_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_findingcost_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=findingcost_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_findingcost_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=findingcost_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_findingcost_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=findingcost_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_findingcost_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=findingcost_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_rig_oilfinding_rate_DIST/"+output_prefix+"_rig_oilfinding_rate_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_rig_oilfinding_rate_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_oilfinding_rate_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_oilfinding_rate_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_oilfinding_rate_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_oilfinding_rate_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p90`/g\" | \\";
                
                f<< "sed 's/TEMPLATE_rig_gasfinding_rate_DIST/"+output_prefix+"_rig_gasfinding_rate_dist/g' | \\";
                f<< "sed \"s/TEMPLATE_rig_gasfinding_rate_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_gasfinding_rate_mode`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_gasfinding_rate_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p10`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_gasfinding_rate_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p50`/g\" | \\";
                f<< "sed \"s/TEMPLATE_rig_gasfinding_rate_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p90`/g\" | \\";

                    //"sed 's/TEMPLATE_rig_boefinding_rate_DIST/"+output_prefix+"_rig_boefinding_rate_dist/g' | \\"
                    //"sed \"s/TEMPLATE_rig_boefinding_rate_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_boefinding_rate_mode`/g\" | \\"
                    //"sed \"s/TEMPLATE_rig_boefinding_rate_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_boefinding_rate_p10`/g\" | \\"
                    //"sed \"s/TEMPLATE_rig_boefinding_rate_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_boefinding_rate_p50`/g\" | \\"
                    //"sed \"s/TEMPLATE_rig_boefinding_rate_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=rig_boefinding_rate_p90`/g\" | \\"

                f<< "sed 's/TEMPLATE_RIGLEVEL_avgop_uncapped_GRAPH/"+output_prefix+"_riglevel_avgop_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_RIGLEVEL_avgop_capped_GRAPH/"+output_prefix+"_riglevel_avgop_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_oilvol_uncapped_WIGPLOT/"+output_prefix+"_campaign_oilvol_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_oilvol_capped_WIGPLOT/"+output_prefix+"_campaign_oilvol_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_gasvol_uncapped_WIGPLOT/"+output_prefix+"_campaign_gasvol_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_gasvol_capped_WIGPLOT/"+output_prefix+"_campaign_gasvol_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_findingcost_uncapped_WIGPLOT/"+output_prefix+"_campaign_findingcost_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_findingcost_capped_WIGPLOT/"+output_prefix+"_campaign_findingcost_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_cost_uncapped_WIGPLOT/"+output_prefix+"_campaign_cost_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_cost_capped_WIGPLOT/"+output_prefix+"_campaign_cost_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_years_uncapped_WIGPLOT/"+output_prefix+"_campaign_rig_years_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_years_capped_WIGPLOT/"+output_prefix+"_campaign_rig_years_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_oilfinding_rate_uncapped_WIGPLOT/"+output_prefix+"_campaign_rig_oilfinding_rate_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_oilfinding_rate_capped_WIGPLOT/"+output_prefix+"_campaign_rig_oilfinding_rate_capped/g' | \\";
                
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_gasfinding_rate_uncapped_WIGPLOT/"+output_prefix+"_campaign_rig_gasfinding_rate_uncapped/g' | \\";
                f<< "sed 's/TEMPLATE_CAMPAIGN_rig_gasfinding_rate_capped_WIGPLOT/"+output_prefix+"_campaign_rig_gasfinding_rate_capped/g' | \\";

                    //"sed 's/TEMPLATE_CAMPAIGN_rig_boefinding_rate_uncapped_WIGPLOT/"+output_prefix+"_campaign_rig_boefinding_rate_uncapped/g' | \\"
                    //"sed 's/TEMPLATE_CAMPAIGN_rig_boefinding_rate_capped_WIGPLOT/"+output_prefix+"_campaign_rig_boefinding_rate_capped/g' | \\"
                    ;
                for(auto ref: p.rrad_references){
                    f<<"sed 's/\\&\\& TEMPLATE_RRAD_REFERENCES/ \\&"+ref+" \\\\\\\\ \\&\\& TEMPLATE_RRAD_REFERENCES/g' | \\";
                }
                f<<"sed 's/\\& TEMPLATE_RRAD_REFERENCES//g' | \\";
                for(auto ref: p.aed_references){
                    f<<"sed 's/\\&\\& TEMPLATE_AED_REFERENCES/ \\&"+ref+" \\\\\\\\ \\&\\& TEMPLATE_AED_REFERENCES/g' | \\";
                }
                f<<"sed 's/\\& TEMPLATE_AED_REFERENCES//g' | \\";
                for(auto ref: p.spot_references){
                    f<<"sed 's/\\&\\& TEMPLATE_SPOT_REFERENCES/ \\&"+ref+" \\\\\\\\ \\&\\& TEMPLATE_SPOT_REFERENCES/g' | \\";
                }
                f<<"sed 's/\\& TEMPLATE_SPOT_REFERENCES//g' | \\";
                for(auto ref: p.pvad_references){
                    f<<"sed 's/\\&\\& TEMPLATE_PVAD_REFERENCES/ \\&"+ref+" \\\\\\\\ \\&\\& TEMPLATE_PVAD_REFERENCES/g' | \\";
                }
                f<<"sed 's/\\& TEMPLATE_PVAD_REFERENCES//g' | \\";

                f<<"cat >$@";
                /*
            mf.add(input_prefix+"_report.pdf", {input_prefix+"_interm_report.tex", "template_latexdoc_top.tex", "template_latexdoc_bot.tex"})
                <<"cat template_latexdoc_top.tex "+input_prefix+"_interm_report.tex template_latexdoc_bot.tex>"+input_prefix+"_report.tex"
                <<"pdflatex "+input_prefix+"_report.tex"
                ;
            */
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////// 
        //create rigaoi based programs
        {
            for(auto aoi:aois){
                // recursive windowing to include dependenices from outside the AOI and assign them zero volume
                string input_prefix="rigaoi_"+aoi.prefix;
                landscape_portfolios.push_back(input_prefix);
                string input_norrad_prefix="rigaoi_norrad_"+aoi.prefix;
                string output_prefix="mcrun_rigaoi_"+aoi.prefix;
                string output_norrad_prefix="mcrun_rigaoi_norrad_"+aoi.prefix;
                mf.add(input_prefix+"_portfolio.t", {"rigaoi.it", "all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    //first window targets within the area
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where rigaoi='"+aoi.prefix+"'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    //recursive search for dependencies outside AOI
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                    ;
                mf.add(input_norrad_prefix+"_portfolio.t", {"rigaoi.it", "all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    //first window targets within the area and execlude unidentified/RRAD concepts for a more representative assessment of required resources
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where rigaoi='"+aoi.prefix+"' and maturity <> 'Unidentified'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    //recursive search for dependencies outside AOI
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                    ;
                mf.add(input_prefix+"_area.shp", "rigaoi.it")
                    <<"atitselect input=$< where=\"rigaoi='"+aoi.prefix+"'\" output=tmp.it"
                    <<"atit2shpfile input=tmp.it x=x y=y poly=true output=$@";
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=030000 mmdollers," _cont
                    " max_rigyears=150 years," _cont
                    "   max_wellgas=040000 bscf," _cont
                    " max_upsidegas=065000 bscf," _cont
                    "   max_welloil=020000 mmbo," _cont
                    " max_upsideoil=030000 mmbo"
                    , "rigaoi<>'"+aoi.prefix+"'");
                //covert portfolio to program
                add_mcrun(input_norrad_prefix, output_norrad_prefix, nitr, false,
                    " max_cost=020000 mmdollers," _cont
                    " max_rigyears=150 years," _cont
                    "   max_wellgas=040000 bscf," _cont
                    " max_upsidegas=065000 bscf," _cont
                    "   max_welloil=020000 mmbo," _cont
                    " max_upsideoil=030000 mmbo"
                    , "rigaoi<>'"+aoi.prefix+"'");
                compute_dep.push_back(output_prefix+"_rig_level.it");
                compute_dep.push_back(output_norrad_prefix+"_rig_level.it");
                //creating a plot
                mf.add( input_prefix+"_graph.pdf" , {input_prefix+"_program_targets.t"})
                    <<"attsqltool $< sql=\"select * from @1 where maturity not like 'Unidentified'\" output=tmp.t"
                    <<"atcontingency_graph input=tmp.t output=tmp.txt title='Dependency Graph for "+aoi.title+"'"
                    <<"dot -Tpdf tmp.txt -o $@"
                        ;
                //another run without rrad inputs for required resources (capital, cost, finding cost, etc.)

                //end of run 2
                //reporting
                strings report_dep;

                report_dep.push_back(output_prefix+"_stat_sum.dat");
                report_dep.push_back("template_area_report.tex");
                report_dep.push_back("map_"+input_prefix+".pdf");
                report_dep.push_back(input_prefix+"_graph.pdf");
                report_dep.push_back(output_prefix+"_identified_wellgas_dist.pdf");
                report_dep.push_back(output_prefix+"_identified_welloil_dist.pdf");
                report_dep.push_back(output_prefix+"_unidentified_wellgas_dist.pdf");
                report_dep.push_back(output_prefix+"_unidentified_welloil_dist.pdf");
                report_dep.push_back(output_prefix+"_assessed_wellgas_dist.pdf");
                report_dep.push_back(output_prefix+"_assessed_welloil_dist.pdf");
                report_dep.push_back(output_prefix+"_discovered_wellgas_dist.pdf");
                report_dep.push_back(output_prefix+"_discovered_welloil_dist.pdf");
                report_dep.push_back(output_prefix+"_identified_upsidegas_dist.pdf");
                report_dep.push_back(output_prefix+"_identified_upsideoil_dist.pdf");
                report_dep.push_back(output_prefix+"_unidentified_upsidegas_dist.pdf");
                report_dep.push_back(output_prefix+"_unidentified_upsideoil_dist.pdf");
                report_dep.push_back(output_prefix+"_assessed_upsidegas_dist.pdf");
                report_dep.push_back(output_prefix+"_assessed_upsideoil_dist.pdf");
                report_dep.push_back(output_prefix+"_discovered_upsidegas_dist.pdf");
                report_dep.push_back(output_prefix+"_discovered_upsideoil_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_stat_sum.dat");
                report_dep.push_back(output_norrad_prefix+"_findingcost_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_rig_gasfinding_rate_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_rig_oilfinding_rate_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_ncomp_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_cost_dist.pdf");
                report_dep.push_back(output_norrad_prefix+"_rig_years_dist.pdf");

                //compiling the report
                auto & f = mf.add(input_prefix+"_interm_report.tex", report_dep);
                f<<"sed <template_area_report.tex 's/TEMPLATE_TITLE/"+aoi.title+"/g' | \\"
                    "sed 's/TEMPLATE_MAP/map_"+input_prefix+"/g' | \\\n"
                    "sed 's/TEMPLATE_PLAN/"+input_prefix+"_graph/g' | \\"
                    //
                    "sed 's/TEMPLATE_identified_wellgas_DIST/"+output_prefix+"_identified_wellgas_dist/g' | \\"
                    "sed \"s/TEMPLATE_identified_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_wellgas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_unidentified_wellgas_DIST/"+output_prefix+"_unidentified_wellgas_dist/g' | \\"
                    "sed \"s/TEMPLATE_unidentified_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_wellgas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_assessed_wellgas_DIST/"+output_prefix+"_assessed_wellgas_dist/g' | \\"
                    "sed \"s/TEMPLATE_assessed_wellgas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_wellgas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_wellgas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_wellgas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_wellgas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_wellgas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_identified_upsidegas_DIST/"+output_prefix+"_identified_upsidegas_dist/g' | \\"
                    "sed \"s/TEMPLATE_identified_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_identified_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=identified_upsidegas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_unidentified_upsidegas_DIST/"+output_prefix+"_unidentified_upsidegas_dist/g' | \\"
                    "sed \"s/TEMPLATE_unidentified_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_unidentified_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=unidentified_upsidegas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_assessed_upsidegas_DIST/"+output_prefix+"_assessed_upsidegas_dist/g' | \\"
                    "sed \"s/TEMPLATE_assessed_upsidegas_POS/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_pos format=\"\%.1f\" number=true mult=100`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_upsidegas_MODE/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_upsidegas_P10/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_upsidegas_P50/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_assessed_upsidegas_P90/`atgetval input="+output_prefix+"_stat_sum.dat var=assessed_upsidegas_sc_p90`/g\" | \\"
                    //
                    "sed 's/TEMPLATE_findingcost_DIST/"+output_norrad_prefix+"_findingcost_dist/g' | \\"
                    "sed \"s/TEMPLATE_findingcost_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=findingcost_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_findingcost_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=findingcost_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_findingcost_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=findingcost_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_findingcost_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=findingcost_p90`/g\" | \\"

                    "sed 's/TEMPLATE_rig_gasfinding_rate_DIST/"+output_norrad_prefix+"_rig_gasfinding_rate_dist/g' | \\"
                    "sed \"s/TEMPLATE_rig_gasfinding_rate_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_gasfinding_rate_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_gasfinding_rate_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_gasfinding_rate_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_gasfinding_rate_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_gasfinding_rate_p90`/g\" | \\"

                    "sed 's/TEMPLATE_rig_oilfinding_rate_DIST/"+output_norrad_prefix+"_rig_oilfinding_rate_dist/g' | \\"
                    "sed \"s/TEMPLATE_rig_oilfinding_rate_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_oilfinding_rate_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_oilfinding_rate_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_oilfinding_rate_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_rig_oilfinding_rate_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_oilfinding_rate_p90`/g\" | \\"

                    "sed 's/TEMPLATE_NCOMP_DIST/"+output_norrad_prefix+"_ncomp_dist/g' | \\"
                    "sed \"s/TEMPLATE_NCOMP_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=ncomp_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_NCOMP_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=ncomp_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_NCOMP_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=ncomp_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_NCOMP_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=ncomp_p90`/g\" | \\"

                    "sed 's/TEMPLATE_cost_DIST/"+output_norrad_prefix+"_cost_dist/g' | \\"
                    "sed \"s/TEMPLATE_cost_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=cost_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_cost_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=cost_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_cost_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=cost_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_cost_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=cost_p90`/g\" | \\"

                    "sed 's/TEMPLATE_RIGYEARS_DIST/"+output_norrad_prefix+"_rig_years_dist/g' | \\"
                    "sed \"s/TEMPLATE_RIGYEARS_MODE/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_years_mode`/g\" | \\"
                    "sed \"s/TEMPLATE_RIGYEARS_P10/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_years_p10`/g\" | \\"
                    "sed \"s/TEMPLATE_RIGYEARS_P50/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_years_p50`/g\" | \\"
                    "sed \"s/TEMPLATE_RIGYEARS_P90/`atgetval input="+output_norrad_prefix+"_stat_sum.dat var=rig_years_p90`/g\" | \\"

                    ;
                f<<"cat >$@";
            }

        }
        //PRMS runs
        {
            {
                string prefix="2p3p";
                string input_prefix="prms_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(prmstype) like 'DELINEATION'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(prmstype)<>upper('DELINEATION')\" output=$@"
                    ;
            }
            {
                string prefix="pros";
                string input_prefix="prms_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(prmstype) like 'PROSPECTIVE'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(prmstype)<>upper('PROSPECTIVE')\" output=$@"
                    ;
            }
            {
                string prefix="ctgt";
                string input_prefix="prms_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(prmstype) like upper('CONTINGENT')\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(prmstype)<>upper('CONTINGENT')\" output=$@"
                    ;
            }
            for(auto prms_type:prms_types){
                string prefix=prms_type;
                string input_prefix="prms_"+prefix;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_"+input_prefix;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=50000 mmdollers," _cont
                    " max_rigyears=500 years," _cont
                    "   max_wellgas=120000 bscf," _cont
                    " max_upsidegas=200000 bscf," _cont
                    "   max_welloil=060000 mmbo," _cont
                    " max_upsideoil=080000 mmbo"
                    , "zero_flag=1", {"zero_flag"});
                compute_dep.push_back(output_prefix+"_stat_sum.dat");
                //creating a plot
                mf.add( input_prefix+"_graph.pdf" , {input_prefix+"_program_targets.t"})
                    <<"atcontingency_graph input=$< output=tmp.txt title='Dependency Graph for 2P/3P PRMS Class'"
                    <<"dot -Tpdf tmp.txt -o $@"
                    ;
            }
        }
        //Area Maturity runs
        {
            {
                string prefix="mature";
                string input_prefix="area_maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(area_maturity) like 'MATURE'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(area_maturity)<>upper('MATURE')\" output=$@"
                    ;
            }
            {
                string prefix="emerging";
                string input_prefix="area_maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(area_maturity) like 'EMERGING'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(area_maturity)<>upper('EMERGING')\" output=$@"
                    ;
            }
            {
                string prefix="frontier";
                string input_prefix="area_maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(area_maturity) like upper('FRONTIER')\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"upper(area_maturity)<>upper('FRONTIER')\" output=$@"
                    ;
            }
            {
                string prefix="frontier_n_emerging";
                string input_prefix="area_maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(area_maturity) in ('FRONTIER', 'EMERGING')\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"not upper(area_maturity) in ('FRONTIER', 'EMERGING')\" output=$@"
                    ;
            }
            for(auto amaturity:vector<string>{"mature", "emerging", "frontier", "frontier_n_emerging"}){
                string prefix=amaturity;
                string input_prefix="area_maturity_"+prefix;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_"+input_prefix;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=80000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=240000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                    , "zero_flag=1", {"zero_flag"});
                compute_dep.push_back(output_prefix+"_stat_sum.dat");
            }
        }
        //maturity run for testing
        {
            {
                string prefix="concluded";
                string input_prefix="maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where upper(stage) in ('CONCLUDED') \" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=130000 mmdollers," _cont
                    " max_rigyears=1400 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=200000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                    );
            }
            // Test for trying to come up with aggregate that is most similar to SPOT aggregats (sum of risked mean)
            // To reach this change  settings to relfect the following:
            //    - nwell=1 (no need for delineations; The first/next well is enough to get the whole volume!)
            //    - remove dependencies
            //    - p_comm =100%, such that POS=Pg*Pcomm P(Dependency is successful).

            {
                string prefix="assessed";
                string input_prefix="maturity_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t", "assigned_pal_gas.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t assigned_pal_gas.t\" sql=\"select * from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play where @2.pros_status_type like 'active'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    <<"attupdate $@ hashtags=\"maturity||' '||stage\""
                    <<"attupdate $@ maturity=\"'Assessed'\""
                    <<"attupdate $@ stage=\"'Proposed'\""
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=100000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=50000 bscf," _cont
                    " max_upsidegas=100000 bscf," _cont
                    "   max_welloil=50000 mmbo," _cont
                    " max_upsideoil=100000 mmbo"
                    );
            }
        }
        //Venture
        {
            for(auto & venture: ventures){
                string prefix= venture.prefix;
                string input_prefix="venture_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t", "plan_vplan_baseline.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where "+venture.filter+"\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"not ("+venture.filter+")\" output=tmp2.t"
                    <<"attsqltool tables=\"tmp2.t plan_vplan_baseline.t\" sql=\"select @1.* from @1 where not id in (select id from @2)\" output=tmp3.t"
                    <<"attupdate input=tmp3.t dep=\"'NONE'\" where=\"not dep in (select id from input)\" output=$@"
                    ;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=040000 mmdollers," _cont
                    " max_rigyears=350 years," _cont
                    "   max_wellgas=050000 bscf," _cont
                    " max_upsidegas=065000 bscf," _cont
                    "   max_welloil=050000 mmbo," _cont
                    " max_upsideoil=060000 mmbo" _cont
                    " maxnrig="+to_string(venture.max_nrig)
                    +" ncampaign=50"
                    , "zero_flag=1", {"zero_flag"});
                compute_dep.push_back(output_prefix+"_rig_level.it");
            }
        }
        //Synergy runs
        {
            {
                string prefix="all";
                string input_prefix="synergy_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t"})
                    <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where hashtags like '\%synergy\%'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attaddcol tmp.t zero_flag=\"not hashtags like '\%synergy\%' \" output=$@"
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=100000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=50000 bscf," _cont
                    " max_upsidegas=100000 bscf," _cont
                    "   max_welloil=50000 mmbo," _cont
                    " max_upsideoil=100000 mmbo"
                    );
            }
        }
        //Single Itration Test Runs
        if(generate_testing_report){
            strings tests = {"tsta", "tstb", "tstc", "tstd"};
            //test f
            for(int i=0; i<tstf_ntest; i++){
                tests.push_back("tstf"+to_string(i));
            }
            //test g
            for(int i=2005; i<current_year; i++){
                tests.push_back("tstg"+to_string(i));
            }
            //test m
            for(auto play: seq_plays){
                tests.push_back("tstm_"+play);
            }
            for(auto prefix:tests){
                string input_prefix="test_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"test_portfolio.t"})
                    <<"attsqltool test_portfolio.t sql=\"select * from @1 where "+hashtag_find(prefix)+"\" output=$@"
                    ;
                //Note: Number of iterations is 1 ONE
                add_mcrun(input_prefix, output_prefix, 1, false,
                    " max_cost=100000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=100000 bscf," _cont
                    " max_upsidegas=100000 bscf," _cont
                    "   max_welloil=080000 mmbo," _cont
                    " max_upsideoil=100000 mmbo"
                    );
            }
        }
        //Multiple-Itrations Test Runs
        if(generate_testing_report){
            strings tests = {"tste"} ;
            ////test f
            //for(int i=2005; i<current_year; i++){
            //    tests.push_back("tste"+to_string(i));
            //}
            //test h
            for(int i=0; i<tstf_ntest; i++){
                tests.push_back("tsth"+to_string(i));
            }
            //test i
            for(int i=2013; i<current_year; i++){
                tests.push_back("tsti"+to_string(i));
            }
            for(auto prefix:tests){
                string input_prefix="test_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"test_portfolio.t"})
                    <<"attsqltool test_portfolio.t sql=\"select * from @1 where "+hashtag_find(prefix)+"\" output=$@"
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=1000 mmdollers," _cont
                    " max_rigyears=50 years," _cont
                    "   max_wellgas=005000 bscf," _cont
                    " max_upsidegas=010000 bscf," _cont
                    "   max_welloil=005000 mmbo," _cont
                    " max_upsideoil=010000 mmbo"
                    );
            }
        }
        //Multiple-Itrations Test Runs with default P-Comm of 100%%
        if(generate_testing_report){
            strings tests;
            //test J
            for(int i=2013; i<current_year; i++){
                tests.push_back("tstj"+to_string(i));
            }
            for(auto prefix:tests){
                string input_prefix="test_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"test_portfolio.t"})
                    <<"attsqltool test_portfolio.t sql=\"select * from @1 where "+hashtag_find(prefix)+"\" output=$@"
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=1000 mmdollers," _cont
                    " max_rigyears=50 years," _cont
                    "   max_wellgas=005000 bscf," _cont
                    " max_upsidegas=010000 bscf," _cont
                    "   max_welloil=005000 mmbo," _cont
                    " max_upsideoil=010000 mmbo"
                    );
            }
        }
        //Running test by play & prms type. This is to invistigate source of mis-prediction
        if(generate_testing_report){
            strings tests;
            //test K
            for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                for(auto play: seq_plays){
                    string test_name = "tstk_"+play+"_"+wcdel;
                    tests.push_back(test_name);
                }
            }
            for(auto prefix:tests){
                string input_prefix="test_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"test_portfolio.t"})
                    <<"attsqltool test_portfolio.t sql=\"select * from @1 where "+hashtag_find(prefix)+"\" output=$@"
                    ;
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=500 mmdollers," _cont
                    " max_rigyears=30 years," _cont
                    "   max_wellgas=005000 bscf," _cont
                    " max_upsidegas=010000 bscf," _cont
                    "   max_welloil=005000 mmbo," _cont
                    " max_upsideoil=010000 mmbo"
                    );
            }
        }
        ///////////////////////////////////////////
        // Fixing extents
        {
            mf.add("test_mcrun_all_axes_extent", "mcrun_all_assessed_upsidegas_dist.g")
                <<HELP("Check the number of samples is enough to capture the high case; run with trial option set to true in the makefile.cpp")
                <<"atcat2 list=\"mcrun_all_assessed_upsidegas_dist.g mcrun_all_identified_upsidegas_dist.g mcrun_all_unidentified_upsidegas_dist.g\" output=tmp.g && atmath tmp2.g=\"(i1()>0)*tmp.g\" && atgraph tmp2.g title=upsidegas"
                <<"atcat2 list=\"mcrun_all_assessed_upsideoil_dist.g mcrun_all_identified_upsideoil_dist.g mcrun_all_unidentified_upsideoil_dist.g\" output=tmp.g && atmath tmp2.g=\"(i1()>0)*tmp.g\" && atgraph tmp2.g title=upsideoil"
                <<"atcat2 list=\"mcrun_all_assessed_wellgas_dist.g mcrun_all_identified_wellgas_dist.g mcrun_all_unidentified_wellgas_dist.g\" output=tmp.g && atmath tmp2.g=\"(i1()>0)*tmp.g\" && atgraph tmp2.g title=wellgas"
                <<"atcat2 list=\"mcrun_all_assessed_welloil_dist.g mcrun_all_identified_welloil_dist.g mcrun_all_unidentified_welloil_dist.g\" output=tmp.g && atmath tmp2.g=\"(i1()>0)*tmp.g\" && atgraph tmp2.g title=welloil"
                <<"atgraph mcrun_all_rig_years_dist.g title=rigyears"
                <<"atgraph mcrun_all_cost_dist.g title=cost "
                ;
        }
        {
            strings dep;
            for(auto & item : plays){
                string input_prefix="play_"+item.first;
                auto & p = item.second;
                string output_prefix="mcrun_play_"+item.first;
                // preparing the report
                dep.push_back(output_prefix+"_rig_level.it");
            }
            auto & f = mf.add("test_mcrun_play_axes_extent", dep);
            f<<HELP("Check the number of samples is enough to capture the high case used for play simulations");
            bool first = true;
            for(auto & play: seq_plays){
                string prefix="play_"+play;
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsidegas_dist.g mcrun_"+prefix+"_identified_upsidegas_dist.g mcrun_"+prefix+"_unidentified_upsidegas_dist.g\" output=tmp_upsidegas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsideoil_dist.g mcrun_"+prefix+"_identified_upsideoil_dist.g mcrun_"+prefix+"_unidentified_upsideoil_dist.g\" output=tmp_upsideoil1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_wellgas_dist.g mcrun_"+prefix+"_identified_wellgas_dist.g mcrun_"+prefix+"_unidentified_wellgas_dist.g\" output=tmp_wellgas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_welloil_dist.g mcrun_"+prefix+"_identified_welloil_dist.g mcrun_"+prefix+"_unidentified_welloil_dist.g\" output=tmp_welloil1.g";
                if(first){
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas1.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil1.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas1.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil1.g";
                    f<<"atmath tmp_rigyears2.g=mcrun_"+prefix+"_rig_years_dist.g";
                    f<<"atmath tmp_cost2.g=mcrun_"+prefix+"_cost_dist.g";
                    first=false;
                }else{
                    f<<"atcat2 list=\"tmp_upsidegas2.g tmp_upsidegas1.g\" output=tmp_upsidegas3.g";
                    f<<"atcat2 list=\"tmp_upsideoil2.g tmp_upsideoil1.g\" output=tmp_upsideoil3.g";
                    f<<"atcat2 list=\"tmp_wellgas2.g tmp_wellgas1.g\" output=tmp_wellgas3.g";
                    f<<"atcat2 list=\"tmp_welloil2.g tmp_welloil1.g\" output=tmp_welloil3.g";
                    f<<"atcat2 list=\"tmp_rigyears2.g mcrun_"+prefix+"_rig_years_dist.g\" output=tmp_rigyears3.g";
                    f<<"atcat2 list=\"tmp_cost2.g mcrun_"+prefix+"_cost_dist.g\" output=tmp_cost3.g";
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas3.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil3.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas3.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil3.g";
                    f<<"atmath tmp_rigyears2.g=tmp_rigyears3.g";
                    f<<"atmath tmp_cost2.g=tmp_cost3.g";
                }
            }
            f<<"atmath tmp.g=\"tmp_upsidegas2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=upsidegas max2=0.01 tbt_scale=true";
            f<<"atmath tmp.g=\"tmp_upsideoil2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=upsideoil max2=0.01 tbt_scale=true";
            f<<"atmath tmp.g=\"tmp_wellgas2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=wellgas max2=0.01 tbt_scale=true";
            f<<"atmath tmp.g=\"tmp_welloil2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=welloil max2=0.01 tbt_scale=true";
            f<<"atmath tmp.g=\"tmp_rigyears2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=rigyears max2=0.01 tbt_scale=true";
            f<<"atmath tmp.g=\"tmp_cost2.g\" && atgraph tmp.g tbt_scale=1 vsize=10 title=cost max2=0.01 tbt_scale=true";
            /*
            f<<"atmath tmp.g=\"(i1()>0)*tmp_upsidegas2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=upsidegas max2=0.01";
            f<<"atmath tmp.g=\"(i1()>0)*tmp_upsideoil2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=upsideoil max2=0.01";
            f<<"atmath tmp.g=\"(i1()>0)*tmp_wellgas2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=wellgas max2=0.01";
            f<<"atmath tmp.g=\"(i1()>0)*tmp_welloil2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=welloil max2=0.01";
            f<<"atmath tmp.g=\"(i1()>0)*tmp_rigyears2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=rigyears max2=0.01";
            f<<"atmath tmp.g=\"(i1()>0)*tmp_cost2.g\" && atwigplot tmp.g tbt_scale=1 vsize=10 title=cost max2=0.01";
            */
        }
        {
            strings dep;
            for(auto aoi:aois){
                string input_prefix="rigaoi_"+aoi.prefix;
                string output_prefix="mcrun_rigaoi_"+aoi.prefix;
                // preparing the report
                dep.push_back(output_prefix+"_rig_level.it");
            }
            auto & f = mf.add("test_mcrun_rigaoi_axes_extent", dep);
            f<<HELP("Check the number of samples is enough to capture the high case used for rig-aoi simulations");
            bool first = true;
            for(auto aoi:aois){
                string prefix="rigaoi_"+aoi.prefix;
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsidegas_dist.g mcrun_"+prefix+"_identified_upsidegas_dist.g mcrun_"+prefix+"_unidentified_upsidegas_dist.g\" output=tmp_upsidegas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsideoil_dist.g mcrun_"+prefix+"_identified_upsideoil_dist.g mcrun_"+prefix+"_unidentified_upsideoil_dist.g\" output=tmp_upsideoil1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_wellgas_dist.g mcrun_"+prefix+"_identified_wellgas_dist.g mcrun_"+prefix+"_unidentified_wellgas_dist.g\" output=tmp_wellgas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_welloil_dist.g mcrun_"+prefix+"_identified_welloil_dist.g mcrun_"+prefix+"_unidentified_welloil_dist.g\" output=tmp_welloil1.g";
                if(first){
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas1.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil1.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas1.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil1.g";
                    f<<"atmath tmp_rigyears2.g=mcrun_"+prefix+"_rig_years_dist.g";
                    f<<"atmath tmp_cost2.g=mcrun_"+prefix+"_cost_dist.g";
                    first=false;
                }else{
                    f<<"atcat2 list=\"tmp_upsidegas2.g tmp_upsidegas1.g\" output=tmp_upsidegas3.g";
                    f<<"atcat2 list=\"tmp_upsideoil2.g tmp_upsideoil1.g\" output=tmp_upsideoil3.g";
                    f<<"atcat2 list=\"tmp_wellgas2.g tmp_wellgas1.g\" output=tmp_wellgas3.g";
                    f<<"atcat2 list=\"tmp_welloil2.g tmp_welloil1.g\" output=tmp_welloil3.g";
                    f<<"atcat2 list=\"tmp_rigyears2.g mcrun_"+prefix+"_rig_years_dist.g\" output=tmp_rigyears3.g";
                    f<<"atcat2 list=\"tmp_cost2.g mcrun_"+prefix+"_cost_dist.g\" output=tmp_cost3.g";
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas3.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil3.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas3.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil3.g";
                    f<<"atmath tmp_rigyears2.g=tmp_rigyears3.g";
                    f<<"atmath tmp_cost2.g=tmp_cost3.g";
                }
            }
            f<<"atgraph tmp_upsidegas2.g title=upsidegas max2=0.01";
            f<<"atgraph tmp_upsideoil2.g title=upsideoil max2=0.01";
            f<<"atgraph tmp_wellgas2.g title=wellgas max2=0.01";
            f<<"atgraph tmp_welloil2.g title=welloil max2=0.01";
            f<<"atgraph tmp_rigyears2.g title=rigyears max2=0.01";
            f<<"atgraph tmp_cost2.g title=cost max2=0.01";
        }
        {
            strings dep;
            for(auto prms_type:prms_types){
                string prefix=prms_type;
                string input_prefix="prms_"+prefix;
                string output_prefix="mcrun_"+input_prefix;
                dep.push_back(output_prefix+"_rig_level.it");
            }
            auto & f = mf.add("test_mcrun_prms_axes_extent", dep);
            f<<HELP("Check the number of samples is enough to capture the high case used for PRMS-Class simulations");
            bool first = true;
            for(auto prms_type:prms_types){
                string prefix="prms_"+prms_type;
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsidegas_dist.g mcrun_"+prefix+"_identified_upsidegas_dist.g mcrun_"+prefix+"_unidentified_upsidegas_dist.g\" output=tmp_upsidegas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_upsideoil_dist.g mcrun_"+prefix+"_identified_upsideoil_dist.g mcrun_"+prefix+"_unidentified_upsideoil_dist.g\" output=tmp_upsideoil1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_wellgas_dist.g mcrun_"+prefix+"_identified_wellgas_dist.g mcrun_"+prefix+"_unidentified_wellgas_dist.g\" output=tmp_wellgas1.g";
                f<<"atcat2 list=\"mcrun_"+prefix+"_assessed_welloil_dist.g mcrun_"+prefix+"_identified_welloil_dist.g mcrun_"+prefix+"_unidentified_welloil_dist.g\" output=tmp_welloil1.g";
                if(first){
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas1.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil1.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas1.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil1.g";
                    f<<"atmath tmp_rigyears2.g=mcrun_"+prefix+"_rig_years_dist.g";
                    f<<"atmath tmp_cost2.g=mcrun_"+prefix+"_cost_dist.g";
                    first=false;
                }else{
                    f<<"atcat2 list=\"tmp_upsidegas2.g tmp_upsidegas1.g\" output=tmp_upsidegas3.g";
                    f<<"atcat2 list=\"tmp_upsideoil2.g tmp_upsideoil1.g\" output=tmp_upsideoil3.g";
                    f<<"atcat2 list=\"tmp_wellgas2.g tmp_wellgas1.g\" output=tmp_wellgas3.g";
                    f<<"atcat2 list=\"tmp_welloil2.g tmp_welloil1.g\" output=tmp_welloil3.g";
                    f<<"atcat2 list=\"tmp_rigyears2.g mcrun_"+prefix+"_rig_years_dist.g\" output=tmp_rigyears3.g";
                    f<<"atcat2 list=\"tmp_cost2.g mcrun_"+prefix+"_cost_dist.g\" output=tmp_cost3.g";
                    f<<"atmath tmp_upsidegas2.g=tmp_upsidegas3.g";
                    f<<"atmath tmp_upsideoil2.g=tmp_upsideoil3.g";
                    f<<"atmath tmp_wellgas2.g=tmp_wellgas3.g";
                    f<<"atmath tmp_welloil2.g=tmp_welloil3.g";
                    f<<"atmath tmp_rigyears2.g=tmp_rigyears3.g";
                    f<<"atmath tmp_cost2.g=tmp_cost3.g";
                }
            }
            f<<"atgraph tmp_upsidegas2.g title=upsidegas max2=0.01";
            f<<"atgraph tmp_upsideoil2.g title=upsideoil max2=0.01";
            f<<"atgraph tmp_wellgas2.g title=wellgas max2=0.01";
            f<<"atgraph tmp_welloil2.g title=welloil max2=0.01";
            f<<"atgraph tmp_rigyears2.g title=rigyears max2=0.01";
            f<<"atgraph tmp_cost2.g title=cost max2=0.01";
        }
        mf.add("precompute_tables", precompute_tables)
            <<HELP("prepare the input tables for all of the forecasts and Monte-Carlo simulations");
        mf.add("precompute", precompute_list)
            <<HELP("prepare the input forecast data for all of the Monte-Carlo simulations needed for the report.");
        mf.add("compute_all", compute_dep)
            <<HELP("run all Monte-Carlo simulations needed for the report.");
    }
    void compile_testing_report(){
        {
            strings dep;
            dep.push_back("template_testing_report.tex");
            dep.push_back("mcrun_test_tsta_ytf_upsidegas_dist.pdf");
            dep.push_back("mcrun_test_tsta_ytf_upsideoil_dist.pdf");
            dep.push_back("mcrun_test_tsta_rig_gasfinding_rate_dist.pdf");
            dep.push_back("mcrun_test_tsta_rig_oilfinding_rate_dist.pdf");
            dep.push_back("mcrun_test_tsta_findingcost_dist.pdf");
            dep.push_back("mcrun_test_tsta_stat_sum.dat");
            //test b
            mf.add("mcrun_test_tstb_compare_upsidegas_dist.pdf", {"forecast_gas_fsd.t", "mcrun_test_tstb_ytf_upsidegas_dist.g"})
                <<"attsqltool forecast_gas_fsd.t sql=\"select upside_gas_bscf_dist as dist from @1 where cond like '\%tstb\%'"
                " union select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"mcrun_test_tstb_ytf_upsidegas_dist.g\\\"}'\""
                " output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=dist o1=0 d1=10 n1=2000 output=tmp.g"
                <<"atgraph tmp.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Ideal vs. Computed Upside Gas Volume' xlabel='Volume [BSCF]' saveto=$@" ;
                ;
            dep.push_back("mcrun_test_tstb_compare_upsidegas_dist.pdf");

            mf.add("mcrun_test_tstb_compare_wellgas_dist.pdf", {"forecast_gas_wad.t", "mcrun_test_tstb_ytf_wellgas_dist.g"})
                <<"attsqltool forecast_gas_wad.t sql=\"select well_gas_bscf_dist as dist from @1 where cond like '\%tstb\%'"
                " union select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"mcrun_test_tstb_ytf_wellgas_dist.g\\\"}'\""
                " output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=dist o1=0 d1=10 n1=2000 output=tmp.g"
                <<"atgraph tmp.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Ideal vs. Computed Well-Add Gas Volume' xlabel='Volume [BSCF]' saveto=$@" ;
                ;
            dep.push_back("mcrun_test_tstb_compare_wellgas_dist.pdf");
            //test c
            mf.add("mcrun_test_tstc_compare_upsidegas_dist.pdf", {"forecast_gas_fsd.t", "mcrun_test_tstc_ytf_upsidegas_dist.g"})
                <<"attsqltool forecast_gas_fsd.t sql=\""
                "select        '{\\\"distribution\\\":\\\"Triangular\\\", \\\"min\\\":200, \\\"mid\\\":400,\\\"max\\\":600}' dist from @1 where cond like '\%tstc\%'"
                " union select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"mcrun_test_tstc_ytf_upsidegas_dist.g\\\"}'\""
                " output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=dist o1=0 d1=10 n1=2000 output=tmp.g"
                <<"atgraph tmp.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Ideal vs. Computed Upside Gas Volume' xlabel='Volume [BSCF]' saveto=$@" ;
                ;
            dep.push_back("mcrun_test_tstc_compare_upsidegas_dist.pdf");
            //test d
            mf.add("mcrun_test_tstd_compare_upsidegas_dist.pdf", {"mcrun_test_tstd_ytf_upsidegas_dist.g", "tstd_ideal_agr.g"})
                <<"attsqltool forecast_gas_fsd.t sql=\""
                " select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"tstd_ideal_agr.g\\\"}' as dist"
                " union"
                " select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"mcrun_test_tstd_ytf_upsidegas_dist.g\\\"}'\""
                " output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=dist o1=0 d1=10 n1=20000 output=tmp.g"
                <<"atnormalize input=tmp.g option=1 output=tmp2.g"
                <<"atgraph tmp2.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Ideal vs. Computed Upside Gas Volume' xlabel='Volume [BSCF]' saveto=$@" ;
                ;
            dep.push_back("mcrun_test_tstd_compare_upsidegas_dist.pdf");
            //test e
            mf.add("mcrun_test_tste_compare_wellgas_dist.pdf", {"mcrun_test_tste_ytf_wellgas_dist.g", "tste_ideal_agr.g"})
                <<"attsqltool forecast_gas_wad.t sql=\""
                " select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"tste_ideal_agr.g\\\"}' as dist"
                " union"
                " select '{\\\"distribution\\\":\\\"RegularlySampled\\\", \\\"load\\\":\\\"mcrun_test_tste_ytf_wellgas_dist.g\\\"}'\""
                " output=tmp.t"
                <<"atgendistarray input=tmp.t jsoncol=dist o1=0 d1=10 n1=20000 output=tmp.g"
                <<"atnormalize input=tmp.g option=1 output=tmp2.g"
                <<"atgraph tmp2.g show_yaxis=false grid=false max1=1000 autoexpand=true wheight=400 wwidth=1200 title='Ideal vs. Computed Well-Add Gas Volume' xlabel='Volume [BSCF]' saveto=$@" ;
                ;
            dep.push_back("mcrun_test_tste_compare_wellgas_dist.pdf");
            //test f
            {
                strings dep;
                for(int i=0; i<tstf_ntest; i++){
                    dep.push_back("mcrun_test_tstf"+to_string(i)+"_ytf_wellgas_dist.g");
                }
                auto & f = mf.add("tstf_gas_results.pdf", dep);
                bool first = true;
                for(int i=0; i<tstf_ntest; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstf"+to_string(i)+"_ytf_wellgas_dist.g";
                        f<<"attsqltool tstf"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstf"+to_string(i)+"_ytf_wellgas_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstf"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), sum(booked_oil), sum(booked_gas) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=1000 output=tmp2.g";
                f<<"echo \"label1='Gas Volume [BSCF]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=gas_sum"
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=$@";
            }
            dep.push_back("tstf_gas_results.pdf");
            {
                strings dep;
                for(int i=0; i<tstf_ntest; i++){
                    dep.push_back("mcrun_test_tstf"+to_string(i)+"_ytf_welloil_dist.g");
                }
                auto & f = mf.add("tstf_oil_results.pdf", dep);
                bool first = true;
                for(int i=0; i<tstf_ntest; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstf"+to_string(i)+"_ytf_welloil_dist.g";
                        f<<"attsqltool tstf"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstf"+to_string(i)+"_ytf_welloil_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstf"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), sum(booked_oil), sum(booked_gas) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=1000 output=tmp2.g";
                f<<"echo \"label1='Oil Volume [MMBO]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=oil_sum"
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=$@";
            }
            dep.push_back("tstf_oil_results.pdf");

            //test g
            {
                strings dep;
                for(int i=2005; i<current_year; i++){
                    dep.push_back("mcrun_test_tstg"+to_string(i)+"_ytf_wellgas_dist.g");
                }
                auto & f = mf.add("tstg_gas_results.pdf", dep);
                bool first = true;
                for(int i=2005; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstg"+to_string(i)+"_ytf_wellgas_dist.g";
                        f<<"attsqltool tstg"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstg"+to_string(i)+"_ytf_wellgas_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstg"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), sum(booked_oil), sum(booked_gas) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=400 output=tmp2.g";
                f<<"echo \"label1='Gas Volume [BSCF]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=gas_sum tbt_scale=true saveto=$@ wwidth=2000 trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5";
            }
            dep.push_back("tstg_gas_results.pdf");
            {
                strings dep;
                for(int i=2005; i<current_year; i++){
                    dep.push_back("mcrun_test_tstg"+to_string(i)+"_ytf_welloil_dist.g");
                }
                auto & f = mf.add("tstg_oil_results.pdf", dep);
                bool first = true;
                for(int i=2005; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstg"+to_string(i)+"_ytf_welloil_dist.g";
                        f<<"attsqltool tstg"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstg"+to_string(i)+"_ytf_welloil_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstg"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), sum(booked_oil), sum(booked_gas) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=400 output=tmp2.g";
                f<<"echo \"label1='Oil Volume [MMBO]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=oil_sum tbt_scale=true wwidth=2000 saveto=$@ trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5";
            }
            dep.push_back("tstg_oil_results.pdf");

            //test m
            {
                strings dep;
                for(auto play: seq_plays){
                    dep.push_back("mcrun_test_tstm_"+play+"_ytf_wellgas_dist.g");
                }
                auto & f = mf.add("tstm_gas_results.pdf", dep);
                bool first = true;
                int i=0;
                for(auto play: seq_plays){
                    i++;
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstm_"+play+"_ytf_wellgas_dist.g";
                        f<<"attsqltool tstm_"+play+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, '"+play+"' as play, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstm_"+play+"_ytf_wellgas_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstm_"+play+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), '"+play+"', sum(booked_oil), sum(booked_gas) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=600 output=tmp2.g";
                f<<"echo \"label1='Gas Volume [BSCF]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=gas_sum tbt_scale=true wwidth=2000 saveto=$@ trace_label_col=play gain=-1 tick_min=0.1 tick_max=0.5";
            }
            dep.push_back("tstm_gas_results.pdf");
            {
                strings dep;
                for(auto play: seq_plays){
                    dep.push_back("mcrun_test_tstm_"+play+"_ytf_welloil_dist.g");
                }
                auto & f = mf.add("tstm_oil_results.pdf", dep);
                bool first = true;
                int i=0;
                for(auto play: seq_plays){
                    i++;
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstm_"+play+"_ytf_welloil_dist.g";
                        f<<"attsqltool tstm_"+play+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, '"+play+"' as play, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_oil) as float) as oil_sum from @1\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstm_"+play+"_ytf_welloil_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tstm_"+play+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), '"+play+"', sum(booked_oil), sum(booked_oil) from @1\" output=tmp.t";
                    }
                }
                f<<"atwindow input=tmp.g n1=600 output=tmp2.g";
                f<<"echo \"label1='Oil Volume [BBO]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=oil_sum tbt_scale=true wwidth=2000 saveto=$@ trace_label_col=play gain=-1 tick_min=0.1 tick_max=0.5";
            }
            dep.push_back("tstm_oil_results.pdf");

            //test h
            {
                strings dep;
                for(int i=0; i<tstf_ntest; i++){
                    dep.push_back("mcrun_test_tsth"+to_string(i)+"_targetsuccessrate_dist.g");
                }
                auto & f = mf.add({
                        "tsth_pos_commercial_results.pdf",
                        "tsth_pos_technical_results.pdf",
                        "tsth_pos_commercial_results_with_stretch.pdf",
                        }, dep);
                bool first = true;
                for(int i=0; i<tstf_ntest; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tsth"+to_string(i)+"_targetsuccessrate_dist.g";
                        f<<"attsqltool tsth"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq"
                            ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                            ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                            " \" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tsth"+to_string(i)+"_targetsuccessrate_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tsth"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int) as seq"
                            ", cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 "
                            ", cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 "
                            " \" output=tmp.t";
                    }
                }
                f<<"atitrsmooth input=tmp.g x2z=0 nitr=4 output=tmp2.g";
                f<<"echo \"label1='Success Rate [%]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=pos_commercial"
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsth_pos_commercial_results.pdf";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=pos_technical"
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsth_pos_technical_results.pdf";
                f<<"atrescaleaxis input=tmp2.g s1=0.7 output=tmp3.g";
                f<<"atwigplot data=tmp3.g attrib=tmp.t overlay_col=pos_commercial"
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsth_pos_commercial_results_with_stretch.pdf";
            }
            dep.push_back("tsth_pos_commercial_results.pdf");
            dep.push_back("tsth_pos_technical_results.pdf");
            dep.push_back("tsth_pos_commercial_results_with_stretch.pdf");
            //test i
            {
                strings dep;
                for(int i=2013; i<current_year; i++){
                    dep.push_back("mcrun_test_tsti"+to_string(i)+"_targetsuccessrate_dist.g");
                }
                auto & f = mf.add({"tsti_pos_results.pdf", 
                        "tsti_pos_commercial_results.pdf",
                        "tsti_pos_technical_results.pdf",
                        "tsti_pos_commercial_results_with_stretch.pdf"
                        }, dep);
                bool first = true;
                for(int i=2013; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tsti"+to_string(i)+"_targetsuccessrate_dist.g";
                        f<<"attsqltool tsti"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq"
                            ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                            ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                            " \" output=tmp.t";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tsti"+to_string(i)+"_targetsuccessrate_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        f<<"attsqltool tables=\"tsti"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int) as seq"
                            ", cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 "
                            ", cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 "
                            " \" output=tmp.t";
                    }
                }
                f<<"atitrsmooth input=tmp.g x2z=0 nitr=4 output=tmp2.g";
                f<<"echo \"label1='Success Rate [%]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=pos_commercial"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsti_pos_commercial_results.pdf"
                    ;
                f<<"atwigplot data=tmp2.g attrib=tmp.t overlay_col=pos_technical"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsti_pos_technical_results.pdf";
                f<<"atrescaleaxis input=tmp2.g s1=0.7 output=tmp3.g";
                f<<"atwigplot data=tmp3.g attrib=tmp.t overlay_col=pos_commercial"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tsti_pos_commercial_results_with_stretch.pdf";
            }
            dep.push_back("tsti_pos_commercial_results.pdf");
            dep.push_back("tsti_pos_technical_results.pdf");
            dep.push_back("tsti_pos_commercial_results_with_stretch.pdf");
            //test j
            {
                strings dep;
                for(int i=2013; i<current_year; i++){
                    dep.push_back("tstj"+to_string(i)+"_portfolio.t");
                }
                auto & f = mf.add("tstj_results.t", dep);
                bool first = true;
                for(int i=2013; i<current_year; i++){
                    if(first){
                        f<<"attsqltool tstj"+to_string(i)+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum"
                            ", cast((select count(*) from @1) as float) as ntest"
                            ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                            ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                            " from @1 where commercial='Yes'\" output=tmp.t";
                        first=false;
                    }else{
                        f<<"attsqltool tables=\"tstj"+to_string(i)+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), sum(booked_oil), sum(booked_gas)"
                            ", cast((select count(*) from @1) as float) as ntest"
                            ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                            ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                            " from @1 where commercial='Yes'\" output=tmp.t";
                    }
                }
                f<<"atttee input=tmp.t output=$@";
            }
            {
                strings dep;
                dep.push_back("tstj_results.t");
                for(int i=2013; i<current_year; i++){
                    dep.push_back("mcrun_test_tstj"+to_string(i)+"_ytf_wellgas_dist.g");
                }
                auto & f = mf.add("tstj_gas_results.pdf", dep);
                bool first = true;
                for(int i=2013; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstj"+to_string(i)+"_ytf_wellgas_dist.g";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstj"+to_string(i)+"_ytf_wellgas_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                    }
                }
                f<<"atwindow input=tmp.g n1=300 output=tmp2.g";
                f<<"echo \"label1='Gas Volume [BSCF]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tstj_results.t overlay_col=gas_sum"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=$@";
            }
            dep.push_back("tstj_gas_results.pdf");
            {
                strings dep;
                dep.push_back("tstj_results.t");
                for(int i=2013; i<current_year; i++){
                    dep.push_back("mcrun_test_tstj"+to_string(i)+"_ytf_welloil_dist.g");
                }
                auto & f = mf.add("tstj_oil_results.pdf", dep);
                bool first = true;
                for(int i=2013; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstj"+to_string(i)+"_ytf_welloil_dist.g";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstj"+to_string(i)+"_ytf_welloil_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                    }
                }
                f<<"atwindow input=tmp.g n1=300 output=tmp2.g";
                f<<"echo \"label1='Oil Volume [MMBO]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tstj_results.t overlay_col=oil_sum"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=$@";
            }
            dep.push_back("tstj_oil_results.pdf");
            {
                strings dep;
                dep.push_back("tstj_results.t");
                for(int i=2013; i<current_year; i++){
                    dep.push_back("mcrun_test_tstj"+to_string(i)+"_targetsuccessrate_dist.g");
                }
                auto & f = mf.add({"tstj_pos_results.pdf", 
                        "tstj_pos_commercial_results.pdf",
                        "tstj_pos_technical_results.pdf",
                        "tstj_pos_commercial_results_with_stretch.pdf"
                        }, dep);
                bool first = true;
                for(int i=2013; i<current_year; i++){
                    if(first){
                        f<<"atmath tmp.g=mcrun_test_tstj"+to_string(i)+"_targetsuccessrate_dist.g";
                        first=false;
                    }else{
                        f<<"atcat2 list=\"tmp.g mcrun_test_tstj"+to_string(i)+"_targetsuccessrate_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                    }
                }
                f<<"atitrsmooth input=tmp.g x2z=0 nitr=4 output=tmp2.g";
                f<<"echo \"label1='Success Rate [%]'\">>tmp2.g";
                f<<"atwigplot data=tmp2.g attrib=tstj_results.t overlay_col=pos_commercial"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                   " tbt_scale=true saveto=tstj_pos_commercial_results.pdf";
                f<<"atwigplot data=tmp2.g attrib=tstj_results.t overlay_col=pos_technical"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " scale=true saveto=tstj_pos_technical_results.pdf";
                f<<"atrescaleaxis input=tmp2.g s1=0.7 output=tmp3.g";
                f<<"atwigplot data=tmp3.g attrib=tstj_results.t overlay_col=pos_commercial"
                    " trace_label_col=seq gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true saveto=tstj_pos_commercial_results_with_stretch.pdf";
            }
            dep.push_back("tstj_pos_commercial_results.pdf");
            dep.push_back("tstj_pos_technical_results.pdf");
            dep.push_back("tstj_pos_commercial_results_with_stretch.pdf");
            //test k
            {
                strings dep;
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        dep.push_back(test_name+"_portfolio.t");
                    }
                }
                int i=0;
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    auto & f = mf.add("tstk_"+wcdel+"_results.t", dep);
                    bool first = true;
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        if(first){
                            f<<"attsqltool "+test_name+"_portfolio.t sql=\"select cast("+to_string(i)+" as int) as seq, '"+play+"' as play, '"+plays[play].title+"' as title, '"+wcdel+"' as prmstype, cast(sum(booked_oil) as float) as oil_sum, cast(sum(booked_gas) as float) as gas_sum"
                                ", cast((select count(*) from @1) as float) as ntest"
                                ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                                ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                                " from @1 where commercial='Yes'\" output=tmp.t";
                            first=false;
                        }else{
                            f<<"attsqltool tables=\""+test_name+"_portfolio.t tmp.t\" sql=\"select * from @2 union select cast("+to_string(i)+" as int), '"+play+"', '"+plays[play].title+"', '"+wcdel+"', sum(booked_oil), sum(booked_gas)"
                                ", cast((select count(*) from @1) as float) as ntest"
                                ", cast(cast((select count(*) from @1 where commercial like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_commercial"
                                ", cast(cast((select count(*) from @1 where technical like 'Yes') as float)/cast((select count(*) from @1) as float)*100 as float) as pos_technical"
                                " from @1 where commercial='Yes'\" output=tmp.t";
                        }
                        i++;
                    }
                    f<<"atttee input=tmp.t output=$@";
                }
            }
            {
                strings dep0;
                dep0.push_back("tstk_Prospective_results.t");
                dep0.push_back("tstk_Delineation_results.t");
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        dep0.push_back("mcrun_test_"+test_name+"_ytf_welloil_dist.g");
                    }
                }
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    auto & f = mf.add("tstk_oil_"+wcdel+"_results.pdf", dep0);
                    bool first = true;
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        if(first){
                            f<<"atmath tmp.g=mcrun_test_"+test_name+"_ytf_welloil_dist.g";
                            first=false;
                        }else{
                            f<<"atcat2 list=\"tmp.g mcrun_test_"+test_name+"_ytf_welloil_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        }
                    }
                    f<<"atwindow input=tmp.g n1=300 output=tmp2.g";
                    f<<"atmath tmp.g=\"(i1()>0)*tmp2.g\"";
                    f<<"echo \"label1='Oil Volume [BBO]'\">>tmp.g";
                    f<<"atwigplot data=tmp.g attrib=tstk_"+wcdel+"_results.t overlay_col=oil_sum"
                        " wwidth=2000"
                        " gain=-1 tick_min=0.1 tick_max=0.5 trace_label_height=300"
                        " tbt_scale=true trace_label_col=title saveto=$@";
                    dep.push_back("tstk_oil_"+wcdel+"_results.pdf");
                }
            }
            {
                strings dep0;
                dep0.push_back("tstk_Prospective_results.t");
                dep0.push_back("tstk_Delineation_results.t");
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        dep0.push_back("mcrun_test_"+test_name+"_ytf_wellgas_dist.g");
                    }
                }
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    auto & f = mf.add("tstk_gas_"+wcdel+"_results.pdf", dep0);
                    bool first = true;
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        if(first){
                            f<<"atmath tmp.g=mcrun_test_"+test_name+"_ytf_wellgas_dist.g";
                            first=false;
                        }else{
                            f<<"atcat2 list=\"tmp.g mcrun_test_"+test_name+"_ytf_wellgas_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        }
                    }
                    f<<"atwindow input=tmp.g n1=500 output=tmp2.g";
                    f<<"atmath tmp.g=\"(i1()>0)*tmp2.g\"";
                    f<<"echo \"label1='Gas Volume [BSCF]'\">>tmp.g";
                    f<<"atwigplot data=tmp.g attrib=tstk_"+wcdel+"_results.t overlay_col=gas_sum"
                        " wwidth=2000"
                        " gain=-1 tick_min=0.1 tick_max=0.5 trace_label_height=300"
                        " tbt_scale=true trace_label_col=title saveto=$@";
                    dep.push_back("tstk_gas_"+wcdel+"_results.pdf");
                }
            }
            {
                strings dep0;
                dep0.push_back("tstk_Prospective_results.t");
                dep0.push_back("tstk_Delineation_results.t");
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        dep.push_back("mcrun_test_"+test_name+"_targetsuccessrate_dist.g");
                    }
                }
                for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                    auto & f = mf.add({
                            "tstk_"+wcdel+"_pos_commercial_results.pdf",
                            "tstk_"+wcdel+"_pos_technical_results.pdf"
                            }, dep0);
                    bool first = true;
                    for(auto play: seq_plays){
                        string test_name = "tstk_"+play+"_"+wcdel;
                        if(first){
                            f<<"atmath tmp.g=mcrun_test_"+test_name+"_targetsuccessrate_dist.g";
                            first=false;
                        }else{
                            f<<"atcat2 list=\"tmp.g mcrun_test_"+test_name+"_targetsuccessrate_dist.g\" output=tmp2.g; atmath tmp.g=tmp2.g";
                        }
                    }
                    f<<"atitrsmooth input=tmp.g x2z=0 nitr=4 output=tmp2.g";
                    f<<"echo \"label1='Success Rate [%]'\">>tmp2.g";
                    f<<"atwigplot data=tmp2.g attrib=tstk_"+wcdel+"_results.t overlay_col=pos_commercial"
                        " gain=-1 tick_min=0.1 tick_max=0.5 trace_label_height=300"
                        " tbt_scale=true trace_label_col=title saveto=tstk_"+wcdel+"_pos_commercial_results.pdf";
                    f<<"atwigplot data=tmp2.g attrib=tstk_"+wcdel+"_results.t overlay_col=pos_technical"
                        " gain=-1 tick_min=0.1 tick_max=0.5 trace_label_height=300"
                        " tbt_scale=true trace_label_col=title saveto=tstk_"+wcdel+"_pos_technical_results.pdf";
                    dep.push_back("tstk_"+wcdel+"_pos_commercial_results.pdf");
                }
            }


            auto & f = mf.add("testing_interm_report.tex", dep);
            f<<"cat <template_testing_report.tex | \\";
            f<<"sed \"s/TEMPLATE_TSTA_UPSIDEGAS_MODE/`atgetval input=mcrun_test_tsta_stat_sum.dat var=ytf_upsidegas_sc_mode`/g\" | \\";
            f<<"sed \"s/TEMPLATE_TSTA_UPSIDEOIL_MODE/`atgetval input=mcrun_test_tsta_stat_sum.dat var=ytf_upsideoil_sc_mode`/g\" | \\";
            f<<"sed \"s/TEMPLATE_TSTA_GASFINDINGRATE_MODE/`atgetval input=mcrun_test_tsta_stat_sum.dat var=rig_gasfinding_rate_sc_mode`/g\" | \\";
            f<<"sed \"s/TEMPLATE_TSTA_OILFINDINGRATE_MODE/`atgetval input=mcrun_test_tsta_stat_sum.dat var=rig_oilfinding_rate_sc_mode`/g\" | \\";
            f<<"sed \"s/TEMPLATE_TSTA_FINDINGCOST_MODE/`atgetval input=mcrun_test_tsta_stat_sum.dat var=findingcost_sc_mode`/g\" | \\";
            f<<"sed \"s/TEMPLATE_TSTA_FINDINGCOST_IDEAL/"+to_string(100.0/(std::stof(plays["abdr"].oil_rf)*200.0+std::stof(plays["abdr"].gas_rf)*100.0/5.4))+"/g\" | \\";
            f<<"cat >$@";
        }
        mf.add( "testing_report.pdf", {"template_latexdoc_top.tex", "template_latexdoc_bot.tex", "testing_interm_report.tex"})
            <<HELP("buid a testing report to validate codes, models, and assumptions.")
            <<"cat template_latexdoc_top.tex testing_interm_report.tex template_latexdoc_bot.tex>testing_report.tex"
            <<"pdflatex testing_report.tex"
            <<"pdflatex testing_report.tex"
            <<"pdflatex testing_report.tex"
            <<TEMP("testing_report.tex")
            ;
    }
    void compile_execsum_report(){
        /////////////////////////////////////////////////////////////////////////////////////////////////// 
        //create a compiled latex report
        bool only_exec=!true;
        {
            strings dep;
            dep.push_back("template_execsum_report.tex");
            dep.push_back("mcrun_all_stat_sum.dat");
            dep.push_back("mcrun_prms_2p3p_stat_sum.dat");
            dep.push_back("mcrun_prms_ctgt_stat_sum.dat");
            dep.push_back("mcrun_prms_pros_stat_sum.dat");
            dep.push_back("graph_legend.pdf");
            dep.push_back("mcrun_area_maturity_frontier_stat_sum.dat");
            dep.push_back("mcrun_area_maturity_emerging_stat_sum.dat");
            dep.push_back("mcrun_area_maturity_mature_stat_sum.dat");
            for(auto item: seq_plays){
                dep.push_back("play_"+item+"_interm_report.tex");
            }
            for(auto item: aois){
                dep.push_back("rigaoi_"+item.prefix+"_interm_report.tex");
            }
            dep.push_back("mcrun_prms_2p3p_rig_level.it");
            dep.push_back("mcrun_prms_pros_rig_level.it");
            mf.add("aed_target_sumfigures_table.tex", {"all_portfolio.t", "spot_active_portfolio.it", "realized_segids.t"})
                <<"@attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play in (select id|| play from @1)\" output=tmp_segids.t headers=false"
                <<"attsqltool tables=\"all_portfolio.t spot_active_portfolio.lt tmp_segids.t\" sql=\""
                "select 1 as seq, 'GEOX-Assessed Targets' as title, (select count(*) from @2 where pg_bscf>0 and geox_anal_id>0) as available, (select count(*) from @2 where seg_id in (select seg_id from @3) and pg_bscf>0 and geox_anal_id>0) as utilized"
                " union "
                "select 2 as seq, 'Legacy\\footnote{Assessed via Rose and Associates Toolset} Targets' as title, (select count(*) from @2 where pg_bscf>0 and geox_anal_id=0) as available, (select count(*) from @2 where seg_id in (select seg_id from @3) and pg_bscf>0 and geox_anal_id=0) as utilized"
                " union "
                "select 3, 'Features', (select count(*) from @1 where maturity like 'Identified%'), (select count(*) from @1 where maturity like 'Identified%') "
                " order by seq\" output=tmp.t"
                <<"attsqltool tmp.t sql=\"select title, available, utilized, cast(cast(round(100*utilized/available) as int) as string) || '\\\%' as percentage from @1 order by seq\" output=tmp2.t" 
                <<"att2latex input=tmp2.t headers=\", Available, Utilized, Percentage\" output=$@"
                ;
            dep.push_back("aed_target_sumfigures_table.tex");

            mf.add("aed_pal_sumfigures_table.tex", {"all_portfolio.t", "spot_active_portfolio.it", "ordered_maturity.t"})
                <<"attsqltool spot_active_portfolio.lt sql=\"select name from @1 where hc_type='GAS' group by name\" output=tmp1.t"
                <<"attsqltool tables=\"all_portfolio.t ordered_maturity.t\" sql=\"select id, min(@2.seq) as seq, @1.maturity as maturity, count(*) as c  from @1 inner join @2 on @1.maturity=@2.maturity group by id\" output=tmp2.t"
                <<"attsqltool tables=\"tmp1.t tmp2.t\" sql=\""
                "select 1 as seq, 'Gas PALs in Portfolio' as title, (select count(*) from @1) as available,(select count(*) from @2 where maturity like 'Assessed') as utilized "
                " union "
                "select 3, 'Stacked Gas-Features', (select count(*) from @2 where maturity  like 'Identified%') , (select count(*) from @2 where maturity like 'Identified%') "
                " order by seq\" output=tmp.t"
                <<"attsqltool tmp.t sql=\"select title, available, utilized, cast(cast(round(100*utilized/available) as int) as string) || '\\\%' as percentage from @1 order by seq\" output=tmp2.t" 
                <<"att2latex input=tmp2.t headers=\", Available, Utilized, Percentage\" output=$@"
                ;
            dep.push_back("aed_pal_sumfigures_table.tex");

            mf.add("aed_sumfeatures_maturity_table.tex", {"aed_portfolio.t"})
                <<"attsqltool tables=\"aed_portfolio.t\" sql=\""
                "select 1 as ord, 'High' as POM, count(*) as c from @1 where maturity like 'IdentifiedHighPOM'"
                " union "
                "select 2 as ord, 'Mid' as POM, count(*) as c from @1 where maturity like 'IdentifiedMidPOM'"
                " union "
                "select 3 as ord, 'Low' as POM, count(*) as c from @1 where maturity like 'IdentifiedLowPOM'"
                "\" output=tmp.t"
                <<"attsqltool tmp.t sql=\"select POM, c from @1 order by ord\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\"POM,Count\" output=$@"
                ;

            dep.push_back("aed_sumfeatures_maturity_table.tex");
            //
            mf.add("aed_portfolio_gas_util_table.tex", {"aed_portfolio.t", "realized_segids.t"})
                <<"@attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play in (select id|| play from @1)\" output=tmp.t headers=false"
                <<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\""
                    "select 1 as ord, 'GEOX-Assessed Targets' as title, "
                    "(select round(sum(pg_bscf)/100)*100 from @1 where pg_bscf>0 and geox_anal_id>0) as available, "
                    "(select round(sum(pg_bscf)/100)*100 from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id in (select seg_id from @2)) as utilized "
                    " union "
                    "select 1 as ord, 'Legacy Targets' as title, "
                    "(select round(sum(pg_bscf)/100)*100 from @1 where pg_bscf>0 and geox_anal_id=0) as available, "
                    "(select round(sum(pg_bscf)/100)*100 from @1 where pg_bscf>0 and geox_anal_id=0 and seg_id in (select seg_id from @2)) as utilized "
                    //"select 2,' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                    //round(sum(pg_bscf))||' bscf from '|| count(*)|| ' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                "\" output=tmp2.t"
                <<"attsqltool tmp2.t sql=\"select title, cast(available as int), cast(utilized as int), cast(cast(round(100*utilized/available) as int) as string) || '\\\%' as percentage from @1 order by ord\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\",Available, Utilized, Percentage\" headers2=\",[BSCF],[BSCF],\" output=$@"
                ;
            dep.push_back("aed_portfolio_gas_util_table.tex");
            //
            mf.add("aed_portfolio_oil_util_table.tex", {"aed_portfolio.t", "realized_segids.t"})
                <<"@attsqltool tables=\"aed_portfolio.t realized_segids.t\" sql=\"select seg_id from @2 where id||play in (select id|| play from @1)\" output=tmp.t headers=false"
                <<"@attsqltool tables=\"spot_active_portfolio.lt tmp.t\" sql=\""
                    "select 1 as ord, 'GEOX-Assessed Targets' as title, "
                    "(select round(sum(pg_mmbo)/100)*100 from @1 where pg_mmbo>0 and geox_anal_id>0) as available, "
                    "(select round(sum(pg_mmbo)/100)*100 from @1 where pg_mmbo>0 and geox_anal_id>0 and seg_id in (select seg_id from @2)) as utilized "
                    " union "
                    "select 1 as ord, 'Legacy Targets' as title, "
                    "(select round(sum(pg_mmbo)/100)*100 from @1 where pg_mmbo>0 and geox_anal_id=0) as available, "
                    "(select round(sum(pg_mmbo)/100)*100 from @1 where pg_mmbo>0 and geox_anal_id=0 and seg_id in (select seg_id from @2)) as utilized "
                    //"select 2,' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                    //round(sum(pg_bscf))||' bscf from '|| count(*)|| ' GEOX-based P&Ls are NOT used in AED plans' from @1 where pg_bscf>0 and geox_anal_id>0 and seg_id not in (select seg_id from @2)\" headers=false";
                "\" output=tmp2.t"
                <<"attsqltool tmp2.t sql=\"select title, cast(available as int), cast(utilized as int), cast(cast(round(100*utilized/available) as int) as string) || '\\\%' as percentage from @1 order by ord\" output=tmp.t"
                <<"att2latex input=tmp.t headers=\",Available, Utilized, Percentage\" headers2=\",[MMBO],[MMBO],\" output=$@"
                ;
            dep.push_back("aed_portfolio_oil_util_table.tex");
            //
            auto & f = mf.add("execsum_interm_report.tex", dep);
            f<<"cat <template_execsum_report.tex | \\";
            f<<"sed \"s/TEMPLATE_DISCOVERED//g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_WELLGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_WELLOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_WELLGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_WELLOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_WELLGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_WELLOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_WELLGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_WELLOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_WELLGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_WELLOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_WELLGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_WELLOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_WELLGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_WELLOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_WELLGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_WELLOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_UPSIDEGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_UPSIDEOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_UPSIDEGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_ASSESSED_UPSIDEOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_UPSIDEGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_UPSIDEOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_UPSIDEGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_IDENTIFIED_UPSIDEOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_UPSIDEGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_UPSIDEOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_UPSIDEGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_UNIDENTIFIED_UPSIDEOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_UPSIDEGAS_P90/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsidegas_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_UPSIDEOIL_P90/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsideoil_sc_p10`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_UPSIDEGAS_P10/\\\\\\textcolor{darkred}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsidegas_sc_p90`}/g\" | \\";
            f<<"sed \"s/TEMPLATE_TOTAL_UPSIDEOIL_P10/\\\\\\textcolor{darkgreen}{`atgetval input=mcrun_all_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsideoil_sc_p90`}/g\" | \\";
            f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
            f<<"cat tmp2.txt | \\";
            for(auto item: seq_plays){
                auto play= plays[item];
                string prefix="mcrun_play_"+item;
                {
                    //PLAYGASSTATTABLE
                    f<<"sed 's/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+play.title+"}{"+play.title+"} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                    //
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_pos mult=99.9 roundfunc=floor`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_pos mult=99.9 roundfunc=floor`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p10`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p90`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_mode`} \\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //add new row
                    f<<"sed 's/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                    //PLAYOILSTATTABLE
                    f<<"sed 's/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+play.title+"}{"+play.title+"} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                    //
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_pos mult=99.9 roundfunc=floor`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_pos mult=99.9 roundfunc=floor`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_oilfinding_rate_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p10`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p90`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_mode`} \\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //add new row
                    f<<"sed 's/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                {
                    //PLAYVOLTABLE
                    //f<<"sed 's/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+play.title+"}{"+play.title+"} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    //
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=welloil_p10`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=welloil_p90`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=welloil_mode`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellgas_p10`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellgas_p90`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //f<<"sed \"s/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellgas_mode`} \\& PLAYVOLTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                    //add new row
                    //f<<"sed 's/\\& PLAYVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PLAYVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
                f<<"cat tmp2.txt | \\";
            }
            f<<"sed 's/\\& PLAYGASSTATTABLE_TEMPLATE_COLUMN//g' | \\";
            f<<"sed 's/\\& PLAYOILSTATTABLE_TEMPLATE_COLUMN//g' | \\";
            f<<"sed 's/\\& PLAYGASTOTALVOLTABLE_TEMPLATE_COLUMN//g' | \\";
            f<<"sed 's/\\& PLAYOILTOTALVOLTABLE_TEMPLATE_COLUMN//g' | \\";
            //f<<"sed 's/\\& PLAYVOLTABLE_TEMPLATE_COLUMN//g' | \\";
            for(auto  aoi: aois){
                string prefix="mcrun_rigaoi_"+aoi.prefix;
                //RIG AOI STATTABLE
                f<<"sed 's/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+aoi.title+"}{"+aoi.title+"} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"sed 's/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+aoi.title+"}{"+aoi.title+"} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_pos mult=99.9 roundfunc=floor`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_pos mult=99.9 roundfunc=floor`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_pos mult=99.9 roundfunc=floor`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_pos mult=99.9 roundfunc=floor`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                //add new row
                f<<"sed 's/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"sed 's/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
                f<<"cat tmp2.txt | \\";
            }
            f<<"sed 's/\\& RIGAOIGASSTATTABLE_TEMPLATE_COLUMN//g' | \\";
            f<<"sed 's/\\& RIGAOIOILSTATTABLE_TEMPLATE_COLUMN//g' | \\";
            f<<"sed 's/\\& RIGAOITOTALVOLTABLE_TEMPLATE_COLUMN//g' | \\";
            for(auto  aoi: aois){
                string prefix="mcrun_rigaoi_"+aoi.prefix;
                //RIG AOI REQTABLE
                f<<"sed 's/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+aoi.title+"}{"+aoi.title+"} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ncomp_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ncomp_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=nprospect_p10`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=nprospect_p90`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=nseismic_mode`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=nstudy_mode`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                //f<<"sed \"s/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_mode`} \\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g\" | \\";
                //add new row
                f<<"sed 's/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& RIGAOIREQTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
                f<<"cat tmp2.txt | \\";
            }
            f<<"sed 's/\\& RIGAOIREQTABLE_TEMPLATE_COLUMN//g' | \\";
            {
                string prefix="mcrun_all";
                //AED's STATTABLE
                f<<"sed 's/\\& AEDSTATTABLE_TEMPLATE_COLUMN/ \\& AEDSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p10`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p90`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                //f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellgas_mode`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p10`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_p90`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=rig_gasfinding_rate_mode`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p10`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_p90`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=rig_years_mode`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p10`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_p90`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.1f\" var=findingcost_mode`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p10`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_p90`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\"; 
                f<<"sed \"s/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{black}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=wellsuccessrate_mode`} \\& AEDSTATTABLE_TEMPLATE_COLUMN/g\" | \\";
                //add new row
                f<<"sed 's/\\& AEDSTATTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& AEDSTATTABLE_TEMPLATE_COLUMN/g' | \\";
                f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
                f<<"cat tmp2.txt | \\";
            }
            f<<"sed 's/\\& AEDSTATTABLE_TEMPLATE_COLUMN//g' | \\";
            {
                //vector<pair<string,string>> prms_list={};
                //documenting PRMS classes
                {
                    string title="2P \\\\\\& 3P";
                    string prefix="mcrun_prms_2p3p";
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //start a new row
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                {
                    string title="Contingent";
                    string prefix="mcrun_prms_ctgt";
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //start a new row
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                {
                    string title="Prospective";
                    string prefix="mcrun_prms_pros";
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_pos mult=99.9 roundfunc=floor`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_pos mult=99.9 roundfunc=floor`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //start a new row
                    f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                {
                    f<<"cat >tmp1.txt; cp tmp1.txt tmp2.txt;";
                    f<<"cat tmp2.txt | \\";
                }
                f<<"sed 's/\\& PRMSGASVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& PRMSOILVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& PRMSTOTALVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& PRMSCOUNTTABLE_TEMPLATE_COLUMN//g' | \\";
            }
            //Area Maturity
            {
                for(auto amaturity: vector<string>{"Frontier", "Emerging", "Mature"}){
                    string title=amaturity;
                    string prefix="mcrun_area_maturity_"+to_lower(title);
                    f<<"sed 's/\\& AREAMATURITYCOUNTTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& AREAMATURITYCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\\\hyperlink{"+title+"}{"+title+"} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";

                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsidegas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_upsideoil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsidegas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_upsideoil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_pos mult=99.9 roundfunc=floor`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsidegas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_upsideoil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsidegas_sc_p10`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsideoil_sc_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkred}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsidegas_sc_p90`} \\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_upsideoil_sc_p90`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    f<<"sed \"s/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\& \\\\\\textcolor{darkgreen}{`atgetval input="+prefix+"_stat_sum.dat number=true roundto=0 format=\"\%.0f\" var=nprospect_p10`} \\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g\" | \\";
                    //start a new row
                    f<<"sed 's/\\& AREAMATURITYCOUNTTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& AREAMATURITYCOUNTTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                    f<<"sed 's/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/\\\\\\\\\\n\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN/g' | \\";
                }
                f<<"sed 's/\\& AREAMATURITYGASVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& AREAMATURITYOILVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& AREAMATURITYTOTALVOLTABLE_TEMPLATE_COLUMN//g' | \\";
                f<<"sed 's/\\& AREAMATURITYCOUNTTABLE_TEMPLATE_COLUMN//g' | \\";
            }
            f<<"cat >$@";
            if(!only_exec){
            //closing
                f<<"echo '\\chapter{Portfolio by Play Group}\\newpage'>>$@";
                for(auto item: seq_plays){
                    f<<"cat play_"+item+"_interm_report.tex >> $@";
                }
                f<<"echo '\\chapter{Portfolio by Rig-Area}\\newpage'>>$@";
                for(auto item: aois){
                    f<<"cat rigaoi_"+item.prefix+"_interm_report.tex >> $@";
                }
                f<<"echo '\\chapter{Portfolio by Modified-PRMS Class}\\newpage'>>$@";
                f<<"echo '\\chapter{Portfolio by BP Requirements}\\newpage'>>$@";
            }
            mf.add( "execsum_report.pdf", {"template_latexdoc_top.tex", "template_latexdoc_bot.tex", "execsum_interm_report.tex", "multirow.sty"})
                <<HELP("buid Exploration Landscape PDF report.")
                <<"cat template_latexdoc_top.tex execsum_interm_report.tex template_latexdoc_bot.tex>execsum_report.tex"
                <<"pdflatex execsum_report.tex"
                <<"pdflatex execsum_report.tex"
                <<"pdflatex execsum_report.tex"
                <<TEMP("execsum_report.tex")
                ;
            {
                auto & f = mf.add("report_by_aoi_gas", "execsum_report.pdf");
                for(auto  aoi: aois){
                    string prefix="mcrun_rigaoi_"+aoi.prefix;
                    f<<"@echo \""+aoi.title+","
                        //+"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor` \t"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p10`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p90`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_mean`,"
                        +"\"" ;
                }
            }
            {
                auto & f = mf.add("report_by_aoi_oil", "execsum_report.pdf");
                for(auto  aoi: aois){
                    string prefix="mcrun_rigaoi_"+aoi.prefix;
                    f<<"@echo \""+aoi.title+","
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p10`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p90`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_mean`,"
                        +"\"" ;
                }
            }
            {
                auto & f = mf.add("report_by_play_gas", "execsum_report.pdf");
                for(auto play: seq_plays){
                    auto & params=plays[play];
                    string prefix="mcrun_play_"+play;
                    f<<"@echo \""+params.title+","
                        //+"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor` \t"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p10`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p90`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_mean`,"
                        +"\"" ;
                }
            }
            {
                auto & f = mf.add("report_by_play_oil", "execsum_report.pdf");
                for(auto play: seq_plays){
                    auto & params=plays[play];
                    string prefix="mcrun_play_"+play;
                    f<<"@echo \""+params.title+","
                        //+"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_pos mult=99.9 roundfunc=floor` \t"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p10`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p90`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_mean`,"
                        +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_mean`,"
                        +"\"" ;
                }
            }
            {
                auto & f = mf.add("report_by_play_concepts", "execsum_report.pdf");
                for(auto play: seq_plays){
                    auto & params=plays[play];
                    string prefix="mcrun_play_"+play;
                    f<<"@echo \""+params.title+","
                        +"`attsqltool rrad_portfolio_concepts.t headers=false sql=\"select cast(count(*) as int) as c from @1 where play='"+play+"'\"`"
                        +"\"" ;
                }
            }
        }
    }
    void create_planning_canvas(){
        vector<string> dep;
        for(auto & item : plays){
            string input_prefix="play_"+item.first;
            dep.push_back(input_prefix+"_stratcol.png");
            dep.push_back(input_prefix+"_spot_active_oil_portfolio.it");
            dep.push_back(input_prefix+"_spot_active_gas_portfolio.it");
            dep.push_back(input_prefix+"_booked_oil.it");
            dep.push_back(input_prefix+"_booked_gas.it");
        }
        dep.push_back( "papgis_sa_onshore.t");
        dep.push_back( "sde_layers/sea_simplified.shp");
        dep.push_back( "sde_layers/basement_simplified.shp");
        dep.push_back( "ordered_plays.t");
        dep.push_back( "sdegis_fldout_oil.it");
        dep.push_back( "sdegis_fldout_gas.it");
        dep.push_back( "trap_types.t");
        dep.push_back( "conv3d.it");
        dep.push_back( "rigaoi.it");
        dep.push_back( "pal_opr.t");
        dep.push_back( "papgis_sa.t");
        auto & f = mf.add({"new_canvas"},dep) ;
        f<<HELP("Update the planning canvas");
        f<<"rm -f canvas.sql3"
        <<"att2sqlite tables=papgis_sa_onshore.t names=sa_onshore_shape output=canvas.sql3"
        <<"att2sqlite tables=papgis_sa.t names=sa_shape output=canvas.sql3"
        <<"att2sqlite ordered_plays.t names=tmpplay output=canvas.sql3"
        <<"sqlite3 canvas.sql3 \"create table ordered_plays(play varchar primary key, seq int, title varchar, icon blob)\""
        <<"sqlite3 canvas.sql3 \"insert into ordered_plays(play, seq, title, icon) select play, seq, title, readfile('play_'|| play || '_stratcol.png') from tmpplay\""
        <<"att2sqlite tables=pal_opr.t names=pal_opr output=canvas.sql3"
        //
        <<"atshp2tbl input=sde_layers/sea_simplified.shp output=tmp.t"
        <<"att2sqlite tables=tmp.t names=sea_shape output=canvas.sql3"
        //
        <<"atshp2itbl input=sde_layers/basement_simplified.shp output=tmp.it"
        <<"atunindextable input=tmp.it output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=basement_shape output=canvas.sql3"
        //
        <<"atshp2itbl input=sde_layers/islands_simplified.shp output=tmp.it"
        <<"atunindextable input=tmp.it output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=islands_shape output=canvas.sql3"
        //
        <<"atunindextable input=sdegis_fldout_oil.it output=tmp1.t"
        <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=fldout_oil_shape output=canvas.sql3"
        //
        <<"atunindextable input=sdegis_fldout_gas.it output=tmp1.t"
        <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=fldout_gas_shape output=canvas.sql3"
        //
        <<"atshp2itbl input=sde_layers/fldout_int.shp output=tmp.it"
        <<"atitselect input=tmp.it where=\"type=1\" output=tmp_oil.t"
        <<"atunindextable input=tmp_oil.it output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=int_fldout_oil_shape output=canvas.sql3"
        <<"atitselect input=tmp.it where=\"type in (2,3)\" output=tmp_gas.t"
        <<"atunindextable input=tmp_gas.it output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=int_fldout_gas_shape output=canvas.sql3"
        //<<"atshp2itbl input=sde_layers/seismic3d.shp output=tmp.it"
        //<<"atunindextable input=completed3d.it output=tmp1.t"
        <<"atunindextable input=conv3d.it output=tmp1.t"
        <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=seismic3d_shape output=canvas.sql3"
        <<"atunindextable input=rigaoi.it output=tmp1.t"
        <<"attsqltool tmp1.t sql=\"select index_col, x as x, y as y from @1\" output=tmp2.t"
        <<"att2sqlite tables=tmp2.t names=rigaoi_shape output=canvas.sql3"
	//
        <<"att2sqlite tables=trap_types.t names=tmp_traptype output=canvas.sql3"
        <<"sqlite3 canvas.sql3 \"create table cc_trap_type(id int primary key, seq int auto increment, title varchar, icon blob)\""
        <<"sqlite3 canvas.sql3 \"insert into cc_trap_type(id, seq, title, icon) select CC_CODE, rowid, class, readfile('trap_types/'|| ICON_CODE || '.png') from tmp_traptype where show='Yes'\""
        ;

        for(auto & item : plays){
            string input_prefix="play_"+item.first;
            f<<"atunindextable input="+input_prefix+"_spot_active_oil_portfolio.it output=tmp1.t"
            <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
            <<"att2sqlite tables=tmp2.t names="+input_prefix+"_spot_active_oil_portfolio output=canvas.sql3";
            f<<"atunindextable input="+input_prefix+"_spot_active_gas_portfolio.it output=tmp1.t"
            <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
            <<"att2sqlite tables=tmp2.t names="+input_prefix+"_spot_active_gas_portfolio output=canvas.sql3";
            ;
        }
        for(auto & item : plays){
            string input_prefix="play_"+item.first;
            f<<"atunindextable input="+input_prefix+"_booked_oil.it output=tmp1.t"
            <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
            <<"att2sqlite tables=tmp2.t names="+input_prefix+"_booked_oil output=canvas.sql3"
            ;
        }
        for(auto & item : plays){
            string input_prefix="play_"+item.first;
            f<<"atunindextable input="+input_prefix+"_booked_gas.it output=tmp1.t"
            <<"attsqltool tmp1.t sql=\"select index_col, u39x as x, u39y as y from @1\" output=tmp2.t"
            <<"att2sqlite tables=tmp2.t names="+input_prefix+"_booked_gas output=canvas.sql3"
            ;
        }
        f<<"mv canvas.sql3 curr.canvas" ;
        f<<TARGET("curr.canvas");

    }

    void evaluate_plan(){
        //proposed plan

        mf.add("rm_plan")
            <<"rm -f plan_selected.t";

        mf.add({"export_selected_plan", "plan_selected.t"})
            <<HELP("Export the selected plan (in the Guwi Application) and produce PPT report")
            <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
            <<"atdbdump db=ppd.portfolio qry=\"select drill_plan.plan, case when rig.sequence1 not null then rig.sequence1 else '' end ||' ' || drill_plan.rig ||' ['|| bop ||']' as rig, drill_plan.project as id, drill_project.stage, drill_plan.sequence, drill_plan.maint, drill_plan.spud_date, drill_plan.completion_date, drill_plan.duration,  drill_plan.is_fixed, drill_plan.comments from drill_plan inner join drill_project on drill_plan.project=drill_project.project inner join plan on plan.id=drill_plan.plan inner join rig on rig.id=drill_plan.rig where plan.selected like 'Yes' order by rig, sequence" 
            "\" output=plan_selected.t"
            ;
        mf.add({"export_vplan_baseline", "plan_vplan_baseline.t"})
            <<HELP("Export the selected plan in Guwi as vplan baseline")
            <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
            <<"atdbdump db=ppd.portfolio qry=\"select drill_plan.plan, drill_plan.rig ||' ['|| bop ||']' as rig, drill_plan.project as id, drill_project.stage, drill_plan.sequence, drill_plan.maint, drill_plan.spud_date, drill_plan.completion_date, drill_plan.duration,  drill_plan.is_fixed, drill_plan.comments from drill_plan inner join drill_project on drill_plan.project=drill_project.project inner join plan on plan.id=drill_plan.plan inner join rig on rig.id=drill_plan.rig where plan.selected like 'Yes' order by rig, sequence" 
            "\" output=plan_vplan_baseline.t"
            ;
        //simulating different options
        strings plans={"selected", "trimmed"}; //A for volumes for future wells & tests & B for success rate and finding cost including past wells
        for(auto prefix:plans){
            //this is for volume calculations assuming all dep are satisfied; i.e. we assume in case of failure well-slot will be filled with something better
            string input_prefix="plan_"+prefix;
            landscape_portfolios.push_back(input_prefix);
            mf.add(input_prefix+"_portfolio.t", {"all_portfolio.t", input_prefix+".t"})
                <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                <<"attsqltool tables=\"tmp_long_tgt_list.t "+input_prefix+".t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id \" output=tmp.t"
                <<"attupdate input=tmp.t dep=\"'NONE'\" where=\"dep not in (select id from input)\" output=tmp.t"
                //<<"attupdate input=tmp.t stage=\"'Scheduled'\""
                //<<"attupdate input=tmp.t dep=\"'NONE'\" output=$@"
                <<"atttee input=tmp.t output=$@"
                ;
        }
        vector<string> dep;
        for(auto plan:plans){
            string prefix=plan;
            string input_prefix="plan_"+prefix;
            string output_prefix="mcrun_"+input_prefix;
            //covert portfolio to program
            add_mcrun(input_prefix, output_prefix, nitr, true,
                    " max_cost=005000 mmdollers," _cont
                    " max_rigyears=30 years," _cont
                    "   max_wellgas=010000 bscf," _cont
                    " max_upsidegas=020000 bscf," _cont
                    "   max_welloil=010000 mmbo," _cont
                    " max_upsideoil=020000 mmbo"
                );
            //creating a plot
            mf.add( input_prefix+"_graph.pdf" , {input_prefix+"_program_targets.t"})
                <<"atcontingency_graph input=$< output=tmp.txt title='Dependency Graph for "+prefix+" Plan'"
                <<"dot -Tpdf tmp.txt -o $@"
                ;
            dep.push_back(output_prefix+"_stat_sum.dat");
        }
        //evaluate the plan by rigaoi
        {
            for(auto aoi:aois){
                // recursive windowing to include dependenices from outside the AOI and assign them zero volume
                string input_prefix="plan_trimmed_rigaoi_"+aoi.prefix;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_plan_trimmed_rigaoi_"+aoi.prefix;
                mf.add(input_prefix+"_portfolio.t", {"rigaoi.it", "plan_trimmed_portfolio.t"})
                    <<"atttee input=plan_trimmed_portfolio.t output=tmp_long_tgt_list.t"
                    //first window targets within the area
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where rigaoi='"+aoi.prefix+"'\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    //recursive search for dependencies outside AOI
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                    ;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=005000 mmdollers," _cont
                    " max_rigyears=30 years," _cont
                    "   max_wellgas=010000 bscf," _cont
                    " max_upsidegas=020000 bscf," _cont
                    "   max_welloil=010000 mmbo," _cont
                    " max_upsideoil=020000 mmbo"
                    , "rigaoi<>'"+aoi.prefix+"'");
                dep.push_back(output_prefix+"_stat_sum.dat");
            }
        }
        //evaluate the selected plan by play
        {
            for(auto item: plays){
                string play = item.first;
                string input_prefix="plan_trimmed_play_"+play;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_"+input_prefix;
                mf.add(input_prefix+"_portfolio.t", {"play_"+play+"_portfolio.t", "plan_trimmed_portfolio.t"})
                    <<"atttee input=play_"+play+"_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attsqltool tables=\"play_"+play+"_portfolio.t plan_trimmed_portfolio.t\" sql=\"select * from @1 where @1.id in (select id from @2)\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attsqltool tables=\"tmp.t plan_trimmed_portfolio.t\" sql=\"select @1.*, 1-exists(select * from @2 where @1.id=@2.id) as zero_flag from @1 \" output=tmp2.t"
                    <<"atttee input=tmp2.t output=$@"
                ;
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=010000 mmdollers," _cont
                    " max_rigyears=200 years," _cont
                    "   max_wellgas=050000 bscf," _cont
                    " max_upsidegas=060000 bscf," _cont
                    "   max_welloil=040000 mmbo," _cont
                    " max_upsideoil=050000 mmbo"
                    , "zero_flag=1", {"zero_flag"});
            }
        }
        //evaluate the selected area maturity
        {
            for(auto amaturity:vector<string>{"mature", "emerging", "frontier"}){
                string input_prefix="plan_trimmed_area_maturity_"+amaturity;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_"+input_prefix;
                string portfolio="area_maturity_"+amaturity+"_portfolio.t";
                mf.add(input_prefix+"_portfolio.t", {portfolio, "plan_trimmed_portfolio.t"})
                    <<"atttee input="+portfolio+" output=tmp_long_tgt_list.t"
                    <<"attsqltool tables=\""+portfolio+" plan_trimmed_portfolio.t\" sql=\"select * from @1 where @1.id in (select id from @2)\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attsqltool tables=\"tmp.t plan_trimmed_portfolio.t\" sql=\"select @1.*, 1-exists(select * from @2 where @1.id=@2.id) as zero_flag from @1 \" output=tmp2.t"
                    <<"atttee input=tmp2.t output=$@"
                ;
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=80000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=240000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                    , "zero_flag=1", {"zero_flag"});

            }
        }
        //evaluate the selected plan by PRMS Class
        {
            for(auto prms_type:prms_types){
                string prefix=prms_type;
                string input_prefix="plan_trimmed_prms_"+prefix;
                landscape_portfolios.push_back(input_prefix);
                string output_prefix="mcrun_"+input_prefix;
                string portfolio="prms_"+prms_type+"_portfolio.t";
                mf.add(input_prefix+"_portfolio.t", {portfolio, "plan_trimmed_portfolio.t"})
                    <<"atttee input="+portfolio+" output=tmp_long_tgt_list.t"
                    <<"attsqltool tables=\""+portfolio+" plan_trimmed_portfolio.t\" sql=\"select * from @1 where @1.id in (select id from @2)\" output=tmp_short_tgt_list.t"
                    <<"rm -f tmp.db"
                    <<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    <<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    <<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    <<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=tmp.t"
                    <<"attsqltool tables=\"tmp.t plan_trimmed_portfolio.t\" sql=\"select @1.*, 1-exists(select * from @2 where @1.id=@2.id) as zero_flag from @1 \" output=tmp2.t"
                    <<"atttee input=tmp2.t output=$@"
                ;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false, 
                    " max_cost=80000 mmdollers," _cont
                    " max_rigyears=1000 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=240000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                    , "zero_flag=1", {"zero_flag"});
            }
        }
        {
            auto & f = mf.add("report_plan_by_aoi_gas", dep);
            for(auto  aoi: aois){
                string prefix="mcrun_plan_trimmed_gas_rigaoi_"+aoi.prefix;
                f<<"@echo \""+aoi.title+","
                    //+"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_pos mult=99.9 roundfunc=floor` \t"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p10`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_p90`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_wellgas_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_wellgas_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_wellgas_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_wellgas_mean`,"
                    +"\"" ;
            }
        }
        {
            auto & f = mf.add("report_plan_by_aoi_oil", dep);
            for(auto  aoi: aois){
                string prefix="mcrun_plan_trimmed_oil_rigaoi_"+aoi.prefix;
                f<<"@echo \""+aoi.title+","
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p10`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_p90`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=ytf_welloil_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=assessed_welloil_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=identified_welloil_mean`,"
                    +"`atgetval input="+prefix+"_stat_sum.dat number=true roundto=2 format=\"\%.0f\" var=unidentified_welloil_mean`,"
                    +"\"" ;
            }
        }
        //reporting how much of the portfolio is being consumed by the selected plan
        {
            mf.add("consumption_report", {
                "mcrun_all_stat_sum.dat",
                "mcrun_plan_trimmed_stat_sum.dat",
                    })
                <<HELP("report how much the trimmed plan consumes from the portfoio by risked mean")
                <<"@echo \"| assesed  |identified |unidentified |   YTF  |\""
                <<"@echo \"|plan  all |plan   all |plan     all |plan all|\""
                <<"@echo `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"assessed_welloil_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"assessed_welloil_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"identified_welloil_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"identified_welloil_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"unidentified_welloil_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"unidentified_welloil_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"ytf_welloil_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"ytf_welloil_mean\"`" 
                <<"@echo `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"assessed_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"assessed_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"identified_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"identified_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"unidentified_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"unidentified_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_plan_trimmed_stat_sum.dat var=\"ytf_wellgas_mean\"`" _cont
                " `atgetval input=mcrun_all_stat_sum.dat var=\"ytf_wellgas_mean\"`" 
                ;
        }

        //split portfolio by Conventional 3D Coverage
        mf.add("conv3d.it","papgis_conv3d_coverage.shp")
            <<"atshp2itbl input=$< output=conv3d.it"
            <<"atwgs84toutm input=conv3d.t lon=x lat=y x=u39x y=u39y"
            <<BYPRODUCT(itable("conv3d.it"))
            ;
        mf.add("proposed3d.it","seismic_proposal.shp")
            <<"atshp2itbl input=$< output=$@"
            //<<"atwgs84toutm input=conv3d.t lon=x lat=y x=u39x y=u39y"
            <<BYPRODUCT(itable("proposed3d.it"))
            ;
        {
            string plan="in3d";
            string input_prefix="seisplan_"+plan;
            string output_prefix="mcrun_seisplan_"+plan;
            mf.add(input_prefix+"_portfolio.t", {"conv3d.it", "all_portfolio.t"})
                <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                //first window targets within the area
                <<"attinpolygons input=tmp_long_tgt_list.t xexpr=u39x yexpr=u39y ply=conv3d.it plyx=u39x plyy=u39y newcol=survey"
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp.t"
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp_short_tgt_list.t"
                <<"attupdate input=tmp.t dep=\"'NONE'\" where=\"dep not in (select id from input)\" output=$@"
                <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                ;
            //covert portfolio to program
            add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=140000 mmdollers," _cont
                    " max_rigyears=1500 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=200000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                , "survey=''", {"survey"});
        }
        {
            string plan="no3d";
            string input_prefix="seisplan_"+plan;
            string output_prefix="mcrun_seisplan_"+plan;
            mf.add(input_prefix+"_portfolio.t", {"conv3d.it", "all_portfolio.t"})
                <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                //first window targets within the area
                <<"attinpolygons input=tmp_long_tgt_list.t xexpr=u39x yexpr=u39y ply=conv3d.it plyx=u39x plyy=u39y newcol=survey"
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey='' \" output=tmp.t"
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey='' \" output=tmp_short_tgt_list.t"
                <<"attupdate input=tmp.t dep=\"'NONE'\" where=\"dep not in (select id from input)\" output=$@"
                <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                ;
            //covert portfolio to program
            add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=140000 mmdollers," _cont
                    " max_rigyears=1500 years," _cont
                    "   max_wellgas=160000 bscf," _cont
                    " max_upsidegas=200000 bscf," _cont
                    "   max_welloil=140000 mmbo," _cont
                    " max_upsideoil=180000 mmbo"
                , "survey<>''", {"survey"});
        }

        //split seismic scenarios
        mf.add("proposed3da.it", "proposed3d.it")
            <<"atitselect input=proposed3d.it where=\"inBP2426A='Yes'\" output=$@" 
            <<BYPRODUCT(itable("proposed3da.it"))
            ;
        mf.add("proposed3db.it", "proposed3d.it")
            <<"atitselect input=proposed3d.it where=\"inBP2426B='Yes'\" output=$@" 
            <<BYPRODUCT(itable("proposed3db.it"))
            ;
        vector<string> seis_plans = {"a", "b"};
        for(auto plan:seis_plans){
            // recursive windowing to include dependenices from outside the AOI and assign them zero volume
            string input_prefix="seisplan_"+plan;
            string output_prefix="mcrun_seisplan_"+plan;
            mf.add(input_prefix+"_portfolio.t", {"proposed3d"+plan+".it", "all_portfolio.t"})
                <<"atttee input=all_portfolio.t output=tmp_long_tgt_list.t"
                //first window targets within the area
                <<"attinpolygons input=tmp_long_tgt_list.t xexpr=u39x yexpr=u39y ply=proposed3d"+plan+".it plyx=u39x plyy=u39y newcol=survey"
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp.t"

                //recursive search for dependencies outside AOI
                <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp_short_tgt_list.t"
                //<<"rm -f tmp.db"
                //<<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                //<<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                //<<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                //<<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                <<"attupdate input=tmp.t dep=\"'NONE'\" where=\"dep not in (select id from input as inputb where input.survey=inputb.survey)\" output=$@"
                <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                ;
            //covert portfolio to program
            add_mcrun(input_prefix, output_prefix, nitr, false,
                    " max_cost=040000 mmdollers," _cont
                    " max_rigyears=350 years," _cont
                    "   max_wellgas=050000 bscf," _cont
                    " max_upsidegas=065000 bscf," _cont
                    "   max_welloil=050000 mmbo," _cont
                    " max_upsideoil=060000 mmbo"
                , "survey=''", {"survey"});
        }
        vector<string> portfolios = {"in3d", "no3d"};
        for(auto portfolio:portfolios){
            for(auto plan:seis_plans){
                string input_prefix="seisplan_"+plan+"_"+portfolio;
                string output_prefix="mcrun_seisplan_"+plan+"_"+portfolio;
                mf.add(input_prefix+"_portfolio.t", {"proposed3d"+plan+".it", "seisplan_"+portfolio+"_portfolio.t"})
                    <<"atttee input=seisplan_"+portfolio+"_portfolio.t output=tmp_long_tgt_list.t"
                    <<"attrmcol tmp_long_tgt_list.t list=\"survey\""
                    //first window targets within the area
                    <<"attinpolygons input=tmp_long_tgt_list.t xexpr=u39x yexpr=u39y ply=proposed3d"+plan+".it plyx=u39x plyy=u39y newcol=survey"
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp.t"

                    //recursive search for dependencies outside AOI
                    <<"attsqltool tmp_long_tgt_list.t sql=\"select * from @1 where survey<>'' \" output=tmp_short_tgt_list.t"
                    //<<"rm -f tmp.db"
                    //<<"att2sqlite tables=\"tmp_long_tgt_list.t tmp_short_tgt_list.t\" output=tmp.db"
                    //<<"atsqlite_run db=tmp.db sql=\"create view tgts_with_dep as with recursive cte_a (id, dep, play) as (select id, dep, play from tmp_short_tgt_list union all select tmp_long_tgt_list.id, tmp_long_tgt_list.dep, tmp_long_tgt_list.play from tmp_long_tgt_list join cte_a on cte_a.play=tmp_long_tgt_list.play and cte_a.dep=tmp_long_tgt_list.id) select * from cte_a\""
                    //<<"atdbdump db=tmp.db qry=\"select * from tgts_with_dep group by id, dep, play\" output=tmp_shortlist.t"
                    //<<"attsqltool tables=\"tmp_long_tgt_list.t tmp_shortlist.t\" sql=\"select @1.* from @1 inner join @2 on @1.id=@2.id and @1.play=@2.play\" output=$@"
                    <<"attupdate input=tmp.t dep=\"'NONE'\" where=\"dep not in (select id from input as inputb where input.survey=inputb.survey)\" output=$@"
                    <<TEMP({"tmp_long_tgt_list.t", "tmp_short_tgt_list.t"})
                    ;
                //covert portfolio to program
                add_mcrun(input_prefix, output_prefix, nitr, false,
                        " max_cost=040000 mmdollers," _cont
                        " max_rigyears=350 years," _cont
                        "   max_wellgas=050000 bscf," _cont
                        " max_upsidegas=065000 bscf," _cont
                        "   max_welloil=050000 mmbo," _cont
                        " max_upsideoil=060000 mmbo"
                    , "survey=''", {"survey"});
            }
        }

        mf.add({"hcpv_from_db", 
                "plan_selected_program_targets_postdrill.t", 
                "plan_selected_program_target_postdrill_wellgasvol.g", 
                "plan_selected_program_target_postdrill_welloilvol.g", 
                "plan_selected_program_target_postdrill_upsidegasvol.g", 
                "plan_selected_program_target_postdrill_upsideoilvol.g"}, 
                {"plan_selected_program_targets.t", 
                "plan_selected_program_target_wellgasvol.g", 
                "plan_selected_program_target_welloilvol.g",
                "plan_selected_program_target_upsidegasvol.g",
                "plan_selected_program_target_upsideoilvol.g",
                } )
            <<"atdbdump db="+portfolio_path+" qry=\"select drill_target.project, drill_target.play_group, maturity, drill_target.comments, drill_target.variables from drill_project inner join drill_target on drill_project.project=drill_target.project where drill_target.maturity in ('Failed', 'CommercialSuccess', 'UnbookedResource', 'BookedResource')\" output=tmp.t"
            <<"attargfromcol input=tmp.t from=variables arg=gas_min  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=gas_max  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=oil_min  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=oil_max  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=gas_low  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=gas_best type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=gas_high type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=oil_low  type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=oil_best type=float default=0.0"
            <<"attargfromcol input=tmp.t from=variables arg=oil_high type=float default=0.0"
            //getting well volumes
            <<"attsqltool tmp.t sql=\"select * from @1 where maturity in ('CommercialSuccess', 'UnbookedResource', 'BookedResource') and (oil_min>0.0 and oil_max>0.0) or (gas_min >0.0 and gas_max>0.0)\" output=tmp2.t"
            <<"attsqltool tmp2.t sql=\"select 'project='''||project||''' and play='''||play_group||''' ' as cond,"
                " '{\\\"distribution\\\":\\\"Uniform\\\", \\\"min\\\":'||gas_min||', \\\"max\\\":'||gas_max||'}' as gas_dist,"
                " '{\\\"distribution\\\":\\\"Uniform\\\", \\\"min\\\":'||oil_min||', \\\"max\\\":'||oil_max||'}' as oil_dist"
                " from @1\" output=tmp1.t" 
            <<"attsetwhere cond=tmp1.t condcol=cond valuecol=gas_dist input=plan_selected_program_targets.t newcol=postdrill_well_gas_bscf_dist output=tmp2.t"
            <<"attsetwhere cond=tmp1.t condcol=cond valuecol=oil_dist input=tmp2.t                         newcol=postdrill_well_oil_mmbo_dist output=plan_selected_program_targets_postdrill.t"
            <<"attupdate input=plan_selected_program_targets_postdrill.t postdrill_well_gas_bscf_dist=well_gas_bscf_dist where=\"postdrill_well_gas_bscf_dist=''\""
            <<"attupdate input=plan_selected_program_targets_postdrill.t postdrill_well_oil_mmbo_dist=well_oil_mmbo_dist where=\"postdrill_well_oil_mmbo_dist=''\""
            <<"atgendistarray input=plan_selected_program_targets_postdrill.t jsoncol=postdrill_well_gas_bscf_dist geom=plan_selected_program_target_wellgasvol.g output=plan_selected_program_target_postdrill_wellgasvol.g"
            <<"atgendistarray input=plan_selected_program_targets_postdrill.t jsoncol=postdrill_well_oil_mmbo_dist geom=plan_selected_program_target_welloilvol.g output=plan_selected_program_target_postdrill_welloilvol.g"

            //now for upside
            <<"attsqltool tmp.t sql=\"select * from @1 where maturity in ('GeologicalSuccess', 'SubcommercialResource', 'CommercialSuccess', 'UnbookedResource', 'BookedResource') and (oil_low>0.0 and oil_high>0.0) or (gas_low >0.0 and gas_high>0.0)\" output=tmp2.t"
            <<"attsqltool tmp2.t sql=\"select 'project='''||project||''' and play='''||play_group||''' ' as cond,"
                " '{\\\"distribution\\\":\\\"Uniform\\\", \\\"min\\\":'||gas_low||', \\\"max\\\":'||gas_high||'}' as gas_upside_dist,"
                " '{\\\"distribution\\\":\\\"Uniform\\\", \\\"min\\\":'||oil_low||', \\\"max\\\":'||oil_high||'}' as oil_upside_dist"
                " from @1\" output=tmp1.t" 
            <<"attsetwhere cond=tmp1.t condcol=cond valuecol=gas_upside_dist input=plan_selected_program_targets_postdrill.t newcol=postdrill_upside_gas_bscf_dist output=tmp2.t"
            <<"attsetwhere cond=tmp1.t condcol=cond valuecol=oil_upside_dist input=tmp2.t                         newcol=postdrill_upside_oil_mmbo_dist output=plan_selected_program_targets_postdrill.t"
            <<"attupdate input=plan_selected_program_targets_postdrill.t postdrill_upside_gas_bscf_dist=upside_gas_bscf_dist where=\"postdrill_upside_gas_bscf_dist=''\""
            <<"attupdate input=plan_selected_program_targets_postdrill.t postdrill_upside_oil_mmbo_dist=upside_oil_mmbo_dist where=\"postdrill_upside_oil_mmbo_dist=''\""
            <<"atgendistarray input=plan_selected_program_targets_postdrill.t jsoncol=postdrill_upside_gas_bscf_dist geom=plan_selected_program_target_upsidegasvol.g output=plan_selected_program_target_postdrill_upsidegasvol.g"
            <<"atgendistarray input=plan_selected_program_targets_postdrill.t jsoncol=postdrill_upside_oil_mmbo_dist geom=plan_selected_program_target_upsideoilvol.g output=plan_selected_program_target_postdrill_upsideoilvol.g"
            ;
        //new workflow
        mf.add({
                "all_stack.t",
                "all_stack_oil_wad.g",
                "all_stack_gas_wad.g",
                },{
                "all_program_targets.t", 
                "all_program_target_pos.g",
                "all_program_target_wellgasvol.g", 
                "all_program_target_welloilvol.g", 
                "all_program_target_upsidegasvol.g", 
                "all_program_target_upsideoilvol.g", 
                }
              )
            <<"atpap_prospect_stack " _cont
            "           targets=all_program_targets.t " _cont
            "    targets_gaswad=all_program_target_wellgasvol.g " _cont
            "    targets_oilwad=all_program_target_welloilvol.g " _cont
            "        target_pos=all_program_target_pos.g " _cont
            "   output_gas_dist=all_stack_gas_wad.g " _cont
            "   output_oil_dist=all_stack_oil_wad.g " _cont
            "      output_table=all_stack.t " _cont
            "          predrill=true" _cont
            "          success=commercial"
            <<"atpap_prospect_stack " _cont
            "           targets=all_program_targets.t " _cont
            "    targets_gaswad=all_program_target_upsidegasvol.g " _cont
            "    targets_oilwad=all_program_target_upsideoilvol.g " _cont
            "        target_pos=all_program_target_pos.g " _cont
            "   output_gas_dist=all_stack_gas_upside.g " _cont
            "   output_oil_dist=all_stack_oil_upside.g " _cont
            "      output_table=tmp_all_stack.t " _cont
            "          predrill=true" _cont
            "          success=commercial"
            ;
        mf.add("all_stack_wstats.t", {"all_stack.t", "all_stack_gas_wad.g", "all_stack_oil_wad.g"})
            <<"atdistribution_summary2d input=all_stack_gas_wad.g input_tbl=all_stack.t  prefix=\"gas_wad\" output=tmp.t"
            <<"atdistribution_summary2d input=all_stack_oil_wad.g input_tbl=tmp.t        prefix=\"oil_wad\" output=tmp.t"
            <<"atdistribution_summary2d input=all_stack_gas_upside.g input_tbl=tmp.t        prefix=\"gas_upside\" output=tmp.t"
            <<"atdistribution_summary2d input=all_stack_oil_upside.g input_tbl=tmp.t        prefix=\"oil_upside\" output=tmp.t"
            <<"atttee input=tmp.t output=$@"
            ;
        mf.add("all_program_durations.t", {"all_program_projects.t", "all_program_project_rigyears.g"})
            <<"atdistribution_summary2d input=all_program_project_rigyears.g input_tbl=all_program_projects.t prefix=\"duration\" output=$@"
            ;
        mf.add("all_program_costs.t", {"all_program_projects.t", "all_program_project_costs.g"})
            <<"atdistribution_summary2d input=all_program_project_costs.g input_tbl=all_program_projects.t prefix=\"cost\" output=$@"
            ;

        mf.add("show_unselected", {"all_stack_wstats.t", "all_program_projects.t", "plan_trimmed.t"})
            <<HELP("Show active wells not selected in the plan")
            <<"attsqltool tables=\"all_stack_wstats.t plan_trimmed.t all_program_projects.t\" sql=\"select @3.rigaoi, @1.* from @1 inner join @3 on @1.project=@3.project where not @1.project in (select id from @2) and stage in ('Proposed', 'Scheduled') and not @1.project GLOB '*AUTO-Del*' \" output=tmp.t"
            <<"attsqltool tmp.t sql=\"select * from @1 order by rigaoi, gas_upside_mean desc\" output=tmp_by_upsidegas.t && attedit tmp_by_upsidegas.t &"
            ;

        mf.add({
                "plan_selected_stack.t",
                "plan_selected_stack_predrill_oil.g",
                "plan_selected_stack_predrill_gas.g",
                },{
                "plan_selected_program_targets.t", 
                "plan_selected_program_target_pos.g",
                "plan_selected_program_target_wellgasvol.g", 
                "plan_selected_program_target_welloilvol.g", 
                }
              )
            <<"atpap_prospect_stack " _cont
            "           targets=plan_selected_program_targets.t " _cont
            "    targets_gaswad=plan_selected_program_target_wellgasvol.g " _cont
            "    targets_oilwad=plan_selected_program_target_welloilvol.g " _cont
            "        target_pos=plan_selected_program_target_pos.g " _cont
            "   output_gas_dist=plan_selected_stack_predrill_gas.g " _cont
            "   output_oil_dist=plan_selected_stack_predrill_oil.g " _cont
            "      output_table=plan_selected_stack.t " _cont
            "          predrill=true" _cont
            "          success=commercial"
            ;
        mf.add({
                "plan_selected_stack_postdrill.t",
                "plan_selected_stack_postdrill_oil.g",
                "plan_selected_stack_postdrill_gas.g",
                },{
                "plan_selected_program_targets.t", 
                "plan_selected_program_target_pos.g",
                "plan_selected_program_target_postdrill_wellgasvol.g", 
                "plan_selected_program_target_postdrill_welloilvol.g", 
                }
              )
            <<"atpap_prospect_stack targets=plan_selected_program_targets.t targets_gaswad=plan_selected_program_target_postdrill_wellgasvol.g targets_oilwad=plan_selected_program_target_postdrill_welloilvol.g target_pos=plan_selected_program_target_pos.g output_gas_dist=plan_selected_stack_postdrill_gas.g output_oil_dist=plan_selected_stack_postdrill_oil.g output_table=plan_selected_stack_postdrill.t predrill=false success=commercial"
            ;
        mf.add({
                "plan_selected_stack_predrill_upsideoil.g",
                "plan_selected_stack_predrill_upsidegas.g",
                },{
                "plan_selected_program_targets.t", 
                "plan_selected_program_target_pos.g",
                "plan_selected_program_target_upsidegasvol.g", 
                "plan_selected_program_target_upsideoilvol.g", 
                }
              )
            <<"atpap_prospect_stack targets=plan_selected_program_targets.t targets_gaswad=plan_selected_program_target_upsidegasvol.g           targets_oilwad=plan_selected_program_target_upsideoilvol.g           target_pos=plan_selected_program_target_pos.g output_gas_dist=plan_selected_stack_predrill_upsidegas.g  output_oil_dist=plan_selected_stack_predrill_upsideoil.g output_table=tmp3.t predrill=true success=geological"
            ;
        mf.add({
                "plan_selected_stack_postdrill_upsideoil.g",
                "plan_selected_stack_postdrill_upsidegas.g",
                },{
                "plan_selected_program_targets.t", 
                "plan_selected_program_target_pos.g",
                "plan_selected_program_target_postdrill_upsidegasvol.g", 
                "plan_selected_program_target_postdrill_upsideoilvol.g", 
                }
              )
            <<"atpap_prospect_stack targets=plan_selected_program_targets.t targets_gaswad=plan_selected_program_target_postdrill_upsidegasvol.g           targets_oilwad=plan_selected_program_target_postdrill_upsideoilvol.g           target_pos=plan_selected_program_target_pos.g output_gas_dist=plan_selected_stack_postdrill_upsidegas.g  output_oil_dist=plan_selected_stack_postdrill_upsideoil.g output_table=tmp3.t predrill=false success=geological"
            ;
        mf.add("plan_plot", {
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                })
            <<HELP("Evaluate the selected plan (in Guwi Application) and produce PPT report")
            <<"atdsplot " _cont
                "                          plan=plan_selected.t " _cont
                "                      projects=plan_selected_program_projects.t" _cont
                "              project_rigyears=plan_selected_program_project_rigyears.g " _cont
                "              project_costs=plan_selected_program_project_costs.g " _cont
                "                         nreal=2 " _cont
                "                         year1="+to_string(year1)+" nyears="+to_string(nyear)+" " _cont
                "                         rigmove_min="+rigmove_min+" rigmove_max="+rigmove_max+" " _cont
                "                    output_ppt=tmp5.pptx &&" _cont
                "wslview tmp5.pptx"
            ;
        mf.add({
                "plan_compare_to_ed_timing",
                }, {
                "match_ds_portfolio.t",
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_project_rigyears.g",
                "drill_schedule.t"
                })
            <<HELP("Compare Exploration Drilling timelines with the forecasted timelines")
            <<"attsqltool tables=\"plan_selected.t drill_schedule.t match_ds_portfolio.t\" sql=\"select plan, rig, id, stage, "
                " cast((select spud  as args2 from @2 inner join @3 on @2.slot_id=@3.slot_id where @3.project like @1.id  union select 0 order by args2 desc) as float) spud_date, "
                " cast((select compl as args2 from @2 inner join @3 on @2.slot_id=@3.slot_id where @3.project like @1.id  union select 0 order by args2 desc) as float) completion_date, "
                " comments from @1\" output=tmp_plan.t"
            <<"attmath tmp_plan.t dur=\"completion_date-spud_date\""
            <<"attsqltool tables=\"plan_selected_program_projects.t tmp_plan.t\" sql=\"select *, (select dur from @2 where id=project) as duration, "
              " project || ' [' || deepest_play|| ']'  as label "
              " from @1\" output=tmp.t"
            <<"atwigplot plan_selected_program_project_rigyears.g attrib=tmp.t overlay_col=duration vsize=10 tbt_scale=true trace_label_col=label "
                    " gain=-1 tick_min=0.1 tick_max=0.5"
                    " tbt_scale=true label1=\"Rig Years\"";
            ;
        mf.add("plan_selected_with_forecasted_timing.t", {
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                })
            <<"atdstimeforecast " _cont
                "                          plan=plan_selected.t " _cont
                "                      projects=plan_selected_program_projects.t" _cont
                "              project_rigyears=plan_selected_program_project_rigyears.g " _cont
                "                         nreal=2 " _cont
                "                         year1="+to_string(year1)+" nyears="+to_string(nyear)+" " _cont
                "                         rigmove_min="+rigmove_min+" rigmove_max="+rigmove_max+" " _cont
                "                    output=$@ "
            ;
        mf.add("plan_selected_program_project_cost_factorized.g", {
                "plan_selected_program_projects.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                })
            <<"atdistributionfractions input=plan_selected_program_project_costs.g maxnfact=4 attrib=plan_selected_program_projects.t distid=cost_dist output=$@"
        ;
        mf.add("plan_trimmed.t", "plan_selected_with_forecasted_timing.t")
            <<"attsqltool $< sql=\"select * from @1 where spud_p10<"+to_string(year1+nyear)+" and compl_p90>="+to_string(year1)+"\" output=$@"
            ;
        mf.add("plan_eval", {
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_targets.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                "plan_selected_program_project_cost_factorized.g",
                "plan_selected_stack.t",
                "plan_selected_stack_predrill_oil.g",
                "plan_selected_stack_predrill_gas.g",
                "plan_selected_stack_postdrill_oil.g",
                "plan_selected_stack_postdrill_gas.g",
                "curr.canvas",
                "rigaoi.it",
                "pap_ip_targets.t",
                "ordered_plays_w_rrad_status.t",
                })
            <<HELP("Evaluate the selected plan (in Guwi Application) and produce PPT report")
            <<"atdseval " _cont
                "               rigaoi_polygons=rigaoi.it " _cont
                "                        canvas=curr.canvas " _cont
                "                       targets=pap_ip_targets.t " _cont
                "                 ordered_plays=ordered_plays_w_rrad_status.t " _cont
                "                          plan=plan_selected.t " _cont
                "                      projects=plan_selected_program_projects.t" _cont
                "               project_targets=plan_selected_program_targets.t" _cont
                "              project_rigyears=plan_selected_program_project_rigyears.g " _cont
                "                 project_costs=plan_selected_program_project_costs.g " _cont
                "       project_cost_fractions=plan_selected_program_project_cost_factorized.g " _cont
                "                         nreal=3 nitr="+to_string(nitr)+" " _cont
                "               project_sim_tbl=plan_selected_stack.t " _cont
                "   project_predrill_sim_gaswad=plan_selected_stack_predrill_gas.g " _cont
                "   project_predrill_sim_oilwad=plan_selected_stack_predrill_oil.g " _cont
                "  project_postdrill_sim_gaswad=plan_selected_stack_postdrill_gas.g " _cont
                "  project_postdrill_sim_oilwad=plan_selected_stack_postdrill_oil.g " _cont
                "                         year1="+to_string(year1)+" nyears="+to_string(nyear)+" " _cont
                "                       max_gas=30000 " _cont
                "                       max_oil=10000 " _cont
                "                   rigmove_min="+rigmove_min+" " _cont
                "                   rigmove_max="+rigmove_max+" " _cont
                "                    output_ppt=tmp_$@.pptx " _cont
                "                    output_oil=tmp_oil.g output_gas=tmp_gas.g &&" _cont
                "wslview tmp_$@.pptx"
            ;
        mf.add("plan_rigcut", {
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_targets.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                "plan_selected_stack.t",
                "plan_selected_stack_predrill_oil.g",
                "plan_selected_stack_predrill_gas.g",
                "plan_selected_stack_postdrill_oil.g",
                "plan_selected_stack_postdrill_gas.g",
                "curr.canvas",
                "rigaoi.it",
                "pap_ip_targets.t",
                "ordered_plays_w_rrad_status.t",
                })
            <<HELP("Evaluate scenarios for rig cuts from a plan and produce PPT report")
            <<"atrigcut " _cont
                "               rigaoi_polygons=rigaoi.it " _cont
                "                        canvas=curr.canvas " _cont
                "                       targets=pap_ip_targets.t " _cont
                "                 ordered_plays=ordered_plays_w_rrad_status.t " _cont
                "                          plan=plan_selected.t " _cont
                "                      projects=plan_selected_program_projects.t" _cont
                "               project_targets=plan_selected_program_targets.t" _cont
                "              project_rigyears=plan_selected_program_project_rigyears.g " _cont
                "              project_costs=plan_selected_program_project_costs.g " _cont
                "                         nreal=1 nitr="+to_string(nitr)+" " _cont
                "               project_sim_tbl=plan_selected_stack.t " _cont
                "   project_predrill_sim_gaswad=plan_selected_stack_predrill_gas.g " _cont
                "   project_predrill_sim_oilwad=plan_selected_stack_predrill_oil.g " _cont
                "  project_postdrill_sim_gaswad=plan_selected_stack_postdrill_gas.g " _cont
                "  project_postdrill_sim_oilwad=plan_selected_stack_postdrill_oil.g " _cont
                "                         year1="+to_string(year1)+" nyears="+to_string(nyear)+" " _cont
                "                       max_gas=30000 " _cont
                "                       max_oil=10000 " _cont
                "                   rigmove_min="+rigmove_min+" " _cont
                "                   rigmove_max="+rigmove_max+" " _cont
                "                    output_ppt=tmp_$@.pptx " _cont
                "                    output_oil=tmp_oil.g output_gas=tmp_gas.g &&" _cont
                "wslview tmp_$@.pptx"
            ;


        mf.add("plan_eval_upside", {
                "plan_selected.t",
                "plan_selected_program_projects.t",
                "plan_selected_program_targets.t",
                "plan_selected_program_project_rigyears.g",
                "plan_selected_program_project_costs.g",
                "plan_selected_stack.t",
                "plan_selected_stack_predrill_upsideoil.g",
                "plan_selected_stack_predrill_upsidegas.g",
                "plan_selected_stack_postdrill_upsideoil.g",
                "plan_selected_stack_postdrill_upsidegas.g",
                "curr.canvas",
                "rigaoi.it",
                "pap_ip_targets.t",
                "ordered_plays_w_rrad_status.t",
                })
            <<HELP("Evaluate the selected plan (in Guwi Application) and produce PPT report")
            <<"atdseval " _cont
                "               rigaoi_polygons=rigaoi.it " _cont
                "                        canvas=curr.canvas " _cont
                "                       targets=pap_ip_targets.t " _cont
                "                 ordered_plays=ordered_plays_w_rrad_status.t " _cont
                "                          plan=plan_selected.t " _cont
                "                      projects=plan_selected_program_projects.t" _cont
                "               project_targets=plan_selected_program_targets.t" _cont
                "              project_rigyears=plan_selected_program_project_rigyears.g " _cont
                "                 project_costs=plan_selected_program_project_costs.g " _cont
                "                         nreal=3 nitr="+to_string(nitr)+" " _cont
                "               project_sim_tbl=plan_selected_stack.t " _cont
                "   project_predrill_sim_gaswad=plan_selected_stack_predrill_upsidegas.g " _cont
                "   project_predrill_sim_oilwad=plan_selected_stack_predrill_upsideoil.g " _cont
                "  project_postdrill_sim_gaswad=plan_selected_stack_postdrill_upsidegas.g " _cont
                "  project_postdrill_sim_oilwad=plan_selected_stack_postdrill_upsideoil.g " _cont
                "                         year1="+to_string(year1)+" nyears="+to_string(nyear)+" " _cont
                "                       max_gas=30000 " _cont
                "                       max_oil=12000 " _cont
                "                   rigmove_min="+rigmove_min+" " _cont
                "                   rigmove_max="+rigmove_max+" " _cont
                "                    output_ppt=tmp_$@.pptx " _cont
                "                    output_oil=tmp_oil.g output_gas=tmp_gas.g &&" _cont
                "wslview tmp_$@.pptx"
            ;

        mf.add({ "match_ds_plan", "match_ds_plan.t"}, {"drill_schedule.t", "plan_selected.t"})
            <<"attsqltool tables=\"drill_schedule.t plan_selected.t\" sql=\"select @1.slot_id, @2.id from @1 inner join @2 on trim(replace(replace(replace(upper(@1.slot_id),' ',''),'.',''),'-',''))=trim(replace(replace(replace(upper(@2.id),' ',''),'.',''),'-','')) and upper(@1.rig)=upper(@2.rig)\" output=tmp_sugg.t"
            <<"atmatchwhich from=plan_selected.t from_ids=\"id\" to=drill_schedule.t to_ids=\"slot_id\" sugg=tmp_sugg.t load=match_ds_plan.t"
            ;
        mf.add({ "match_ds_portfolio", "match_ds_portfolio.t"}, {"drill_schedule.t"})
            <<HELP("match drilling schedule to the portfolio")
            <<"cp "+portfolio_path+" ppd.portfolio 2>/dev/null || :"
            <<"atdbdump db=ppd.portfolio qry=\"select *, (select group_concat(play_group, ', ') as plays from drill_target where project=drill_project.project group by project) as reservoirs from drill_project\" output=tmp.t"
            <<"attsqltool tables=\"drill_schedule.t tmp.t\" sql=\"select @1.slot_id, @2.project from @1 inner join @2 on trim(replace(replace(replace(upper(@1.slot_id),' ',''),'.',''),'-',''))=trim(replace(replace(replace(upper(@2.project),' ',''),'.',''),'-',''))\" output=tmp_sugg.t"
            <<"atmatchwhich to=tmp.t to_ids=\"project\" from=drill_schedule.t from_ids=\"slot_id\" sugg=tmp_sugg.t load=match_ds_portfolio.t"
            ;
        //generating the portfolio PPT report
        {
            string titles;
            vector<string> result_files;
            bool first=true;
            for(auto & portfolio: landscape_portfolios){
                result_files.push_back("mcrun_"+portfolio+"_stat_sum.dat");
                if(first)
                    first=false;
                else
                    titles+=",";
                titles+="a";
            }
            auto input_str = to_string(result_files);
            strings dep (result_files);
            dep.push_back("pap_ip_targets.t");
            dep.push_back("rigaoi.it");
            dep.push_back("ordered_plays.t");
            mf.add("landscape.pptx", dep)
                <<"atlandscape_ppt input=\""+input_str+"\" output_ppt=tmp.pptx " _cont
                "               rigaoi_polygons=rigaoi.it " _cont
                "                 ordered_plays=ordered_plays.t " _cont
                " && wslview tmp.pptx"
                ;
            
        }
    }

    void venture_plan(){
        vector<string> dep;
        string titles;
        bool first=true;
        for(auto & venture: ventures){
            dep.push_back("mcrun_venture_"+venture.prefix+"_results.p");
            if(first)
                first=false;
            else
                titles+=",";
            titles+=venture.title;
        }
        auto ventures_str = to_string(dep);
        dep.push_back("pap_ip_targets.t");
        mf.add("vplan", dep)
            <<"atventureplan ventures=\""+ventures_str+"\" titles=\""+titles+"\" targets=pap_ip_targets.t target_gas_volume=low_bscf target_oil_volume=low_mmbo target_cost=capex loadX=BP2025_27budget_add200MM_in2025_options.it loadX=bp_update_1.24_1.0_1.0_scenario1.t vplan2.t nyear=3 "
            ;
        mf.add("vplan_upside", dep)
            <<"atventureplan ventures=\""+ventures_str+"\" titles=\""+titles+"\" targets=pap_ip_targets.t target_gas_volume=high_bscf target_oil_volume=high_mmbo target_cost=capex option=upside loadX=vplan2.t nyear=3 "
            ;
    }
    void benchmark(){
        mf.add("benchmark_oil_wad", {"plan_selected_program_targets_postdrill.t", "plan_selected_program_target_welloilvol.g", "plan_selected_program_target_postdrill_welloilvol.g"})
            <<HELP("benchmark oil well-additions for the concluded wells in the selected plan")
            <<"attsqltool plan_selected_program_targets_postdrill.t sql=\"select *, project||' ['||play||']' as welltarget, cast(maturity in ('CommercialSuccess', 'UnbookedResource', 'BookedResource') and variables like '\%oil_min=\%' as int) as towind from @1\" output=tmp1.t"
            <<"attwind din=plan_selected_program_target_welloilvol.g tin=tmp1.t tout=tmp2.t dout=tmp2.g expr=\"towind\""
            <<"attwind din=plan_selected_program_target_postdrill_welloilvol.g tin=tmp1.t tout=tmp3.t dout=tmp3.g expr=\"towind\""
            <<"atwigplot tmp2.g diff=tmp3.g attrib=tmp2.t trace_label_col=welltarget tbt_scale=true vsize=10 trace_label_height=150"
                " gain=-1 tick_min=0.1 tick_max=0.5"
                    ;

        mf.add("benchmark_gas_wad", {"plan_selected_program_targets_postdrill.t", "plan_selected_program_target_wellgasvol.g", "plan_selected_program_target_postdrill_wellgasvol.g"})
            <<HELP("benchmark gas well-additions for the concluded wells in the selected plan")
            <<"attsqltool plan_selected_program_targets_postdrill.t sql=\"select *, project||' ['||play||']' as welltarget, cast(maturity in ('CommercialSuccess', 'UnbookedResource', 'BookedResource')  and variables like '\%gas_min=\%'as int) as towind from @1\" output=tmp1.t"
            <<"attwind din=plan_selected_program_target_wellgasvol.g tin=tmp1.t tout=tmp2.t dout=tmp2.g expr=\"towind\""
            <<"attwind din=plan_selected_program_target_postdrill_wellgasvol.g tin=tmp1.t tout=tmp3.t dout=tmp3.g expr=\"towind\""
            <<"atwigplot tmp2.g diff=tmp3.g attrib=tmp2.t trace_label_col=welltarget tbt_scale=true vsize=10 trace_label_height=150";
        mf.add("benchmark_oil_fsd")
            <<HELP("benchmark oil field-size distribution for the recently generated prospects and leads")
            ;
        mf.add("benchmark_gas_fsd")
            <<HELP("benchmark gas field-size distribution for the recently generated prospects and leads")
            ;

        mf.add("benchmark_rigdays",{"plan_selected_program_projects.t", "plan_selected.t", "plan_selected_program_project_rigyears.g"})
            <<HELP("benchmark rig-days for completed wells in the selected plan")
            <<"attsqltool tables=\"plan_selected_program_projects.t plan_selected.t\" sql=\"select *, cast(365*(@2.completion_date-@2.spud_date) as float) as duration, rigdays_cond ||' ['|| project ||']' as label from @1 inner join @2 on @1.project=@2.id\" output=tmp1.t"
            <<"attwind din=plan_selected_program_project_rigyears.g tin=tmp1.t tout=tmp2.t dout=tmp2.g expr=\"duration_1>0\""
            <<"attstrsort input=tmp2.t key=rigdays_cond keep_index=true output=tmp3.t"
            <<"attracesort tin=tmp3.t din=tmp2.g skey=new_index tout=tmp4.t dout=tmp4.g"
            <<"atrescaleaxis input=tmp4.g s1=365 output=tmp4b.g"
            <<"atwigplot tmp4b.g attrib=tmp4.t overlay_col=duration_1 trace_label_col=label trace_label_height=400 trace_label_align_left=true tbt_scale=true vsize=10 numsize=10"
                " gain=-1 tick_min=0.1 tick_max=0.5 &"
            <<"atpercentilerank dist=tmp4b.g value_tbl=tmp4.t value_col=duration_1 output=tmp_rank.t"
            <<"atthistogram input=tmp_rank.t expr=\"rank\" output=tmp_rank2.t"
            <<"attxycplot tin=tmp_rank2.t x=rank y=dist polyline=1 miny=0 minx=0 maxx=1"
            ;
        mf.add("benchmark_cost",{"plan_selected_program_projects.t", "plan_selected.t", "plan_selected_program_project_costs.g", "drill_cost.t"})
            <<HELP("benchmark cost for completed wells in the selected plan")
            <<"attsqltool tables=\"plan_selected_program_projects.t plan_selected.t drill_cost.t\""
            " sql=\"select *, "
            " cast((select cost_mm*"+cost_ff+" from @3 where w_gnr_name like project) as float) as cost,"
            "  cost_cond ||' ['|| project ||']' as label from @1 inner join @2 on @1.project=@2.id\" output=tmp1.t"
            <<"attwind din=plan_selected_program_project_costs.g tin=tmp1.t tout=tmp2.t dout=tmp2.g expr=\"cost>0\""
            <<"atwindow input=tmp2.g n1=100 output=tmp3.g"
            <<"attstrsort input=tmp2.t key=cost_cond keep_index=true output=tmp3.t"
            <<"attracesort tin=tmp3.t din=tmp3.g skey=new_index tout=tmp4.t dout=tmp4.g"
            <<"atwigplot tmp4.g attrib=tmp4.t overlay_col=cost trace_label_col=label trace_label_height=450 trace_label_align_left=true tbt_scale=true vsize=15 numsize=10"
                " gain=-1 tick_min=0.1 tick_max=0.5 &"
            <<"attmath tmp4.t cost_f=cost output=tmp5.t"
            <<"atpercentilerank dist=tmp4.g value_tbl=tmp5.t value_col=cost_f output=tmp_rank.t"
            <<"atthistogram input=tmp_rank.t expr=\"rank\" output=tmp_rank2.t"
            <<"attxycplot tin=tmp_rank2.t x=rank y=dist polyline=1 miny=0 minx=0 maxx=1"
                    ;
            ;
    }
    void build_makefile(){
        mf.add("gis", gis_targets)
            <<HELP("Prepare GIS layers for viewing in QGIS project pap_qgis.qgz which has geo-spatial play input parameters");

        mf.hidden_nodes.insert("template_play_report.tex");
        mf.hidden_nodes.insert("template_area_report.tex");
        mf.hidden_nodes.insert("template_execsum_report.tex");
        mf.hidden_nodes.insert("template_latexdoc_top.tex");
        mf.hidden_nodes.insert("template_latexdoc_bot.tex");
        mf.hidden_nodes.insert("multirow.sty");
        mf.hidden_nodes.insert("supertabular.sty");
        mf.hidden_nodes.insert("template_teamreview.dbm");
        mf.hidden_nodes.insert("template_rigaoi_view.dbm");
        mf.hidden_nodes.insert("template_wf_top.tsk");
        mf.hidden_nodes.insert("template_wf_bot.tsk");
        mf.hidden_nodes.insert("template_wf_plotmap.tsk");
        mf.hidden_nodes.insert("template_wf_plotmap2.tsk");
        mf.hidden_nodes.insert("script_clone_portfolio.sql");
        mf.hidden_nodes.insert("script_color_schedule_by_completion_year.sh");
        mf.hidden_nodes.insert("script_color_schedule_by_dependency.sh");
        mf.hidden_nodes.insert("script_color_schedule_by_maturity.sh");
        mf.hidden_nodes.insert("script_color_schedule_by_wcdel.sh");
        mf.hidden_nodes.insert("script_find_well.sh");
        mf.hidden_nodes.insert("script_load_bp2426.sql");
        mf.hidden_nodes.insert("script_my_plan_eval.sql");
        mf.hidden_nodes.insert("script_shpcopy.sh");
        mf.hidden_nodes.insert("script_shpop.sh");
        mf.hidden_nodes.insert("sttool_compare_formations.sh");
        mf.hidden_nodes.insert("sttool_compare_formations_based_epprt_tops.sh");
        mf.hidden_nodes.insert("sttool_find_formation_occurances.sh");
        mf.hidden_nodes.insert("sttool_locate_formation.sh");
        mf.hidden_nodes.insert("sttool_remaining_tops.sh");
        //folders
        mf.hidden_nodes.insert("Play_Concepts");
        mf.hidden_nodes.insert("trap_types");
        mf.hidden_nodes.insert("sde_layers");
        mf.hidden_nodes.insert("source_rock_phase");
        //mf.hidden_nodes.insert("");
        // data
        mf.hidden_nodes.insert(portfolio_path);
        mf.hidden_nodes.insert("pap_qgis.qgz");
        mf.hidden_nodes.insert("aed_portfolio.dbm");
        mf.hidden_nodes.insert("README");
        //mf.hidden_nodes.insert("");
        mf.hidden_nodes.insert("sde_layers");
        mf.hidden_nodes.insert("placeholder.shp");
        mf.hidden_nodes.insert("placeholder_play_arrows.shp");
        mf.hidden_nodes.insert("placeholder_play_locations.shp");
        mf.hidden_nodes.insert("placeholder_play_rrad_locations.shp");
        mf.hidden_nodes.insert("pap_qgis.qgz");
        mf.hidden_nodes.insert("dump");
        mf.hidden_nodes.insert("bu");
        mf.hidden_nodes.insert("aed_portfolio.dbm");
        mf.hidden_nodes.insert("README");
        //mf.hidden_nodes.insert("");
        //planning files
        mf.hidden_nodes.insert("curr.canvas");
        mf.hidden_nodes.insert("");
        mf.hidden_nodes.insert("");
        for(auto & item : plays){
            if(item.first=="unza"){
                continue;
            }
            {
                string input_prefix="play_"+item.first;
                mf.hidden_nodes.insert(input_prefix+"_stratcol.pdf");
                mf.hidden_nodes.insert(input_prefix+"_stratcol.png");
            }
            {
                string play=item.first;
                string basename="papgis_play_"+play+"_phasemap";
                mf.hidden_nodes.insert(basename+".it");
                mf.hidden_nodes.insert("play_"+play+"_booked.shp");
                mf.hidden_nodes.insert("play_"+play+"_booked.it");
                mf.hidden_nodes.insert("analysis_play_"+play+"_wc_pos");
                mf.hidden_nodes.insert("analysis_play_"+play+"_wc_pos.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_del_pos");
                mf.hidden_nodes.insert("analysis_play_"+play+"_del_pos.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_gas_cdf");
                mf.hidden_nodes.insert("analysis_play_"+play+"_gas_fsd");
                mf.hidden_nodes.insert("analysis_play_"+play+"_gas_fsd.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_gas_wad");
                mf.hidden_nodes.insert("analysis_play_"+play+"_gas_wad.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_oil_cdf");
                mf.hidden_nodes.insert("analysis_play_"+play+"_oil_fsd");
                mf.hidden_nodes.insert("analysis_play_"+play+"_oil_fsd.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_oil_wad");
                mf.hidden_nodes.insert("analysis_play_"+play+"_oil_wad.g");
                mf.hidden_nodes.insert("analysis_play_"+play+"_pos_cdf");
                mf.hidden_nodes.insert("analysis_play_"+play+"_geox_gas_fsd");
                mf.hidden_nodes.insert("analysis_play_"+play+"_geox_oil_fsd");

                /*
                for(auto phase : vector<string>({"oil", "gas", "boe"})){
                    mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_"+phase+"findingcost.g");
                    mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_rig_"+phase+"finding_rate.g");
                    mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_rig_"+phase+"finding_rate_dist.pdf");
                    mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_"+phase+"finding_rate_dist.pdf");
                    mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_"+phase+"finding_rate_dist.g");
                    for(auto maturity : vector<string>({"assessed","identified","unidentified", "ytf"})){
                        for(auto scale : vector<string>({"upside", "well"})){
                            mf.hidden_nodes.insert("mcrun_play_"+play+"_"+maturity+"_"+scale+phase+"_dist.g");
                            mf.hidden_nodes.insert("mcrun_play_"+play+"_"+maturity+"_"+scale+phase+"_dist.pdf");
                            mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_"+maturity+"_"+scale+phase+"vol.g");
                        }
                    }
                }
                mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_cost.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_findingcost.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_campaign_rig_years.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_cost_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_cost_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_findingcost_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_findingcost_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_ncomp_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_ncomp_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_nprospect_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_nprospect_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_nseismic_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_nstudy_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_ntargetsuccess_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_targetsuccessrate_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_targetsuccessrate_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_ntargettest_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_nwellsuccess_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_wellsuccessrate_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_wellsuccessrate_dist.pdf");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_level.it");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_stat_sum.dat");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_years_dist.g");
                mf.hidden_nodes.insert("mcrun_play_"+play+"_rig_years_dist.pdf");
                */

            }
        }
        {
            for(int i=1; i<tstf_ntest; i++){
                string test_name = "tstf"+to_string(i);
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(int i=2006; i<current_year; i++){
                string test_name = "tstg"+to_string(i);
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(auto play:seq_plays){
                string test_name = "tstm_"+play;
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(int i=0; i<tstf_ntest; i++){
                string test_name = "tsth"+to_string(i);
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(int i=2014; i<current_year; i++){
                string test_name = "tsti"+to_string(i);
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(int i=2014; i<current_year; i++){
                string test_name = "tstj"+to_string(i);
                mf.hidden_nodes.insert(test_name+"_portfolio.txt");
            }
            for(auto wcdel: vector<string>({"Prospective", "Delineation"})){
                for(auto play: seq_plays){
                    if(play=="unza"){
                        continue;
                    }
                    string test_name = "tstk_"+play+"_"+wcdel;
                    mf.hidden_nodes.insert(test_name+"_portfolio.txt");
                    mf.hidden_nodes.insert(test_name+"_portfolio.t");
                }
            }
        }
        //rules
        mf.hidden_nodes.insert("rm_db_data");
        mf.hidden_nodes.insert("load_db_data");
        mf.hidden_nodes.insert("reload_db_data");
        mf.hidden_nodes.insert("guwi");
        mf.hidden_nodes.insert("rm_db_exports");
        mf.hidden_nodes.insert("export_from_guwi");
        mf.hidden_nodes.insert("import_rrad_shapefiles");
        //mf.hidden_nodes.insert("");
        mf.add("rm_interm")
            <<"rm -f mcrun_* play_* rigaoi_* prms_* test_* tst* bplan_* seisplan_* *~ *.aux *.log *.out *.toc *.acn *.bbl *.blg *.glo *.ist *.sbl"
            ;
        mf.generate_graph();
        mf.generate();
    };

    Main(){
        define_plays();
        define_aois();
        define_drill_zones();
        define_ventures();
        generate_pap_tables();
        load_pap_text_files();
        load_pap_shape_files();
        load_sde_layers();
        create_planning_canvas();
        load_team_portfolios();
        rigdays_analysis();
        wellcost_analysis();
        qc_spot_inputs();
        qc_pap_framework(); //check if we miss any play with gas
        split_input_data_by_play();
        qc_aed_inputs_stage1();
        qc_rrad_inputs_stage1();
        join_spot_tables_with_aed_portfolio();
        join_pvad_tables_with_aed_portfolio();
        qc_ppd_inputs_consistency();
        assign_fsd_and_wad_to_targets();
        create_maps_for_plays_rigaois();
        play_level_analysis();
        monte_carlo_simulations();
        if(generate_testing_report){
            compile_testing_report();
        }
        compile_execsum_report();
        evaluate_plan();
        venture_plan();
        benchmark();
    }
};

int main(int argc, char **argv) {
    Main().build_makefile();
}
