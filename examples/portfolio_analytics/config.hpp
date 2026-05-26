#pragma once
#include <string>
#include <vector>
using std::string;
using std::vector;

// Workflow options — toggle by commenting/uncommenting
#define TRIAL        // use 10 iterations instead of 5000
//#define WITH_RARE    // include rare-earth deposits (slower simulations)
#define SHARED       // work on shared portfolio file

#ifdef TRIAL
const int mc_iterations = 10;
#else
const int mc_iterations = 5000;
#endif

struct Region {
    string title;
    string prefix;
};

struct MiningZone {
    string title;
    string prefix;
    string db_name;
    string boundary_shp;
};

struct Deposit {
    string prefix;
    string title;
    string zone;
    string gold_rf;
    string ore_rf;
    vector<string> db_references;
};

vector<Region> regions = {
    {"Northern Range",  "north"},
    {"Central Basin",   "central"},
    {"Southern Shelf",  "south"},
};

vector<MiningZone> zones = {
    {"Kalgoorlie",  "kalg",  "kalg_zone",  "kalg_boundary.shp"},
    {"Pilbara",     "pilb",  "pilb_zone",  "pilb_boundary.shp"},
    {"Yilgarn",     "yilg",  "yilg_zone",  "yilg_boundary.shp"},
};

vector<Deposit> deposits = {
    {"bodd",  "Boddington",     "kalg",  "0.72", "0.85", {"BODD_01", "BODD_02"}},
    {"supe",  "Super Pit",      "kalg",  "0.68", "0.91", {"SUPE_01"}},
    {"teln",  "Telfer North",   "pilb",  "0.55", "0.78", {"TELN_01", "TELN_02", "TELN_03"}},
    {"roys",  "Roy Hill South", "pilb",  "0.82", "0.93", {"ROYS_01"}},
    {"mtke",  "Mt Keith",       "yilg",  "0.61", "0.70", {"MTKE_01", "MTKE_02"}},
};

string portfolio_file =
#ifdef SHARED
    "/shared/portfolio/master.db";
#else
    "local_portfolio.db";
#endif
