#include "modules/string_comparator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

namespace crossbow {
namespace modules {

StringComparator::StringComparator()
    : config_() {}

StringComparator::StringComparator(const StringComparisonConfig& config)
    : config_(config) {}

void StringComparator::set_config(const StringComparisonConfig& config) {
    config_ = config;
}

const StringComparisonConfig& StringComparator::get_config() const {
    return config_;
}

double StringComparator::normalize(
    double value,
    double min,
    double max,
    bool higher_is_better
) const {
    if (std::abs(max - min) < 1e-12) {
        return 0.5;
    }
    double clamped = std::clamp(value, std::min(min, max), std::max(min, max));
    double normalized = (clamped - min) / (max - min);
    return higher_is_better ? normalized : (1.0 - normalized);
}

double StringComparator::estimate_stored_energy(const BowstringMaterial& mat) const {
    double draw_energy_j = 0.5 * config_.draw_weight_N * config_.draw_length_m * (2.0 / 3.0);
    double efficiency = mat.interpolate_efficiency(1.0);
    return draw_energy_j * efficiency;
}

double StringComparator::estimate_velocity(const BowstringMaterial& mat) const {
    double string_mass_kg = mat.calculate_string_mass(config_.string_length_m);
    double arrow_mass_kg = config_.arrow_mass_g * 0.001;
    double effective_mass = arrow_mass_kg + string_mass_kg * 0.25;
    double usable_energy = estimate_stored_energy(mat);
    if (effective_mass <= 0.0) return 0.0;
    return std::sqrt(2.0 * usable_energy / effective_mass);
}

StringComparisonMetric StringComparator::evaluate_material(BowstringMaterialType type) const {
    const auto& mat = BowstringMaterialDatabase::instance().get(type);
    return evaluate_material(mat);
}

StringComparisonMetric StringComparator::evaluate_material(const BowstringMaterial& material) const {
    StringComparisonMetric metric;
    metric.type = material.type;
    metric.name = material.name;

    double radius_m = (config_.string_diameter_mm / 2.0) * 0.001;
    double cross_section_m2 = M_PI * radius_m * radius_m;

    metric.string_mass_g = material.calculate_string_mass(config_.string_length_m) * 1000.0;
    metric.stiffness_Nm = material.calculate_stiffness(config_.string_length_m, cross_section_m2);
    metric.energy_efficiency = material.interpolate_efficiency(1.0);
    metric.stored_energy_J = estimate_stored_energy(material);
    metric.estimated_velocity_ms = estimate_velocity(material);

    double tension_N = config_.draw_weight_N;
    metric.max_stress_MPa = (tension_N / std::max(cross_section_m2, 1e-12)) / 1e6;

    metric.fatigue_life_cycles = material.estimate_lifespan(
        static_cast<int>(config_.cycles_per_day),
        config_.avg_humidity
    );

    metric.creep_factor = material.calculate_creep_factor(24.0, 25.0);
    metric.overall_score = 0.0;

    return metric;
}

std::vector<StringComparisonMetric> StringComparator::compare_all_materials() const {
    std::vector<BowstringMaterialType> types = {
        BowstringMaterialType::SINEW,
        BowstringMaterialType::HORN,
        BowstringMaterialType::SILK,
        BowstringMaterialType::HEMP
    };
    return compare_materials(types);
}

std::vector<StringComparisonMetric> StringComparator::compare_materials(
    const std::vector<BowstringMaterialType>& types
) const {
    std::vector<StringComparisonMetric> results;
    results.reserve(types.size());

    for (const auto& type : types) {
        auto metric = evaluate_material(type);
        if (metric.type != BowstringMaterialType::UNKNOWN) {
            results.push_back(metric);
        }
    }

    for (auto& metric : results) {
        metric.overall_score = compute_overall_score(metric, results);
    }

    std::sort(results.begin(), results.end(),
              [](const StringComparisonMetric& a, const StringComparisonMetric& b) {
                  return a.overall_score > b.overall_score;
              });

    return results;
}

double StringComparator::compute_overall_score(
    const StringComparisonMetric& metric,
    const std::vector<StringComparisonMetric>& all
) const {
    if (all.empty()) return 0.0;

    auto get_minmax = [&](const std::function<double(const StringComparisonMetric&)>& extractor) {
        double min_val = extractor(all.front());
        double max_val = min_val;
        for (const auto& m : all) {
            double v = extractor(m);
            min_val = std::min(min_val, v);
            max_val = std::max(max_val, v);
        }
        return std::make_pair(min_val, max_val);
    };

    auto mass_range = get_minmax([](const StringComparisonMetric& m) { return m.string_mass_g; });
    auto eff_range = get_minmax([](const StringComparisonMetric& m) { return m.energy_efficiency; });
    auto vel_range = get_minmax([](const StringComparisonMetric& m) { return m.estimated_velocity_ms; });
    auto life_range = get_minmax([](const StringComparisonMetric& m) { return m.fatigue_life_cycles; });
    auto creep_range = get_minmax([](const StringComparisonMetric& m) { return m.creep_factor; });

    double norm_mass = normalize(metric.string_mass_g, mass_range.first, mass_range.second, false);
    double norm_eff = normalize(metric.energy_efficiency, eff_range.first, eff_range.second, true);
    double norm_vel = normalize(metric.estimated_velocity_ms, vel_range.first, vel_range.second, true);
    double norm_life = normalize(metric.fatigue_life_cycles, life_range.first, life_range.second, true);
    double norm_creep = normalize(metric.creep_factor, creep_range.first, creep_range.second, false);

    double total_weight = config_.weight_mass + config_.weight_efficiency +
                          config_.weight_velocity + config_.weight_lifespan +
                          config_.weight_creep;

    if (total_weight <= 0.0) total_weight = 1.0;

    double score = (norm_mass * config_.weight_mass +
                    norm_eff * config_.weight_efficiency +
                    norm_vel * config_.weight_velocity +
                    norm_life * config_.weight_lifespan +
                    norm_creep * config_.weight_creep) / total_weight;

    return score;
}

StringOptimizationResult StringComparator::find_optimal_material(
    const std::vector<BowstringMaterialType>& candidates
) const {
    StringOptimizationResult result;

    std::vector<BowstringMaterialType> types = candidates;
    if (types.empty()) {
        types = {
            BowstringMaterialType::SINEW,
            BowstringMaterialType::HORN,
            BowstringMaterialType::SILK,
            BowstringMaterialType::HEMP
        };
    }

    result.rankings = compare_materials(types);

    if (result.rankings.empty()) {
        result.best_material = BowstringMaterialDatabase::instance().get(BowstringMaterialType::UNKNOWN);
        result.best_score = 0.0;
        result.recommendation = "No valid materials found for comparison.";
        return result;
    }

    const auto& best = result.rankings.front();
    result.best_material = BowstringMaterialDatabase::instance().get(best.type);
    result.best_score = best.overall_score;

    std::ostringstream oss;
    oss << "Recommended material: " << best.name
        << " (overall score: " << std::fixed << std::setprecision(3) << best.overall_score << "). "
        << "Efficiency: " << std::fixed << std::setprecision(2) << (best.energy_efficiency * 100.0) << "%, "
        << "Velocity: " << std::fixed << std::setprecision(1) << best.estimated_velocity_ms << " m/s, "
        << "Estimated lifespan: " << std::fixed << std::setprecision(0) << best.fatigue_life_cycles << " cycles.";
    result.recommendation = oss.str();

    return result;
}

std::vector<std::pair<double, double>> StringComparator::generate_efficiency_curve(
    BowstringMaterialType type,
    int num_points
) const {
    const auto& material = BowstringMaterialDatabase::instance().get(type);
    std::vector<std::pair<double, double>> curve;

    int n = std::max(2, num_points);
    curve.reserve(n);

    for (int i = 0; i < n; ++i) {
        double pull_ratio = static_cast<double>(i) / static_cast<double>(n - 1);
        double efficiency = material.interpolate_efficiency(pull_ratio);
        curve.emplace_back(pull_ratio, efficiency);
    }

    return curve;
}

std::map<std::string, double> StringComparator::get_parameter_sensitivity(
    BowstringMaterialType type,
    double variation_pct
) const {
    std::map<std::string, double> sensitivity;
    const auto& material = BowstringMaterialDatabase::instance().get(type);
    auto base_metric = evaluate_material(material);

    double var = std::clamp(variation_pct, 0.01, 0.5);

    {
        StringComparisonConfig cfg = config_;
        cfg.string_length_m *= (1.0 + var);
        StringComparator sc(cfg);
        auto m = sc.evaluate_material(material);
        double delta = std::abs(m.energy_efficiency - base_metric.energy_efficiency);
        sensitivity["string_length"] = delta / std::max(base_metric.energy_efficiency, 1e-6);
    }

    {
        StringComparisonConfig cfg = config_;
        cfg.string_diameter_mm *= (1.0 + var);
        StringComparator sc(cfg);
        auto m = sc.evaluate_material(material);
        double delta = std::abs(m.estimated_velocity_ms - base_metric.estimated_velocity_ms);
        sensitivity["string_diameter"] = delta / std::max(base_metric.estimated_velocity_ms, 1e-6);
    }

    {
        StringComparisonConfig cfg = config_;
        cfg.draw_weight_N *= (1.0 + var);
        StringComparator sc(cfg);
        auto m = sc.evaluate_material(material);
        double delta = std::abs(m.stored_energy_J - base_metric.stored_energy_J);
        sensitivity["draw_weight"] = delta / std::max(base_metric.stored_energy_J, 1e-6);
    }

    {
        StringComparisonConfig cfg = config_;
        cfg.draw_length_m *= (1.0 + var);
        StringComparator sc(cfg);
        auto m = sc.evaluate_material(material);
        double delta = std::abs(m.stored_energy_J - base_metric.stored_energy_J);
        sensitivity["draw_length"] = delta / std::max(base_metric.stored_energy_J, 1e-6);
    }

    {
        StringComparisonConfig cfg = config_;
        cfg.arrow_mass_g *= (1.0 + var);
        StringComparator sc(cfg);
        auto m = sc.evaluate_material(material);
        double delta = std::abs(m.estimated_velocity_ms - base_metric.estimated_velocity_ms);
        sensitivity["arrow_mass"] = delta / std::max(base_metric.estimated_velocity_ms, 1e-6);
    }

    return sensitivity;
}

std::string StringComparator::generate_report(
    const std::vector<StringComparisonMetric>& results
) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    oss << "========================================\n";
    oss << "    Bowstring Material Comparison Report\n";
    oss << "========================================\n\n";

    oss << "Configuration:\n";
    oss << "  String Length:    " << config_.string_length_m << " m\n";
    oss << "  String Diameter:  " << config_.string_diameter_mm << " mm\n";
    oss << "  Draw Length:      " << config_.draw_length_m << " m\n";
    oss << "  Draw Weight:      " << config_.draw_weight_N << " N\n";
    oss << "  Arrow Mass:       " << config_.arrow_mass_g << " g\n";
    oss << "  Avg Humidity:     " << (config_.avg_humidity * 100.0) << " %\n";
    oss << "  Cycles per Day:   " << config_.cycles_per_day << "\n\n";

    if (results.empty()) {
        oss << "No materials to report.\n";
        return oss.str();
    }

    oss << std::setw(5) << "Rank"
        << std::setw(20) << "Material"
        << std::setw(12) << "Mass (g)"
        << std::setw(12) << "Stiffness"
        << std::setw(12) << "Efficiency"
        << std::setw(12) << "Vel (m/s)"
        << std::setw(14) << "Life (cycles)"
        << std::setw(12) << "Score\n";
    oss << std::string(99, '-') << "\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        oss << std::setw(5) << (i + 1)
            << std::setw(20) << r.name
            << std::setw(12) << r.string_mass_g
            << std::setw(12) << r.stiffness_Nm
            << std::setw(11) << (r.energy_efficiency * 100.0) << "%"
            << std::setw(12) << r.estimated_velocity_ms
            << std::setw(14) << r.fatigue_life_cycles
            << std::setw(12) << r.overall_score << "\n";
    }

    oss << "\n========================================\n";
    return oss.str();
}

} // namespace modules
} // namespace crossbow
