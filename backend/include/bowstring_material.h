#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>

namespace crossbow {

enum class BowstringMaterialType {
    UNKNOWN = 0,
    SINEW = 1,
    HORN = 2,
    SILK = 3,
    HEMP = 4,
    CUSTOM = 99
};

struct BowstringMaterial {
    BowstringMaterialType type;
    std::string name;
    std::string description;
    std::string historical_origin;

    double young_modulus;
    double tensile_strength;
    double density;
    double elongation_at_break;
    double damping_coefficient;
    double energy_efficiency;
    double fatigue_life_cycles;
    double moisture_sensitivity;
    double temperature_coefficient;

    double thickness_mm;
    int strand_count;

    double calculate_string_mass(double length_m) const;
    double calculate_stiffness(double length_m, double cross_section_m2) const;
    double calculate_wave_speed(double tension_N, double linear_density) const;
    double calculate_damping_force(double velocity, double length_m) const;
    double calculate_efficiency_at_pull(double pull_ratio) const;
    double calculate_creep_factor(double hours_under_load, double temperature_c) const;
    double estimate_lifespan(int cycles_per_day, double avg_humidity) const;
};

struct MaterialComparisonResult {
    struct PerMaterialMetrics {
        BowstringMaterial material;
        double estimated_muzzle_velocity;
        double estimated_energy;
        double max_tension_before_break;
        double string_mass;
        double damping_loss_ratio;
        double lifespan_days;
        double accuracy_penalty_factor;
    };

    std::vector<PerMaterialMetrics> materials;
    std::string crossbow_name;
    double crossbow_draw_weight_kg;
    double reference_velocity;
    int64_t timestamp_ms;
};

struct BowstringShotResult {
    BowstringMaterialType material_type;
    double string_mass;
    double actual_energy_efficiency;
    double damping_loss_joules;
    double creep_stretch_mm;
    double remaining_life_cycles;
    double vibration_decay_time_s;
    double wave_speed_mps;
    ShotRecord core_record;
};

class BowstringMaterialDatabase {
public:
    static BowstringMaterialDatabase& instance();

    const BowstringMaterial& get(BowstringMaterialType type) const;
    const BowstringMaterial& get(const std::string& name) const;
    std::vector<BowstringMaterial> list_all() const;

    BowstringMaterial create_custom(
        const std::string& name,
        double young_modulus,
        double tensile_strength,
        double density,
        double elongation,
        double damping,
        double efficiency
    );

private:
    BowstringMaterialDatabase();
    void initialize_defaults();

    std::map<BowstringMaterialType, BowstringMaterial> materials_;
    std::map<std::string, BowstringMaterialType> name_to_type_;
};

class BowstringPerformanceAnalyzer {
public:
    explicit BowstringPerformanceAnalyzer(const CrossbowType& crossbow);

    void set_crossbow(const CrossbowType& crossbow);
    void set_draw_distance(double draw_distance_m);
    void set_arrow_mass(double arrow_mass_g);

    BowstringShotResult simulate_shot(
        const BowstringMaterial& material,
        double launch_angle_deg = 15.0,
        double initial_tension_N = 0.0
    );

    MaterialComparisonResult compare_all_materials(
        double launch_angle_deg = 15.0
    );

    MaterialComparisonResult compare_materials(
        const std::vector<BowstringMaterialType>& material_types,
        double launch_angle_deg = 15.0
    );

    std::vector<std::pair<double, double>> calculate_efficiency_curve(
        const BowstringMaterial& material,
        double min_pull_ratio = 0.1,
        double max_pull_ratio = 1.0,
        int samples = 50
    );

    std::vector<std::pair<double, double>> calculate_lifespan_curve(
        const BowstringMaterial& material,
        int max_days = 365,
        double cycles_per_day = 200,
        double avg_humidity = 0.5
    );

    BowstringMaterial optimize_material_for_accuracy(
        const std::vector<BowstringMaterial>& candidates,
        double target_accuracy_weight = 0.6,
        double power_weight = 0.25,
        double durability_weight = 0.15
    );

private:
    CrossbowType crossbow_;
    double draw_distance_m_;
    double arrow_mass_kg_;
    double string_length_m_;
    double strand_cross_section_m2_;

    void update_derived_params();
    double calculate_max_possible_tension(const BowstringMaterial& mat) const;
    double estimate_velocity_from_energy(
        const BowstringMaterial& mat,
        double draw_energy_j,
        double arrow_mass_kg
    );
};

} // namespace crossbow
