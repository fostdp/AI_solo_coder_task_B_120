#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <random>

namespace crossbow {

enum class SightType {
    BASIC_NOTCH = 0,
    PIN_SIGHT = 1,
    RAMP_SIGHT = 2,
    MULTI_PIN = 3,
    TANGENT_SIGHT = 4,
    PRECISION_PEEP = 5,
    CUSTOM = 99
};

struct SightDesign {
    SightType type;
    std::string name;
    std::string description;
    std::string historical_origin;

    double sight_height_mm;
    double sight_radius_mm;
    double notch_width_mm;
    double post_diameter_mm;
    double aperture_diameter_mm;

    int scale_count;
    double scale_range_min_m;
    double scale_range_max_m;
    double scale_interval_m;

    double material_density;
    double thermal_expansion_coeff;
    double manufacturing_precision_mm;
    double total_weight_g;
    double estimated_cost;

    double user_experience_rating;
    double adjustability_rating;
    double durability_rating;
};

struct OpticalErrorBudget {
    double visual_acuity_arcmin;
    double parallax_error_mrad;
    double parallax_distance_m;

    double ambient_light_lux;
    double glare_intensity_factor;
    double contrast_ratio;

    double atmospheric_refraction_mrad;
    double heat_haze_mrad;
    double turbulence_strength;

    double eye_relaxation_factor;
    double aiming_time_seconds;
    double user_experience_factor;

    double mechanical_play_mm;
    double thermal_distortion_mrad;
    double vibration_misalignment_mrad;
};

struct SightAimingResult {
    double raw_aim_angle_deg;
    double optical_error_elevation_mrad;
    double optical_error_azimuth_mrad;
    double mechanical_error_elevation_mrad;
    double mechanical_error_azimuth_mrad;
    double environmental_error_elevation_mrad;
    double environmental_error_azimuth_mrad;
    double total_error_elevation_mrad;
    double total_error_azimuth_mrad;
    double corrected_elevation_deg;
    double corrected_azimuth_deg;
    double predicted_impact_offset_x_m;
    double predicted_impact_offset_y_m;
    double aiming_confidence;
    int64_t timestamp_ms;
};

struct SightOptimizationResult {
    SightDesign optimal_design;
    double overall_score;
    double accuracy_score;
    double usability_score;
    double manufacturing_score;
    double historical_authenticity_score;
    double ceph_improvement_percent;
    double average_total_error_mrad;
    double optimal_height_mm;
    double optimal_radius_mm;
    double optimal_scale_interval_m;
    std::vector<std::pair<std::string, double>> design_tradeoffs;
};

struct SightScale {
    int index;
    double target_range_m;
    double scale_height_mm;
    double calibrated_angle_deg;
    double actual_angle_deg;
    double calibration_error_mrad;
};

class SightDesignDatabase {
public:
    static SightDesignDatabase& instance();
    const SightDesign& get(SightType type) const;
    const SightDesign& get(const std::string& name) const;
    std::vector<SightDesign> list_all() const;

private:
    SightDesignDatabase();
    void initialize_defaults();
    std::map<SightType, SightDesign> designs_;
    std::map<std::string, SightType> name_to_type_;
};

class OpticalErrorSimulator {
public:
    explicit OpticalErrorSimulator(uint64_t seed = 0);

    void set_ambient_conditions(double light_lux, double temperature_c,
                                double humidity, double wind_speed_mps);
    void set_user_profile(double visual_acuity_arcmin = 1.0,
                          double experience_factor = 0.5);
    void set_distance(double target_distance_m);

    OpticalErrorBudget get_current_error_budget() const;
    double calculate_visual_acuity_error(double distance_m) const;
    double calculate_parallax_error(double sight_radius_mm,
                                     double parallax_distance_m,
                                     double eye_offset_mm = 2.0) const;
    double calculate_atmospheric_refraction(double distance_m,
                                            double temperature_c = 15.0,
                                            double pressure_hpa = 1013.25) const;
    double calculate_heat_haze_error(double distance_m,
                                     double temperature_c,
                                     double ground_temp_c = 35.0) const;
    double calculate_mechanical_play_error(const SightDesign& sight) const;
    double calculate_thermal_distortion(const SightDesign& sight,
                                         double delta_temperature_c) const;
    std::pair<double, double> calculate_glare_effect(double light_lux,
                                                      double sun_angle_deg = 30.0) const;

    SightAimingResult simulate_single_aim(
        const SightDesign& sight,
        double nominal_elevation_deg,
        double nominal_azimuth_deg = 0.0,
        double target_distance_m = 200.0
    );

    std::vector<SightAimingResult> simulate_multiple_aims(
        const SightDesign& sight,
        double nominal_elevation_deg,
        double nominal_azimuth_deg,
        double target_distance_m,
        int num_samples = 100
    );

    double estimate_sight_cep(
        const SightDesign& sight,
        double target_distance_m,
        int num_samples = 500
    );

private:
    mutable std::mt19937 rng_;
    double ambient_light_lux_;
    double ambient_temperature_c_;
    double ambient_humidity_;
    double wind_speed_mps_;
    double user_visual_acuity_;
    double user_experience_;
    double current_distance_m_;

    double sample_normal(double mean, double stddev) const;
    double sample_cauchy(double scale) const;
};

class SightScaleCalibrator {
public:
    SightScaleCalibrator();

    void set_crossbow(const CrossbowType& crossbow);
    void set_sight_design(const SightDesign& sight);

    std::vector<SightScale> calibrate_scales(
        double start_range_m = 50.0,
        double end_range_m = 600.0,
        double step_m = 50.0
    );

    SightScale calibrate_single_scale(double target_range_m);
    double estimate_range_from_sight_height(
        double sight_height_mm,
        const std::vector<SightScale>& calibrated_scales
    );

    std::vector<std::pair<double, double>> generate_ballistic_table(
        double start_range_m,
        double end_range_m,
        double step_m
    );

private:
    CrossbowType crossbow_;
    SightDesign sight_;
    std::vector<double> ballistic_angles_;
    std::vector<double> ballistic_ranges_;

    double compute_required_elevation(double target_range_m);
    double sight_height_to_angle(double sight_height_mm, double sight_radius_mm);
    double angle_to_sight_height(double angle_deg, double sight_radius_mm);
};

class SightOptimizer {
public:
    SightOptimizer();

    void set_crossbow(const CrossbowType& crossbow);
    void set_target_range(double distance_m);
    void set_constraints(double max_height_mm = 80.0,
                         double max_weight_g = 200.0,
                         double max_cost = 100.0);
    void set_objective_weights(double accuracy_w = 0.45,
                               double usability_w = 0.25,
                               double manufacturing_w = 0.15,
                               double historical_w = 0.15);

    SightOptimizationResult optimize(SightType base_type = SightType::RAMP_SIGHT);
    SightOptimizationResult grid_search_optimize(
        SightType base_type,
        int height_samples = 20,
        int radius_samples = 15,
        int scale_samples = 10
    );

    std::vector<std::tuple<double, double, double>> explore_parameter_space(
        SightType base_type,
        int samples_per_dim = 10
    );

private:
    CrossbowType crossbow_;
    double target_range_m_;
    double max_height_mm_;
    double max_weight_g_;
    double max_cost_;
    double weight_accuracy_;
    double weight_usability_;
    double weight_manufacturing_;
    double weight_historical_;

    OpticalErrorSimulator error_sim_;

    double evaluate_design(const SightDesign& design);
    double calculate_accuracy_score(const SightDesign& design);
    double calculate_usability_score(const SightDesign& design);
    double calculate_manufacturing_score(const SightDesign& design);
    double calculate_historical_score(const SightDesign& design);
};

} // namespace crossbow
