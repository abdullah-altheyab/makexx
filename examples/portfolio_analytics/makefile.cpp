// Portfolio analytics workflow for mineral deposit evaluation.
//
// Demonstrates: config separation, domain classes, parameterized rules,
// feature flags, multi-line shell commands, and menu organization.
//
// Edit config.hpp to change deposits, regions, or toggle TRIAL mode.
// Then run: make (auto-regenerates if config.hpp changed)

#include "makefile.hpp"
#include "config.hpp"
using namespace std;
#define _cont " \\\n"

string sql(string table, string query) {
    return "sqlite3 " + portfolio_file + " \"" + query + "\" > " + table;
}

string iitable(string name) {
    return "data/" + name;
}

int main() {
    Makefile mf;
    mf.title = "Mining Portfolio Analytics";
    mf.description = "Evaluates mineral deposit portfolios using Monte Carlo "
        "simulation. Manages data import, QC, forecasting, and reporting "
        "across multiple deposits, zones, and regions.";

    // ─── Data Import ─────────────────────────────────────────────────

    mf << MENU("Data");

    mf.add("portfolio.db", portfolio_file)
        << HELP("import portfolio database")
        << "cp $< $@";

    vector<string> db_tables;
    for (auto& z : zones) {
        string tbl = z.prefix + "_deposits.t";
        db_tables.push_back(tbl);
        mf.add(tbl, "portfolio.db")
            << HELP("extract " + z.title + " deposits")
            << RETAIN(iitable(z.prefix + "_boundary.it"))
            << sql(tbl, "SELECT * FROM deposits WHERE zone='" + z.db_name + "'");
    }

    mf.add("all_deposits.t", db_tables)
        << HELP("merge all deposit tables")
        << ("cat $^ | sort -u > $@");

    // ─── QC ──────────────────────────────────────────────────────────

    mf << MENU("QC");

    for (auto& d : deposits) {
        string qc = d.prefix + "_qc.pdf";
        mf.add(qc, {d.prefix + "_forecast_gold.t", d.prefix + "_forecast_ore.t"})
            << HELP("QC " + d.title + " inputs")
            << ("qc_tool --deposit=" + d.prefix + " $^ -o $@");
    }

    mf.add("portfolio_qc.pdf", "all_deposits.t")
        << HELP("cross-check portfolio consistency")
        << "portfolio_qc $< -o $@";

    // ─── Forecasting ─────────────────────────────────────────────────

    mf << MENU("Forecast");

    for (auto& d : deposits) {
        string gold_out = d.prefix + "_forecast_gold.t";
        string ore_out  = d.prefix + "_forecast_ore.t";
        string dep_tbl  = d.zone + "_deposits.t";

        mf.add(gold_out, dep_tbl)
            << HELP("forecast gold for " + d.title)
            << ("mcsim --commodity=gold"
                _cont "  --rf=" + d.gold_rf +
                _cont "  --iter=" + to_string(mc_iterations) +
                _cont "  --deposit=" + d.prefix +
                _cont "  $< -o $@");

        mf.add(ore_out, dep_tbl)
            << HELP("forecast ore for " + d.title)
            << ("mcsim --commodity=ore"
                _cont "  --rf=" + d.ore_rf +
                _cont "  --iter=" + to_string(mc_iterations) +
                _cont "  --deposit=" + d.prefix +
                _cont "  $< -o $@");

        mf.add(d.prefix + "_forecast.t", {gold_out, ore_out})
            << TEMP({gold_out, ore_out})
            << ("merge_forecasts $^ > $@");
    }

    // ─── Regional Aggregation ────────────────────────────────────────

    mf << MENU("Forecast/Regional");

    for (auto& r : regions) {
        vector<string> region_deps;
        for (auto& d : deposits)
            region_deps.push_back(d.prefix + "_forecast.t");

        mf.add(r.prefix + "_regional.t", region_deps)
            << HELP("aggregate " + r.title)
            << ("aggregate --region=" + r.prefix + " $^ -o $@");
    }

    // ─── Simulation Validation ───────────────────────────────────────

    mf << MENU("Validation");

    mf.add("mc_convergence.pdf", "all_deposits.t")
        << HELP("check Monte Carlo convergence ("
            + to_string(mc_iterations) + " iterations)")
        << ("convergence_check --iter=" + to_string(mc_iterations) + " $< -o $@");

#ifdef WITH_RARE
    mf.add("rare_earth_validation.pdf", "all_deposits.t")
        << HELP("validate rare-earth deposit models")
        << "rare_earth_qc $< -o $@";
#endif

    // ─── Reports ─────────────────────────────────────────────────────

    mf << MENU("Reports");

    vector<string> all_forecasts;
    for (auto& d : deposits)
        all_forecasts.push_back(d.prefix + "_forecast.t");

    mf.add("landscape_report.pdf", all_forecasts)
        << FINAL
        << HELP("build portfolio landscape report")
        << "report_tool --type=landscape $^ -o $@";

    mf.add("executive_summary.pdf", "landscape_report.pdf")
        << FINAL
        << HELP("generate executive summary")
        << "report_tool --type=execsum $< -o $@";

    for (auto& d : deposits) {
        mf.add(d.prefix + "_report.pdf", d.prefix + "_forecast.t")
            << HELP("report for " + d.title)
            << ("report_tool --type=deposit"
                _cont "  --title=\"" + d.title + "\""
                _cont "  $< -o $@");
    }

    // ─── Benchmarking ────────────────────────────────────────────────

    mf << MENU("Benchmark");

    mf.add("benchmark_gold.pdf", all_forecasts)
        << HELP("benchmark gold recovery across deposits")
        << "benchmark --commodity=gold $^ -o $@";

    mf.add("benchmark_ore.pdf", all_forecasts)
        << HELP("benchmark ore grade across deposits")
        << "benchmark --commodity=ore $^ -o $@";

    // ─── GIS ─────────────────────────────────────────────────────────

    mf.add("gis_layers.qgz", "all_deposits.t")
        << MENU("GIS")
        << HELP("prepare GIS layers for QGIS")
        << "gis_export --format=qgz $< -o $@";

    // ─── Utilities ───────────────────────────────────────────────────

    mf << MENU("Utilities");

    // Hold a reference to the rule and append commands across statements.
    // The rule executes its commands in the order they were appended,
    // so we can mix a fixed header with a loop over data.
    auto& list_dep = mf.add("list_deposits");
    list_dep << HELP("list all configured deposits")
             << "echo 'Deposits:'";
    for (auto& d : deposits)
        list_dep << ("echo '  " + d.prefix + " - " + d.title + "'");

    auto& list_zn = mf.add("list_zones");
    list_zn << HELP("list all mining zones")
            << "echo 'Mining Zones:'";
    for (auto& z : zones)
        list_zn << ("echo '  " + z.prefix + " - " + z.title + "'");

    mf.generate();
}
