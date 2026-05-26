#include "makefile.hpp"
#include <map>
#include <set>
#include <vector>
#include <cassert>
using namespace std;

#define O "./obj/"
#define I "./inc/"
#define S "./src/"
#define B "./bin/"
#define P "./proj/"
#define D "./dev/"
#define R "./rsc/"


#define ICPC "icpx -std=c++2a -w -O3 -qopenmp -restrict -parallel "
//uncomment if intel compiler is not working
//#define ICPC "g++ -std=c++2a -w -O3 -pthread -ldl"
#define GPP "g++ -std=c++2a -w -O3 -pthread"
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
        string const runsys = "win64";
        string const obj_build= GPP;
        string const exec_build= GPP;
        #define EXT ".exe"
    #else
        string const runsys = "win32";
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
        string const obj_build= "clang++ -std=c++2a -stdlib=libc++ -w -O3 -vec-report3";
        string const exec_build= "clang++ -std=c++2a -stdlib=libc++ -w -O3 -vec-report3";
        #define EXT ""
    #else
        #   error "Unknown Apple platform"
    #endif
#elif __linux__
    // linux
    string const runsys = "linux";
    string const obj_build= ICPC ;
    string const exec_build= ICPC ;
    #define EXT ""
#elif __unix__ // all unices not caught above
    // Unixt
    string const runsys = "unix";
    string const obj_build= ICPC ;
    string const exec_build= ICPC ;
    #define EXT ""
#elif defined(_POSIX_VERSION)
    // POSIX
    string const runsys = "posix";
    string const obj_build= ICPC ;
    string const exec_build= ICPC ;
    #define EXT ""
#else
    #error "Unknown compiler"
#endif

class CompileRule {
public:
    string target;
    vector<string> dep;
    string flags;
    CompileRule(string target, vector<string> dep, string flags="") {
        this->target=target;
        this->dep= dep;
        this->flags=flags;
    }
};

void addexec(BuildGraph& bg, std::vector<CompileRule> & rules, bool optional=false, vector<string> groups={}) {
    string docpath="doc/";
    for(auto r:rules) {
        auto tgt= r.target;
        auto flags = r.flags;
        auto depv = r.dep;
        assert(depv.size()>0);
        string dep;
        for(auto k:depv) {
            dep+=k+ " ";
        }
        string docfname=basename(depv[0])+".h";
        auto & mr = bg.add(tgt, depv);
        if(optional)
            mr<<OPTIONAL;
        else
            mr<<FINAL;
        if(runsys=="win64"){
            mr<<exec_build+" -I" I " -I " R " -I./doc/ -I./lib -L../libs "+dep+" -o $@ "+flags;
        }else{
            mr<<string()+"bash ./atautodocgen "+depv[0]+" "+r.target+" >./doc/"+docfname;
            mr<<exec_build+" -I" I " -I " R " -I./doc/ -I./lib -L../libs -DAUTODOC='\""+docfname+"\"' "+dep+" -o $@ "+flags;
        }
        for(auto g:groups){
            auto & mr = bg.get_rule(g);
            bg.add_source(mr, tgt);
        }
    }
};
void addobj(BuildGraph& bg, string tgt, vector<string> depv){
    bg.add(tgt, depv)
            <<obj_build+" -I" I " -I" R " -c "+depv[0]+" -o $@";
}
int main(int argc, char **argv) {
    BuildGraph bg;
    cout<<"compiling on "<<runsys<<endl;

    bg.add("astyle")
        <<HELP("style source files")
        <<"astyle --options=astylerc java/*.java src/*.cpp inc/*.hpp dev/* qt/*/*.h qt/*/*.cpp";
    bg.add("documentation.txt")
            <<"ls bin/* | awk '{print \"echo \"$0\"############################################### 1>&2; \"$0}' | sh 2>$@";

    bg.add("rm_all")
        <<HELP("remove all object and binary files")
        <<"rm -rf *.o obj/* bin/*";

    //special builds using gcc
    //sqlite library
    bg.add(O "sqlite3.o", {S "sqlite3.c", I "sqlite3.h"})
            <<"gcc -I" I " -c $< -o $@ -lpthread -ldl";
    //shapefiles handling library
    bg.add(O "shpopen.o", {S "shpopen.c"})
            <<"gcc -I" I " -c $< -o $@  ";
    bg.add(O "dbfopen.o", {S "dbfopen.c"})
            <<"gcc -I" I " -c $< -o $@ ";
    bg.add(O "safeileio.o", {S "safileio.c"})
            <<"gcc -I" I " -c $< -o $@ ";
    //UTM-geographic coordinate conversion library
    bg.add(O "UTM.o", {S "UTM.cpp", I "UTM.h"})
            <<"gcc -I" I " -c $< -o $@ ";
    // hash-code library (checksum)
    if(!(runsys=="win64")){
        bg.add(O "sha256.o", {S "sha256.cpp", I "sha256.h"})
            <<"gcc -I" I " -c $< -o $@ ";
    }
    //zipfile handling library
    bg.add(O "miniz.o", {S "miniz.c", I "miniz.h"})
            <<"gcc -I" I " -c $< -o $@ ";
    //XML library
    bg.add(O "pugixml.o", {S "pugixml.cpp", I "pugixml.hpp", I "pugiconfig.hpp"})
            <<"gcc -std=c++2a -I" I " -c $< -o $@ ";
    bg.add(O "jsoncpp.o", {S "jsoncpp.cpp", I "json.h", I "json-forwards.h"})
            <<"gcc -std=c++2a -I" I " -c $< -o $@ ";
    //FFTW library
    bg.add(O "atfftw.o", {S "atfftw.cpp", I "atfftw.hpp", I "atlib.hpp"})
            <<obj_build+" -I" I " -c src/atfftw.cpp -L/home/exp_8/theyabaa/libs -o $@";

    string instantclient_path = "/work0/usr/instantclient_23_6/";
    /*
    bg.add(B "ateppr2tbl" EXT, {S "ateppr2tbl.cpp", O "atlib.o", O "attable.o", O "attutil.o"}) <<OPTIONAL
            <<string()+"bash ./atautodocgen src/ateppr2tbl.cpp >./doc/ateppr2tbl.h"
            <<exec_build+" -I" I" -I./doc/ -DAUTODOC='\"ateppr2tbl.h\"' -I"+instantclient_path+"sdk/include -L"+instantclient_path+" $^ -o $@ -locci -lclntsh -lclntshcore -loramysql -lociei -lnnz21 -D_GLIBCXX_USE_CXX11_ABI=0";
    bg.add(B "atsde2tbl" EXT, {S "atsde2tbl.cpp", O "atlib.o", O "attable.o", O "attutil.o"}) <<OPTIONAL
            <<string()+"bash ./atautodocgen src/atsde2tbl.cpp >./doc/atsde2tbl.h"
            <<exec_build+" -I" I" -I./doc/ -DAUTODOC='\"atsde2tbl.h\"' -I"+instantclient_path+"sdk/include -L"+instantclient_path+" $^ -o $@ -locci -lclntsh -lnnz12 -lclntshcore -lmql1 -lons -lipc1";
            */
    //oracle uses and old cxx11 ABI, so I had to revert to g++4.8 compiler!
    bg.add(B "atsdeget" EXT, {S "atsdeget.cpp"}) <<OPTIONAL
            <<string()+"bash ./atautodocgen src/atsdeget.cpp >./doc/atsdeget.h"
            <<"/usr/bin/g++ -std=c++2a -c src/atlib.cpp -I" I " -o " O "atlib_x.o"
            <<"/usr/bin/g++ -std=c++2a -c src/attable.cpp -I" I " -o " O "attable_x.o"
            <<"/usr/bin/g++ -std=c++2a -I" I" -I./doc/ -DAUTODOC='\"atoracle2tbl.h\"' -I"+instantclient_path+"sdk/include -L"+instantclient_path+" $^ obj/atlib_x.o obj/attable_x.o -o $@ -locci -lclntsh -lnnz -lclntshcore ";
    bg.add(B "atoracle2tbl" EXT, {S "atoracle2tbl.cpp", S "atlib.cpp", S "attable.cpp", I "atlib.hpp", I "attable.hpp"}) <<OPTIONAL
            <<string()+"bash ./atautodocgen src/atsde2tbl.cpp >./doc/atoracle2tbl.h"
            <<"/usr/bin/g++ -std=c++2a -w -c src/atlib.cpp -I" I " -o " O "atlib_x.o"
            <<"/usr/bin/g++ -std=c++2a -w -c src/attable.cpp -I" I " -o " O "attable_x.o"
            //<<"/usr/bin/g++ -std=c++2a -I" I" -I./doc/ -DAUTODOC='\"atoracle2tbl.h\"' -I"+instantclient_path+"sdk/include -L"+instantclient_path+" $^ obj/atlib_x.o obj/attable_x.o -o $@ -locci -lclntsh -lnnz21 -lclntshcore ";
            <<"/usr/bin/g++ -std=c++2a -w -I" I" -I./doc/ -DAUTODOC='\"atoracle2tbl.h\"' -I"+instantclient_path+"sdk/include -L"+instantclient_path+" "+S+"atoracle2tbl.cpp obj/atlib_x.o obj/attable_x.o -o $@ -locci -lclntsh -lnnz -lclntshcore ";
    bg.add("oracle", {B "atoracle2tbl" EXT, B "atsdeget" EXT})
        <<HELP("build Oracle-based executables")
        <<OPTIONAL;

    //petrosys tools
    bg.add(I "atpseis_rasterhdr_seg1_xxd.hpp", I "atpseis_rasterhdr_seg1.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_rasterhdr_seg2_xxd.hpp", I "atpseis_rasterhdr_seg2.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_rasterhdr_seg3_xxd.hpp", I "atpseis_rasterhdr_seg3.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_dbm_header_xxd.hpp", I "atpseis_dbm_header.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_dbm_bglayers_xxd.hpp", I "atpseis_dbm_bglayers.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_dbm_gri_entry_xxd.hpp", I "atpseis_dbm_gri_entry.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_dbm_shp_entry_xxd.hpp", I "atpseis_dbm_shp_entry.h")
            <<"xxd -i $< >$@";
    bg.add(I "atpseis_dbm_group_xxd.hpp", I "atpseis_dbm_group.h")
            <<"xxd -i $< >$@";

    //compiling resources to be included in source files as hpp includes
    bg.add(R "theme1.xml_xxd.hpp", R "theme1.xml")
            <<"xxd -i $< >$@";


    //automatically generated rules
    std::vector<CompileRule> obj_rules= {
        {O "atlib.o", {S "atlib.cpp ", I "atlib.hpp"}},
        {O "attable.o", {S "attable.cpp", I "attable.hpp", I "atlib.hpp"}},
        {O "atsqlite.o", {S "atsqlite.cpp", I "atsqlite.hpp", I "atlib.hpp"}},
        {O "atsqlitetools.o", {S "atsqlitetools.cpp", I "atsqlitetools.hpp", I "atlib.hpp", I "atsqlite.hpp"}},
        {O "atexpr.o", {S "atexpr.cpp", I "atexpr.hpp", I "atlib.hpp"}},
        {O "atjson.o", {S "atjson.cpp", I "atjson.hpp", I "atlib.hpp"}},
        {O "atvecop.o", {S "atvecop.cpp", I "atvecop.hpp", I "atlib.hpp"}},
        {O "atgeom.o", {S "atgeom.cpp", I "atgeom.hpp", I "attable.hpp", I "atvecop.hpp"}},
        {O "atinv.o", {S "atinv.cpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp"}},
        {O "atml.o", {S "atml.cpp", I "atml.hpp", I "atlib.hpp", I "atvecop.hpp", I "atinv.hpp"}},
        {O "atutil.o", {S "atutil.cpp", I "atutil.hpp", I "atlib.hpp"}},
        {O "attutil.o", {S "attutil.cpp", I "attutil.hpp", I "atlib.hpp", I "atvecop.hpp", I "atgeom.hpp"}},
        {O "attau.o", {S "attau.cpp", I "attau.hpp", I "atlib.hpp", I "atvecop.hpp", I "atgeom.hpp"}},
        {O "atwave.o", {S "atwave.cpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp"}},
        //{O "atfftw.o", {S "atfftw.cpp", I "atfftw.hpp", I "atlib.hpp"}},
        {O "atwav.o", {S "atwav.cpp", I "atwav.hpp"}},
        {O "atdecision.o", {S "atdecision.cpp", I "atdecision.hpp", I "attable.hpp", I "atlib.hpp"}},
        {O "atpseis.o", {S "atpseis.cpp", I "atpseis.hpp", I "atlib.hpp",
                I "atpseis_rasterhdr_seg1_xxd.hpp",
                I "atpseis_rasterhdr_seg2_xxd.hpp",
                I "atpseis_rasterhdr_seg3_xxd.hpp",
                I "atpseis_dbm_header_xxd.hpp",
                I "atpseis_dbm_bglayers_xxd.hpp",
                I "atpseis_dbm_gri_entry_xxd.hpp",
                I "atpseis_dbm_shp_entry_xxd.hpp",
                I "atpseis_dbm_group_xxd.hpp",
            }
        },
        {   O "atppt.o", {S "atppt.cpp", I "atppt.hpp", I "atlib.hpp",
                R "theme1.xml_xxd.hpp"
            }
        },
        {O "atspot.o", {S "atspot.cpp", I "atspot.hpp", I "attable.hpp", I "atlib.hpp", I "atppt.hpp", I "atsqlite.hpp"}},
    };
    for(auto r:obj_rules) {
        auto tgt= r.target;
        auto flags = r.flags;
        auto depv = r.dep;
        assert(depv.size()>0);
        bg.add(tgt, depv)
                <<obj_build+" -I" I " -I" R " -c "+depv[0]+" -o $@";
    }
    std::vector<CompileRule> lop_rules= {
        {O "ConvolutionLOp.o", {S "ConvolutionLOp.cpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "ConvolutionLOp.hpp"}},
        {O "WaveformLOpUBC.o", {S "WaveformLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp", I "WaveformLOp.hpp"}, "-DUBC"},
        {O "WaveformLOpVBC.o", {S "WaveformLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp", I "WaveformLOp.hpp"}, "-DVBC"},
        {O "LocalWaveformLOp.o", {S "LocalWaveformLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp", I "LocalWaveformLOp.hpp"}},
        {O "ShotWindowedWaveformLOpUBC.o", {S "ShotWindowedWaveformLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp", I "ShotWindowedWaveformLOp.hpp"}, "-DUBC"},
        {O "ShotWindowedWaveformLOpVBC.o", {S "ShotWindowedWaveformLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "atwave.hpp", I "ShotWindowedWaveformLOp.hpp"}, "-DVBC"},
        {O "TomoLOpV2D.o", {S "TomoLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "attau.hpp", I "atutil.hpp", I "atvecop.hpp", I "TomoLOp.hpp"}},
        {O "TomoLOpV3D.o", {S "TomoLOp.cpp", I "atgeom.hpp", I "atinv.hpp", I "atlib.hpp", I "attau.hpp", I "atutil.hpp", I "atvecop.hpp", I "TomoLOp.hpp"}, "-DV3D"},
        {O "SmootherLOpV2D.o", {S "SmootherLOp.cpp", I "atinv.hpp", I "SmootherLOp.hpp"}},
        {O "SmootherLOpV3D.o", {S "SmootherLOp.cpp", I "atinv.hpp", I "SmootherLOp.hpp"}, "-DV3D"},
        {O "TauPLOp.o", {S "TauPLOp.cpp", I "atfftw.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "TauPLOp.hpp"}},
        {O "TauVLOp.o", {S "TauVLOp.cpp", I "atfftw.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "TauVLOp.hpp"}},
        {O "T2StretchLOp.o", {S "T2StretchLOp.cpp", I "atfftw.hpp", I "atinv.hpp", I "atlib.hpp", I "atvecop.hpp", I "T2StretchLOp.hpp"}},
        {O "IterativeSmootherLOpV2D.o", {S "IterativeSmootherLOp.cpp", I "atinv.hpp", I "atlib.hpp", I "atutil.hpp", I "atvecop.hpp", I "IterativeSmootherLOp.hpp"}},
        {O "IterativeSmoother3DLOp.o", {S "IterativeSmoother3DLOp.cpp", I "atinv.hpp", I "atlib.hpp", I "atutil.hpp", I "atvecop.hpp", I "IterativeSmoother3DLOp.hpp"}},
        {O "StrightRayTomoLOp.o", {S "StrightRayTomoLOp.cpp", I "atinv.hpp", I "atlib.hpp", I "attable.hpp", I "attau.hpp", I "atvecop.hpp", I "StrightRayTomoLOp.hpp"}}
    };
    for(auto r:lop_rules) {
        auto tgt= r.target;
        auto flags = r.flags;
        auto depv = r.dep;
        assert(depv.size()>0);
        bg.add(tgt, depv)
                <<obj_build+" -I" I " "+flags+" -I" R " -c "+depv[0]+" -o $@";
    }
    //translators rules
    std::vector<CompileRule> transl_rules= {
        {B "at2gnuplot" EXT, {S "at2gnuplot.cpp", O "atlib.o", O "atutil.o"}},
        {B "at2ximage" EXT, {S "at2ximage.cpp", O "atlib.o"}},
        {B "at2dpik" EXT, {S "at2dpik.cpp", O "atlib.o"}},
        {B "atxyz2kml" EXT, {S "atxyz2kml.cpp", O "atlib.o", O "attable.o"}},
        {B "atsegyhdr" EXT, {S "atsegyhdr.cpp", O "atlib.o"}},
        {B "atsegyntrace" EXT, {S "atsegyntrace.cpp", O "atlib.o"}},
        {B "atreadsegy_trchdr" EXT, {S "atreadsegy_trchdr.cpp", O "atlib.o"}},
        {B "atreadsegy_trchdr_aramco" EXT, {S "atreadsegy_trchdr_aramco.cpp", O "atlib.o", O "attable.o"}},
        {B "atreadsegy" EXT, {S "atreadsegy.cpp", O "atlib.o"}},
        {B "atreadsegy_petrel" EXT, {S "atreadsegy_petrel.cpp", O "atlib.o", O "attable.o"}},
        {B "atreadsegy_aramco" EXT, {S "atreadsegy_aramco.cpp", O "atlib.o", O "attable.o"}},
        {B "atsegywrite" EXT, {S "atsegywrite.cpp", O "atlib.o"}},
        {B "atreadseg2" EXT, {S "atreadseg2.cpp", O "atlib.o"}},
        {B "atread_poststack_segy_trace_headrs" EXT, {S "atread_poststack_segy_trace_headrs.cpp", O "atlib.o", O "attable.o"}},
        {B "atxdr2native" EXT, {S "atxdr2native.cpp", O "atlib.o"}},
        {B "atgrid2ascii" EXT, {S "atgrid2ascii.cpp", O "atlib.o"}},
        {B "atgrid2tbl" EXT, {S "atgrid2tbl.cpp", O "atlib.o", O "attable.o"}},
        {B "atbin2text" EXT, {S "atbin2text.cpp", O "atlib.o", O "atutil.o"}},
        {B "attext2bin" EXT, {S "attext2bin.cpp", O "atlib.o", O "atutil.o"}},
        {B "atbytefilpreverse" EXT, {S "atbyteflipreverse.cpp", O "atlib.o"}},
        {B "atl2b" EXT, {S "atl2b.cpp"}},
        {B "atchar2int" EXT, {S "atchar2int.cpp", O "atlib.o"}},
        {B "atwritezmap" EXT, {S "atwritezmap.cpp", O "atlib.o"}},
        {B "atreaddpik" EXT, {S "atreaddpik.cpp", O "atlib.o"}},
        {B "att2petrel" EXT, {S "att2petrel.cpp", O "atlib.o", O "attable.o"}},
        {B "att2csv" EXT, {S "att2csv.cpp", O "atlib.o", O "attable.o"}},
        {B "att2ascii" EXT, {S "att2ascii.cpp", O "atlib.o", O "attable.o"}},
        {B "att2latex" EXT, {S "att2latex.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        //{B "att2ppt" EXT, {S "att2ppt.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atinterval2ppt" EXT, {S "atinterval2ppt.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "att2shpfile" EXT, {S "att2shpfile.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atit2shpfile" EXT, {S "atit2shpfile.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atit2plottext" EXT, {S "atit2plottext.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atshp2tbl" EXT, {S "atshp2tbl.cpp", O "atlib.o", O "attable.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atshp2itbl" EXT, {S "atshp2itbl.cpp", O "atlib.o", O "attable.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atreadsac" EXT, {S "atreadsac.cpp", O "atlib.o", O "attable.o"}},
        {B "atdbdump" EXT, {S "atdbdump.cpp", O "atlib.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "atsqlite_run" EXT, {S "atsqlite_run.cpp", O "atlib.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "att2sqlite" EXT, {S "att2sqlite.cpp", O "atlib.o", O "atsqlite.o", O "attable.o",  O "sqlite3.o"}},
        {B "atpseis_new_dbm" EXT, {S "atpseis_new_dbm.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atpseis_addgrp" EXT, {S "atpseis_addgrp.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atpseis_addlyr_bg" EXT, {S "atpseis_addlyr_bg.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atpseis_addlyr_raster" EXT, {S "atpseis_addlyr_raster.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atpseis_addlyr_gri" EXT, {S "atpseis_addlyr_gri.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atpseis_addlyr_shapefile" EXT, {S "atpseis_addlyr_shapefile.cpp", O "atlib.o", O "atpseis.o", O "atutil.o"}},
        {B "atcharstat" EXT, {S "atcharstat.cpp", O "atlib.o", O "attable.o"}},
        {B "atutm2wgs84" EXT, {S "atutm2wgs84.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "UTM.o"}},
        {B "atwgs84toutm" EXT, {S "atwgs84toutm.cpp", O "atlib.o", O "attable.o", O "UTM.o"}},
        {B "atunifyutm" EXT, {S "atunifyutm.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "UTM.o"}},
        {B "atutmfunc" EXT, {S "atutmfunc.cpp", O "atlib.o", O "atvecop.o", O "UTM.o"}},
        {B "atmakeppt" EXT, {S "atmakeppt.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "miniz.o", O "pugixml.o", O "atppt.o"}},
        {B "atitsplitshapes" EXT, {S "atitsplitshapes.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
    };
    if(!(runsys=="win64")){
        transl_rules.push_back({B "atreadlas" EXT, {S "atreadlas.cpp", O "atlib.o"}});
        transl_rules.push_back({B "atreadhorizon" EXT, {S "atreadhorizon.cpp", O "atlib.o", O "attable.o"}});
        transl_rules.push_back({B "atreadascii2grid" EXT, {S "atreadascii2grid.cpp", O "atlib.o"}});
        transl_rules.push_back({B "atreadascii2tbl" EXT, {S "atreadascii2tbl.cpp", O "atlib.o", O "attable.o"}});
        transl_rules.push_back({B "atreadpetrel2tbl" EXT, {S "atreadpetrel2tbl.cpp", O "atlib.o", O "attable.o"}});
        transl_rules.push_back({B "atwavencode" EXT, {S "atwavencode.cpp", O "atwav.o", O "atlib.o", O "atutil.o", O "atfftw.o", O "sha256.o"}, "-lfftw3f"});
        transl_rules.push_back({B "atwavdecode" EXT, {S "atwavdecode.cpp", O "atwav.o", O "atlib.o", O "atutil.o", O "atfftw.o", O "sha256.o"}, "-lfftw3f"});
    }
    addexec(bg, transl_rules);
    //general utilities
    std::vector<CompileRule> genutil_rules= {
        {B "atgetval" EXT, {S "atgetval.cpp", O "atlib.o", O "atutil.o"}},
        {B "atgetval_from_std" EXT, {S "atgetval_from_std.cpp", O "atlib.o"}},
        {B "atbjob" EXT, {S "atbjob.cpp", O "atlib.o"}},
        {B "atsjob" EXT, {S "atsjob.cpp", O "atlib.o"}},
        {B "atmpirankfilegen" EXT, {S "atmpirankfilegen.cpp", O "atlib.o"}},
        {B "atmpitrack" EXT, {S "atmpitrack.cpp", O "atlib.o"}},
        {B "atmpirun" EXT, {S "atmpirun.cpp", O "atlib.o"}},
        {B "atallocate" EXT, {S "atallocate.cpp", O "atlib.o"}},
        {B "atpptslidegrid" EXT, {S "atpptslidegrid.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
    };
    addexec(bg, genutil_rules);

    //grid operations
    std::vector<CompileRule> arrayop_rules= {
        {B "atin" EXT, {S "atin.cpp", O "atlib.o"}},
        {B "atreshape" EXT, {S "atreshape.cpp", O "atlib.o"}},
        {B "atrefine" EXT, {S "atrefine.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atlastframe" EXT, {S "atlastframe.cpp", O "atlib.o"}},
        {B "atwindow" EXT, {S "atwindow.cpp", O "atlib.o"}},
        {B "atautowindow" EXT, {S "atautowindow.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atreverse1" EXT, {S "atreverse1.cpp", O "atlib.o"}},
        {B "atreverse3" EXT, {S "atreverse3.cpp", O "atlib.o"}},
        {B "atrescaleaxis" EXT, {S "atrescaleaxis.cpp", O "atlib.o"}},
        {B "atdfd3" EXT, {S "atdfd3.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atsetboundary" EXT, {S "atsetboundary.cpp", O "atlib.o"}},
        {B "atpad" EXT, {S "atpad.cpp", O "atlib.o"}},
        {B "atunpad" EXT, {S "atunpad.cpp", O "atlib.o"}},
        {B "atkillnan" EXT, {S "atkillnan.cpp", O "atlib.o"}},
        {B "atcat1" EXT, {S "atcat1.cpp", O "atlib.o"}},
        {B "atcat2" EXT, {S "atcat2.cpp", O "atlib.o"}},
        {B "atcat3" EXT, {S "atcat3.cpp", O "atlib.o"}},
        {B "attransp" EXT, {S "attransp.cpp", O "atlib.o"}},
        {B "attransp23" EXT, {S "attransp23.cpp", O "atlib.o"}},
        {B "attaper" EXT, {S "attaper.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atautobalance" EXT, {S "atautobalance.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        {B "attrimvol" EXT, {S "attrimvol.cpp", O "atlib.o"}},
        {B "atstack2" EXT, {S "atstack2.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atstack3" EXT, {S "atstack3.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atmath" EXT, {S "atmath.cpp", O "atlib.o"}},
        {B "atnewgridbyextent" EXT, {S "atnewgridbyextent.cpp", O "atlib.o"}},
        {B "atgridstats" EXT, {S "atgridstats.cpp", O "atlib.o", O "atutil.o"}},
        {B "atgridvalueat" EXT, {S "atgridvalueat.cpp", O "atlib.o"}},
        {B "atlaplace" EXT, {S "atlaplace.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atsmooth" EXT, {S "atsmooth.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "SmootherLOpV2D.o"}},
        {B "atsmooth3d" EXT, {S "atsmooth.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "SmootherLOpV3D.o"}, "-DV3D"},
        {B "atitrsmooth" EXT, {S "atitrsmooth.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atitrsmooth3d" EXT, {S "atitrsmooth3d.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atpercentile1d" EXT, {S "atpercentile1d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atpercentile2d" EXT, {S "atpercentile2d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atpercentile3d" EXT, {S "atpercentile3d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atdespike2d" EXT, {S "atdespike2d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atdfd1" EXT, {S "atdfd1.cpp", O "atlib.o"}},
        {B "atdfd1_fast" EXT, {S "atdfd1_fast.cpp", O "atlib.o"}},
        {B "atdfd2" EXT, {S "atdfd2.cpp", O "atlib.o"}},
        {B "atlmax1" EXT, {S "atlmax1.cpp", O "atlib.o"}},
        {B "atintegrate" EXT, {S "atintegrate.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atboxsmooth" EXT, {S "atboxsmooth.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atmedian3d" EXT, {S "atmedian3d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atmedian2d" EXT, {S "atmedian2d.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atidft3" EXT, {S "atidft3.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atfft1" EXT, {S "atfft1.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atfft2" EXT, {S "atfft2.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atfft3" EXT, {S "atfft3.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atifft1" EXT, {S "atifft1.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atidft1" EXT, {S "atidft1.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atifft3" EXT, {S "atifft3.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atfddfd1" EXT, {S "atfddfd1.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atclusterswap" EXT, {S "atclusterswap.cpp", O "atlib.o"}},
        {B "atautocrop" EXT, {S "atautocrop.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        {B "athilbert_phase" EXT, {S "athilbert_phase.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atbestscale" EXT, {S "atbestscale.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
    };
    addexec(bg, arrayop_rules);

    //signal processing // any thing that deals with waveforms
    std::vector<CompileRule> signal_rules= {
        {B "atwlet1" EXT, {S "atwlet1.cpp", O "atlib.o"}},
        {B "atwlet2" EXT, {S "atwlet2.cpp", O "atlib.o"}},
        {B "atwlet3" EXT, {S "atwlet3.cpp", O "atlib.o"}},
        {B "atwlet4" EXT, {S "atwlet4.cpp", O "atlib.o"}},
        {B "atnormalize" EXT, {S "atnormalize.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atagc" EXT, {S "atagc.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atxcorr1d2d" EXT, {S "atxcorr1d2d.cpp", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atgapdecon" EXT, {S "atgapdecon.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        {B "atmatchfilter" EXT, {S "atmatchfilter.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        {B "atconv" EXT, {S "atconv.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atxcorr" EXT, {S "atxcorr.cpp", O "atlib.o", O "atutil.o"}},
        {B "atxcorrmaxlag" EXT, {S "atxcorrmaxlag.cpp", O "atlib.o", O "atutil.o"}},
        {B "atdotprod" EXT, {S "atdotprod.cpp", O "atlib.o"}},
        {B "atmatchd" EXT, {S "atmatchd.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atmatchd_windowed" EXT, {S "atmatchd_windowed.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atfilter" EXT, {S "atfilter.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atasubtract" EXT, {S "atasubtract.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "attracerms" EXT, {S "attracerms.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atspecnormalize" EXT, {S "atspecnormalize.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atphasediff" EXT, {S "atphasediff.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atfkfilt" EXT, {S "atfkfilt.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atfkfilt3d" EXT, {S "atfkfilt3d.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atconv1" EXT, {S "atconv1.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atbpfilt" EXT, {S "atbpfilt.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atbang" EXT, {S "atbang.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atwlet6" EXT, {S "atwlet6.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atwlet5" EXT, {S "atwlet5.cpp", O "atfftw.o", O "atinv.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atresamp" EXT, {S "atresamp.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atspectralfactorization" EXT, {S "atspectralfactorization.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}, "-lfftw3f"},
    };
    addexec(bg, signal_rules);


    //grids and tables
    std::vector<CompileRule> gnt_rules= {
        {B "atkeymute" EXT, {S "atkeymute.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atkeyshift" EXT, {S "atkeyshift.cpp", O "atfftw.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atkeystack" EXT, {S "atkeystack.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atxyzfill" EXT, {S "atxyzfill.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atxyzfillsmooth" EXT, {S "atxyzfillsmooth.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atxygaussfill" EXT, {S "atxygaussfill.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atimgpocs_prep" EXT, {S "atimgpocs_prep.cpp", O "atlib.o", O "attable.o"}},
        {B "atimgpocs" EXT, {S "atimgpocs.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atsplice2" EXT, {S "atsplice2.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
    };
    addexec(bg, gnt_rules);

    //table operations
    std::vector<CompileRule> tblop_rules= {
        {B "attregression" EXT, {S "attregression.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "attregressionl1" EXT, {S "attregressionl1.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "attwind" EXT, {S "attwind.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atttee" EXT, {S "atttee.cpp", O "atlib.o", O "attable.o"}},
        {B "attdiff" EXT, {S "attdiff.cpp", O "atlib.o", O "attable.o"}},
        {B "attuniq" EXT, {S "attuniq.cpp", O "atlib.o", O "attable.o"}},
        {B "attaddstringcol" EXT, {S "attaddstringcol.cpp", O "atlib.o", O "attable.o"}},
        {B "attaddidcol" EXT, {S "attaddidcol.cpp", O "atlib.o", O "attable.o"}},
        {B "attaccumulate" EXT, {S "attaccumulate.cpp", O "atlib.o", O "attable.o"}},
        {B "attrmcol" EXT, {S "attrmcol.cpp", O "atlib.o", O "attable.o"}},
        {B "attconstcols" EXT, {S "attconstcols.cpp", O "atlib.o", O "attable.o"}},
        {B "attcolstats" EXT, {S "attcolstats.cpp", O "atlib.o", O "attable.o"}},
        {B "att2colstats" EXT, {S "att2colstats.cpp", O "atlib.o", O "attable.o"}},
        {B "attint2strcol" EXT, {S "attint2strcol.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attrange" EXT, {S "attrange.cpp", O "atlib.o", O "attable.o"}},
        {B "attin" EXT, {S "attin.cpp", O "atlib.o", O "attable.o"}},
        {B "attassert_nrow" EXT, {S "attassert_nrow.cpp", O "atlib.o", O "attable.o"}},
        {B "attprintrow" EXT, {S "attprintrow.cpp", O "atlib.o", O "attable.o"}},
        {B "atnewtable" EXT, {S "atnewtable.cpp", O "atlib.o", O "attable.o"}},
        {B "atthead" EXT, {S "atthead.cpp", O "atlib.o", O "attable.o"}},
        {B "attcoldump" EXT, {S "attcoldump.cpp", O "atlib.o", O "attable.o"}},
        {B "attmath" EXT, {S "attmath.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "attrotate" EXT, {S "attrotate.cpp", O "atlib.o", O "attable.o"}},
        {B "attgrid2col" EXT, {S "attgrid2col.cpp", O "atlib.o", O "attable.o"}},
        {B "attcat" EXT, {S "attcat.cpp", O "atlib.o", O "attable.o"}},
        {B "atttransp" EXT, {S "atttransp.cpp", O "atlib.o", O "attable.o"}},
        {B "atdrawbox" EXT, {S "atdrawbox.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atdrawline" EXT, {S "atdrawline.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "attsort" EXT, {S "attsort.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "attstrsort" EXT, {S "attstrsort.cpp", O "atlib.o", O "attable.o"}},
        {B "atthistogram" EXT, {S "atthistogram.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attlocalstat" EXT, {S "attlocalstat.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attracesort" EXT, {S "attracesort.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "attinpolygon" EXT, {S "attinpolygon.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attinpolygons" EXT, {S "attinpolygons.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attinpolygonrel" EXT, {S "attinpolygonrel.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attrelstats" EXT, {S "attrelstats.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atpolygonoverlap" EXT, {S "atpolygonoverlap.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atpolygonoverlaparea" EXT, {S "atpolygonoverlaparea.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atitinpolygons" EXT, {S "atitinpolygons.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atsurfinpolygon" EXT, {S "atsurfinpolygon.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atsurfinpolygons" EXT, {S "atsurfinpolygons.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attsqltool" EXT, {S "attsqltool.cpp", O "atlib.o", O "atutil.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "attaddcol" EXT, {S "attaddcol.cpp", O "atlib.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "attupdate" EXT, {S "attupdate.cpp", O "atlib.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "atitaddattrib" EXT, {S "atitaddattrib.cpp", O "atlib.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "attsetwhere" EXT, {S "attsetwhere.cpp", O "atlib.o", O "atutil.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "atitselect" EXT, {S "atitselect.cpp", O "atlib.o", O "atutil.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "atitwind" EXT, {S "atitwind.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitintervalwind" EXT, {S "atitintervalwind.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitsubset" EXT, {S "atitsubset.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitdumptable" EXT, {S "atitdumptable.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitaddtable" EXT, {S "atitaddtable.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitsort" EXT, {S "atitsort.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atitstat" EXT, {S "atitstat.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "atitslope" EXT, {S "atitslope.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "atittee" EXT, {S "atittee.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atindextable" EXT, {S "atindextable.cpp", O "atlib.o", O "attable.o"}},
        {B "atunindextable" EXT, {S "atunindextable.cpp", O "atlib.o", O "attable.o"}},
        {B "attindexcolumns" EXT, {S "attindexcolumns.cpp", O "atlib.o", O "attable.o"}},
        {B "atitjoin" EXT, {S "atitjoin.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atitmerge" EXT, {S "atitmerge.cpp", O "atlib.o", O "attable.o"}},
        {B "atitlookup" EXT, {S "atitlookup.cpp", O "atlib.o", O "attable.o"}},
        {B "atitsortindex" EXT, {S "atitsortindex.cpp", O "atlib.o", O "attable.o"}},
        {B "atitdumpasarray" EXT, {S "atitdumpasarray.cpp", O "atlib.o", O "attable.o"}},
        {B "atlargestpolygon" EXT, {S "atlargestpolygon.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atareacalc" EXT, {S "atareacalc.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "attcategory2num" EXT, {S "attcategory2num.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atdistance_to_polygon" EXT, {S "atdistance_to_polygon.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atdistance_to_polygons" EXT, {S "atdistance_to_polygons.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atclosest_polygon" EXT, {S "atclosest_polygon.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "attforeach" EXT, {S "attforeach.cpp", O "atlib.o", O "atutil.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
        {B "attlistcol2it" EXT, {S "attlistcol2it.cpp", O "atlib.o", O "attable.o"}},
        {B "attargfromcol" EXT, {S "attargfromcol.cpp", O "atlib.o", O "attable.o"}},
        {B "attloosesearch" EXT, {S "attloosesearch.cpp", O "atlib.o", O "attable.o"}},
        {B "attgroupby" EXT, {S "attgroupby.cpp", O "atlib.o", O "atutil.o", O "atsqlite.o", O "attable.o", O "sqlite3.o"}},
    };
    if(!(runsys=="win64")){
        tblop_rules.push_back({B "attdate2int" EXT, {S "attdate2int.cpp", O "atlib.o", O "attable.o"}});
    }
    addexec(bg, tblop_rules);

    //matrix operations
    //   - few developed during Long-Beach project
    std::vector<CompileRule> matrixop_rules= {
        {B "atcgsolve" EXT, {S "atcgsolve.cpp", O "atlib.o"}},
        {B "atlcurve" EXT, {S "atlcurve.cpp", O "atlib.o", O "atutil.o"}},
        {B "atminreg" EXT, {S "atminreg.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atmatmult" EXT, {S "atmatmult.cpp", O "atlib.o"}},
        {B "atmatvecmult" EXT, {S "atmatvecmult.cpp", O "atlib.o"}},
        {B "atdiag" EXT, {S "atdiag.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atccmat" EXT, {S "atccmat.cpp", O "atlib.o", O "atvecop.o"}},
    };
    addexec(bg, matrixop_rules);

    //geological computations
    std::vector<CompileRule> geo_rules= {
        {B "atgeodist" EXT, {S "atgeodist.cpp", O "atlib.o"}},
        {B "atalphavel" EXT, {S "atalphavel.cpp", O "atlib.o", O "atvecop.o"}},
    };
    addexec(bg, geo_rules);

    //seismic modeling
    std::vector<CompileRule> seismdl_rules= {
        {B "atfullapr2d" EXT, {S "atfullapr2d.cpp", O "atlib.o", O "attable.o"}},
        {B "atobs3d" EXT, {S "atobs3d.cpp", O "atlib.o", O "attable.o"}},
        {B "atfixedspread" EXT, {S "atfixedspread.cpp", O "atlib.o", O "attable.o"}},
        {B "atrandloc" EXT, {S "atrandloc.cpp", O "atlib.o", O "attable.o"}},
        {B "atcigloc" EXT, {S "atcigloc.cpp", O "atlib.o", O "attable.o"}},
        {B "atsrc" EXT, {S "atsrc.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atmsrc" EXT, {S "atmsrc.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "ateikonal2d" EXT, {S "ateikonal3d.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "ateikonal3d" EXT, {S "ateikonal3d.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}, "-DV3D"},
        {B "atraytrace" EXT, {S "atraytrace.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "attmodel" EXT, {S "attmodel.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "TomoLOpV2D.o"}},
        {B "attmodel3d" EXT, {S "attmodel.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "TomoLOpV3D.o"}, "-DV3D"},
        {B "atprop1d" EXT, {S "atprop1d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atprop1d_ubc" EXT, {S "atprop1d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-DUBC"},
        {B "atwmodel1d" EXT, {S "atwmodel1d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atprop2d" EXT, {S "atprop2d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atprop2d_ubc" EXT, {S "atprop2d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-DUBC"},
        {B "atprop2d_vbc" EXT, {S "atprop2d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-DVBC"},
        {B "atprop3d" EXT, {S "atprop3d.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o"}},
        {B "at3dcost" EXT, {S "at3dcost.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o"}},
        {B "ateprop" EXT, {S "ateprop.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "ateprop3d" EXT, {S "ateprop.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-DV3D"},
        {B "atemodel1d" EXT, {S "atemodel1d.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atbprop" EXT, {S "atbprop.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atbackprop" EXT, {S "atbackprop.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        {B "atemodel2d" EXT, {S "atemodel.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        {B "atemodel3d" EXT, {S "atemodel.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DV3D"},
        {B "atawemodel" EXT, {S "atawemodel.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        {B "atanalytical_rayleigh" EXT, {S "atanalytical_rayleigh.cpp", O "atlib.o"}},
        {B "atwaterphases" EXT, {S "atwaterphases.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        {B "atphaseshiftprop" EXT, {S "atphaseshiftprop.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-lfftw3f"},
        {B "atpsprop1d" EXT, {S "atpsprop1d.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-lfftw3f"},
        {B "atpsmodel1d" EXT, {S "atpsmodel1d.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-lfftw3f"},
        //{B "atpsmigres" EXT, {S "atpsmigres.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o", O "atwave.o"}, "-lfftw3f"},
    };
    addexec(bg, seismdl_rules);

    //seismic computations
    std::vector<CompileRule> seisproc_rules= {
        {B "atvrms" EXT, {S "atvrms.cpp", O "atlib.o"}},
        {B "atnmo" EXT, {S "atnmo.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atcmpstack" EXT, {S "atcmpstack.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atpstm" EXT, {S "atpstm.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atprestackpstm" EXT, {S "atprestackpstm.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atautopickfirstbreak" EXT, {S "atautopickfirstbreak.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atodcig2adcig" EXT, {S "atodcig2adcig.cpp", O "atlib.o", O "atvecop.o", O "atwave.o"}},
        {B "atlsadcig" EXT, {S "atlsadcig.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        {B "at3dprecond" EXT, {S "at3dprecond.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atautorotategeom" EXT, {S "atautorotategeom.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atthdr2svy" EXT, {S "atthdr2svy.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        //{B "atthdr2srcgeom" EXT, {S "atthdr2srcgeom.cpp", O "atlib.o", O "attable.o"}},
        //{B "atthdr2recgeom" EXT, {S "atthdr2recgeom.cpp", O "atlib.o", O "attable.o"}},
        {B "atgeom2svy" EXT, {S "atgeom2svy.cpp", O "atlib.o", O "attable.o"}},
        {B "atgridgeom" EXT, {S "atgridgeom.cpp", O "atlib.o", O "attable.o"}},
        {B "ataddstn" EXT, {S "ataddstn.cpp", O "atgeom.o", O "atlib.o", O "attable.o"}},
        {B "atgeom2movie" EXT, {S "atgeom2movie.cpp", O "atgeom.o", O "atlib.o", O "attable.o"}},
        {B "atmap3dgeom" EXT, {S "atmap3dgeom.cpp", O "atgeom.o", O "atlib.o", O "attable.o"}},
        {B "atraydensity" EXT, {S "atraydensity.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "TomoLOpV2D.o"}},
        {B "atraydensity3d" EXT, {S "atraydensity.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "TomoLOpV3D.o"}, "-DV3D"},
        {B "atraytomo2d" EXT, {S "atraytomo.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "SmootherLOpV2D.o", O "TomoLOpV2D.o"}},
        {B "atraytomo3d" EXT, {S "atraytomo.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "SmootherLOpV3D.o", O "TomoLOpV3D.o"}, "-DV3D"},
        {B "atstrightrayrefltomo" EXT, {S "atstrightrayrefltomo.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o", O "SmootherLOpV2D.o", O "StrightRayTomoLOp.o"}},
        {B "atstrightraymig" EXT, {S "atstrightraymig.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atstrightraycag" EXT, {S "atstrightraycag.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "attimetable" EXT, {S "attimetable.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atkirchmig" EXT, {S "atkirchmig.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atkirchmig_voz" EXT, {S "atkirchmig_voz.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atfwi_ubc" EXT, {S "atfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "WaveformLOpUBC.o"}, "-DUBC"},
        {B "atfwi_vbc" EXT, {S "atfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "WaveformLOpVBC.o"}, "-DVBC"},
        {B "atfwi_linesearch_ubc" EXT, {S "atfwi_linesearch.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "WaveformLOpUBC.o"}, "-DUBC"},
        {B "atarctanfwi_ubc" EXT, {S "atarctanfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "ConvolutionLOp.o", O "WaveformLOpUBC.o"}, "-DUBC"},
        {B "atsdsfwi" EXT, {S "atsdsfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "LocalWaveformLOp.o"}, "-DUBC" },
        {B "atodsfwi" EXT, {S "atodsfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "ShotWindowedWaveformLOpUBC.o"}, "-DUBC" },
        {B "atseqfwi_ubc" EXT, {S "atseqfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "ShotWindowedWaveformLOpUBC.o"}, "-DUBC"},
        {B "atseqfwi_vbc" EXT, {S "atseqfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "ShotWindowedWaveformLOpVBC.o"}, "-DVBC"},
        //{B "atprestackrtm" EXT, {S "atprestackrtm.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "ShotWindowedWaveformLOpUBC.o"}, "-DUBC"},
        //{B "atrollingoffsetfwi" EXT, {S "atrollingoffsetfwi.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atwave.o", O "WaveformLOpUBC.o"}, "-DUBC"},
        //{B "atbeacon2d" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        //{B "atbeacon3d" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DV3D"},
        {B "atbeacon2d_ubc" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DUBC"},
        //{B "atbeacon2d_vbc" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DVBC"},
        //{B "atbeacon3d_ubc" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DV3D -DUBC"},
        //{B "atbeacon2d_ubc_swf" EXT, {S "atbeacon.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DUBC -DSAVEWF"},
        {B "atharbor_ubc" EXT, {S "atharbor.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DUBC"},
        //{B "atharbor_ubc_swf" EXT, {S "atharbor.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DUBC -DSAVEWF"},
        {B "atodcig_vbc" EXT, {S "atodcig.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DVBC"},
        {B "atlssrcest" EXT, {S "atlssrcest.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}},
        //{B "atlssrcest_vbc" EXT, {S "atlssrcest.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DVBC"},
        {B "atlssrcest_ubc" EXT, {S "atlssrcest.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o", O "atwave.o"}, "-DUBC"},
        {B "attaup" EXT, {S "attaup.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atlstaup" EXT, {S "atlstaup.cpp", O "atfftw.o", O "atinv.o", O "atlib.o", O "atvecop.o", O "T2StretchLOp.o", O "TauPLOp.o", O "TauVLOp.o"}, "-lfftw3f"},
    };
    addexec(bg, seisproc_rules);

    //seismic interferometry
    std::vector<CompileRule> seisif_rules= {
        {B "atnmig" EXT, {S "atnmig.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atmcninv" EXT, {S "atmcninv.cpp", O "atlib.o", O "attable.o"}},
        {B "atnaturalmig" EXT, {S "atnaturalmig.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atnaturalhessian" EXT, {S "atnaturalhessian.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atnaturalmute" EXT, {S "atnaturalmute.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atnaturaldemig" EXT, {S "atnaturaldemig.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atnormalg" EXT, {S "atnormalg.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atpassive2active" EXT, {S "atpassive2active.cpp", O "atlib.o", O "attable.o"}},
        {B "atpassive2active_all" EXT, {S "atpassive2active_all.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        //stuff from USArray natural migration project
        {B "atusacatsac" EXT, {P "atusacatsac.cpp", O "atlib.o", O "attable.o"}},
        {B "atusachecksacstn" EXT, {P "atusachecksacstn.cpp", O "atlib.o"}},
        {B "atusarecip" EXT, {P "atusarecip.cpp", O "atlib.o", O "attable.o"}},
        {B "atusasetloc" EXT, {P "atusasetloc.cpp", O "atlib.o", O "attable.o"}},
        {B "atusangfmig" EXT, {P "atusangfmig.cpp", O "atlib.o", O "attable.o"}},
        {B "atusangfmigac" EXT, {P "atusangfmigac.cpp", O "atlib.o", O "attable.o"}},
        {B "atusangfborn" EXT, {P "atusangfborn.cpp", O "atlib.o", O "attable.o"}},
        {B "atusa2stn" EXT, {P "atusa2stn.cpp", O "atlib.o", O "attable.o"}},
        {B "atusaslm" EXT, {P "atusaslm.cpp", O "atlib.o", O "attable.o"}},
        {B "atusareadtt" EXT, {P "atusareadtt.cpp", O "atlib.o", O "attable.o"}},
        {B "atusattinterp" EXT, {P "atusattinterp.cpp", O "atlib.o", O "attable.o"}},
        {B "atusatbl2grid" EXT, {P "atusatbl2grid.cpp", O "atlib.o", O "attable.o"}},
        {B "atusatimetable" EXT, {P "atusatimetable.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atusaslice" EXT, {P "atusaslice.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusaflatten" EXT, {P "atusaflatten.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusaenvelop" EXT, {P "atusaenvelop.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusakirchmig01" EXT, {P "atusakirchmig01.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attau.o", O "atutil.o", O "atvecop.o"}},
        {B "atusareadhgt" EXT, {P "atusareadhgt.cpp", O "atlib.o"}},
        {B "atusademmrg" EXT, {P "atusademmrg.cpp", O "atlib.o"}},
        {B "atusaplanewave" EXT, {P "atusaplanewave.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusaslmtrace" EXT, {P "atusaslmtrace.cpp", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusadeflatten" EXT, {P "atusadeflatten.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusaconstvelflatten" EXT, {P "atusaconstvelflatten.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusawavepath" EXT, {P "atusawavepath.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusamonochromlsm" EXT, {P "atusamonochromlsm.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atusacoh" EXT, {P "atusacoh.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusawetomo" EXT, {P "atusawetomo.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atusaimgpntgees" EXT, {P "atusaimgpntgees.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atusaslicegees" EXT, {P "atusaslicegees.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atusahrngfmig" EXT, {P "atusahrngfmig.cpp", O "atlib.o", O "attable.o"}},
        {B "atusamonochromidft" EXT, {P "atusamonochromidft.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atusamonochrommig" EXT, {P "atusamonochrommig.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atusamonochromborn" EXT, {P "atusamonochromborn.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atusaifft" EXT, {P "atusaifft.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        //{B "atlb_gstack" EXT, {P "atlb_gstack.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o"}},
        {B "atlb_makereg" EXT, {P "atlb_makereg.cpp", O "atlib.o", O "attable.o"}},
        {B "atlb_distmatrix" EXT, {P "atlb_distmatrix.cpp", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atlb_lonlat2xy" EXT, {P "atlb_lonlat2xy.cpp", O "atlib.o", O "attable.o"}},
        {B "atlb_slm" EXT, {P "atlb_slm.cpp", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atlb_lavgmatrix" EXT, {P "atlb_lavgmatrix.cpp", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atlb_maskoffdiag" EXT, {P "atlb_maskoffdiag.cpp", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atlb_median" EXT, {P "atlb_median.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atlb_medianfilt" EXT, {P "atlb_medianfilt.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atlb_cleanimg" EXT, {P "atlb_cleanimg.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atlb_denoisehessinv" EXT, {P "atlb_denoisehessinv.cpp", O "atlib.o"}},
    };
    if(!(runsys=="win64")){
        seisif_rules.push_back({B "atusaginterp" EXT, {P "atusaginterp.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}});
    }
    addexec(bg, seisif_rules);


    //seismic interpretation tools
    std::vector<CompileRule> seisinterp_rules= {
        {B "atouteronly" EXT, {S "atouteronly.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
        {B "atcontourval" EXT, {S "atcontourval.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atfourways" EXT, {S "atfourways.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atmscont" EXT, {S "atmscont.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atgridcontour" EXT, {S "atgridcontour.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "attimedepthconvert" EXT, {S "attimedepthconvert.cpp", O "atlib.o", O "atutil.o"}},
        //{B "atgtime" EXT, {S "atgtime.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "attutil.o", O "atvecop.o", O "SmootherLOpV3D.o"}, "-DV3D"},
        {B "atrelage" EXT, {S "atrelage.cpp", O "atinv.o", O "atlib.o", O "atutil.o", O "atvecop.o"}},
        {B "atagevol" EXT, {S "atagevol.cpp", O "atinv.o", O "atlib.o", O "atvecop.o", O "SmootherLOpV3D.o"}, "-DV3D"},
        {B "atdecontour2d" EXT, {S "atdecontour2d.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o", O "IterativeSmootherLOpV2D.o"}},
        {B "atdecontour3d" EXT, {S "atdecontour3d.cpp", O "atinv.o", O "atlib.o", O "atutil.o", O "atvecop.o", O "IterativeSmoother3DLOp.o"}},
        {B "atreconstruct" EXT, {S "atreconstruct.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atseqstratmovie" EXT, {S "atseqstratmovie.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atwheeler" EXT, {S "atwheeler.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atfaultpolygon" EXT, {S "atfaultpolygon.cpp", O "atlib.o", O "attable.o"}},
        {B "atshift" EXT, {S "atshift.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atinlinedip" EXT, {S "atinlinedip.cpp", O "atfftw.o", O "atlib.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atxlinedip" EXT, {S "atxlinedip.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atwsmooth" EXT, {S "atwsmooth.cpp", O "atlib.o", O "atvecop.o"}},
        {B "at1waytime" EXT, {S "at1waytime.cpp", O "atlib.o", O "atvecop.o"}},
        //{B "atzeig" EXT, {S "atzeig.cpp", O "atlib.o", O "atvecop.o"}},
        //{B "ateigdiff" EXT, {D "ateigdiff.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atgeobody_autocolor" EXT, {S "atgeobody_autocolor.cpp", O "atlib.o"}},
        {B "athzclean" EXT, {S "athzclean.cpp", O "atlib.o"}},
        {B "athzmrg" EXT, {S "athzmrg.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "athz2it" EXT, {S "athz2it.cpp", O "atlib.o", O "attable.o"}},
        {B "atsedmodel" EXT, {S "atsedmodel.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atregtd" EXT, {S "atregtd.cpp", O "atlib.o", O "attable.o"}},
        //{B "atfetcharea" EXT, {D "atfetcharea.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atmd2xyz" EXT, {S "atmd2xyz.cpp", O "atlib.o", O "attable.o"}},
        {B "attrackgen" EXT, {S "attrackgen.cpp", O "atlib.o", O "attable.o"}},
        {B "atwelleval" EXT, {S "atwelleval.cpp", O "atlib.o", O "attable.o"}},
        {B "atsurfpen" EXT, {S "atsurfpen.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atsurfeval" EXT, {S "atsurfeval.cpp", O "atfftw.o", O "atlib.o", O "attable.o"}, "-lfftw3f"},
        {B "atsurfresamp" EXT, {S "atsurfresamp.cpp", O "atfftw.o", O "atlib.o", O "attable.o"}, "-lfftw3f"},
        {B "atfillgaps" EXT, {S "atfillgaps.cpp", O "atfftw.o", O "atlib.o", O "attable.o"}, "-lfftw3f"},
        {B "atmake_wellnetwork" EXT, {S "atmake_wellnetwork.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        //{B "atshp_surfgrad" EXT, {S "atshp_surfgrad.cpp", O "atlib.o"}},
        //{B "atshp_migpath" EXT, {S "atshp_migpath.cpp", O "atfftw.o", O "atlib.o"}, "-lfftw3f"},
        {B "atdownsampletrack" EXT, {S "atdownsampletrack.cpp", O "atlib.o", O "attable.o"}},
        {B "atgriderror" EXT, {S "atgriderror.cpp", O "atfftw.o", O "atgeom.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}, "-lfftw3f"},
        {B "atbhsearch" EXT, {S "atbhsearch.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atpen2interval" EXT, {S "atpen2interval.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "atreglogs" EXT, {S "atreglogs.cpp", O "atlib.o", O "attable.o"}},
        {B "atflattenlogs" EXT, {S "atflattenlogs.cpp", O "atlib.o", O "attable.o"}},
        {B "atdistmatrix" EXT, {S "atdistmatrix.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atfirst_sidetrack" EXT, {S "atfirst_sidetrack.cpp", O "atlib.o", O "attable.o"}},
        {B "atnearestwell" EXT, {S "atnearestwell.cpp", O "atlib.o", O "attable.o"}},
        {B "atnearestmarker" EXT, {S "atnearestmarker.cpp", O "atlib.o", O "attable.o"}},
        {B "atwelllogcorr" EXT, {S "atwelllogcorr.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "atwelllogcorr_warping" EXT, {D "atwelllogcorr_warping.cpp", O "atgeom.o", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "attopscorrection" EXT, {S "attopscorrection.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atvolgrid" EXT, {S "atvolgrid.cpp", O "atlib.o"}},
        {B "atvolslice" EXT, {S "atvolslice.cpp", O "atlib.o"}},
        {B "atvolpercentile" EXT, {S "atvolpercentile.cpp", O "atlib.o", O "atutil.o"}},
        {B "atvolhdrs" EXT, {S "atvolhdrs.cpp", O "atlib.o", O "attable.o"}},
        {B "atnullify_zero_traces" EXT, {S "atnullify_zero_traces.cpp", O "atlib.o", O "atutil.o"}},
        {B "atdiffusion_solve" EXT, {S "atdiffusion_solve.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atcogrid" EXT, {S "atcogrid.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "IterativeSmootherLOpV2D.o"}},
        {B "atgridtie" EXT, {S "atgridtie.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atgridtrim" EXT, {S "atgridtrim.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "IterativeSmootherLOpV2D.o"}},
        {B "atgridscore" EXT, {S "atgridscore.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "IterativeSmootherLOpV2D.o"}},
        {B "atantigrad" EXT, {S "atantigrad.cpp", O "atinv.o", O "atlib.o", O "attable.o", O "atvecop.o"}},
        {B "atranktops" EXT, {S "atranktops.cpp", O "atlib.o", O "attable.o"}},
        {B "atnudge" EXT, {S "atnudge.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
    };
    addexec(bg, seisinterp_rules);

    std::vector<CompileRule> ai_rules= {
        {B "atdbscan" EXT, {S "atdbscan.cpp", O "atlib.o", O "attable.o", O "attutil.o"}},
        {B "atnntrain" EXT, {S "atnntrain.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "atml.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atnnrun" EXT, {S "atnnrun.cpp", O "atgeom.o", O "atinv.o", O "atlib.o", O "atml.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atcovid" EXT, {S "atcovid.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o"}},
    };
    addexec(bg, ai_rules);

    addobj(bg, O "atdseval1.o", {S "atdseval1.cpp", I "atdseval1.hpp"});

    std::vector<CompileRule> decision_rules= {
        {B "atdecisiontree" EXT, {S "atdecisiontree.cpp", O "atlib.o", O "atdecision.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "jsoncpp.o"}},
        {B "atdecisioneval" EXT, {S "atdecisioneval.cpp", O "atlib.o", O "atdecision.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o"}},
        {B "atdseval" EXT, {S "atdseval.cpp", O "atdseval1.o", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atmakepptmap" EXT, {S "atmakepptmap.cpp", O "atlib.o",  O "atppt.o", O "atutil.o", O "atspot.o", O "atdecision.o", O "pugixml.o", O "miniz.o",O "atsqlite.o", O "attable.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atdsplot" EXT, {S "atdsplot.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atportfolio_ppt" EXT, {S "atportfolio_ppt.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atlandscape_ppt" EXT, {S "atlandscape_ppt.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atgendist" EXT, {S "atgendist.cpp", O "atlib.o", O "atdecision.o", O "atjson.o", O "jsoncpp.o", O "atutil.o"}},
        {B "atgendistarray" EXT, {S "atgendistarray.cpp", O "atlib.o", O "attable.o", O "atdecision.o", O "atjson.o", O "jsoncpp.o", O "atutil.o"}},
        {B "atdistribution_summary" EXT, {S "atdistribution_summary.cpp", O "atlib.o", O "atutil.o", O "atdecision.o", O "atjson.o", O "jsoncpp.o"}},
        {B "atdistribution_summary2d" EXT, {S "atdistribution_summary2d.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atdecision.o", O "atjson.o", O "jsoncpp.o"}},
        {B "atjson_test" EXT, {S "atjson_test.cpp", O "atlib.o", O "jsoncpp.o"}},
        //{B "atjvc_test" EXT, {S "atjvc_test.cpp", O "atlib.o"}},
        {B "atdistfactorization" EXT, {S "atdistfactorization.cpp", O "atfftw.o", O "atlib.o", O "attable.o", O "atutil.o"}, "-lfftw3f"},
        {B "atmcmath" EXT, {S "atmcmath.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atrigcut" EXT, {S "atrigcut.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atpercentilerank" EXT, {S "atpercentilerank.cpp", O "atlib.o", O "attable.o", O "atvecop.o", O "atutil.o", O "atdecision.o", O "atjson.o", O "jsoncpp.o"}},
        {B "atseishist" EXT, {S "atseishist.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "attutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atpptmap" EXT, {S "atpptmap.cpp", O "atlib.o", O "atdecision.o", O "atspot.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atppt.o", O "pugixml.o", O "miniz.o", O "jsoncpp.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atollama" EXT, {S "atollama.cpp", O "atlib.o"}},
        {B "atllama" EXT, {S "atllama.cpp", O "atlib.o"}, "-I/work0/usr/include -L/work0/usr/lib64 -lllama"},
    };
    addexec(bg, decision_rules);
    //pap tools
    std::vector<CompileRule> pap_rules= {
        {B "atbplan_gen" EXT, {S "atbplan_gen.cpp", O "atlib.o", O "atutil.o"}}, //check this one !
        {B "atsde2it" EXT, {S "atsde2it.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atriglevel" EXT, {S "atriglevel.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atds2tex" EXT, {S "atds2tex.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atdrillseq2gv" EXT, {S "atdrillseq2gv.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atcontingency_graph" EXT, {S "atcontingency_graph.cpp", O "atlib.o", O "attable.o"}},
        {B "atrelationgraph" EXT, {S "atrelationgraph.cpp", O "atlib.o", O "attable.o"}},
        {B "atportfolio2program" EXT, {S "atportfolio2program.cpp", O "atlib.o", O "attable.o"}},
        {B "atprogram_eval" EXT, {S "atprogram_eval.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atspotaggregate" EXT, {S "atspotaggregate.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atseqaggregate" EXT, {S "atseqaggregate.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "attutil.o", O "atvecop.o"}},
        {B "atspot_segment_stack" EXT, {S "atspot_segment_stack.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atspot_prospect_stack" EXT, {S "atspot_prospect_stack.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o"}},
        {B "atspot_check_tent_location" EXT, {S "atspot_check_tent_location.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "atutil.o", O "atvecop.o"}},
        {B "atpap_prospect_stack" EXT, {S "atpap_prospect_stack.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "ataveragepdf" EXT, {S "ataveragepdf.cpp", O "atlib.o", O "atutil.o"}},
        {B "atgeox2array" EXT, {S "atgeox2array.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atdecision.o", O "atexpr.o", O "atjson.o", O "jsoncpp.o"}},
        {B "atsplitdistribution" EXT, {S "atsplitdistribution.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o"}},
        {B "atsplitdistributions" EXT, {S "atsplitdistributions.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o"}},
        {B "atdistributionfractions" EXT, {S "atdistributionfractions.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o"}},
        {B "atbestlognormalfit" EXT, {S "atbestlognormalfit.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "atunrisked2risked_dist" EXT, {S "atunrisked2risked_dist.cpp", O "atlib.o", O "atvecop.o"}},
        {B "atrisked2unrisked_dist" EXT, {S "atrisked2unrisked_dist.cpp", O "atlib.o", O "atvecop.o"}},
        //{B "atjointdist" EXT, {S "atjointdist.cpp", O "atlib.o", O "attable.o"}},
        {B "atposteriorwellvolumes" EXT, {S "atposteriorwellvolumes.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "atnwell" EXT, {S "atnwell.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "atautogenerate_portfolio" EXT, {S "atautogenerate_portfolio.cpp", O "atlib.o", O "attable.o"}},
        {B "atloadshapefiles" EXT, {S "atloadshapefiles.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atconcepts2features" EXT, {S "atconcepts2features.cpp", O "atlib.o", O "attable.o", O "atutil.o"}},
        {B "atrradportfolio2features" EXT, {S "atrradportfolio2features.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "shpopen.o", O "dbfopen.o", O "safeileio.o"}},
        {B "atfsdgen" EXT, {S "atfsdgen.cpp", O "atlib.o", O "attable.o"}},
        {B "atantigravity" EXT, {S "atantigravity.cpp", O "atlib.o", O "attable.o", O "atvecop.o", O "atutil.o"}},
        {B "atexpleff" EXT, {S "atexpleff.cpp", O "atlib.o", O "atutil.o", O "attable.o", O "atvecop.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "atdstimeforecast" EXT, {S "atdstimeforecast.cpp", O "atlib.o", O "atdecision.o", O "atexpr.o", O "atjson.o", O "attable.o", O "atutil.o", O "atvecop.o", O "jsoncpp.o"}},
        {B "atjointdistribution" EXT, {S "atjointdistribution.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o", O "atdecision.o", O "jsoncpp.o"}},
        {B "atwellcostrate" EXT, {S "atwellcostrate.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atvecop.o", O "atinv.o", O "atdecision.o", O "jsoncpp.o"}},
    };
    addexec(bg, pap_rules);
    //family tree
    std::vector<CompileRule> family_rules= {
        //{B "atgedcom2sql" EXT, {S "atgedcom2sql.cpp", O "atlib.o", O "attable.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atfamilytree2gv" EXT, {S "atfamilytree2gv.cpp", O "atlib.o", O "attable.o", O "atutil.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atalitree" EXT, {S "atalitree.cpp", O "atlib.o", O "attable.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atcommontree" EXT, {S "atcommontree.cpp", O "atlib.o", O "attable.o", O "atsqlite.o", O "sqlite3.o"}},
        {B "atfamily2json" EXT, {S "atfamily2json.cpp", O "atlib.o", O "attable.o", O "atsqlite.o", O "atutil.o", O "sqlite3.o", O "atjson.o", O "jsoncpp.o"}},
        {B "atjson2family" EXT, {S "atjson2family.cpp", O "atlib.o", O "attable.o", O "atsqlite.o", O "atutil.o", O "sqlite3.o", O "atjson.o", O "jsoncpp.o"}},
        
    };
    addexec(bg, family_rules);

    //social
    std::vector<CompileRule> design_rules= {
        {B "atvecdesigner" EXT, {S "atvecdesigner.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "miniz.o", O "pugixml.o", O "atppt.o"}},
        {B "atmakemp4" EXT, {S "atmakemp4.cpp", O "atlib.o", O "attable.o", O "attutil.o"},"-I/usr/include/opencv4/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio"},
        {B "atsocial" EXT, {S "atsocial.cpp", O "atlib.o", O "attable.o", O "attutil.o"},"-I/usr/include/opencv4/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio -lcurl"},
        {B "attweet" EXT, {S "attweet.cpp", O "atlib.o", O "attable.o", O "attutil.o"},"-I/usr/include/opencv4/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio -lcurl -lcpr -lssl -lcrypto "},
        {B "atyoutubepost" EXT, {S "atyoutubepost.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "jsoncpp.o"},"-I/usr/include/opencv4/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio -lcurl -lcpr -lssl -lcrypto "},
        {B "attiktokpost" EXT, {S "attiktokpost.cpp", O "atlib.o", O "attable.o", O "attutil.o", O "jsoncpp.o"},"-I/usr/include/opencv4/ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio -lcurl -lcpr -lssl -lcrypto "},
    };
    addexec(bg, design_rules, true, {"social"});

    //testing and development codes
    std::vector<CompileRule> testing_rules= {
        //{B "atparallelblocksolver_test" EXT, {D "atparallelblocksolver_test.cpp", O "atinv.o", O "atlib.o", O "atvecop.o"}},
        //{B "atsqlitetest" EXT, {D "atsqlitetest.cpp", O "atlib.o", O "atsqlite.o"}},
    };
    addexec(bg, testing_rules);

    //std::vector<CompileRule> cairo_rules={
    //    {B "at2png" EXT, {S "at2png.cpp", O "atlib.o"}},
    //    {B "atpngmix" EXT, {S "atpngmix.cpp", O "atlib.o"}},
    //    {B "atpngvtile" EXT, {S "atpngvtile.cpp", O "atlib.o"}},
    //    {B "atpnghtile" EXT, {S "atpnghtile.cpp", O "atlib.o"}},
    //};
    //addexec(bg, cairo_rules);

    bg.silent=false;
    bg.echo=false;
    bg.dump_makefile();
}
