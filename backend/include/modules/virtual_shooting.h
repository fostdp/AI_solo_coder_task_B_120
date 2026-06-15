#pragma once

#include "common.h"
#include "dynamics_model.h"
#include <vector>
#include <cstdint>
#include <random>

namespace crossbow {
namespace modules {

struct VirtualTarget {
    uint32_t id;
    double x_m;
    double y_m;
    double radius_m;
    double health;
    bool destroyed;
};

struct VirtualShot {
    uint32_t id;
    double aim_x;
    double aim_y;
    double actual_x;
    double actual_y;
    double launch_angle;
    double velocity;
    double score;
    uint32_t hit_target_id;
};

struct ShootingSessionConfig {
    uint32_t max_targets = 10;
    double target_radius_min_m = 0.05;
    double target_radius_max_m = 0.5;
    double min_distance_m = 5.0;
    double max_distance_m = 100.0;
    double time_limit_s = 120.0;
    double arrow_mass_g = 30.0;
    CrossbowType crossbow_type;
};

struct ShootingStats {
    uint32_t total_shots;
    uint32_t hits;
    uint32_t misses;
    double score;
    double accuracy_pct;
    double avg_velocity_ms;
    double avg_distance_m;
    uint32_t destroyed_count;
};

class VirtualShooting {
public:
    VirtualShooting();

    void start_session(const ShootingSessionConfig& config);
    bool add_target(double x_m, double y_m, double radius_m, double health = 1.0);
    bool remove_target(uint32_t target_id);
    void aim(double x_m, double y_m);
    VirtualShot fire();
    std::vector<TrajectoryPoint> get_trajectory(double launch_angle, double initial_velocity) const;
    bool validate_parameters(double aim_x, double aim_y, double velocity, double launch_angle_deg) const;
    ShootingStats get_stats() const;
    void reset();

    const std::vector<VirtualTarget>& get_targets() const;
    const std::vector<VirtualShot>& get_shots() const;
    bool is_session_active() const;

private:
    bool session_active_;
    ShootingSessionConfig config_;
    std::vector<VirtualTarget> targets_;
    std::vector<VirtualShot> shots_;
    double current_aim_x_;
    double current_aim_y_;
    uint32_t next_target_id_;
    uint32_t next_shot_id_;
    double total_velocity_sum_;
    double total_distance_sum_;
    mutable std::mt19937 rng_;

    double compute_distance(double x, double y) const;
    double compute_aim_error(double distance) const;
    double calculate_score(const VirtualTarget* target, double distance) const;
};

} // namespace modules
} // namespace crossbow
