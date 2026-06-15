#include "modules/virtual_shooting.h"
#include "dynamics_model.h"
#include "common.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace crossbow {
namespace modules {

static constexpr double MIN_VELOCITY_MS = 10.0;
static constexpr double MAX_VELOCITY_MS = 200.0;
static constexpr double MIN_LAUNCH_ANGLE_DEG = 0.0;
static constexpr double MAX_LAUNCH_ANGLE_DEG = 85.0;
static constexpr double AIM_ERROR_BASE_M = 0.01;
static constexpr double AIM_ERROR_PER_METER = 0.002;
static constexpr double BASE_HIT_SCORE = 10.0;
static constexpr double BONUS_SMALL_RADIUS_THRESHOLD = 0.15;
static constexpr double BONUS_FAR_DISTANCE_THRESHOLD = 50.0;
static constexpr double RADIUS_BONUS_MULTIPLIER = 5.0;
static constexpr double DISTANCE_BONUS_MULTIPLIER = 0.1;

VirtualShooting::VirtualShooting()
    : session_active_(false)
    , config_()
    , targets_()
    , shots_()
    , current_aim_x_(0.0)
    , current_aim_y_(0.0)
    , next_target_id_(1)
    , next_shot_id_(1)
    , total_velocity_sum_(0.0)
    , total_distance_sum_(0.0)
    , rng_(std::random_device{}())
{
}

void VirtualShooting::start_session(const ShootingSessionConfig& config) {
    config_ = config;
    session_active_ = true;
    targets_.clear();
    shots_.clear();
    current_aim_x_ = 0.0;
    current_aim_y_ = 0.0;
    next_target_id_ = 1;
    next_shot_id_ = 1;
    total_velocity_sum_ = 0.0;
    total_distance_sum_ = 0.0;
}

bool VirtualShooting::add_target(double x_m, double y_m, double radius_m, double health) {
    if (!session_active_) return false;
    if (targets_.size() >= config_.max_targets) return false;

    double dist = compute_distance(x_m, y_m);
    if (dist < config_.min_distance_m || dist > config_.max_distance_m) return false;

    double clamped_radius = std::clamp(radius_m, config_.target_radius_min_m, config_.target_radius_max_m);

    VirtualTarget target;
    target.id = next_target_id_++;
    target.x_m = x_m;
    target.y_m = y_m;
    target.radius_m = clamped_radius;
    target.health = std::max(0.0, health);
    target.destroyed = target.health <= 0.0;

    targets_.push_back(target);
    return true;
}

bool VirtualShooting::remove_target(uint32_t target_id) {
    if (!session_active_) return false;

    auto it = std::remove_if(targets_.begin(), targets_.end(),
        [target_id](const VirtualTarget& t) { return t.id == target_id; });

    if (it == targets_.end()) return false;

    targets_.erase(it, targets_.end());
    return true;
}

void VirtualShooting::aim(double x_m, double y_m) {
    current_aim_x_ = x_m;
    current_aim_y_ = y_m;
}

VirtualShot VirtualShooting::fire() {
    VirtualShot shot{};
    shot.id = next_shot_id_++;
    shot.aim_x = current_aim_x_;
    shot.aim_y = current_aim_y_;
    shot.score = 0.0;
    shot.hit_target_id = 0;

    if (!session_active_) {
        shots_.push_back(shot);
        return shot;
    }

    double aim_distance = compute_distance(current_aim_x_, current_aim_y_);
    double launch_angle_deg = aim_distance > 0.0
        ? std::atan2(current_aim_y_, current_aim_x_) * 180.0 / M_PI
        : 45.0;

    DynamicsModel model(config_.crossbow_type);
    double initial_velocity = model.calculate_initial_velocity(
        config_.crossbow_type.draw_weight,
        config_.crossbow_type.bow_length * 0.75,
        config_.arrow_mass_g
    );

    if (!validate_parameters(current_aim_x_, current_aim_y_, initial_velocity, launch_angle_deg)) {
        shots_.push_back(shot);
        return shot;
    }

    shot.launch_angle = launch_angle_deg;
    shot.velocity = initial_velocity;

    double error_std = compute_aim_error(aim_distance);
    std::normal_distribution<double> error_dist(0.0, error_std);
    double error_x = error_dist(rng_);
    double error_y = error_dist(rng_);

    auto trajectory = model.simulate_trajectory(
        initial_velocity,
        launch_angle_deg * M_PI / 180.0,
        0.0, 0.0, 0.001
    );

    ShotRecord results = model.calculate_shot_results(trajectory);
    shot.actual_x = results.impact_point.x + error_x;
    shot.actual_y = results.impact_point.y + error_y;

    VirtualTarget* hit_target = nullptr;
    for (auto& target : targets_) {
        if (target.destroyed) continue;
        double dx = shot.actual_x - target.x_m;
        double dy = shot.actual_y - target.y_m;
        double dist_to_center = std::sqrt(dx * dx + dy * dy);

        if (dist_to_center <= target.radius_m) {
            hit_target = &target;
            break;
        }
    }

    if (hit_target != nullptr) {
        shot.hit_target_id = hit_target->id;
        hit_target->health -= 1.0;
        if (hit_target->health <= 0.0) {
            hit_target->destroyed = true;
        }
        shot.score = calculate_score(hit_target, aim_distance);
    }

    total_velocity_sum_ += initial_velocity;
    total_distance_sum_ += aim_distance;
    shots_.push_back(shot);

    return shot;
}

std::vector<TrajectoryPoint> VirtualShooting::get_trajectory(double launch_angle, double initial_velocity) const {
    DynamicsModel model(config_.crossbow_type);
    return model.simulate_trajectory(
        initial_velocity,
        launch_angle * M_PI / 180.0,
        0.0, 0.0, 0.001
    );
}

bool VirtualShooting::validate_parameters(double aim_x, double aim_y, double velocity, double launch_angle_deg) const {
    double dist = compute_distance(aim_x, aim_y);
    if (dist < config_.min_distance_m || dist > config_.max_distance_m) return false;
    if (velocity < MIN_VELOCITY_MS || velocity > MAX_VELOCITY_MS) return false;
    if (launch_angle_deg < MIN_LAUNCH_ANGLE_DEG || launch_angle_deg > MAX_LAUNCH_ANGLE_DEG) return false;
    return true;
}

ShootingStats VirtualShooting::get_stats() const {
    ShootingStats stats{};
    stats.total_shots = static_cast<uint32_t>(shots_.size());

    for (const auto& shot : shots_) {
        if (shot.hit_target_id != 0) {
            stats.hits++;
        } else {
            stats.misses++;
        }
        stats.score += shot.score;
    }

    stats.accuracy_pct = stats.total_shots > 0
        ? (static_cast<double>(stats.hits) / static_cast<double>(stats.total_shots)) * 100.0
        : 0.0;

    stats.avg_velocity_ms = stats.total_shots > 0
        ? total_velocity_sum_ / static_cast<double>(stats.total_shots)
        : 0.0;

    stats.avg_distance_m = stats.total_shots > 0
        ? total_distance_sum_ / static_cast<double>(stats.total_shots)
        : 0.0;

    stats.destroyed_count = 0;
    for (const auto& target : targets_) {
        if (target.destroyed) {
            stats.destroyed_count++;
        }
    }

    return stats;
}

void VirtualShooting::reset() {
    session_active_ = false;
    targets_.clear();
    shots_.clear();
    current_aim_x_ = 0.0;
    current_aim_y_ = 0.0;
    next_target_id_ = 1;
    next_shot_id_ = 1;
    total_velocity_sum_ = 0.0;
    total_distance_sum_ = 0.0;
    config_ = ShootingSessionConfig();
}

const std::vector<VirtualTarget>& VirtualShooting::get_targets() const {
    return targets_;
}

const std::vector<VirtualShot>& VirtualShooting::get_shots() const {
    return shots_;
}

bool VirtualShooting::is_session_active() const {
    return session_active_;
}

double VirtualShooting::compute_distance(double x, double y) const {
    return std::sqrt(x * x + y * y);
}

double VirtualShooting::compute_aim_error(double distance) const {
    return AIM_ERROR_BASE_M + AIM_ERROR_PER_METER * distance;
}

double VirtualShooting::calculate_score(const VirtualTarget* target, double distance) const {
    if (target == nullptr) return 0.0;

    double score = BASE_HIT_SCORE;

    if (target->radius_m < BONUS_SMALL_RADIUS_THRESHOLD) {
        double radius_bonus = (BONUS_SMALL_RADIUS_THRESHOLD - target->radius_m) * RADIUS_BONUS_MULTIPLIER * 10.0;
        score += radius_bonus;
    }

    if (distance > BONUS_FAR_DISTANCE_THRESHOLD) {
        double distance_bonus = (distance - BONUS_FAR_DISTANCE_THRESHOLD) * DISTANCE_BONUS_MULTIPLIER;
        score += distance_bonus;
    }

    return score;
}

} // namespace modules
} // namespace crossbow
