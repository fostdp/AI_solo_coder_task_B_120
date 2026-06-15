#include "modules/sight_optimizer.h"
#include "sight_optics.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace crossbow {
namespace modules {

static constexpr double DEG_TO_RAD = M_PI / 180.0;
static constexpr double DEG_TO_MRAD = M_PI * 1000.0 / 180.0;

namespace {

CrossbowType default_crossbow() {
    CrossbowType cb;
    cb.id = 7;
    cb.name = "宋神臂弩";
    cb.dynasty = "宋朝";
    cb.draw_weight = 350;
    cb.bow_length = 1.75;
    cb.string_length = 1.8;
    cb.arrow_mass = 0.080;
    cb.effective_range = 300;
    cb.max_range = 550;
    return cb;
}

} // anonymous namespace

SightOptimizer::SightOptimizer() {
    config_.visual_acuity_arcmin = 1.0;
    config_.aperture_diameter_mm = 1.8;
    config_.eye_relief_mm = 50.0;
    config_.sample_count = 500;

    target_.range_m = 200.0;
    target_.wind_speed_ms = 0.0;
    target_.temperature_c = 15.0;
    target_.humidity = 0.5;
    target_.target_radius_m = 0.25;
}

void SightOptimizer::set_config(const SightOptimizerConfig& config) {
    config_ = config;
    if (config_.sample_count < 1) config_.sample_count = 1;
    if (config_.visual_acuity_arcmin < 0.1) config_.visual_acuity_arcmin = 0.1;
    if (config_.aperture_diameter_mm < 0.1) config_.aperture_diameter_mm = 0.1;
    if (config_.eye_relief_mm < 1.0) config_.eye_relief_mm = 1.0;
}

void SightOptimizer::set_target(const SightOptimizationTarget& target) {
    target_ = target;
    if (target_.range_m < 1.0) target_.range_m = 1.0;
    if (target_.target_radius_m < 0.01) target_.target_radius_m = 0.01;
    if (target_.humidity < 0.0) target_.humidity = 0.0;
    if (target_.humidity > 1.0) target_.humidity = 1.0;
}

SightDesign SightOptimizer::synthesize_design(SightType type) const {
    auto& db = SightDesignDatabase::instance();
    SightDesign design = db.get(type);

    if (design.type == SightType::CUSTOM) {
        if (type == DIOPTER_SIGHT) {
            design.type = DIOPTER_SIGHT;
            design.name = "diopter_sight";
            design.description = "Diopter sight with adjustable aperture, modern precision design";
            design.historical_origin = "Modern competitive shooting diopter system";
            design.sight_height_mm = 65.0;
            design.sight_radius_mm = 980.0;
            design.notch_width_mm = 0.0;
            design.post_diameter_mm = 0.0;
            design.aperture_diameter_mm = config_.aperture_diameter_mm;
            design.scale_count = 16;
            design.scale_range_min_m = 50.0;
            design.scale_range_max_m = 700.0;
            design.scale_interval_m = 43.75;
            design.material_density = 7850.0;
            design.thermal_expansion_coeff = 12e-6;
            design.manufacturing_precision_mm = 0.05;
            design.total_weight_g = 180.0;
            design.estimated_cost = 200.0;
            design.user_experience_rating = 0.95;
            design.adjustability_rating = 0.95;
            design.durability_rating = 0.50;
        } else if (type == TELESCOPIC) {
            design.type = TELESCOPIC;
            design.name = "telescopic_sight";
            design.description = "Telescopic sight with magnification, long-range precision";
            design.historical_origin = "Modern telescopic riflescope technology";
            design.sight_height_mm = 75.0;
            design.sight_radius_mm = 1100.0;
            design.notch_width_mm = 0.0;
            design.post_diameter_mm = 0.0;
            design.aperture_diameter_mm = 32.0;
            design.scale_count = 20;
            design.scale_range_min_m = 50.0;
            design.scale_range_max_m = 1000.0;
            design.scale_interval_m = 50.0;
            design.material_density = 2700.0;
            design.thermal_expansion_coeff = 23e-6;
            design.manufacturing_precision_mm = 0.02;
            design.total_weight_g = 450.0;
            design.estimated_cost = 500.0;
            design.user_experience_rating = 0.98;
            design.adjustability_rating = 1.0;
            design.durability_rating = 0.40;
        }
    }

    return design;
}

double SightOptimizer::compute_scale_calibration(SightType type, const SightDesign& design) const {
    (void)type;
    SightScaleCalibrator calibrator;
    calibrator.set_crossbow(default_crossbow());
    calibrator.set_sight_design(design);

    double range = std::max(1.0, target_.range_m);
    double start = std::max(10.0, range * 0.25);
    double end = std::max(start + 10.0, range * 1.5);
    double step = std::max(5.0, (end - start) / 10.0);

    auto scales = calibrator.calibrate_scales(start, end, step);
    if (scales.empty()) return 0.0;

    double total_err = 0.0;
    for (const auto& s : scales) {
        total_err += std::abs(s.calibration_error_mrad);
    }
    return total_err / static_cast<double>(scales.size());
}

double SightOptimizer::compute_improvement_pct(double cep, const std::vector<double>& all_ceps) const {
    if (all_ceps.empty()) return 0.0;

    double worst = *std::max_element(all_ceps.begin(), all_ceps.end());
    if (worst <= 0.0) return 0.0;
    if (cep >= worst) return 0.0;

    double improvement = (worst - cep) / worst * 100.0;
    return std::max(0.0, std::min(100.0, improvement));
}

OpticalErrorBudget SightOptimizer::generate_error_budget(SightType type) {
    SightDesign design = synthesize_design(type);

    OpticalErrorSimulator simulator;
    simulator.set_ambient_conditions(
        500.0,
        target_.temperature_c,
        target_.humidity,
        target_.wind_speed_ms
    );
    simulator.set_user_profile(config_.visual_acuity_arcmin, 0.7);
    simulator.set_distance(target_.range_m);

    (void)design;
    return simulator.get_current_error_budget();
}

SightOptimizationResult SightOptimizer::evaluate(SightType type) {
    SightOptimizationResult result;
    result.best_sight_type = type;

    SightDesign design = synthesize_design(type);

    OpticalErrorSimulator simulator;
    simulator.set_ambient_conditions(
        500.0,
        target_.temperature_c,
        target_.humidity,
        target_.wind_speed_ms
    );
    simulator.set_user_profile(config_.visual_acuity_arcmin, 0.7);
    simulator.set_distance(target_.range_m);

    result.cep_mrad = simulator.estimate_sight_cep(design, target_.range_m, config_.sample_count);
    result.error_budget = simulator.get_current_error_budget();
    result.scale_calibration = compute_scale_calibration(type, design);
    result.improvement_pct = 0.0;

    return result;
}

SightOptimizationResult SightOptimizer::optimize(const std::vector<SightType>& type_list) {
    if (type_list.empty()) {
        return compare_all().front().second;
    }

    std::vector<std::pair<SightType, SightOptimizationResult>> results;
    std::vector<double> ceps;

    for (SightType type : type_list) {
        auto r = evaluate(type);
        results.emplace_back(type, r);
        ceps.push_back(r.cep_mrad);
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.second.cep_mrad < b.second.cep_mrad;
    });

    SightOptimizationResult best = results.front().second;
    best.improvement_pct = compute_improvement_pct(best.cep_mrad, ceps);
    return best;
}

std::vector<std::pair<SightType, SightOptimizationResult>> SightOptimizer::compare_all() {
    std::vector<SightType> all_types = {
        SightType::BASIC_NOTCH,
        SightType::MULTI_PIN,
        SightType::PIN_SIGHT,
        DIOPTER_SIGHT,
        TELESCOPIC
    };

    std::vector<std::pair<SightType, SightOptimizationResult>> results;
    std::vector<double> ceps;

    for (SightType type : all_types) {
        auto r = evaluate(type);
        results.emplace_back(type, r);
        ceps.push_back(r.cep_mrad);
    }

    for (auto& pair : results) {
        pair.second.improvement_pct = compute_improvement_pct(pair.second.cep_mrad, ceps);
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.second.cep_mrad < b.second.cep_mrad;
    });

    return results;
}

std::string SightOptimizer::recommend_sight(const SightOptimizationTarget& target) {
    SightOptimizationTarget prev = target_;
    target_ = target;

    auto results = compare_all();

    SightDesign best_design = synthesize_design(results.front().first);
    SightDesign basic_design = synthesize_design(SightType::BASIC_NOTCH);

    double best_cep = results.front().second.cep_mrad;
    double basic_cep = 0.0;
    for (const auto& r : results) {
        if (r.first == SightType::BASIC_NOTCH) {
            basic_cep = r.second.cep_mrad;
            break;
        }
    }

    double target_angular_radius_mrad = (target.target_radius_m / std::max(1.0, target.range_m)) * 1000.0;
    bool meets_requirement = best_cep <= target_angular_radius_mrad;

    std::ostringstream oss;

    oss << "Sight Recommendation for target at " << target.range_m << "m:\n";
    oss << "  Target radius: " << target.target_radius_m << "m ("
        << target_angular_radius_mrad << " mrad)\n";
    oss << "  Wind: " << target.wind_speed_ms << " m/s, Temp: "
        << target.temperature_c << " C, Humidity: " << (target.humidity * 100.0) << "%\n";
    oss << "\n";
    oss << "Recommended sight: " << best_design.name << "\n";
    oss << "  CEP: " << best_cep << " mrad\n";
    oss << "  Improvement over basic notch: "
        << (basic_cep > 0 ? ((basic_cep - best_cep) / basic_cep * 100.0) : 0.0) << "%\n";
    oss << "  " << (meets_requirement ? "MEETS" : "DOES NOT MEET")
        << " target accuracy requirement\n";

    if (!meets_requirement) {
        oss << "  Consider: reduce range, increase target size, or use telescopic sight\n";
    }

    oss << "\nRanking (lowest CEP first):\n";
    int rank = 1;
    for (const auto& r : results) {
        SightDesign d = synthesize_design(r.first);
        oss << "  " << rank++ << ". " << d.name << ": "
            << r.second.cep_mrad << " mrad (" << r.second.improvement_pct << "% improvement)\n";
    }

    target_ = prev;
    return oss.str();
}

} // namespace modules
} // namespace crossbow
