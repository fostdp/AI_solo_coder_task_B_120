#include "modules/formation_simulator_module.h"
#include "formation_simulator.h"
#include <algorithm>
#include <cmath>

namespace crossbow {
namespace modules {

FormationSimulatorModule::FormationSimulatorModule()
    : simulator_(std::make_unique<FormationSimulator>(0))
    , rl_optimizer_(std::make_unique<FormationRLOptimizer>(12345)) {
}

FormationSimulatorModule::FormationSimulatorModule(const FormationModuleConfig& config)
    : config_(config)
    , simulator_(std::make_unique<FormationSimulator>(0))
    , rl_optimizer_(std::make_unique<FormationRLOptimizer>(12345)) {
}

void FormationSimulatorModule::set_config(const FormationModuleConfig& config) {
    config_ = config;
}

const FormationModuleConfig& FormationSimulatorModule::get_config() const {
    return config_;
}

void FormationSimulatorModule::set_crossbow_types(const std::map<int, CrossbowType>& types) {
    crossbow_types_ = types;
    simulator_->set_crossbow_types(types);
    rl_optimizer_->set_crossbow_types(types);
}

void FormationSimulatorModule::set_target(const TargetSpecification& target) {
    current_target_ = target;
    simulator_->set_target(target);
    rl_optimizer_->set_target(target);
}

std::vector<CrossbowPlacement> FormationSimulatorModule::generate_placement(
    FormationType type,
    double center_x_m,
    double center_y_m) const {
    double spacing_multiplier = 1.0;
    const auto& design = FormationDatabase::instance().get(type);
    if (design.minimum_safety_spacing_m > 0.0) {
        spacing_multiplier = config_.spacing_m / design.minimum_safety_spacing_m;
    }
    return simulator_->generate_formation(
        type,
        config_.num_crossbows,
        center_x_m,
        center_y_m,
        config_.orientation_deg,
        spacing_multiplier
    );
}

double FormationSimulatorModule::compute_average_distance(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target) const {
    if (placements.empty()) return 0.0;
    double total = 0.0;
    for (const auto& p : placements) {
        double dx = p.x_m - target.center_x_m;
        double dy = p.y_m - target.center_y_m;
        total += std::sqrt(dx * dx + dy * dy);
    }
    return total / static_cast<double>(placements.size());
}

double FormationSimulatorModule::compute_concentration_score(
    const std::vector<std::pair<double, double>>& hit_pattern,
    const TargetSpecification& target) const {
    if (hit_pattern.empty()) return 0.0;
    double mean_x = 0.0, mean_y = 0.0;
    for (const auto& p : hit_pattern) {
        mean_x += p.first;
        mean_y += p.second;
    }
    mean_x /= static_cast<double>(hit_pattern.size());
    mean_y /= static_cast<double>(hit_pattern.size());
    double variance = 0.0;
    for (const auto& p : hit_pattern) {
        double dx = p.first - mean_x;
        double dy = p.second - mean_y;
        variance += dx * dx + dy * dy;
    }
    variance /= static_cast<double>(hit_pattern.size());
    double std_dev = std::sqrt(variance);
    double target_area = 0.0;
    switch (target.type) {
        case TargetType::SINGLE_POINT:
            target_area = 1.0;
            break;
        case TargetType::AREA_RECTANGLE:
        case TargetType::LINEAR_DEFENSE:
        case TargetType::CONVOY_PATH:
        case TargetType::FORTIFICATION:
            target_area = target.width_m * target.depth_m;
            break;
        case TargetType::AREA_CIRCLE:
            target_area = M_PI * target.radius_m * target.radius_m;
            break;
        default:
            target_area = 1.0;
            break;
    }
    double target_radius_eq = target_area > 0.0 ? std::sqrt(target_area / M_PI) : 1.0;
    double concentration = target_radius_eq / (std_dev + 1e-6);
    return std::min(1.0, concentration);
}

FireCoverageResult FormationSimulatorModule::evaluate_coverage(FormationType type) const {
    auto placements = generate_placement(type, 0.0, 0.0);
    FireCoverageResult result;
    if (placements.empty()) {
        result.coverage_pct = 0.0;
        result.projectiles_per_m2 = 0.0;
        result.hit_probability = 0.0;
        result.avg_distance_m = 0.0;
        result.concentration_score = 0.0;
        return result;
    }
    EngagementResult engagement = simulator_->simulate_engagement(
        placements,
        current_target_,
        config_.salvo_count
    );
    result.coverage_pct = engagement.target_coverage_percent;
    result.projectiles_per_m2 = engagement.firing_plan.density_at_target_projectiles_per_m2;
    result.hit_probability = engagement.overall_hit_probability;
    result.avg_distance_m = compute_average_distance(placements, current_target_);
    result.concentration_score = compute_concentration_score(engagement.hit_pattern, current_target_);
    return result;
}

std::vector<std::pair<FormationType, FireCoverageResult>> FormationSimulatorModule::compare_formations(
    const std::vector<FormationType>& type_list) const {
    std::vector<FormationType> types_to_eval = type_list;
    if (types_to_eval.empty()) {
        types_to_eval = {
            FormationType::LINE_ABREAST,
            FormationType::WEDGE,
            FormationType::ECHELON_LEFT,
            FormationType::ECHELON_RIGHT,
            FormationType::VEE,
            FormationType::ARCHER_PARABOLA,
            FormationType::COILED_BOW,
            FormationType::CRANE_WING,
            FormationType::SQUARE_GRID,
            FormationType::FISH_SCALE
        };
    }
    std::vector<std::pair<FormationType, FireCoverageResult>> results;
    results.reserve(types_to_eval.size());
    for (const auto& type : types_to_eval) {
        results.emplace_back(type, evaluate_coverage(type));
    }
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) {
            return a.second.coverage_pct > b.second.coverage_pct;
        });
    return results;
}

std::pair<FormationType, FireCoverageResult> FormationSimulatorModule::find_optimal_formation(
    const TargetSpecification& target) const {
    auto all_results = compare_formations({});
    if (all_results.empty()) {
        return {FormationType::LINE_ABREAST, FireCoverageResult{}};
    }
    return all_results.front();
}

FireCoverageResult FormationSimulatorModule::get_fire_density(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target) const {
    FireCoverageResult result;
    if (placements.empty()) {
        result.coverage_pct = 0.0;
        result.projectiles_per_m2 = 0.0;
        result.hit_probability = 0.0;
        result.avg_distance_m = 0.0;
        result.concentration_score = 0.0;
        return result;
    }
    EngagementResult engagement = simulator_->simulate_engagement(
        placements,
        target,
        config_.salvo_count
    );
    result.coverage_pct = engagement.target_coverage_percent;
    result.projectiles_per_m2 = engagement.firing_plan.density_at_target_projectiles_per_m2;
    result.hit_probability = engagement.overall_hit_probability;
    result.avg_distance_m = compute_average_distance(placements, target);
    result.concentration_score = compute_concentration_score(engagement.hit_pattern, target);
    return result;
}

std::vector<std::vector<double>> FormationSimulatorModule::generate_heatmap(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target) const {
    const int grid_size = 20;
    std::vector<std::vector<double>> heatmap(grid_size, std::vector<double>(grid_size, 0.0));
    if (placements.empty()) return heatmap;
    double half_w = target.width_m > 0 ? target.width_m / 2.0 : 50.0;
    double half_d = target.depth_m > 0 ? target.depth_m / 2.0 : 50.0;
    if (target.radius_m > 0) {
        half_w = target.radius_m;
        half_d = target.radius_m;
    }
    double min_x = target.center_x_m - half_w;
    double max_x = target.center_x_m + half_w;
    double min_y = target.center_y_m - half_d;
    double max_y = target.center_y_m + half_d;
    double cell_w = (max_x - min_x) / static_cast<double>(grid_size);
    double cell_d = (max_y - min_y) / static_cast<double>(grid_size);
    for (const auto& placement : placements) {
        if (!placement.is_active) continue;
        auto pattern = simulator_->project_hit_pattern(placement, target, 100);
        for (const auto& hit : pattern) {
            int gx = static_cast<int>((hit.first - min_x) / cell_w);
            int gy = static_cast<int>((hit.second - min_y) / cell_d);
            if (gx >= 0 && gx < grid_size && gy >= 0 && gy < grid_size) {
                heatmap[gy][gx] += 1.0;
            }
        }
    }
    double max_val = 0.0;
    for (const auto& row : heatmap) {
        for (double v : row) {
            max_val = std::max(max_val, v);
        }
    }
    if (max_val > 0.0) {
        for (auto& row : heatmap) {
            for (double& v : row) {
                v /= max_val;
            }
        }
    }
    return heatmap;
}

RLOptimizationResult FormationSimulatorModule::train_rl(
    const TargetSpecification& target,
    int episodes) const {
    rl_optimizer_->set_target(target);
    rl_optimizer_->set_training_conditions(
        std::max(4, config_.num_crossbows / 2),
        config_.num_crossbows * 2
    );
    return rl_optimizer_->train_q_learning(
        config_.num_crossbows,
        episodes
    );
}

bool FormationSimulatorModule::validate_placement(
    const std::vector<CrossbowPlacement>& placements) const {
    const double min_distance_m = 1.0;
    for (size_t i = 0; i < placements.size(); ++i) {
        for (size_t j = i + 1; j < placements.size(); ++j) {
            double dx = placements[i].x_m - placements[j].x_m;
            double dy = placements[i].y_m - placements[j].y_m;
            double dist_sq = dx * dx + dy * dy;
            if (dist_sq < min_distance_m * min_distance_m) {
                return false;
            }
        }
    }
    return true;
}

} // namespace modules
} // namespace crossbow
