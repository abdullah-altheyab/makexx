#pragma once
#include <string>
#include <vector>
using std::string;
using std::vector;

// Toggle between trial and production runs
#define TRIAL true

// Simulation parameters
int iterations = TRIAL ? 10 : 5000;
string solver  = "implicit";

struct Run {
    string name;
    string grid;
    string params;
};

vector<Run> runs = {
    {"baseline",    "grid_coarse.dat",  "--viscosity=1.0"},
    {"high_visc",   "grid_coarse.dat",  "--viscosity=5.0"},
    {"fine_grid",   "grid_fine.dat",    "--viscosity=1.0"},
};
