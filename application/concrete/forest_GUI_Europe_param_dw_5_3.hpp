#ifndef G4M_EUROPE_DG_FOREST_GUI_EUROPE_PARAM_DW_5_3_DG_HPP
#define G4M_EUROPE_DG_FOREST_GUI_EUROPE_PARAM_DW_5_3_DG_HPP

#include <iostream>
#include <array>
#include <tuple>
#include <string>
#include <set>
#include <unordered_set>
#include <random>

#include "../../log.hpp"
#include "../../diagnostics/debugging.hpp"
#include "../../diagnostics/timer.hpp"
#include "../abstract/application.hpp"

#include "../../start_data/start_data.hpp"

#include "../../init/settings.hpp"
#include "../../init/data_struct.hpp"
#include "../../init/coef_struct.hpp"
#include "../../init/data_grid.hpp"
#include "../../structs/lw_price.hpp"
#include "../../structs/harvest_residues.hpp"
#include "../../structs/dat.hpp"

#include "../../increment/age_struct.hpp"

using namespace std;

using namespace g4m::structs;
using namespace g4m::misc::concrete;
using namespace g4m::increment;
using namespace g4m::diagnostics;

using namespace g4m::application::abstract;

using namespace g4m::StartData;

namespace g4m::application::concrete {

    class Forest_GUI_Europe_param_dw_5_3 : public Application {
    public:
        Forest_GUI_Europe_param_dw_5_3(const int argc, const char *const argv[]) : Application{argc, argv} {}

        explicit Forest_GUI_Europe_param_dw_5_3(const span<const string_view> &args_) : Application{args_} {}

        void Run() final {
            Timer timer{format("{}_{}", full_scenario, inputPriceC)};
            INFO("Application {}_{} is running", full_scenario, inputPriceC);
            INFO("Scenario to read in: {}", full_scenario);
            INFO("Scenario to read GL: {}", full_scenario_gl);

            correctBelgium();

            CountryData countriesForestArea;
        }

    protected:
        array<string, 3> c_scenario = {args[1], args[2], args[3]};
        int inputPriceC = stoi(args[4]);
        string full_scenario = c_scenario[0] + '_' + c_scenario[1] + '_' + c_scenario[2];
        string full_scenario_gl = full_scenario;
        string local_suffix = string{suffix} + full_scenario + (inputPriceC == 0 ? "_Pco2_0" : "");
        string suffix0 = string{suffix} + c_scenario[1] + "_" + c_scenario[2];

        // !!! don't change order of initialization, because mergeDatamap will throw access violation reading location
        datamapType appLandPrice = mergeDatamap(histLandPrice, landPriceScenarios, "Land Price");
        datamapType appWoodPrice = mergeDatamap(histWoodPrice, woodPriceScenarios, "Wood Price");
        datamapType appWoodDemand = mergeDatamap(histWoodDemand, woodDemandScenarios, "Wood Demand");
        datamapType appResiduesDemand = mergeDatamap(histResiduesDemand, residuesDemandScenarios, "Residues Demand");

        // wood from outside forests in Belgium to cover the inconsistency between FAOSTAT removals and Forest Europe increment and felling
        Ipol<double> woodSupplement = appWoodDemand[20];

        const size_t resLatitude = lround(180 / gridStep);

        // Initializing forest cover array by gridcells
        DataGrid<double> harvestGrid{resLatitude};
        DataGrid<double> maiForest{resLatitude};
        DataGrid<double> rotationForest{resLatitude};
        DataGrid<double> rotationForestNew{resLatitude};
        DataGrid<double> thinningForest{resLatitude};
        DataGrid<double> thinningForestNew{resLatitude};
        DataGrid<double> thinningForest10{resLatitude};
        DataGrid<double> thinningForest30{resLatitude};

        DataGrid<float> OforestShGrid{resLatitude};

        DataGrid<char> decisionGrid{resLatitude};
        DataGrid<char> managedForest{resLatitude};
        DataGrid<char> manageChForest{resLatitude};
        DataGrid<char> rotationType{resLatitude};
        DataGrid<char> unmanaged{resLatitude};

        DataGrid<double> MaxNPVGrid{resLatitude};
        DataGrid<double> salvageLogging{resLatitude}; // salvage logging wood

        datamapType mergeDatamap(datamapType histDatamap, const heterDatamapScenariosType &scenariosDatamaps,
                                 const string_view message) {
            if (!scenariosDatamaps.contains(full_scenario_gl)) {
                ERROR("{} is not filled in, check scenarios!!!, full_scenario_gl = {}", message, full_scenario_gl);
                throw runtime_error{"no scenario in datamapScenarios"};
            }

            // Swiss project 21.04.2022, Nicklas Forsell
            datamapType datamapDest = scenariosDatamaps.at(full_scenario_gl);
            datamapDest.merge(histDatamap);

            TRACE("Merged {}:", message);
            for (const auto &[id, ipol]: datamapDest)
                TRACE("{}\n{}", idCountryGLOBIOM[id], ipol.str());

            TRACE("Obsolete {}:", message);
            for (const auto &[id, ipol]: histDatamap)
                TRACE("{}\n{}", idCountryGLOBIOM[id], ipol.str());

            return datamapDest;
        }

        void correctBelgium() noexcept {
            // 05.04.2023: we assume that 14% of round-wood comes from outside forest
            // the Forest Europe net increment and felling values are less than FAOSTAT round-wood
            const double forestWood = 0.86;
            appWoodDemand[20] *= forestWood;  // Belgium
            woodSupplement *= 1 - forestWood;
        }
    };
}

#endif
