#include "bowstring_material.h"
#include "dynamics_model.h"
#include "logger.h"
#include <algorithm>
#include <random>
#include <numeric>

namespace crossbow {

double BowstringMaterial::calculate_string_mass(double length_m) const {
    double radius_m = (thickness_mm / 2.0) * 0.001;
    double cross_section_m2 = M_PI * radius_m * radius_m * strand_count;
    return density * length_m * cross_section_m2;
}

double BowstringMaterial::calculate_stiffness(
    double length_m,
    double cross_section_m2
) const {
    if (length_m <= 0.0) return 0.0;
    return (young_modulus * cross_section_m2) / length_m;
}

double BowstringMaterial::calculate_wave_speed(
    double tension_N,
    double linear_density
) const {
    if (linear_density <= 0.0) return 0.0;
    return std::sqrt(tension_N / linear_density);
}

double BowstringMaterial::calculate_damping_force(
    double velocity,
    double length_m
) const {
    double base_damping = damping_coefficient * std::abs(velocity);
    double length_factor = 1.0 + 0.3 * (length_m - 1.0);
    double velocity_exponent = 1.15 + 0.1 * std::min(1.0, std::abs(velocity) / 100.0);
    return base_damping * length_factor * std::pow(std::abs(velocity) + 1e-6, velocity_exponent - 1.0);
}

double BowstringMaterial::calculate_efficiency_at_pull(double pull_ratio) const {
    double r = std::clamp(pull_ratio, 0.0, 1.0);
    double baseline = energy_efficiency;
    double peak_shift = 0.7;
    double width = 0.5;
    double gaussian_envelope = std::exp(-std::pow((r - peak_shift) / width, 2.0));
    double min_efficiency = baseline * 0.75;
    double efficiency_variation = baseline - min_efficiency;
    return min_efficiency + efficiency_variation * (0.3 + 0.7 * gaussian_envelope);
}

double BowstringMaterial::calculate_creep_factor(
    double hours_under_load,
    double temperature_c
) const {
    double temp_factor = 1.0 + temperature_coefficient * std::max(0.0, temperature_c - 20.0);
    double creep_rate = elongation_at_break * 0.0001 * temp_factor;
    double time_factor = 1.0 - std::exp(-hours_under_load / 1000.0);
    return 1.0 + creep_rate * time_factor * 100.0;
}

double BowstringMaterial::estimate_lifespan(
    int cycles_per_day,
    double avg_humidity
) const {
    double humidity_factor = 1.0 / (1.0 + moisture_sensitivity * std::max(0.0, avg_humidity - 0.4));
    double cycle_factor = std::pow(1000.0 / (static_cast<double>(cycles_per_day) + 1), 0.7);
    double base_lifespan_days = fatigue_life_cycles / (static_cast<double>(cycles_per_day) * 365.0 + 1);
    return base_lifespan_days * humidity_factor * cycle_factor * 365.0;
}

BowstringMaterialDatabase::BowstringMaterialDatabase() {
    initialize_defaults();
}

BowstringMaterialDatabase& BowstringMaterialDatabase::instance() {
    static BowstringMaterialDatabase db;
    return db;
}

void BowstringMaterialDatabase::initialize_defaults() {
    BowstringMaterial sinew;
    sinew.type = BowstringMaterialType::SINEW;
    sinew.name = "animal_sinew";
    sinew.description = "动物筋腱弓弦，古代复合弓标配，弹性极佳、储能高";
    sinew.historical_origin = "欧亚草原，斯基泰人、匈奴人广泛使用，汉代传入中原";
    sinew.young_modulus = 1.8e9;
    sinew.tensile_strength = 120e6;
    sinew.density = 1150.0;
    sinew.elongation_at_break = 0.22;
    sinew.damping_coefficient = 1.2;
    sinew.energy_efficiency = 0.88;
    sinew.fatigue_life_cycles = 80000.0;
    sinew.moisture_sensitivity = 0.65;
    sinew.temperature_coefficient = 0.008;
    sinew.thickness_mm = 1.8;
    sinew.strand_count = 14;
    materials_[BowstringMaterialType::SINEW] = sinew;
    name_to_type_[sinew.name] = BowstringMaterialType::SINEW;
    name_to_type_["筋"] = BowstringMaterialType::SINEW;
    name_to_type_["sinew"] = BowstringMaterialType::SINEW;

    BowstringMaterial horn;
    horn.type = BowstringMaterialType::HORN;
    horn.name = "animal_horn_sinew";
    horn.description = "角筋复合弓弦，水牛角片+筋腱压制，强度高但偏脆";
    horn.historical_origin = "中国汉代角弓技术，唐代达到巅峰，《考工记》载六材之制";
    horn.young_modulus = 7.5e9;
    horn.tensile_strength = 210e6;
    horn.density = 1350.0;
    horn.elongation_at_break = 0.09;
    horn.damping_coefficient = 3.8;
    horn.energy_efficiency = 0.91;
    horn.fatigue_life_cycles = 45000.0;
    horn.moisture_sensitivity = 0.45;
    horn.temperature_coefficient = 0.004;
    horn.thickness_mm = 2.2;
    horn.strand_count = 10;
    materials_[BowstringMaterialType::HORN] = horn;
    name_to_type_[horn.name] = BowstringMaterialType::HORN;
    name_to_type_["角"] = BowstringMaterialType::HORN;
    name_to_type_["horn"] = BowstringMaterialType::HORN;

    BowstringMaterial silk;
    silk.type = BowstringMaterialType::SILK;
    silk.name = "silk_thread";
    silk.description = "蚕丝弓弦，质地轻盈、一致性好，适合高精度射击但成本高";
    silk.historical_origin = "中国古代精锐部队配置，宋代神臂弩手标配蚕丝弦";
    silk.young_modulus = 10.5e9;
    silk.tensile_strength = 420e6;
    silk.density = 1380.0;
    silk.elongation_at_break = 0.17;
    silk.damping_coefficient = 2.1;
    silk.energy_efficiency = 0.93;
    silk.fatigue_life_cycles = 25000.0;
    silk.moisture_sensitivity = 0.30;
    silk.temperature_coefficient = 0.002;
    silk.thickness_mm = 1.2;
    silk.strand_count = 32;
    materials_[BowstringMaterialType::SILK] = silk;
    name_to_type_[silk.name] = BowstringMaterialType::SILK;
    name_to_type_["丝"] = BowstringMaterialType::SILK;
    name_to_type_["silk"] = BowstringMaterialType::SILK;

    BowstringMaterial hemp;
    hemp.type = BowstringMaterialType::HEMP;
    hemp.name = "hemp_fiber";
    hemp.description = "麻纤维弓弦，成本低廉、耐候性好，民兵及训练通用耗材";
    hemp.historical_origin = "古代各国最普及的弓弦材料，古埃及、古希腊、古中国均有使用";
    hemp.young_modulus = 35e9;
    hemp.tensile_strength = 180e6;
    hemp.density = 1500.0;
    hemp.elongation_at_break = 0.035;
    hemp.damping_coefficient = 5.5;
    hemp.energy_efficiency = 0.76;
    hemp.fatigue_life_cycles = 120000.0;
    hemp.moisture_sensitivity = 0.25;
    hemp.temperature_coefficient = 0.0015;
    hemp.thickness_mm = 3.5;
    hemp.strand_count = 24;
    materials_[BowstringMaterialType::HEMP] = hemp;
    name_to_type_[hemp.name] = BowstringMaterialType::HEMP;
    name_to_type_["麻"] = BowstringMaterialType::HEMP;
    name_to_type_["hemp"] = BowstringMaterialType::HEMP;

    LOG_INFO("BowstringMaterialDatabase: initialized {} default materials", materials_.size());
}

const BowstringMaterial& BowstringMaterialDatabase::get(BowstringMaterialType type) const {
    auto it = materials_.find(type);
    if (it == materials_.end()) {
        static BowstringMaterial unknown{};
        unknown.type = BowstringMaterialType::UNKNOWN;
        unknown.name = "unknown";
        return unknown;
    }
    return it->second;
}

const BowstringMaterial& BowstringMaterialDatabase::get(const std::string& name) const {
    auto it = name_to_type_.find(name);
    if (it == name_to_type_.end()) {
        return get(BowstringMaterialType::UNKNOWN);
    }
    return get(it->second);
}

std::vector<BowstringMaterial> BowstringMaterialDatabase::list_all() const {
    std::vector<BowstringMaterial> result;
    result.reserve(materials_.size());
    for (const auto& [type, mat] : materials_) {
        result.push_back(mat);
    }
    return result;
}

BowstringMaterial BowstringMaterialDatabase::create_custom(
    const std::string& name,
    double young_modulus,
    double tensile_strength,
    double density,
    double elongation,
    double damping,
    double efficiency
) {
    BowstringMaterial custom;
    custom.type = BowstringMaterialType::CUSTOM;
    custom.name = name;
    custom.description = "Custom user-defined bowstring material";
    custom.historical_origin = "User-defined";
    custom.young_modulus = young_modulus;
    custom.tensile_strength = tensile_strength;
    custom.density = density;
    custom.elongation_at_break = elongation;
    custom.damping_coefficient = damping;
    custom.energy_efficiency = efficiency;
    custom.fatigue_life_cycles = 50000.0;
    custom.moisture_sensitivity = 0.4;
    custom.temperature_coefficient = 0.005;
    custom.thickness_mm = 2.0;
    custom.strand_count = 16;
    return custom;
}

BowstringPerformanceAnalyzer::BowstringPerformanceAnalyzer(const CrossbowType& crossbow)
    : crossbow_(crossbow)
    , draw_distance_m_(0.75 * crossbow.bow_length)
    , arrow_mass_kg_(crossbow.arrow_mass)
    , string_length_m_(2.0 * crossbow.string_length)
    , strand_cross_section_m2_(0.0)
{
    update_derived_params();
}

void BowstringPerformanceAnalyzer::set_crossbow(const CrossbowType& crossbow) {
    crossbow_ = crossbow;
    draw_distance_m_ = 0.75 * crossbow.bow_length;
    arrow_mass_kg_ = crossbow.arrow_mass;
    string_length_m_ = 2.0 * crossbow.string_length;
    update_derived_params();
}

void BowstringPerformanceAnalyzer::set_draw_distance(double draw_distance_m) {
    draw_distance_m_ = std::max(0.1, draw_distance_m);
}

void BowstringPerformanceAnalyzer::set_arrow_mass(double arrow_mass_g) {
    arrow_mass_kg_ = std::max(0.005, arrow_mass_g * 0.001);
}

void BowstringPerformanceAnalyzer::update_derived_params() {
    strand_cross_section_m2_ = 0.0;
}

double BowstringPerformanceAnalyzer::calculate_max_possible_tension(
    const BowstringMaterial& mat
) const {
    double strand_radius_m = (mat.thickness_mm * 0.001) / 2.0;
    double per_strand_area = M_PI * strand_radius_m * strand_radius_m;
    return mat.tensile_strength * per_strand_area * mat.strand_count * 0.8;
}

double BowstringPerformanceAnalyzer::estimate_velocity_from_energy(
    const BowstringMaterial& mat,
    double draw_energy_j,
    double arrow_mass_kg
) {
    double string_mass = mat.calculate_string_mass(string_length_m_);
    double effective_mass = arrow_mass_kg + string_mass * 0.25;
    double efficiency = mat.calculate_efficiency_at_pull(1.0);
    double usable_energy = draw_energy_j * efficiency;
    return std::sqrt(2.0 * usable_energy / effective_mass);
}

BowstringShotResult BowstringPerformanceAnalyzer::simulate_shot(
    const BowstringMaterial& material,
    double launch_angle_deg,
    double initial_tension_N
) {
    BowstringShotResult result{};
    result.material_type = material.type;

    double draw_weight_N = crossbow_.draw_weight;
    double draw_energy_j = 0.5 * draw_weight_N * draw_distance_m_ * (2.0 / 3.0);

    result.string_mass = material.calculate_string_mass(string_length_m_);
    result.wave_speed_mps = material.calculate_wave_speed(
        draw_weight_N,
        result.string_mass / string_length_m_
    );

    double actual_efficiency = material.calculate_efficiency_at_pull(1.0);
    result.actual_energy_efficiency = actual_efficiency;

    double effective_mass_kg = arrow_mass_kg_ + result.string_mass * 0.28;
    double usable_energy = draw_energy_j * actual_efficiency;
    double muzzle_velocity = std::sqrt(2.0 * usable_energy / effective_mass_kg);

    result.damping_loss_joules = draw_energy_j * (1.0 - actual_efficiency) * 0.6;
    result.vibration_decay_time_s = result.string_mass / (2.0 * material.damping_coefficient + 1e-6) * 10.0;
    result.creep_stretch_mm = material.calculate_creep_factor(2.0, 25.0) * 0.05;
    result.remaining_life_cycles = std::max(0.0, material.fatigue_life_cycles - 1000.0);

    DynamicsModel model(crossbow_);
    auto trajectory = model.simulate_trajectory(
        muzzle_velocity,
        launch_angle_deg * M_PI / 180.0,
        0, 0, 0.001
    );

    result.core_record = model.calculate_shot_results(trajectory);
    result.core_record.shot_id = generate_uuid();
    result.core_record.timestamp = std::chrono::system_clock::now();
    result.core_record.crossbow_id = crossbow_.id;

    return result;
}

MaterialComparisonResult BowstringPerformanceAnalyzer::compare_all_materials(
    double launch_angle_deg
) {
    auto types = {
        BowstringMaterialType::SINEW,
        BowstringMaterialType::HORN,
        BowstringMaterialType::SILK,
        BowstringMaterialType::HEMP
    };
    std::vector<BowstringMaterialType> type_vec(types);
    return compare_materials(type_vec, launch_angle_deg);
}

MaterialComparisonResult BowstringPerformanceAnalyzer::compare_materials(
    const std::vector<BowstringMaterialType>& material_types,
    double launch_angle_deg
) {
    MaterialComparisonResult result;
    result.crossbow_name = crossbow_.name;
    result.crossbow_draw_weight_kg = crossbow_.draw_weight;
    auto now = std::chrono::system_clock::now();
    result.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    auto& db = BowstringMaterialDatabase::instance();
    double reference_velocity = 0.0;

    for (const auto& type : material_types) {
        const auto& material = db.get(type);
        if (material.type == BowstringMaterialType::UNKNOWN) continue;

        MaterialComparisonResult::PerMaterialMetrics metrics;
        metrics.material = material;

        auto shot_result = simulate_shot(material, launch_angle_deg);
        metrics.estimated_muzzle_velocity = shot_result.core_record.initial_velocity;
        metrics.estimated_energy = 0.5 * arrow_mass_kg_ *
            metrics.estimated_muzzle_velocity * metrics.estimated_muzzle_velocity;
        metrics.max_tension_before_break = calculate_max_possible_tension(material);
        metrics.string_mass = shot_result.string_mass;
        metrics.damping_loss_ratio = shot_result.damping_loss_joules /
            (0.5 * crossbow_.draw_weight * draw_distance_m_ + 1e-6);
        metrics.lifespan_days = material.estimate_lifespan(200, 0.5);

        double accuracy_penalty = 0.0;
        accuracy_penalty += metrics.string_mass * 2.0;
        accuracy_penalty += metrics.damping_loss_ratio * 10.0;
        accuracy_penalty += material.damping_coefficient * 0.5;
        accuracy_penalty = 1.0 + std::tanh(accuracy_penalty / 20.0);
        metrics.accuracy_penalty_factor = accuracy_penalty;

        if (reference_velocity == 0.0) {
            reference_velocity = metrics.estimated_muzzle_velocity;
        }

        result.materials.push_back(metrics);
    }

    result.reference_velocity = reference_velocity;
    return result;
}

std::vector<std::pair<double, double>> BowstringPerformanceAnalyzer::calculate_efficiency_curve(
    const BowstringMaterial& material,
    double min_pull_ratio,
    double max_pull_ratio,
    int samples
) {
    std::vector<std::pair<double, double>> curve;
    curve.reserve(samples);

    double step = (max_pull_ratio - min_pull_ratio) / std::max(1, samples - 1);
    for (int i = 0; i < samples; ++i) {
        double pull_ratio = min_pull_ratio + i * step;
        double efficiency = material.calculate_efficiency_at_pull(pull_ratio);
        curve.emplace_back(pull_ratio, efficiency);
    }
    return curve;
}

std::vector<std::pair<double, double>> BowstringPerformanceAnalyzer::calculate_lifespan_curve(
    const BowstringMaterial& material,
    int max_days,
    double cycles_per_day,
    double avg_humidity
) {
    std::vector<std::pair<double, double>> curve;
    curve.reserve(max_days);

    double total_cycles_per_day = cycles_per_day;
    double humidity_factor = 1.0 + material.moisture_sensitivity * std::max(0.0, avg_humidity - 0.4);
    double max_cycles = material.fatigue_life_cycles;

    for (int day = 1; day <= max_days; ++day) {
        double elapsed_cycles = total_cycles_per_day * day * humidity_factor;
        double remaining_ratio = std::max(0.0, 1.0 - elapsed_cycles / max_cycles);
        double efficiency_decay = 1.0 - 0.25 * (1.0 - remaining_ratio);
        curve.emplace_back(static_cast<double>(day), efficiency_decay);
    }
    return curve;
}

BowstringMaterial BowstringPerformanceAnalyzer::optimize_material_for_accuracy(
    const std::vector<BowstringMaterial>& candidates,
    double target_accuracy_weight,
    double power_weight,
    double durability_weight
) {
    if (candidates.empty()) {
        return BowstringMaterialDatabase::instance().get(BowstringMaterialType::UNKNOWN);
    }

    double best_score = -std::numeric_limits<double>::max();
    const BowstringMaterial* best_material = &candidates[0];

    std::vector<double> scores(candidates.size());
    std::vector<double> accuracy_scores(candidates.size());
    std::vector<double> power_scores(candidates.size());
    std::vector<double> durability_scores(candidates.size());

    double max_accuracy_penalty = 0.0;
    double max_power = 0.0;
    double max_lifespan = 0.0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& mat = candidates[i];
        auto shot = simulate_shot(mat, 15.0);
        double power = 0.5 * arrow_mass_kg_ *
            shot.core_record.initial_velocity * shot.core_record.initial_velocity;
        double lifespan = mat.estimate_lifespan(200, 0.5);
        double acc_penalty = shot.string_mass * 2.0 +
            (shot.damping_loss_joules / (power + 1e-6)) * 5.0;

        accuracy_scores[i] = 1.0 / (1.0 + acc_penalty);
        power_scores[i] = power;
        durability_scores[i] = lifespan;

        max_accuracy_penalty = std::max(max_accuracy_penalty, 1.0 / (1.0 + acc_penalty));
        max_power = std::max(max_power, power);
        max_lifespan = std::max(max_lifespan, lifespan);
    }

    for (size_t i = 0; i < candidates.size(); ++i) {
        double acc_norm = accuracy_scores[i] / (max_accuracy_penalty + 1e-6);
        double power_norm = power_scores[i] / (max_power + 1e-6);
        double dur_norm = durability_scores[i] / (max_lifespan + 1e-6);

        double score = acc_norm * target_accuracy_weight +
                      power_norm * power_weight +
                      dur_norm * durability_weight;

        scores[i] = score;
        if (score > best_score) {
            best_score = score;
            best_material = &candidates[i];
        }
    }

    LOG_INFO("Material optimization complete: best={}, score={:.4f}",
             best_material->name, best_score);
    return *best_material;
}

} // namespace crossbow
