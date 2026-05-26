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
    mf.help_title = "Simulation Workflow";
    mf.description("Runs numerical simulations with configurable parameters.");

    mf.set_current_menu("Simulate");
    for (auto& r : runs) {
        string output = r.name + "_result.bin";
        mf.add(output, r.grid)
            << FINAL
            << HELP("Run " + r.name + " simulation")
            << ("runsim --solver=" + solver
                + " --iter=" + to_string(iterations)
                + " " + r.params
                + " $< > $@");
    }

    mf.set_current_menu("Reports");
    vector<string> all_results;
    for (auto& r : runs)
        all_results.push_back(r.name + "_result.bin");

    mf.add("summary.pdf", all_results)
        << FINAL
        << HELP("Generate comparison report")
        << "plot_results $^ > $@";

    mf.generate();
}
