#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <random>
#include <array>

namespace crossbow {

enum class FormationType {
    LINE_ABREAST = 0,
    WEDGE = 1,
    ECHELON_LEFT = 2,
    ECHELON_RIGHT = 3,
    VEE = 4,
    ARCHER_PARABOLA = 5,
    COILED_BOW = 6,
    CRANE_WING = 7,
    SQUARE_GRID = 8,
    FISH_SCALE = 9,
    CUSTOM = 99
};

enum class TargetType {
    SINGLE_POINT = 0,
    AREA_RECTANGLE = 1,
    AREA_CIRCLE = 2,
    LINEAR_DEFENSE = 3,
    CONVOY_PATH = 4,
    FORTIFICATION = 5
};

struct CrossbowPlacement {
    int id;
    int crossbow_type_id;
    std::string name;
    double x_m;
    double y_m;
    double z_m;
    double facing_deg;
    double elevation_deg;
    double firing_delay_ms;
    double salvo_group;
    bool is_active;
    int crew_size;
    double supply_status;
};

struct TargetSpecification {
    TargetType type;
    std::string name;
    double center_x_m;
    double center_y_m;
    double width_m;
    double depth_m;
    double radius_m;
    double priority_factor;
    double hardness_factor;
    int personnel_estimate;
};

struct FiringPlan {
    int round_id;
    int64_t start_timestamp_ms;
    int64_t end_timestamp_ms;
    int total_projectiles;
    int crossbows_firing;
    double salvo_interval_ms;
    double time_on_target_s;
    double average_volleys_per_minute;
    double density_at_target_projectiles_per_m2;
    double estimated_kill_probability;
    double estimated_casualties;
    double ammunition_consumed;
    double supply_burden_kg;
};

struct EngagementResult {
    FormationType formation_type;
    std::string formation_name;
    int num_crossbows;
    int total_shots;
    int total_hits;
    double overall_hit_probability;
    double target_coverage_percent;
    double saturation_score;
    double suppression_effect;
    double enemy_response_time_s;
    double friendly_risk_radius_m;
    double logistical_efficiency;
    FiringPlan firing_plan;
    std::vector<std::pair<double, double>> hit_pattern;
    int64_t timestamp_ms;
};

struct FormationDesign {
    FormationType type;
    std::string name;
    std::string description;
    std::string historical_origin;
    int min_crossbows;
    int optimal_crossbows;
    int max_crossbows;
    double frontage_per_unit_m;
    double depth_per_unit_m;
    double minimum_safety_spacing_m;
    double best_suited_range_m;
    double defensive_utility;
    double offensive_utility;
    double siege_utility;
    double mobility_score;
    double command_and_control_score;
};

struct SalvoOptimization {
    int optimal_salvo_size;
    double optimal_interval_ms;
    int total_volleys;
    double saturation_time_s;
    double average_projectile_spacing_m;
    double time_over_target_seconds;
    double minimum_overkill_penalty;
};

class FormationDatabase {
public:
    static FormationDatabase& instance();
    const FormationDesign& get(FormationType type) const;
    const FormationDesign& get(const std::string& name) const;
    std::vector<FormationDesign> list_all() const;

private:
    FormationDatabase();
    void initialize_defaults();
    std::map<FormationType, FormationDesign> formations_;
    std::map<std::string, FormationType> name_to_type_;
};

class FormationSimulator {
public:
    FormationSimulator(uint64_t seed = 0);

    void set_crossbow_types(const std::map<int, CrossbowType>& types);
    void set_target(const TargetSpecification& target);

    std::vector<CrossbowPlacement> generate_formation(
        FormationType type,
        int num_crossbows,
        double center_x_m,
        double center_y_m,
        double orientation_deg = 0.0,
        double spacing_multiplier = 1.0
    );

    std::vector<CrossbowPlacement> generate_custom_formation(
        const std::vector<int>& crossbow_type_ids,
        const std::vector<std::pair<double, double>>& positions,
        const std::vector<double>& facings_deg
    );

    EngagementResult simulate_engagement(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target,
        int volleys = 3,
        double wind_x_mps = 0.0,
        double wind_y_mps = 0.0,
        double visibility_km = 10.0
    );

    SalvoOptimization optimize_salvo_timing(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target,
        double target_density_projectiles_per_m2 = 0.5
    );

    std::vector<EngagementResult> compare_formations(
        const std::vector<FormationType>& formation_types,
        int num_crossbows,
        const TargetSpecification& target,
        int volleys = 3
    );

    double calculate_target_coverage(
        const std::vector<CrossbowPlacement>& placements,
        const TargetSpecification& target
    );

    double estimate_hit_probability(
        const CrossbowPlacement& placement,
        const TargetSpecification& target
    );

    std::vector<std::pair<double, double>> project_hit_pattern(
        const CrossbowPlacement& placement,
        const TargetSpecification& target,
        int projectiles_per_crossbow = 50
    );

    FiringPlan generate_firing_plan(
        const std::vector<CrossbowPlacement>& placements,
        int volleys,
        double salvo_interval_ms = 500.0
    );

private:
    mutable std::mt19937 rng_;
    std::map<int, CrossbowType> crossbow_types_;
    TargetSpecification current_target_;

    std::pair<double, double> rotate_point(
        double x, double y, double cx, double cy, double angle_deg
    ) const;

    double distance_to_target(
        const CrossbowPlacement& placement,
        const TargetSpecification& target
    ) const;

    double calculate_target_area(const TargetSpecification& target) const;
    bool is_point_in_target(double x, double y, const TargetSpecification& target) const;
};

struct RLObservation {
    int formation_type_id;
    int num_crossbows;
    double target_center_x;
    double target_center_y;
    double target_size;
    double avg_distance;
    double current_hit_rate;
    double current_coverage;
    double wind_speed;
    double wind_direction;
    double action_taken;
    double reward;
    int step;
};

struct RLOptimizationResult {
    FormationType best_formation;
    std::vector<double> best_placement_params;
    double best_reward;
    int episodes_trained;
    double convergence_rate;
    std::vector<std::pair<int, double>> episode_rewards;
    std::vector<RLObservation> training_trajectory;
    std::vector<std::pair<FormationType, double>> formation_value_estimates;
    int64_t timestamp_ms;
};

class FormationRLOptimizer {
public:
    FormationRLOptimizer(uint64_t seed = 12345);

    void set_crossbow_types(const std::map<int, CrossbowType>& types);
    void set_target(const TargetSpecification& target);
    void set_environment(double wind_x, double wind_y, double visibility_km);

    RLOptimizationResult train_q_learning(
        int num_crossbows = 24,
        int episodes = 1000,
        double learning_rate = 0.1,
        double discount_factor = 0.95,
        double exploration_rate = 0.2,
        double exploration_decay = 0.995
    );

    RLOptimizationResult train_policy_gradient(
        int num_crossbows = 24,
        int episodes = 500,
        int steps_per_episode = 20,
        double learning_rate = 0.01,
        double baseline_weight = 0.9
    );

    std::vector<CrossbowPlacement> apply_optimal_policy(
        const RLOptimizationResult& result,
        double center_x,
        double center_y,
        double orientation
    );

    double evaluate_reward(
        const EngagementResult& engagement,
        double weight_hit_rate = 0.4,
        double weight_coverage = 0.25,
        double weight_saturation = 0.2,
        double weight_logistics = 0.1,
        double weight_casualties = 0.05
    );

    void set_training_conditions(
        int min_crossbows,
        int max_crossbows,
        bool allow_formation_switching = true,
        bool allow_spacing_optimization = true,
        bool allow_orientation_optimization = true
    );

private:
    mutable std::mt19937 rng_;
    FormationSimulator simulator_;
    std::map<int, CrossbowType> crossbow_types_;
    TargetSpecification target_;
    double wind_x_;
    double wind_y_;
    double visibility_km_;
    int min_crossbows_;
    int max_crossbows_;
    bool allow_formation_switch_;
    bool allow_spacing_switch_;
    bool allow_orientation_switch_;

    std::vector<std::vector<double>> q_table_;
    std::map<int, double> formation_value_function_;

    int discretize_state(
        FormationType formation,
        int crossbows,
        double avg_distance,
        double coverage
    );

    int get_num_states() const;
    int get_num_actions() const;
    int select_action(int state, double exploration_rate);
    double calculate_spacing_modifier(int action_id);
    FormationType get_formation_for_action(int action_id);
};

} // namespace crossbow
