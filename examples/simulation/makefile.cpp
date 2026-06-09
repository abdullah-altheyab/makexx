// Simulation workflow — separating config from rules.
//
// config.hpp holds parameters (runs, solver, iterations, trial flag).
// Edit config.hpp and run make — the makefile auto-regenerates because
// makexx tracks the dependency via -MMD.

#include "makefile.hpp"
#include "config.hpp"
using std::to_string;

int main() {
    Makefile mf;
    mf.title = "Simulation Workflow";
    mf.description = "Runs numerical simulations with configurable parameters.";

    // External tools, each with an install hint shown by `make check_tools`.
    mf << TOOLDESC("runsim", "spack install runsim");
    mf << TOOLDESC("plot_results", "pip install plot-results");

    for (auto& r : runs) {
        string output = r.name + "_result.bin";
        mf.add(output, r.grid)
            << MENU("Simulate")
            << FINAL
            << HELP("Run " + r.name + " simulation")
            << TOOL("runsim")   // mtime-tracked prereq: results rebuild if the solver changes
            << ("runsim --solver=" + solver
                + " --iter=" + to_string(iterations)
                + " " + r.params
                + " $< > $@");
    }

    vector<string> all_results;
    for (auto& r : runs)
        all_results.push_back(r.name + "_result.bin");

    mf.add("summary.pdf", all_results)
        << MENU("Reports")
        << FINAL
        << HELP("Generate comparison report")
        << "plot_results $^ > $@";

    mf.generate();
}
