#pragma once

#include "bowstring_material.h"
#include "common.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

namespace crossbow {
namespace modules {

struct StringComparisonMetric {
    BowstringMaterialType type;
    std::string name;
    double string_mass_g;
    double stiffness_Nm;
    double energy_efficiency;
    double stored_energy_J;
    double estimated_velocity_ms;
    double max_stress_MPa;
    double fatigue_life_cycles;
    double creep_factor;
    double overall_score;
};

struct StringOptimizationResult {
    BowstringMaterial best_material;
    double best_score;
    std::vector<StringComparisonMetric> rankings;
    std::string recommendation;
};

struct StringComparisonConfig {
    double string_length_m = 1.2;
    double string_diameter_mm = 3.0;
    double draw_length_m = 0.55;
    double draw_weight_N = 350.0;
    double arrow_mass_g = 50.0;
    double avg_humidity = 0.5;
    double cycles_per_day = 100;

    double weight_mass = 0.15;
    double weight_efficiency = 0.30;
    double weight_velocity = 0.25;
    double weight_lifespan = 0.20;
    double weight_creep = 0.10;
};

class StringComparator {
public:
    StringComparator();
    explicit StringComparator(const StringComparisonConfig& config);

    void set_config(const StringComparisonConfig& config);
    const StringComparisonConfig& get_config() const;

    StringComparisonMetric evaluate_material(BowstringMaterialType type) const;
    StringComparisonMetric evaluate_material(const BowstringMaterial& material) const;

    std::vector<StringComparisonMetric> compare_all_materials() const;
    std::vector<StringComparisonMetric> compare_materials(
        const std::vector<BowstringMaterialType>& types) const;

    StringOptimizationResult find_optimal_material(
        const std::vector<BowstringMaterialType>& candidates = {}) const;

    std::vector<std::pair<double, double>> generate_efficiency_curve(
        BowstringMaterialType type,
        int num_points = 50) const;

    std::map<std::string, double> get_parameter_sensitivity(
        BowstringMaterialType type,
        double variation_pct = 0.1) const;

    std::string generate_report(const std::vector<StringComparisonMetric>& results) const;

private:
    StringComparisonConfig config_;

    double normalize(double value, double min, double max, bool higher_is_better = true) const;
    double compute_overall_score(const StringComparisonMetric& metric,
                                 const std::vector<StringComparisonMetric>& all) const;
    double estimate_stored_energy(const BowstringMaterial& mat) const;
    double estimate_velocity(const BowstringMaterial& mat) const;
};

} // namespace modules
} // namespace crossbow
