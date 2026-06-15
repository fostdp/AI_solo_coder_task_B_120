#pragma once

#include "formation_simulator.h"
#include "common.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace crossbow {
namespace modules {

struct FireCoverageResult {
    double coverage_pct;
    double projectiles_per_m2;
    double hit_probability;
    double avg_distance_m;
    double concentration_score;
};

struct FormationModuleConfig {
    int num_crossbows = 24;
    double spacing_m = 3.0;
    double orientation_deg = 0.0;
    double range_m = 200.0;
    int salvo_count = 3;
    double salvo_interval_ms = 500.0;
};

class FormationSimulatorModule {
public:
    FormationSimulatorModule();
    explicit FormationSimulatorModule(const FormationModuleConfig& config);

    void set_config(const FormationModuleConfig& config);
    const FormationModuleConfig& get_config() const;

    void set_crossbow_types(const std::map<int, CrossbowType>& types);
    void set_target(const TargetSpecification& target);

    FireCoverageResult evaluate_coverage(FormationType type) const;

    std::vector<std::pair<FormationType, FireCoverageResult>> compare_formations(
        const std::vector<FormationType>& type_list) const;

    std::pair<FormationType, FireCoverageResult> find_optimal_formation(
        const TargetSpecification& target) const;

    FireCoverageResult get_fire_density(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target) const;

    std::vector<std::vector<double>> generate_heatmap(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target) const;

    RLOptimizationResult train_rl(
        const TargetSpecification& target,
        int episodes = 1000) const;

    bool validate_placement(
        const std::vector<CrossbowPlacement>& placements) const;

    std::vector<CrossbowPlacement> generate_placement(
        FormationType type,
        double center_x_m = 0.0,
        double center_y_m = 0.0) const;

private:
    FormationModuleConfig config_;
    std::unique_ptr<FormationSimulator> simulator_;
    std::unique_ptr<FormationRLOptimizer> rl_optimizer_;
    std::map<int, CrossbowType> crossbow_types_;
    TargetSpecification current_target_;

    double compute_concentration_score(
        const std::vector<std::pair<double, double>>& hit_pattern,
        const TargetSpecification& target) const;

    double compute_average_distance(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target) const;
};

} // namespace modules
} // namespace crossbow
