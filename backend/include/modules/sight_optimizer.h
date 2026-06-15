#pragma once

#include "sight_optics.h"
#include "common.h"
#include <string>
#include <vector>

namespace crossbow {
namespace modules {

constexpr SightType DIOPTER_SIGHT = static_cast<SightType>(6);
constexpr SightType TELESCOPIC = static_cast<SightType>(7);

struct SightOptimizationTarget {
    double range_m;
    double wind_speed_ms;
    double temperature_c;
    double humidity;
    double target_radius_m;
};

struct SightOptimizationResult {
    SightType best_sight_type;
    double cep_mrad;
    double improvement_pct;
    double scale_calibration;
    OpticalErrorBudget error_budget;
};

struct SightOptimizerConfig {
    double visual_acuity_arcmin;
    double aperture_diameter_mm;
    double eye_relief_mm;
    int sample_count;
};

class SightOptimizer {
public:
    SightOptimizer();

    void set_config(const SightOptimizerConfig& config);
    void set_target(const SightOptimizationTarget& target);

    SightOptimizationResult optimize(const std::vector<SightType>& type_list);
    SightOptimizationResult evaluate(SightType type);
    OpticalErrorBudget generate_error_budget(SightType type);
    std::string recommend_sight(const SightOptimizationTarget& target);
    std::vector<std::pair<SightType, SightOptimizationResult>> compare_all();

private:
    SightOptimizerConfig config_;
    SightOptimizationTarget target_;

    SightDesign synthesize_design(SightType type) const;
    double compute_scale_calibration(SightType type, const SightDesign& design) const;
    double compute_improvement_pct(double cep, const std::vector<double>& all_ceps) const;
};

} // namespace modules
} // namespace crossbow
