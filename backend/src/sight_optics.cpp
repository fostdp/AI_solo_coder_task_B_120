#include "sight_optics.h"
#include "dynamics_model.h"
#include "logger.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <limits>

namespace crossbow {

static constexpr double DEG_TO_RAD = M_PI / 180.0;
static constexpr double RAD_TO_DEG = 180.0 / M_PI;
static constexpr double MRAD_TO_DEG = 180.0 / (M_PI * 1000.0);
static constexpr double DEG_TO_MRAD = M_PI * 1000.0 / 180.0;
static constexpr double ARCMIN_TO_RAD = M_PI / (180.0 * 60.0);

SightDesignDatabase::SightDesignDatabase() {
    initialize_defaults();
}

SightDesignDatabase& SightDesignDatabase::instance() {
    static SightDesignDatabase db;
    return db;
}

void SightDesignDatabase::initialize_defaults() {
    SightDesign basic;
    basic.type = SightType::BASIC_NOTCH;
    basic.name = "basic_notch";
    basic.description = "原始缺口望山，殷周早期形制，后视图槽口对准箭尾";
    basic.historical_origin = "甲骨文及西周青铜器铭文所载，最早的机械瞄准具雏形";
    basic.sight_height_mm = 25.0;
    basic.sight_radius_mm = 600.0;
    basic.notch_width_mm = 4.0;
    basic.post_diameter_mm = 3.5;
    basic.aperture_diameter_mm = 0.0;
    basic.scale_count = 3;
    basic.scale_range_min_m = 50.0;
    basic.scale_range_max_m = 200.0;
    basic.scale_interval_m = 75.0;
    basic.material_density = 7850.0;
    basic.thermal_expansion_coeff = 12e-6;
    basic.manufacturing_precision_mm = 1.0;
    basic.total_weight_g = 35.0;
    basic.estimated_cost = 5.0;
    basic.user_experience_rating = 0.35;
    basic.adjustability_rating = 0.20;
    basic.durability_rating = 0.90;
    designs_[SightType::BASIC_NOTCH] = basic;
    name_to_type_[basic.name] = SightType::BASIC_NOTCH;
    name_to_type_["缺口"] = SightType::BASIC_NOTCH;

    SightDesign pin;
    pin.type = SightType::PIN_SIGHT;
    pin.name = "single_pin";
    pin.description = "单针式望山，汉代制式，铜制立针配合刻度槽";
    pin.historical_origin = "居延汉简记载的六石弩标配，张家山汉简《算术书》有刻度校准法";
    pin.sight_height_mm = 45.0;
    pin.sight_radius_mm = 750.0;
    pin.notch_width_mm = 3.0;
    pin.post_diameter_mm = 2.0;
    pin.aperture_diameter_mm = 0.0;
    pin.scale_count = 5;
    pin.scale_range_min_m = 50.0;
    pin.scale_range_max_m = 300.0;
    pin.scale_interval_m = 62.5;
    pin.material_density = 8700.0;
    pin.thermal_expansion_coeff = 17e-6;
    pin.manufacturing_precision_mm = 0.5;
    pin.total_weight_g = 58.0;
    pin.estimated_cost = 15.0;
    pin.user_experience_rating = 0.55;
    pin.adjustability_rating = 0.45;
    pin.durability_rating = 0.80;
    designs_[SightType::PIN_SIGHT] = pin;
    name_to_type_[pin.name] = SightType::PIN_SIGHT;
    name_to_type_["单针"] = SightType::PIN_SIGHT;

    SightDesign ramp;
    ramp.type = SightType::RAMP_SIGHT;
    ramp.name = "ramp_sight";
    ramp.description = "斜坡式望山，唐宋成熟形制，可调滑块配合斜面刻度";
    ramp.historical_origin = "宋代《武经总要》绘图所载神臂弩附件，西夏陵出土实物";
    ramp.sight_height_mm = 55.0;
    ramp.sight_radius_mm = 850.0;
    ramp.notch_width_mm = 2.5;
    ramp.post_diameter_mm = 1.8;
    ramp.aperture_diameter_mm = 0.0;
    ramp.scale_count = 9;
    ramp.scale_range_min_m = 50.0;
    ramp.scale_range_max_m = 450.0;
    ramp.scale_interval_m = 50.0;
    ramp.material_density = 7850.0;
    ramp.thermal_expansion_coeff = 12e-6;
    ramp.manufacturing_precision_mm = 0.3;
    ramp.total_weight_g = 95.0;
    ramp.estimated_cost = 35.0;
    ramp.user_experience_rating = 0.70;
    ramp.adjustability_rating = 0.75;
    ramp.durability_rating = 0.70;
    designs_[SightType::RAMP_SIGHT] = ramp;
    name_to_type_[ramp.name] = SightType::RAMP_SIGHT;
    name_to_type_["斜坡"] = SightType::RAMP_SIGHT;

    SightDesign multi;
    multi.type = SightType::MULTI_PIN;
    multi.name = "multi_pin";
    multi.description = "多针式望山，明代制式，五根立针对应五档射程";
    multi.historical_origin = "明《武备志》所录《天工开物》弩机插图，戚继光蓟镇练兵使用";
    multi.sight_height_mm = 62.0;
    multi.sight_radius_mm = 900.0;
    multi.notch_width_mm = 2.2;
    multi.post_diameter_mm = 1.5;
    multi.aperture_diameter_mm = 0.0;
    multi.scale_count = 5;
    multi.scale_range_min_m = 100.0;
    multi.scale_range_max_m = 500.0;
    multi.scale_interval_m = 100.0;
    multi.material_density = 8700.0;
    multi.thermal_expansion_coeff = 17e-6;
    multi.manufacturing_precision_mm = 0.35;
    multi.total_weight_g = 120.0;
    multi.estimated_cost = 45.0;
    multi.user_experience_rating = 0.75;
    multi.adjustability_rating = 0.80;
    multi.durability_rating = 0.60;
    designs_[SightType::MULTI_PIN] = multi;
    name_to_type_[multi.name] = SightType::MULTI_PIN;
    name_to_type_["多针"] = SightType::MULTI_PIN;

    SightDesign tangent;
    tangent.type = SightType::TANGENT_SIGHT;
    tangent.name = "tangent_bar_sight";
    tangent.description = "立轴切向望山，元代西域传入手艺，刻度精确到分";
    tangent.historical_origin = "伊尔汗国火器工匠流入中原，《元一统志》记载回回砲匠技术";
    tangent.sight_height_mm = 72.0;
    tangent.sight_radius_mm = 1000.0;
    tangent.notch_width_mm = 2.0;
    tangent.post_diameter_mm = 1.2;
    tangent.aperture_diameter_mm = 0.0;
    tangent.scale_count = 14;
    tangent.scale_range_min_m = 50.0;
    tangent.scale_range_max_m = 600.0;
    tangent.scale_interval_m = 42.3;
    tangent.material_density = 7200.0;
    tangent.thermal_expansion_coeff = 19e-6;
    tangent.manufacturing_precision_mm = 0.15;
    tangent.total_weight_g = 155.0;
    tangent.estimated_cost = 80.0;
    tangent.user_experience_rating = 0.85;
    tangent.adjustability_rating = 0.95;
    tangent.durability_rating = 0.50;
    designs_[SightType::TANGENT_SIGHT] = tangent;
    name_to_type_[tangent.name] = SightType::TANGENT_SIGHT;
    name_to_type_["切向"] = SightType::TANGENT_SIGHT;

    SightDesign peep;
    peep.type = SightType::PRECISION_PEEP;
    peep.name = "precision_peep";
    peep.description = "精密觇孔望山，现代复原设计，小孔成像原理提高精度";
    peep.historical_origin = "基于19世纪后膛步枪觇孔技术改良，非历史真实设计";
    peep.sight_height_mm = 58.0;
    peep.sight_radius_mm = 950.0;
    peep.notch_width_mm = 0.0;
    peep.post_diameter_mm = 0.0;
    peep.aperture_diameter_mm = 1.8;
    peep.scale_count = 12;
    peep.scale_range_min_m = 50.0;
    peep.scale_range_max_m = 600.0;
    peep.scale_interval_m = 50.0;
    peep.material_density = 7850.0;
    peep.thermal_expansion_coeff = 12e-6;
    peep.manufacturing_precision_mm = 0.08;
    peep.total_weight_g = 130.0;
    peep.estimated_cost = 120.0;
    peep.user_experience_rating = 0.92;
    peep.adjustability_rating = 0.90;
    peep.durability_rating = 0.55;
    designs_[SightType::PRECISION_PEEP] = peep;
    name_to_type_[peep.name] = SightType::PRECISION_PEEP;
    name_to_type_["觇孔"] = SightType::PRECISION_PEEP;

    LOG_INFO("SightDesignDatabase: initialized {} default sight designs", designs_.size());
}

const SightDesign& SightDesignDatabase::get(SightType type) const {
    auto it = designs_.find(type);
    if (it == designs_.end()) {
        static SightDesign unknown{};
        unknown.type = SightType::CUSTOM;
        unknown.name = "unknown";
        return unknown;
    }
    return it->second;
}

const SightDesign& SightDesignDatabase::get(const std::string& name) const {
    auto it = name_to_type_.find(name);
    if (it == name_to_type_.end()) {
        return get(SightType::CUSTOM);
    }
    return get(it->second);
}

std::vector<SightDesign> SightDesignDatabase::list_all() const {
    std::vector<SightDesign> result;
    result.reserve(designs_.size());
    for (const auto& [type, design] : designs_) {
        result.push_back(design);
    }
    return result;
}

OpticalErrorSimulator::OpticalErrorSimulator(uint64_t seed)
    : rng_(seed == 0 ? std::random_device{}() : seed)
    , ambient_light_lux_(10000.0)
    , ambient_temperature_c_(20.0)
    , ambient_humidity_(0.5)
    , wind_speed_mps_(2.0)
    , user_visual_acuity_(1.0)
    , user_experience_(0.6)
    , current_distance_m_(200.0)
{
}

void OpticalErrorSimulator::set_ambient_conditions(
    double light_lux, double temperature_c,
    double humidity, double wind_speed_mps
) {
    ambient_light_lux_ = std::max(1.0, light_lux);
    ambient_temperature_c_ = temperature_c;
    ambient_humidity_ = std::clamp(humidity, 0.0, 1.0);
    wind_speed_mps_ = std::max(0.0, wind_speed_mps);
}

void OpticalErrorSimulator::set_user_profile(
    double visual_acuity_arcmin,
    double experience_factor
) {
    user_visual_acuity_ = std::max(0.25, visual_acuity_arcmin);
    user_experience_ = std::clamp(experience_factor, 0.0, 1.0);
}

void OpticalErrorSimulator::set_distance(double target_distance_m) {
    current_distance_m_ = std::max(1.0, target_distance_m);
}

OpticalErrorBudget OpticalErrorSimulator::get_current_error_budget() const {
    OpticalErrorBudget budget;
    budget.visual_acuity_arcmin = user_visual_acuity_;
    budget.parallax_error_mrad = 0.1;
    budget.parallax_distance_m = current_distance_m_;
    budget.ambient_light_lux = ambient_light_lux_;
    budget.glare_intensity_factor = std::clamp(ambient_light_lux_ / 100000.0, 0.0, 1.0);
    budget.contrast_ratio = 10.0 + 90.0 * std::tanh(ambient_light_lux_ / 5000.0);
    budget.atmospheric_refraction_mrad = calculate_atmospheric_refraction(current_distance_m_, ambient_temperature_c_);
    budget.heat_haze_mrad = calculate_heat_haze_error(current_distance_m_, ambient_temperature_c_);
    budget.turbulence_strength = wind_speed_mps_ * 0.05;
    budget.eye_relaxation_factor = 0.7 + 0.3 * user_experience_;
    budget.aiming_time_seconds = 3.0 - 1.5 * user_experience_;
    budget.user_experience_factor = user_experience_;
    budget.mechanical_play_mm = 0.1;
    budget.thermal_distortion_mrad = 0.05;
    budget.vibration_misalignment_mrad = 0.03 + wind_speed_mps_ * 0.02;
    return budget;
}

double OpticalErrorSimulator::calculate_visual_acuity_error(double distance_m) const {
    double acuity_rad = user_visual_acuity_ * ARCMIN_TO_RAD;
    double distance_factor = 1.0 + 0.002 * std::max(0.0, distance_m - 100.0);
    double light_factor = 1.0 + 0.5 * std::exp(-ambient_light_lux_ / 1000.0);
    double experience_bonus = 1.0 - 0.3 * user_experience_;
    return acuity_rad * distance_factor * light_factor * experience_bonus * 1000.0;
}

double OpticalErrorSimulator::calculate_parallax_error(
    double sight_radius_mm,
    double parallax_distance_m,
    double eye_offset_mm
) const {
    if (sight_radius_mm <= 0.0 || parallax_distance_m <= 0.0) return 0.0;
    double target_distance_m = current_distance_m_;
    double ratio = std::abs(target_distance_m - parallax_distance_m) /
                   std::max(1.0, target_distance_m + parallax_distance_m);
    double angle_rad = std::atan(eye_offset_mm / (sight_radius_mm + 1e-6));
    return angle_rad * ratio * 1000.0 * 2.0;
}

double OpticalErrorSimulator::calculate_atmospheric_refraction(
    double distance_m, double temperature_c, double pressure_hpa
) const {
    if (distance_m <= 0.0) return 0.0;
    double T = temperature_c + 273.15;
    double n_minus_1 = 7.86e-7 * (pressure_hpa / T);
    double height_per_km = 4000.0;
    double ray_bending_mrad_per_km = n_minus_1 * 1e6 * 150.0 / height_per_km;
    double distance_km = distance_m / 1000.0;
    return ray_bending_mrad_per_km * distance_km * 0.15;
}

double OpticalErrorSimulator::calculate_heat_haze_error(
    double distance_m,
    double temperature_c,
    double ground_temp_c
) const {
    double delta_t = ground_temp_c - temperature_c;
    if (delta_t <= 0.0 || distance_m <= 0.0) return 0.0;
    double gradient_strength = delta_t / std::max(1.0, distance_m / 100.0);
    double distance_factor = 1.0 - std::exp(-distance_m / 400.0);
    return gradient_strength * 0.08 * distance_factor;
}

double OpticalErrorSimulator::calculate_mechanical_play_error(
    const SightDesign& sight
) const {
    if (sight.sight_radius_mm <= 0.0) return 0.0;
    double total_play = sight.manufacturing_precision_mm * 1.5;
    return std::atan(total_play / (sight.sight_radius_mm + 1e-6)) * 1000.0;
}

double OpticalErrorSimulator::calculate_thermal_distortion(
    const SightDesign& sight,
    double delta_temperature_c
) const {
    double expansion_mm = sight.sight_height_mm * sight.thermal_expansion_coeff * delta_temperature_c;
    if (sight.sight_radius_mm <= 0.0) return 0.0;
    return std::atan(expansion_mm / (sight.sight_radius_mm + 1e-6)) * 1000.0;
}

std::pair<double, double> OpticalErrorSimulator::calculate_glare_effect(
    double light_lux,
    double sun_angle_deg
) const {
    double glare_factor = std::clamp(light_lux / 80000.0, 0.0, 1.0);
    double angle_factor = std::exp(-std::pow((sun_angle_deg - 15.0) / 30.0, 2.0));
    double glare_mrad = glare_factor * angle_factor * 0.8;
    double contrast_reduction = glare_factor * 0.35;
    return {glare_mrad, contrast_reduction};
}

double OpticalErrorSimulator::sample_normal(double mean, double stddev) const {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(rng_);
}

double OpticalErrorSimulator::sample_cauchy(double scale) const {
    std::uniform_real_distribution<double> dist(-M_PI / 2.0 + 1e-4, M_PI / 2.0 - 1e-4);
    return scale * std::tan(dist(rng_));
}

SightAimingResult OpticalErrorSimulator::simulate_single_aim(
    const SightDesign& sight,
    double nominal_elevation_deg,
    double nominal_azimuth_deg,
    double target_distance_m
) {
    SightAimingResult result;
    result.raw_aim_angle_deg = nominal_elevation_deg;
    auto now = std::chrono::system_clock::now();
    result.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    current_distance_m_ = target_distance_m;

    double va_error = calculate_visual_acuity_error(target_distance_m) / 2.0;
    double parallax_error = calculate_parallax_error(
        sight.sight_radius_mm, target_distance_m, 3.0) / 2.0;
    auto [glare, contrast_loss] = calculate_glare_effect(ambient_light_lux_);

    double va_elev = sample_normal(0.0, va_error);
    double va_azim = sample_normal(0.0, va_error * 0.9);
    double para_elev = sample_cauchy(parallax_error * 0.5);
    double para_azim = sample_cauchy(parallax_error * 0.4);

    result.optical_error_elevation_mrad = va_elev + para_elev;
    result.optical_error_azimuth_mrad = va_azim + para_azim + glare * sample_normal(0.0, 1.0);

    double mech_error = calculate_mechanical_play_error(sight) / 2.0;
    double therm_error = calculate_thermal_distortion(sight, ambient_temperature_c_ - 20.0);
    result.mechanical_error_elevation_mrad = sample_normal(0.0, mech_error) + therm_error * 0.3;
    result.mechanical_error_azimuth_mrad = sample_normal(0.0, mech_error * 0.8);

    double heat_haze = calculate_heat_haze_error(target_distance_m, ambient_temperature_c_);
    double refraction = calculate_atmospheric_refraction(target_distance_m, ambient_temperature_c_);
    double turbulence = wind_speed_mps_ * 0.03;
    result.environmental_error_elevation_mrad =
        sample_normal(-refraction * 0.5, heat_haze * 0.4) + sample_normal(0.0, turbulence);
    result.environmental_error_azimuth_mrad =
        sample_normal(0.0, heat_haze * 0.5) + sample_normal(0.0, turbulence * 1.3);

    result.total_error_elevation_mrad =
        result.optical_error_elevation_mrad +
        result.mechanical_error_elevation_mrad +
        result.environmental_error_elevation_mrad;
    result.total_error_azimuth_mrad =
        result.optical_error_azimuth_mrad +
        result.mechanical_error_azimuth_mrad +
        result.environmental_error_azimuth_mrad;

    result.corrected_elevation_deg = nominal_elevation_deg +
                                      result.total_error_elevation_mrad * MRAD_TO_DEG;
    result.corrected_azimuth_deg = nominal_azimuth_deg +
                                    result.total_error_azimuth_mrad * MRAD_TO_DEG;

    result.predicted_impact_offset_x_m =
        std::tan(result.total_error_azimuth_mrad / 1000.0) * target_distance_m;
    result.predicted_impact_offset_y_m =
        std::tan(result.total_error_elevation_mrad / 1000.0) * target_distance_m;

    double total_error_magnitude_mrad = std::sqrt(
        result.total_error_elevation_mrad * result.total_error_elevation_mrad +
        result.total_error_azimuth_mrad * result.total_error_azimuth_mrad
    );
    double max_expected_error = va_error * 5.0 + mech_error * 3.0 + heat_haze * 3.0 + 0.5;
    result.aiming_confidence = std::max(0.0, 1.0 - total_error_magnitude_mrad /
                                       std::max(0.1, max_expected_error));
    result.aiming_confidence *= (0.5 + 0.5 * user_experience_);

    return result;
}

std::vector<SightAimingResult> OpticalErrorSimulator::simulate_multiple_aims(
    const SightDesign& sight,
    double nominal_elevation_deg,
    double nominal_azimuth_deg,
    double target_distance_m,
    int num_samples
) {
    std::vector<SightAimingResult> results;
    results.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        results.push_back(simulate_single_aim(
            sight, nominal_elevation_deg, nominal_azimuth_deg, target_distance_m
        ));
    }
    return results;
}

double OpticalErrorSimulator::estimate_sight_cep(
    const SightDesign& sight,
    double target_distance_m,
    int num_samples
) {
    auto results = simulate_multiple_aims(sight, 15.0, 0.0, target_distance_m, num_samples);
    double sum_x = 0.0, sum_y = 0.0;
    double sum_x2 = 0.0, sum_y2 = 0.0;
    for (const auto& r : results) {
        sum_x += r.predicted_impact_offset_x_m;
        sum_y += r.predicted_impact_offset_y_m;
        sum_x2 += r.predicted_impact_offset_x_m * r.predicted_impact_offset_x_m;
        sum_y2 += r.predicted_impact_offset_y_m * r.predicted_impact_offset_y_m;
    }
    double n = static_cast<double>(num_samples);
    double var_x = (sum_x2 - sum_x * sum_x / n) / std::max(1.0, n - 1);
    double var_y = (sum_y2 - sum_y * sum_y / n) / std::max(1.0, n - 1);
    double sigma = std::sqrt(var_x + var_y);
    return sigma * 1.1774;
}

SightScaleCalibrator::SightScaleCalibrator() = default;

void SightScaleCalibrator::set_crossbow(const CrossbowType& crossbow) {
    crossbow_ = crossbow;
    ballistic_angles_.clear();
    ballistic_ranges_.clear();
}

void SightScaleCalibrator::set_sight_design(const SightDesign& sight) {
    sight_ = sight;
}

double SightScaleCalibrator::compute_required_elevation(double target_range_m) {
    DynamicsModel model(crossbow_);
    double arrow_mass_kg = crossbow_.arrow_mass;
    double draw_energy_j = 0.5 * crossbow_.draw_weight *
                            0.75 * crossbow_.bow_length * 2.0 / 3.0;
    double v0 = std::sqrt(2.0 * draw_energy_j * 0.78 / arrow_mass_kg);

    double best_angle = 15.0 * DEG_TO_RAD;
    double best_error = 1e9;
    double lo = 0.0 * DEG_TO_RAD;
    double hi = 45.0 * DEG_TO_RAD;

    for (int iter = 0; iter < 100; ++iter) {
        double mid = (lo + hi) / 2.0;
        auto traj = model.simulate_trajectory(v0, mid, 0, 0, 0.001);
        if (traj.empty()) break;
        double range = traj.back().position.x;
        double error = range - target_range_m;

        if (std::abs(error) < best_error) {
            best_error = std::abs(error);
            best_angle = mid;
        }
        if (error > 0) hi = mid;
        else lo = mid;
    }

    ballistic_ranges_.push_back(target_range_m);
    ballistic_angles_.push_back(best_angle);
    return best_angle;
}

double SightScaleCalibrator::sight_height_to_angle(
    double sight_height_mm,
    double sight_radius_mm
) {
    if (sight_radius_mm <= 0.0) return 0.0;
    return std::atan(sight_height_mm / sight_radius_mm) * RAD_TO_DEG;
}

double SightScaleCalibrator::angle_to_sight_height(
    double angle_deg,
    double sight_radius_mm
) {
    return sight_radius_mm * std::tan(angle_deg * DEG_TO_RAD);
}

SightScale SightScaleCalibrator::calibrate_single_scale(double target_range_m) {
    SightScale scale;
    scale.target_range_m = target_range_m;
    double elevation_rad = compute_required_elevation(target_range_m);
    scale.calibrated_angle_deg = elevation_rad * RAD_TO_DEG;
    scale.actual_angle_deg = scale.calibrated_angle_deg;
    scale.calibration_error_mrad = 0.0;
    scale.scale_height_mm = angle_to_sight_height(scale.calibrated_angle_deg, sight_.sight_radius_mm);
    scale.index = static_cast<int>(ballistic_ranges_.size()) - 1;
    return scale;
}

std::vector<SightScale> SightScaleCalibrator::calibrate_scales(
    double start_range_m, double end_range_m, double step_m
) {
    std::vector<SightScale> scales;
    if (step_m <= 0.0) return scales;

    int steps = static_cast<int>(std::ceil((end_range_m - start_range_m) / step_m)) + 1;
    steps = std::min(steps, sight_.scale_count);
    scales.reserve(steps);

    double actual_step = (end_range_m - start_range_m) / std::max(1, steps - 1);
    for (int i = 0; i < steps; ++i) {
        double range = start_range_m + i * actual_step;
        auto scale = calibrate_single_scale(range);
        scale.index = i;
        if (i > 0) {
            double manufacturing_error = (std::rand() % 100 - 50) / 100.0 *
                                        sight_.manufacturing_precision_mm;
            scale.scale_height_mm += manufacturing_error;
            double actual_angle = sight_height_to_angle(
                scale.scale_height_mm, sight_.sight_radius_mm);
            scale.calibration_error_mrad =
                (actual_angle - scale.calibrated_angle_deg) * DEG_TO_MRAD;
        }
        scales.push_back(scale);
    }
    return scales;
}

double SightScaleCalibrator::estimate_range_from_sight_height(
    double sight_height_mm,
    const std::vector<SightScale>& calibrated_scales
) {
    if (calibrated_scales.size() < 2) return -1.0;

    double angle = sight_height_to_angle(sight_height_mm, sight_.sight_radius_mm);
    for (size_t i = 1; i < calibrated_scales.size(); ++i) {
        const auto& prev = calibrated_scales[i-1];
        const auto& curr = calibrated_scales[i];
        if ((angle >= prev.calibrated_angle_deg && angle <= curr.calibrated_angle_deg) ||
            (angle <= prev.calibrated_angle_deg && angle >= curr.calibrated_angle_deg)) {
            double t = (angle - prev.calibrated_angle_deg) /
                       std::max(1e-6, curr.calibrated_angle_deg - prev.calibrated_angle_deg);
            return prev.target_range_m + t * (curr.target_range_m - prev.target_range_m);
        }
    }

    if (angle < calibrated_scales.front().calibrated_angle_deg) {
        return calibrated_scales.front().target_range_m * 0.5;
    }
    return calibrated_scales.back().target_range_m * 1.2;
}

std::vector<std::pair<double, double>> SightScaleCalibrator::generate_ballistic_table(
    double start_range_m, double end_range_m, double step_m
) {
    std::vector<std::pair<double, double>> table;
    if (step_m <= 0.0) return table;
    int steps = static_cast<int>(std::ceil((end_range_m - start_range_m) / step_m)) + 1;
    table.reserve(steps);
    for (int i = 0; i < steps; ++i) {
        double range = start_range_m + i * step_m;
        double angle_rad = compute_required_elevation(range);
        table.emplace_back(range, angle_rad * RAD_TO_DEG);
    }
    return table;
}

SightOptimizer::SightOptimizer()
    : target_range_m_(300.0)
    , max_height_mm_(80.0)
    , max_weight_g_(200.0)
    , max_cost_(100.0)
    , weight_accuracy_(0.45)
    , weight_usability_(0.25)
    , weight_manufacturing_(0.15)
    , weight_historical_(0.15)
    , error_sim_(42)
{
}

void SightOptimizer::set_crossbow(const CrossbowType& crossbow) {
    crossbow_ = crossbow;
}

void SightOptimizer::set_target_range(double distance_m) {
    target_range_m_ = std::max(1.0, distance_m);
}

void SightOptimizer::set_constraints(
    double max_height_mm, double max_weight_g, double max_cost
) {
    max_height_mm_ = max_height_mm;
    max_weight_g_ = max_weight_g;
    max_cost_ = max_cost;
}

void SightOptimizer::set_objective_weights(
    double accuracy_w, double usability_w,
    double manufacturing_w, double historical_w
) {
    double total = accuracy_w + usability_w + manufacturing_w + historical_w;
    if (total <= 0.0) total = 1.0;
    weight_accuracy_ = accuracy_w / total;
    weight_usability_ = usability_w / total;
    weight_manufacturing_ = manufacturing_w / total;
    weight_historical_ = historical_w / total;
}

double SightOptimizer::calculate_accuracy_score(const SightDesign& design) {
    double cep_m = error_sim_.estimate_sight_cep(design, target_range_m_, 300);
    double target_cep_ratio = cep_m / std::max(1.0, target_range_m_ * 0.01);
    double cep_score = std::exp(-target_cep_ratio * 0.8);

    double height_stability = 1.0 / (1.0 + std::pow(design.sight_height_mm / 100.0, 2.0));
    double radius_benefit = 1.0 - std::exp(-design.sight_radius_mm / 800.0);
    double precision_benefit = 1.0 - std::exp(-0.1 / std::max(0.01, design.manufacturing_precision_mm));

    return (cep_score * 0.5 + height_stability * 0.15 +
            radius_benefit * 0.2 + precision_benefit * 0.15);
}

double SightOptimizer::calculate_usability_score(const SightDesign& design) {
    double scale_benefit = 1.0 - std::exp(-design.scale_count / 6.0);
    double interval_benefit = std::exp(-std::pow(
        (design.scale_interval_m - 50.0) / 50.0, 2.0));
    double height_access = 1.0 - std::pow(
        std::clamp((design.sight_height_mm - 30.0) / 100.0, 0.0, 1.0), 0.5);
    double weight_penalty = 1.0 / (1.0 + design.total_weight_g / 300.0);

    return (design.user_experience_rating * 0.25 +
            scale_benefit * 0.25 +
            interval_benefit * 0.2 +
            height_access * 0.15 +
            weight_penalty * 0.15);
}

double SightOptimizer::calculate_manufacturing_score(const SightDesign& design) {
    double precision_cost = 1.0 - std::exp(-0.1 / std::max(0.02, design.manufacturing_precision_mm));
    double cost_score = 1.0 - std::clamp(design.estimated_cost / max_cost_, 0.0, 1.0);
    double weight_score = 1.0 - std::clamp(design.total_weight_g / max_weight_g_, 0.0, 1.0);
    double durability_score = design.durability_rating;

    return (precision_cost * 0.25 +
            cost_score * 0.35 +
            weight_score * 0.2 +
            durability_score * 0.2);
}

double SightOptimizer::calculate_historical_score(const SightDesign& design) {
    if (design.type == SightType::CUSTOM) return 0.3;
    if (design.type == SightType::PRECISION_PEEP) return 0.2;
    if (design.type == SightType::TANGENT_SIGHT) return 0.5;
    if (design.type == SightType::BASIC_NOTCH) return 0.95;
    if (design.type == SightType::PIN_SIGHT) return 0.9;
    if (design.type == SightType::RAMP_SIGHT) return 0.8;
    if (design.type == SightType::MULTI_PIN) return 0.7;
    return 0.5;
}

double SightOptimizer::evaluate_design(const SightDesign& design) {
    double acc = calculate_accuracy_score(design);
    double usa = calculate_usability_score(design);
    double mfg = calculate_manufacturing_score(design);
    double his = calculate_historical_score(design);

    double penalty = 0.0;
    if (design.sight_height_mm > max_height_mm_) penalty += 0.5;
    if (design.total_weight_g > max_weight_g_) penalty += 0.3;
    if (design.estimated_cost > max_cost_) penalty += 0.2;

    return (acc * weight_accuracy_ +
            usa * weight_usability_ +
            mfg * weight_manufacturing_ +
            his * weight_historical_) * (1.0 - penalty);
}

SightOptimizationResult SightOptimizer::optimize(SightType base_type) {
    return grid_search_optimize(base_type, 16, 12, 8);
}

SightOptimizationResult SightOptimizer::grid_search_optimize(
    SightType base_type, int height_samples, int radius_samples, int scale_samples
) {
    auto& db = SightDesignDatabase::instance();
    SightDesign base = db.get(base_type);
    if (base.type == SightType::CUSTOM) {
        base = db.get(SightType::RAMP_SIGHT);
    }

    SightDesign best_design = base;
    double best_score = evaluate_design(base);
    double best_cep = error_sim_.estimate_sight_cep(base, target_range_m_, 200);

    double h_min = std::max(15.0, base.sight_height_mm * 0.5);
    double h_max = std::min(max_height_mm_, base.sight_height_mm * 1.6);
    double r_min = std::max(400.0, base.sight_radius_mm * 0.7);
    double r_max = base.sight_radius_mm * 1.3;
    int sc_min = std::max(3, base.scale_count - 3);
    int sc_max = std::min(16, base.scale_count + 6);

    for (int hi = 0; hi < height_samples; ++hi) {
        double h = h_min + (h_max - h_min) * hi / std::max(1, height_samples - 1);
        for (int ri = 0; ri < radius_samples; ++ri) {
            double r = r_min + (r_max - r_min) * ri / std::max(1, radius_samples - 1);
            for (int si = 0; si < scale_samples; ++si) {
                int sc = sc_min + (sc_max - sc_min) * si / std::max(1, scale_samples - 1);

                SightDesign candidate = base;
                candidate.sight_height_mm = h;
                candidate.sight_radius_mm = r;
                candidate.scale_count = sc;
                candidate.scale_interval_m =
                    (candidate.scale_range_max_m - candidate.scale_range_min_m) /
                    std::max(1, sc - 1);
                candidate.total_weight_g = base.total_weight_g *
                    (h / base.sight_height_mm) * 0.6 +
                    (r / base.sight_radius_mm) * 0.4;
                candidate.manufacturing_precision_mm =
                    std::max(0.05, base.manufacturing_precision_mm *
                        (1.0 + 0.05 * (sc - base.scale_count)));

                double score = evaluate_design(candidate);
                if (score > best_score) {
                    best_score = score;
                    best_design = candidate;
                    best_cep = error_sim_.estimate_sight_cep(candidate, target_range_m_, 200);
                }
            }
        }
    }

    SightOptimizationResult result;
    result.optimal_design = best_design;
    result.overall_score = best_score;
    result.accuracy_score = calculate_accuracy_score(best_design);
    result.usability_score = calculate_usability_score(best_design);
    result.manufacturing_score = calculate_manufacturing_score(best_design);
    result.historical_authenticity_score = calculate_historical_score(best_design);
    result.average_total_error_mrad = best_cep / std::max(1.0, target_range_m_) * 1000.0;
    result.ceph_improvement_percent =
        (best_cep - error_sim_.estimate_sight_cep(base, target_range_m_, 200)) /
        std::max(0.01, error_sim_.estimate_sight_cep(base, target_range_m_, 200)) * -100.0;
    result.optimal_height_mm = best_design.sight_height_mm;
    result.optimal_radius_mm = best_design.sight_radius_mm;
    result.optimal_scale_interval_m = best_design.scale_interval_m;

    result.design_tradeoffs.emplace_back("精度提升vs历史真实度",
        result.accuracy_score - calculate_accuracy_score(base));
    result.design_tradeoffs.emplace_back("制造成本变化",
        (best_design.estimated_cost - base.estimated_cost) / std::max(0.1, base.estimated_cost));
    result.design_tradeoffs.emplace_back("重量变化%",
        (best_design.total_weight_g - base.total_weight_g) / std::max(0.1, base.total_weight_g) * 100.0);

    LOG_INFO("Sight optimization complete: type={}, score={:.4f}, CEP={:.3f}m @ {:.0f}m",
             static_cast<int>(base_type), best_score, best_cep, target_range_m_);
    return result;
}

std::vector<std::tuple<double, double, double>> SightOptimizer::explore_parameter_space(
    SightType base_type, int samples_per_dim
) {
    std::vector<std::tuple<double, double, double>> space;
    if (samples_per_dim < 2) return space;
    space.reserve(samples_per_dim * samples_per_dim);

    auto& db = SightDesignDatabase::instance();
    SightDesign base = db.get(base_type);
    if (base.type == SightType::CUSTOM) base = db.get(SightType::RAMP_SIGHT);

    for (int hi = 0; hi < samples_per_dim; ++hi) {
        double h = 15.0 + (max_height_mm_ - 15.0) * hi / (samples_per_dim - 1);
        for (int ri = 0; ri < samples_per_dim; ++ri) {
            double r = 400.0 + 800.0 * ri / (samples_per_dim - 1);
            SightDesign cand = base;
            cand.sight_height_mm = h;
            cand.sight_radius_mm = r;
            double score = evaluate_design(cand);
            space.emplace_back(h, r, score);
        }
    }
    return space;
}

} // namespace crossbow
