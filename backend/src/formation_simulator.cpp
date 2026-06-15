#include "formation_simulator.h"
#include "dynamics_model.h"
#include "bowstring_material.h"
#include "sight_optics.h"
#include "logger.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>

namespace crossbow {

FormationDatabase::FormationDatabase() {
    initialize_defaults();
}

FormationDatabase& FormationDatabase::instance() {
    static FormationDatabase db;
    return db;
}

void FormationDatabase::initialize_defaults() {
    FormationDesign line;
    line.type = FormationType::LINE_ABREAST;
    line.name = "line_abreast";
    line.description = "一字横阵，正面火力最强，适合阵地防御";
    line.historical_origin = "秦汉时期步兵弩阵基础阵型，《尉缭子·兵教上》所载";
    line.min_crossbows = 3;
    line.optimal_crossbows = 20;
    line.max_crossbows = 200;
    line.frontage_per_unit_m = 2.5;
    line.depth_per_unit_m = 1.0;
    line.minimum_safety_spacing_m = 1.5;
    line.best_suited_range_m = 250.0;
    line.defensive_utility = 0.95;
    line.offensive_utility = 0.55;
    line.siege_utility = 0.70;
    line.mobility_score = 0.40;
    line.command_and_control_score = 0.90;
    formations_[FormationType::LINE_ABREAST] = line;
    name_to_type_[line.name] = FormationType::LINE_ABREAST;
    name_to_type_["一字阵"] = FormationType::LINE_ABREAST;

    FormationDesign wedge;
    wedge.type = FormationType::WEDGE;
    wedge.name = "wedge";
    wedge.description = "楔形攻击阵，中央突破，两翼掩护";
    wedge.historical_origin = "战国魏武卒方阵演变，《吴子·治兵》鱼丽阵变体";
    wedge.min_crossbows = 7;
    wedge.optimal_crossbows = 37;
    wedge.max_crossbows = 180;
    wedge.frontage_per_unit_m = 2.0;
    wedge.depth_per_unit_m = 2.5;
    wedge.minimum_safety_spacing_m = 1.8;
    wedge.best_suited_range_m = 180.0;
    wedge.defensive_utility = 0.65;
    wedge.offensive_utility = 0.90;
    wedge.siege_utility = 0.50;
    wedge.mobility_score = 0.65;
    wedge.command_and_control_score = 0.70;
    formations_[FormationType::WEDGE] = wedge;
    name_to_type_[wedge.name] = FormationType::WEDGE;
    name_to_type_["楔形阵"] = FormationType::WEDGE;

    FormationDesign echelon_l;
    echelon_l.type = FormationType::ECHELON_LEFT;
    echelon_l.name = "echelon_left";
    echelon_l.description = "左斜梯次阵，掩护右翼，反骑兵冲击首选";
    echelon_l.historical_origin = "唐代《太白阴经》所载抵御骑兵阵法";
    echelon_l.min_crossbows = 5;
    echelon_l.optimal_crossbows = 30;
    echelon_l.max_crossbows = 150;
    echelon_l.frontage_per_unit_m = 2.8;
    echelon_l.depth_per_unit_m = 2.2;
    echelon_l.minimum_safety_spacing_m = 2.0;
    echelon_l.best_suited_range_m = 220.0;
    echelon_l.defensive_utility = 0.85;
    echelon_l.offensive_utility = 0.65;
    echelon_l.siege_utility = 0.45;
    echelon_l.mobility_score = 0.75;
    echelon_l.command_and_control_score = 0.80;
    formations_[FormationType::ECHELON_LEFT] = echelon_l;
    name_to_type_[echelon_l.name] = FormationType::ECHELON_LEFT;
    name_to_type_["左梯次"] = FormationType::ECHELON_LEFT;

    FormationDesign echelon_r;
    echelon_r.type = FormationType::ECHELON_RIGHT;
    echelon_r.name = "echelon_right";
    echelon_r.description = "右斜梯次阵，掩护左翼";
    echelon_r.historical_origin = "唐代《太白阴经》所载抵御骑兵阵法";
    echelon_r.min_crossbows = 5;
    echelon_r.optimal_crossbows = 30;
    echelon_r.max_crossbows = 150;
    echelon_r.frontage_per_unit_m = 2.8;
    echelon_r.depth_per_unit_m = 2.2;
    echelon_r.minimum_safety_spacing_m = 2.0;
    echelon_r.best_suited_range_m = 220.0;
    echelon_r.defensive_utility = 0.85;
    echelon_r.offensive_utility = 0.65;
    echelon_r.siege_utility = 0.45;
    echelon_r.mobility_score = 0.75;
    echelon_r.command_and_control_score = 0.80;
    formations_[FormationType::ECHELON_RIGHT] = echelon_r;
    name_to_type_[echelon_r.name] = FormationType::ECHELON_RIGHT;
    name_to_type_["右梯次"] = FormationType::ECHELON_RIGHT;

    FormationDesign vee;
    vee.type = FormationType::VEE;
    vee.name = "vee";
    vee.description = "V形反包围阵，诱敌深入，两侧交叉火力夹击";
    vee.historical_origin = "孙膑兵法'雁行之阵'，马陵之战伏击阵原型";
    vee.min_crossbows = 9;
    vee.optimal_crossbows = 45;
    vee.max_crossbows = 200;
    vee.frontage_per_unit_m = 2.2;
    vee.depth_per_unit_m = 3.0;
    vee.minimum_safety_spacing_m = 1.8;
    vee.best_suited_range_m = 150.0;
    vee.defensive_utility = 0.90;
    vee.offensive_utility = 0.80;
    vee.siege_utility = 0.35;
    vee.mobility_score = 0.50;
    vee.command_and_control_score = 0.60;
    formations_[FormationType::VEE] = vee;
    name_to_type_[vee.name] = FormationType::VEE;
    name_to_type_["雁行阵"] = FormationType::VEE;
    name_to_type_["V形阵"] = FormationType::VEE;

    FormationDesign parabola;
    parabola.type = FormationType::ARCHER_PARABOLA;
    parabola.name = "archer_parabola";
    parabola.description = "弧形射击阵，中央后退两翼前出，火力呈抛物线覆盖";
    parabola.historical_origin = "汉代匈奴骑兵战术改良，卫青漠北之战远程弩阵";
    parabola.min_crossbows = 10;
    parabola.optimal_crossbows = 50;
    parabola.max_crossbows = 250;
    parabola.frontage_per_unit_m = 2.4;
    parabola.depth_per_unit_m = 2.0;
    parabola.minimum_safety_spacing_m = 1.8;
    parabola.best_suited_range_m = 300.0;
    parabola.defensive_utility = 0.88;
    parabola.offensive_utility = 0.75;
    parabola.siege_utility = 0.60;
    parabola.mobility_score = 0.55;
    parabola.command_and_control_score = 0.75;
    formations_[FormationType::ARCHER_PARABOLA] = parabola;
    name_to_type_[parabola.name] = FormationType::ARCHER_PARABOLA;
    name_to_type_["弧形阵"] = FormationType::ARCHER_PARABOLA;

    FormationDesign coiled;
    coiled.type = FormationType::COILED_BOW;
    coiled.name = "coiled_bow";
    coiled.description = "盘弓阵，圆环状防御，四面射击无死角";
    coiled.historical_origin = "宋代《武经总要》所载'偃月营'变体，抗骑兵环形阵";
    coiled.min_crossbows = 12;
    coiled.optimal_crossbows = 48;
    coiled.max_crossbows = 240;
    coiled.frontage_per_unit_m = 1.8;
    coiled.depth_per_unit_m = 1.8;
    coiled.minimum_safety_spacing_m = 1.5;
    coiled.best_suited_range_m = 200.0;
    coiled.defensive_utility = 0.98;
    coiled.offensive_utility = 0.35;
    coiled.siege_utility = 0.90;
    coiled.mobility_score = 0.20;
    coiled.command_and_control_score = 0.55;
    formations_[FormationType::COILED_BOW] = coiled;
    name_to_type_[coiled.name] = FormationType::COILED_BOW;
    name_to_type_["盘弓阵"] = FormationType::COILED_BOW;
    name_to_type_["偃月营"] = FormationType::COILED_BOW;

    FormationDesign crane;
    crane.type = FormationType::CRANE_WING;
    crane.name = "crane_wing";
    crane.description = "鹤翼阵，中央薄两翼厚，类似V形但中央可接敌近战";
    crane.historical_origin = "春秋《左传》所载郑庄公鱼丽阵，后传为日本武田信玄绝学";
    crane.min_crossbows = 11;
    crane.optimal_crossbows = 55;
    crane.max_crossbows = 220;
    crane.frontage_per_unit_m = 2.3;
    crane.depth_per_unit_m = 2.8;
    crane.minimum_safety_spacing_m = 1.8;
    crane.best_suited_range_m = 160.0;
    crane.defensive_utility = 0.70;
    crane.offensive_utility = 0.95;
    crane.siege_utility = 0.40;
    crane.mobility_score = 0.60;
    crane.command_and_control_score = 0.50;
    formations_[FormationType::CRANE_WING] = crane;
    name_to_type_[crane.name] = FormationType::CRANE_WING;
    name_to_type_["鹤翼阵"] = FormationType::CRANE_WING;

    FormationDesign grid;
    grid.type = FormationType::SQUARE_GRID;
    grid.name = "square_grid";
    grid.description = "方形格阵，训练方阵，号令规整便于指挥";
    grid.historical_origin = "西周司马法基础方阵，春秋演化为乘丘之制";
    grid.min_crossbows = 9;
    grid.optimal_crossbows = 36;
    grid.max_crossbows = 144;
    grid.frontage_per_unit_m = 2.5;
    grid.depth_per_unit_m = 2.5;
    grid.minimum_safety_spacing_m = 2.0;
    grid.best_suited_range_m = 200.0;
    grid.defensive_utility = 0.85;
    grid.offensive_utility = 0.70;
    grid.siege_utility = 0.75;
    grid.mobility_score = 0.55;
    grid.command_and_control_score = 0.95;
    formations_[FormationType::SQUARE_GRID] = grid;
    name_to_type_[grid.name] = FormationType::SQUARE_GRID;
    name_to_type_["方阵"] = FormationType::SQUARE_GRID;

    FormationDesign fish;
    fish.type = FormationType::FISH_SCALE;
    fish.name = "fish_scale";
    fish.description = "鱼鳞阵，交错排列，连续火力波次输出";
    fish.historical_origin = "《孙膑兵法·十阵》所载，多排交替射击战术";
    fish.min_crossbows = 8;
    fish.optimal_crossbows = 40;
    fish.max_crossbows = 200;
    fish.frontage_per_unit_m = 2.2;
    fish.depth_per_unit_m = 1.5;
    fish.minimum_safety_spacing_m = 1.8;
    fish.best_suited_range_m = 280.0;
    fish.defensive_utility = 0.80;
    fish.offensive_utility = 0.85;
    fish.siege_utility = 0.65;
    fish.mobility_score = 0.70;
    fish.command_and_control_score = 0.75;
    formations_[FormationType::FISH_SCALE] = fish;
    name_to_type_[fish.name] = FormationType::FISH_SCALE;
    name_to_type_["鱼鳞阵"] = FormationType::FISH_SCALE;

    LOG_INFO("FormationDatabase: initialized {} default formations", formations_.size());
}

const FormationDesign& FormationDatabase::get(FormationType type) const {
    auto it = formations_.find(type);
    if (it == formations_.end()) {
        static FormationDesign unknown{};
        unknown.type = FormationType::CUSTOM;
        unknown.name = "custom";
        return unknown;
    }
    return it->second;
}

const FormationDesign& FormationDatabase::get(const std::string& name) const {
    auto it = name_to_type_.find(name);
    if (it == name_to_type_.end()) {
        return get(FormationType::CUSTOM);
    }
    return get(it->second);
}

std::vector<FormationDesign> FormationDatabase::list_all() const {
    std::vector<FormationDesign> result;
    result.reserve(formations_.size());
    for (const auto& [type, f] : formations_) result.push_back(f);
    return result;
}

FormationSimulator::FormationSimulator(uint64_t seed)
    : rng_(seed == 0 ? std::random_device{}() : seed)
{
}

void FormationSimulator::set_crossbow_types(const std::map<int, CrossbowType>& types) {
    crossbow_types_ = types;
}

void FormationSimulator::set_target(const TargetSpecification& target) {
    current_target_ = target;
}

std::pair<double, double> FormationSimulator::rotate_point(
    double x, double y, double cx, double cy, double angle_deg
) const {
    double rad = angle_deg * M_PI / 180.0;
    double dx = x - cx;
    double dy = y - cy;
    return {
        cx + dx * std::cos(rad) - dy * std::sin(rad),
        cy + dx * std::sin(rad) + dy * std::cos(rad)
    };
}

double FormationSimulator::distance_to_target(
    const CrossbowPlacement& placement,
    const TargetSpecification& target
) const {
    double dx = placement.x_m - target.center_x_m;
    double dy = placement.y_m - target.center_y_m;
    return std::sqrt(dx * dx + dy * dy);
}

double FormationSimulator::calculate_target_area(const TargetSpecification& target) const {
    switch (target.type) {
        case TargetType::SINGLE_POINT: return 1.0;
        case TargetType::AREA_RECTANGLE: return target.width_m * target.depth_m;
        case TargetType::AREA_CIRCLE: return M_PI * target.radius_m * target.radius_m;
        case TargetType::LINEAR_DEFENSE: return target.width_m * 3.0;
        case TargetType::CONVOY_PATH: return target.width_m * target.depth_m * 0.5;
        case TargetType::FORTIFICATION: return target.width_m * target.depth_m * 1.2;
        default: return 1.0;
    }
}

bool FormationSimulator::is_point_in_target(
    double x, double y, const TargetSpecification& target
) const {
    double dx = x - target.center_x_m;
    double dy = y - target.center_y_m;
    switch (target.type) {
        case TargetType::SINGLE_POINT:
            return std::sqrt(dx*dx + dy*dy) < 2.0;
        case TargetType::AREA_RECTANGLE:
            return std::abs(dx) < target.width_m/2 && std::abs(dy) < target.depth_m/2;
        case TargetType::AREA_CIRCLE:
            return dx*dx + dy*dy < target.radius_m * target.radius_m;
        case TargetType::LINEAR_DEFENSE:
            return std::abs(dx) < target.width_m/2 && std::abs(dy) < 1.5;
        case TargetType::CONVOY_PATH: {
            double t = std::clamp(dx / std::max(0.1, target.width_m), 0.0, 1.0);
            double path_y = -dy + t * target.depth_m;
            return std::abs(path_y) < target.radius_m;
        }
        case TargetType::FORTIFICATION:
            return std::abs(dx) < target.width_m/2 * 1.1 &&
                   std::abs(dy) < target.depth_m/2 * 1.1;
        default: return false;
    }
}

std::vector<CrossbowPlacement> FormationSimulator::generate_formation(
    FormationType type,
    int num_crossbows,
    double center_x_m,
    double center_y_m,
    double orientation_deg,
    double spacing_multiplier
) {
    auto& design = FormationDatabase::instance().get(type);
    num_crossbows = std::max(design.min_crossbows,
                             std::min(num_crossbows, design.max_crossbows));
    double space_x = design.frontage_per_unit_m * spacing_multiplier;
    double space_z = design.depth_per_unit_m * spacing_multiplier;

    std::vector<CrossbowPlacement> placements;
    placements.reserve(num_crossbows);

    auto default_type = crossbow_types_.empty() ? CrossbowType{} :
        crossbow_types_.begin()->second;
    int default_id = crossbow_types_.empty() ? 1 : crossbow_types_.begin()->first;

    auto add_crossbow = [&](double x, double y, double face, double delay, int group) {
        CrossbowPlacement p;
        p.id = static_cast<int>(placements.size());
        p.crossbow_type_id = default_id;
        p.name = default_type.name + "_" + std::to_string(p.id);
        auto rotated = rotate_point(x, y, center_x_m, center_y_m, orientation_deg);
        p.x_m = rotated.first;
        p.y_m = rotated.second;
        p.z_m = 0.0;
        p.facing_deg = face + orientation_deg;
        auto dx = current_target_.center_x_m - p.x_m;
        auto dy = current_target_.center_y_m - p.y_m;
        p.elevation_deg = std::atan2(50.0, std::sqrt(dx*dx + dy*dy)) * 180.0 / M_PI;
        p.firing_delay_ms = delay;
        p.salvo_group = group;
        p.is_active = true;
        p.crew_size = 2;
        p.supply_status = 1.0;
        placements.push_back(p);
    };

    int id_counter = 0;

    switch (type) {
        case FormationType::LINE_ABREAST: {
            int n = num_crossbows;
            double half = (n - 1) / 2.0;
            for (int i = 0; i < n; ++i) {
                double x = center_x_m + (i - half) * space_x;
                double y = center_y_m;
                double target_angle = std::atan2(
                    current_target_.center_y_m - y,
                    current_target_.center_x_m - x
                ) * 180.0 / M_PI;
                add_crossbow(x, y, target_angle, (i % 3) * 100.0, i % 3);
            }
            break;
        }
        case FormationType::WEDGE: {
            int placed = 0, row = 0;
            while (placed < num_crossbows) {
                int row_count = 1 + row * 2;
                for (int i = 0; i < row_count && placed < num_crossbows; ++i) {
                    double x = center_x_m + (i - row_count / 2.0) * space_x;
                    double y = center_y_m - row * space_z;
                    double angle = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    add_crossbow(x, y, angle, row * 200.0, row % 4);
                    placed++;
                }
                row++;
            }
            break;
        }
        case FormationType::ECHELON_LEFT:
        case FormationType::ECHELON_RIGHT: {
            int dir = (type == FormationType::ECHELON_LEFT) ? -1 : 1;
            for (int i = 0; i < num_crossbows; ++i) {
                double x = center_x_m + i * space_x * 0.6;
                double y = center_y_m + dir * i * space_z * 0.8;
                double angle = std::atan2(
                    current_target_.center_y_m - y,
                    current_target_.center_x_m - x
                ) * 180.0 / M_PI;
                add_crossbow(x, y, angle, (i % 5) * 80.0, i % 5);
            }
            break;
        }
        case FormationType::VEE: {
            int n = (num_crossbows + 1) / 2;
            for (int side = 0; side < 2; ++side) {
                double sdir = (side == 0) ? -1.0 : 1.0;
                for (int i = 1; i <= n && placements.size() < num_crossbows; ++i) {
                    double x = center_x_m + sdir * i * space_x * 0.5;
                    double y = center_y_m + i * space_z * 0.8;
                    double angle = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    add_crossbow(x, y, angle, i * 120.0, (i + side * 3) % 6);
                }
            }
            break;
        }
        case FormationType::ARCHER_PARABOLA: {
            int n = num_crossbows;
            for (int i = 0; i < n; ++i) {
                double t = (i - (n-1)/2.0) / std::max(1.0, (n-1)/2.0);
                double x = center_x_m + t * n * space_x * 0.4;
                double y = center_y_m + t * t * n * space_z * 0.3;
                double angle = std::atan2(
                    current_target_.center_y_m - y,
                    current_target_.center_x_m - x
                ) * 180.0 / M_PI;
                add_crossbow(x, y, angle, (i % 4) * 150.0, i % 4);
            }
            break;
        }
        case FormationType::COILED_BOW: {
            int rings = std::max(2, (int)std::ceil(std::sqrt(num_crossbows / M_PI)));
            int placed = 0;
            for (int r = 0; r < rings && placed < num_crossbows; ++r) {
                double radius = (r + 1) * design.frontage_per_unit_m * spacing_multiplier * 0.8;
                int count_in_ring = (int)(2 * M_PI * radius / space_x);
                count_in_ring = std::max(6, count_in_ring);
                for (int j = 0; j < count_in_ring && placed < num_crossbows; ++j) {
                    double theta = 2 * M_PI * j / count_in_ring;
                    double x = center_x_m + radius * std::cos(theta);
                    double y = center_y_m + radius * std::sin(theta);
                    double outward_angle = theta * 180.0 / M_PI + 180.0;
                    double to_center = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    double angle = (r == 0) ? to_center : outward_angle;
                    add_crossbow(x, y, angle, j * 50.0 + r * 300.0, (r + j) % 8);
                    placed++;
                }
            }
            break;
        }
        case FormationType::CRANE_WING: {
            int n = num_crossbows;
            int wing_each = n / 3;
            int center_count = n - 2 * wing_each;
            double half_c = (center_count - 1) / 2.0;
            for (int i = 0; i < center_count; ++i) {
                double x = center_x_m + (i - half_c) * space_x * 0.9;
                double y = center_y_m;
                double angle = std::atan2(
                    current_target_.center_y_m - y,
                    current_target_.center_x_m - x
                ) * 180.0 / M_PI;
                add_crossbow(x, y, angle, 0.0, 0);
            }
            for (int wing = 0; wing < 2; ++wing) {
                double sdir = (wing == 0) ? -1 : 1;
                for (int i = 1; i <= wing_each; ++i) {
                    double x = center_x_m + sdir * (half_c * space_x * 0.9 + i * space_x * 0.7);
                    double y = center_y_m + i * space_z * 1.2;
                    double angle = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    add_crossbow(x, y, angle, i * 100.0, wing + 1);
                }
            }
            break;
        }
        case FormationType::SQUARE_GRID: {
            int side = (int)std::ceil(std::sqrt(num_crossbows));
            double half = (side - 1) / 2.0;
            for (int i = 0; i < side && placements.size() < num_crossbows; ++i) {
                for (int j = 0; j < side && placements.size() < num_crossbows; ++j) {
                    double x = center_x_m + (j - half) * space_x;
                    double y = center_y_m + (i - half) * space_z;
                    double angle = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    int group = (i + j) % 4;
                    add_crossbow(x, y, angle, group * 200.0, group);
                }
            }
            break;
        }
        case FormationType::FISH_SCALE: {
            int rows = std::max(3, (int)std::ceil(std::sqrt(num_crossbows * 0.6)));
            int cols = (num_crossbows + rows - 1) / rows;
            for (int r = 0; r < rows; ++r) {
                int in_row = std::min(cols, num_crossbows - r * cols);
                double half = (in_row - 1) / 2.0;
                double offset = (r % 2 == 0) ? 0.0 : space_x * 0.5;
                for (int c = 0; c < in_row; ++c) {
                    double x = center_x_m + (c - half) * space_x + offset;
                    double y = center_y_m + r * space_z;
                    double angle = std::atan2(
                        current_target_.center_y_m - y,
                        current_target_.center_x_m - x
                    ) * 180.0 / M_PI;
                    int group = r;
                    add_crossbow(x, y, angle, group * 150.0, group);
                }
            }
            break;
        }
        default: {
            for (int i = 0; i < num_crossbows; ++i) {
                double x = center_x_m + (i % 5 - 2) * space_x;
                double y = center_y_m + (i / 5) * space_z;
                add_crossbow(x, y, 0.0, i * 50.0, i % 4);
            }
        }
    }

    LOG_INFO("Generated formation: type={}, crossbows={}",
             static_cast<int>(type), placements.size());
    return placements;
}

std::vector<CrossbowPlacement> FormationSimulator::generate_custom_formation(
    const std::vector<int>& crossbow_type_ids,
    const std::vector<std::pair<double, double>>& positions,
    const std::vector<double>& facings_deg
) {
    size_t n = std::min({crossbow_type_ids.size(), positions.size(), facings_deg.size()});
    std::vector<CrossbowPlacement> result;
    result.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        CrossbowPlacement p;
        p.id = static_cast<int>(i);
        p.crossbow_type_id = crossbow_type_ids[i];
        auto it = crossbow_types_.find(p.crossbow_type_id);
        p.name = (it != crossbow_types_.end()) ? it->second.name : "crossbow_" + std::to_string(i);
        p.x_m = positions[i].first;
        p.y_m = positions[i].second;
        p.z_m = 0.0;
        p.facing_deg = facings_deg[i];
        auto dx = current_target_.center_x_m - p.x_m;
        auto dy = current_target_.center_y_m - p.y_m;
        p.elevation_deg = std::atan2(30.0, std::sqrt(dx*dx + dy*dy)) * 180.0 / M_PI;
        p.firing_delay_ms = (i % 4) * 120.0;
        p.salvo_group = i % 4;
        p.is_active = true;
        p.crew_size = 2;
        p.supply_status = 1.0;
        result.push_back(p);
    }
    return result;
}

double FormationSimulator::estimate_hit_probability(
    const CrossbowPlacement& placement,
    const TargetSpecification& target
) {
    double dist = distance_to_target(placement, target);
    auto it = crossbow_types_.find(placement.crossbow_type_id);
    double eff_range = (it != crossbow_types_.end()) ?
        it->second.effective_range : 180.0;
    double mx_range = (it != crossbow_types_.end()) ?
        it->second.max_range : 350.0;

    double range_factor = 1.0;
    if (dist < eff_range) {
        range_factor = 1.0 - 0.1 * (dist / eff_range);
    } else if (dist < mx_range) {
        range_factor = 0.9 * (1.0 - (dist - eff_range) / (mx_range - eff_range));
    } else {
        range_factor = 0.05 * std::exp(-(dist - mx_range) / 100.0);
    }

    double area = calculate_target_area(target);
    double sigma = std::max(1.0, dist * 0.015);
    double coverage_prob = 1.0 - std::exp(-area / (2 * M_PI * sigma * sigma));

    double angle_diff = std::atan2(
        target.center_y_m - placement.y_m,
        target.center_x_m - placement.x_m
    ) * 180.0 / M_PI - placement.facing_deg;
    while (angle_diff > 180.0) angle_diff -= 360.0;
    while (angle_diff < -180.0) angle_diff += 360.0;
    double angle_factor = std::exp(-std::pow(angle_diff / 45.0, 2.0));

    return std::clamp(range_factor * coverage_prob * angle_factor * target.priority_factor,
                      0.001, 0.98);
}

std::vector<std::pair<double, double>> FormationSimulator::project_hit_pattern(
    const CrossbowPlacement& placement,
    const TargetSpecification& target,
    int projectiles_per_crossbow
) {
    std::vector<std::pair<double, double>> hits;
    hits.reserve(projectiles_per_crossbow);

    double dist = distance_to_target(placement, target);
    double hit_prob = estimate_hit_probability(placement, target);
    double sigma = std::max(1.0, dist * 0.012);

    double base_angle = std::atan2(
        target.center_y_m - placement.y_m,
        target.center_x_m - placement.x_m
    );

    std::normal_distribution<double> dist_ang(0.0, sigma / dist);
    std::normal_distribution<double> dist_range(0.0, sigma);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    for (int i = 0; i < projectiles_per_crossbow; ++i) {
        double hit_x, hit_y;
        if (uniform(rng_) < hit_prob) {
            double da = dist_ang(rng_);
            double dr = dist_range(rng_);
            double actual_dist = dist + dr;
            double actual_angle = base_angle + da;
            hit_x = placement.x_m + actual_dist * std::cos(actual_angle);
            hit_y = placement.y_m + actual_dist * std::sin(actual_angle);
        } else {
            double da = dist_ang(rng_) * 2.5;
            double dr = dist_range(rng_) * 2.5;
            double actual_dist = dist + dr;
            double actual_angle = base_angle + da;
            hit_x = placement.x_m + actual_dist * std::cos(actual_angle);
            hit_y = placement.y_m + actual_dist * std::sin(actual_angle);
        }
        hits.emplace_back(hit_x, hit_y);
    }
    return hits;
}

FiringPlan FormationSimulator::generate_firing_plan(
    const std::vector<CrossbowPlacement>& placements,
    int volleys,
    double salvo_interval_ms
) {
    FiringPlan plan;
    plan.round_id = 1;
    plan.total_projectiles = static_cast<int>(placements.size()) * std::max(1, volleys);
    plan.crossbows_firing = static_cast<int>(placements.size());

    std::map<int, int> groups;
    double max_delay = 0.0;
    for (const auto& p : placements) {
        groups[static_cast<int>(p.salvo_group)]++;
        max_delay = std::max(max_delay, p.firing_delay_ms);
    }

    plan.salvo_interval_ms = salvo_interval_ms;
    double total_time_ms = max_delay + (volleys - 1) * salvo_interval_ms + 500.0;
    plan.start_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    plan.end_timestamp_ms = plan.start_timestamp_ms +
                            static_cast<int64_t>(total_time_ms);

    plan.time_on_target_s = total_time_ms / 1000.0;
    plan.average_volleys_per_minute = 60000.0 / salvo_interval_ms;

    plan.ammunition_consumed = plan.total_projectiles * 0.08;
    plan.supply_burden_kg = plan.ammunition_consumed * 0.075;
    plan.density_at_target_projectiles_per_m2 =
        plan.total_projectiles / std::max(1.0, calculate_target_area(current_target_));

    double avg_hit_prob = 0.0;
    for (const auto& p : placements) {
        avg_hit_prob += estimate_hit_probability(p, current_target_);
    }
    avg_hit_prob /= std::max<size_t>(1, placements.size());

    double lambda = avg_hit_prob * plan.total_projectiles /
                    std::max(1, current_target_.personnel_estimate);
    plan.estimated_kill_probability = 1.0 - std::exp(-lambda);
    plan.estimated_casualties = current_target_.personnel_estimate *
                                 plan.estimated_kill_probability * 0.7;

    return plan;
}

double FormationSimulator::calculate_target_coverage(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target
) {
    if (placements.empty()) return 0.0;

    double total_area = calculate_target_area(target);
    if (total_area <= 0.0) return 0.0;

    double covered_area = 0.0;
    int grid_points = 2000;
    int hits = 0;

    std::uniform_real_distribution<double> ux(target.center_x_m - target.width_m * 0.8,
                                               target.center_x_m + target.width_m * 0.8);
    std::uniform_real_distribution<double> uy(target.center_y_m - target.depth_m * 0.8,
                                               target.center_y_m + target.depth_m * 0.8);

    for (int i = 0; i < grid_points; ++i) {
        double gx = ux(rng_);
        double gy = uy(rng_);
        if (!is_point_in_target(gx, gy, target)) continue;

        bool covered = false;
        for (const auto& p : placements) {
            CrossbowPlacement test = p;
            TargetSpecification point_target = target;
            point_target.type = TargetType::SINGLE_POINT;
            point_target.center_x_m = gx;
            point_target.center_y_m = gy;
            double prob = estimate_hit_probability(test, point_target);
            if (prob > 0.3) {
                covered = true;
                break;
            }
        }
        hits += covered ? 1 : 0;
    }

    return static_cast<double>(hits) / grid_points * 100.0;
}

EngagementResult FormationSimulator::simulate_engagement(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target,
    int volleys,
    double wind_x_mps,
    double wind_y_mps,
    double visibility_km
) {
    EngagementResult result;
    result.formation_name = "simulated";
    result.num_crossbows = static_cast<int>(placements.size());
    result.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    int total_shots = result.num_crossbows * std::max(1, volleys);
    result.total_shots = total_shots;

    int hits = 0;
    std::vector<std::pair<double, double>> all_hits;
    double avg_hit_prob = 0.0;
    double wind_speed = std::sqrt(wind_x_mps * wind_x_mps + wind_y_mps * wind_y_mps);
    double visibility_factor = 1.0 - 0.5 * std::exp(-visibility_km / 3.0);

    std::uniform_real_distribution<double> roll(0.0, 1.0);
    int per_cb_samples = 3;

    for (const auto& p : placements) {
        double base_prob = estimate_hit_probability(p, target);
        double wind_penalty = 1.0 - std::min(0.6, wind_speed * 0.05);
        double final_prob = base_prob * wind_penalty * visibility_factor;
        avg_hit_prob += final_prob;

        auto pattern = project_hit_pattern(p, target, per_cb_samples);
        for (const auto& h : pattern) {
            all_hits.push_back(h);
            if (is_point_in_target(h.first, h.second, target)) hits++;
        }

        for (int v = 0; v < volleys; ++v) {
            if (roll(rng_) < final_prob) hits++;
        }
    }

    avg_hit_prob /= std::max<size_t>(1, placements.size());
    result.total_hits = hits;
    result.overall_hit_probability = static_cast<double>(hits) /
                                      std::max<size_t>(1, (size_t)(total_shots + per_cb_samples * placements.size()));
    result.target_coverage_percent = calculate_target_coverage(placements, target);

    double area = calculate_target_area(target);
    double saturation = all_hits.size() / std::max(1.0, area * 3.0);
    result.saturation_score = std::min(1.0, saturation);

    result.suppression_effect = std::min(1.0, result.saturation_score * 0.7 +
                                              result.target_coverage_percent / 200.0);
    result.enemy_response_time_s = 10.0 - result.suppression_effect * 6.0;

    double min_dist = 1e9;
    for (const auto& p : placements) {
        double d = distance_to_target(p, target);
        if (d < min_dist) min_dist = d;
    }
    result.friendly_risk_radius_m = std::max(50.0, min_dist * 0.4);

    double total_supply_demand = 0.0;
    for (const auto& p : placements) {
        total_supply_demand += 2.0 / std::max(0.1, p.supply_status);
    }
    result.logistical_efficiency = 1.0 / (1.0 + total_supply_demand / (10.0 + placements.size()));

    result.firing_plan = generate_firing_plan(placements, volleys);
    result.hit_pattern = std::move(all_hits);

    LOG_INFO("Engagement simulated: crossbows={}, shots={}, hits={}, P(hit)={:.2f}%, coverage={:.1f}%",
             result.num_crossbows, result.total_shots, result.total_hits,
             result.overall_hit_probability * 100, result.target_coverage_percent);
    return result;
}

SalvoOptimization FormationSimulator::optimize_salvo_timing(
    const std::vector<CrossbowPlacement>& placements,
    const TargetSpecification& target,
    double target_density
) {
    SalvoOptimization opt;

    if (placements.empty()) return opt;

    double area = calculate_target_area(target);
    int total_projectiles_needed = (int)(target_density * area);
    int per_cb = std::max(1, (total_projectiles_needed + (int)placements.size() - 1) /
                              (int)placements.size());

    std::map<int, std::vector<const CrossbowPlacement*>> groups;
    for (const auto& p : placements) {
        groups[static_cast<int>(p.salvo_group)].push_back(&p);
    }

    int group_count = std::max(1, (int)groups.size());
    double avg_time_of_flight = 0.0;
    for (const auto& p : placements) {
        double dist = distance_to_target(p, target);
        auto it = crossbow_types_.find(p.crossbow_type_id);
        double v0 = (it != crossbow_types_.end()) ? 90.0 : 80.0;
        avg_time_of_flight += dist / v0 * 1.5;
    }
    avg_time_of_flight /= std::max<size_t>(1, placements.size());

    opt.optimal_salvo_size = (int)placements.size();
    opt.optimal_interval_ms = std::max(100.0, avg_time_of_flight * 1000.0 / group_count);
    opt.total_volleys = per_cb;
    opt.saturation_time_s = (per_cb - 1) * opt.optimal_interval_ms / 1000.0 +
                              avg_time_of_flight;
    opt.average_projectile_spacing_m = std::sqrt(area / std::max(1, total_projectiles_needed));
    opt.time_over_target_seconds = opt.saturation_time_s + avg_time_of_flight * 0.5;
    opt.minimum_overkill_penalty = std::max(0.0, 1.0 - (double)placements.size() / 50.0);

    return opt;
}

std::vector<EngagementResult> FormationSimulator::compare_formations(
    const std::vector<FormationType>& formation_types,
    int num_crossbows,
    const TargetSpecification& target,
    int volleys
) {
    std::vector<EngagementResult> results;
    results.reserve(formation_types.size());

    for (const auto& ftype : formation_types) {
        auto& design = FormationDatabase::instance().get(ftype);
        auto placements = generate_formation(ftype, num_crossbows, 0.0, 0.0, 0.0, 1.0);
        auto eng = simulate_engagement(placements, target, volleys, 2.0, 0.0, 10.0);
        eng.formation_type = ftype;
        eng.formation_name = design.name;
        results.push_back(std::move(eng));
    }
    return results;
}

FormationRLOptimizer::FormationRLOptimizer(uint64_t seed)
    : rng_(seed)
    , simulator_(seed + 1)
    , wind_x_(0.0)
    , wind_y_(0.0)
    , visibility_km_(10.0)
    , min_crossbows_(6)
    , max_crossbows_(48)
    , allow_formation_switch_(true)
    , allow_spacing_switch_(true)
    , allow_orientation_switch_(true)
{
}

void FormationRLOptimizer::set_crossbow_types(const std::map<int, CrossbowType>& types) {
    crossbow_types_ = types;
    simulator_.set_crossbow_types(types);
}

void FormationRLOptimizer::set_target(const TargetSpecification& target) {
    target_ = target;
    simulator_.set_target(target);
}

void FormationRLOptimizer::set_environment(
    double wind_x, double wind_y, double visibility_km
) {
    wind_x_ = wind_x;
    wind_y_ = wind_y;
    visibility_km_ = visibility_km;
}

void FormationRLOptimizer::set_training_conditions(
    int min_crossbows, int max_crossbows,
    bool allow_formation_switching,
    bool allow_spacing_optimization,
    bool allow_orientation_optimization
) {
    min_crossbows_ = min_crossbows;
    max_crossbows_ = max_crossbows;
    allow_formation_switch_ = allow_formation_switching;
    allow_spacing_switch_ = allow_spacing_optimization;
    allow_orientation_switch_ = allow_orientation_optimization;
}

int FormationRLOptimizer::get_num_states() const {
    int n_formations = 10;
    int n_crossbow_bins = 8;
    int n_distance_bins = 5;
    int n_coverage_bins = 5;
    return n_formations * n_crossbow_bins * n_distance_bins * n_coverage_bins;
}

int FormationRLOptimizer::get_num_actions() const {
    int actions = 1;
    if (allow_formation_switch_) actions += 9;
    if (allow_spacing_switch_) actions += 5;
    if (allow_orientation_switch_) actions += 8;
    return actions;
}

int FormationRLOptimizer::discretize_state(
    FormationType formation,
    int crossbows,
    double avg_distance,
    double coverage
) {
    int f = std::min(9, std::max(0, (int)formation));
    int cb_bin = std::min(7, std::max(0, (crossbows - min_crossbows_) /
                             std::max(1, (max_crossbows_ - min_crossbows_) / 7)));
    int d_bin = std::min(4, std::max(0, (int)(avg_distance / 100.0)));
    int c_bin = std::min(4, std::max(0, (int)(coverage / 20.0)));
    return f * 200 + cb_bin * 25 + d_bin * 5 + c_bin;
}

FormationType FormationRLOptimizer::get_formation_for_action(int action_id) {
    if (!allow_formation_switch_ || action_id < 10) {
        return static_cast<FormationType>(action_id % 10);
    }
    return FormationType::LINE_ABREAST;
}

double FormationRLOptimizer::calculate_spacing_modifier(int action_id) {
    if (!allow_spacing_switch_ || action_id < 10) return 1.0;
    int spacing_action = (action_id - 10) % 5;
    return 0.7 + spacing_action * 0.15;
}

int FormationRLOptimizer::select_action(int state, double exploration_rate) {
    int n_actions = get_num_actions();
    std::uniform_real_distribution<double> u(0.0, 1.0);
    if (u(rng_) < exploration_rate) {
        std::uniform_int_distribution<int> a(0, n_actions - 1);
        return a(rng_);
    }

    if (state >= (int)q_table_.size()) return 0;
    const auto& actions = q_table_[state];
    int best_a = 0;
    double best_q = -1e9;
    for (int a = 0; a < (int)actions.size() && a < n_actions; ++a) {
        if (actions[a] > best_q) {
            best_q = actions[a];
            best_a = a;
        }
    }
    return best_a;
}

double FormationRLOptimizer::evaluate_reward(
    const EngagementResult& engagement,
    double weight_hit_rate,
    double weight_coverage,
    double weight_saturation,
    double weight_logistics,
    double weight_casualties
) {
    double r = 0.0;
    r += engagement.overall_hit_probability * 100.0 * weight_hit_rate;
    r += engagement.target_coverage_percent * weight_coverage;
    r += engagement.saturation_score * 100.0 * weight_saturation;
    r += engagement.logistical_efficiency * 100.0 * weight_logistics;
    r += std::min(100.0, engagement.firing_plan.estimated_casualties) * weight_casualties;
    r -= engagement.friendly_risk_radius_m / 100.0 * 5.0;
    return r / 100.0;
}

RLOptimizationResult FormationRLOptimizer::train_q_learning(
    int num_crossbows,
    int episodes,
    double learning_rate,
    double discount_factor,
    double exploration_rate,
    double exploration_decay
) {
    RLOptimizationResult result;
    result.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result.episode_rewards.reserve(episodes);

    int n_states = get_num_states();
    int n_actions = get_num_actions();
    q_table_.assign(n_states, std::vector<double>(n_actions, 0.0));
    formation_value_function_.clear();

    FormationType best_formation = FormationType::LINE_ABREAST;
    double best_reward = -1e9;
    std::vector<double> best_params = {1.0, 0.0};

    auto all_formations = {
        FormationType::LINE_ABREAST, FormationType::WEDGE,
        FormationType::ECHELON_LEFT, FormationType::ECHELON_RIGHT,
        FormationType::VEE, FormationType::ARCHER_PARABOLA,
        FormationType::COILED_BOW, FormationType::CRANE_WING,
        FormationType::SQUARE_GRID, FormationType::FISH_SCALE
    };

    double current_eps = exploration_rate;
    double moving_avg = 0.0;
    int converged_count = 0;
    int train_steps = 0;

    for (int ep = 0; ep < episodes; ++ep) {
        FormationType current_f = FormationType::LINE_ABREAST;
        double spacing = 1.0;
        double orientation = 0.0;
        int effective_crossbows = num_crossbows;

        if (ep % 7 == 0) {
            effective_crossbows = min_crossbows_ +
                std::uniform_int_distribution<int>(0, std::max(0, max_crossbows_ - min_crossbows_))(rng_);
        }

        auto placements = simulator_.generate_formation(
            current_f, effective_crossbows, 0.0, 0.0, orientation, spacing
        );
        auto engagement = simulator_.simulate_engagement(
            placements, target_, 2, wind_x_, wind_y_, visibility_km_
        );

        double avg_dist = 0.0;
        for (const auto& p : placements) {
            double dx = p.x_m - target_.center_x_m;
            double dy = p.y_m - target_.center_y_m;
            avg_dist += std::sqrt(dx*dx + dy*dy);
        }
        avg_dist /= std::max<size_t>(1, placements.size());

        int state = discretize_state(current_f, effective_crossbows, avg_dist,
                                      engagement.target_coverage_percent);
        double reward = evaluate_reward(engagement);
        int steps_in_ep = std::max(5, 15 - ep / (episodes / 15));

        for (int step = 0; step < steps_in_ep; ++step) {
            int action = select_action(state, current_eps);
            FormationType next_f = current_f;
            double next_spacing = spacing;
            double next_orientation = orientation;

            if (allow_formation_switch_ && action >= 0 && action < 10) {
                next_f = get_formation_for_action(action);
            } else if (allow_spacing_switch_ && action >= 10 && action < 20) {
                next_spacing = calculate_spacing_modifier(action);
            } else if (allow_orientation_switch_ && action >= 20) {
                int o = (action - 20) % 8;
                next_orientation = o * 45.0 - 180.0;
            }

            auto next_placements = simulator_.generate_formation(
                next_f, effective_crossbows, 0.0, 0.0, next_orientation, next_spacing
            );
            auto next_engagement = simulator_.simulate_engagement(
                next_placements, target_, 2, wind_x_, wind_y_, visibility_km_
            );

            double next_dist = 0.0;
            for (const auto& p : next_placements) {
                double dx = p.x_m - target_.center_x_m;
                double dy = p.y_m - target_.center_y_m;
                next_dist += std::sqrt(dx*dx + dy*dy);
            }
            next_dist /= std::max<size_t>(1, next_placements.size());

            int next_state = discretize_state(next_f, effective_crossbows, next_dist,
                                               next_engagement.target_coverage_percent);
            double next_reward = evaluate_reward(next_engagement);

            double max_next_q = 0.0;
            if (next_state < (int)q_table_.size()) {
                for (double q : q_table_[next_state]) max_next_q = std::max(max_next_q, q);
            }

            if (state < (int)q_table_.size() && action < (int)q_table_[state].size()) {
                q_table_[state][action] += learning_rate *
                    (next_reward + discount_factor * max_next_q - q_table_[state][action]);
            }

            RLObservation obs;
            obs.formation_type_id = static_cast<int>(next_f);
            obs.num_crossbows = effective_crossbows;
            obs.target_center_x = target_.center_x_m;
            obs.target_center_y = target_.center_y_m;
            obs.target_size = target_.width_m;
            obs.avg_distance = next_dist;
            obs.current_hit_rate = next_engagement.overall_hit_probability;
            obs.current_coverage = next_engagement.target_coverage_percent;
            obs.wind_speed = std::sqrt(wind_x_*wind_x_ + wind_y_*wind_y_);
            obs.wind_direction = std::atan2(wind_y_, wind_x_) * 180.0 / M_PI;
            obs.action_taken = static_cast<double>(action);
            obs.reward = next_reward;
            obs.step = train_steps++;
            result.training_trajectory.push_back(obs);

            current_f = next_f;
            spacing = next_spacing;
            orientation = next_orientation;
            state = next_state;
            reward = next_reward;

            if (next_reward > best_reward) {
                best_reward = next_reward;
                best_formation = next_f;
                best_params = {next_spacing, next_orientation};
            }
        }

        if (ep == 0) moving_avg = reward;
        else moving_avg = 0.95 * moving_avg + 0.05 * reward;
        result.episode_rewards.emplace_back(ep, moving_avg);
        current_eps *= exploration_decay;

        if (ep > episodes * 0.3 && current_eps < 0.05) {
            if (std::abs(reward - moving_avg) < 0.01) converged_count++;
            else converged_count = 0;
            if (converged_count > 100) break;
        }
    }

    for (auto f : all_formations) {
        auto placements = simulator_.generate_formation(
            f, num_crossbows, 0.0, 0.0, 0.0, 1.0
        );
        auto eng = simulator_.simulate_engagement(
            placements, target_, 3, wind_x_, wind_y_, visibility_km_
        );
        double v = evaluate_reward(eng);
        formation_value_function_[static_cast<int>(f)] = v;
        result.formation_value_estimates.emplace_back(f, v);
    }

    result.best_formation = best_formation;
    result.best_placement_params = best_params;
    result.best_reward = best_reward;
    result.episodes_trained = static_cast<int>(result.episode_rewards.size());
    result.convergence_rate = result.episodes_trained > 10 ?
        std::max(0.0, 1.0 - std::abs(result.episode_rewards.back().second -
                                       result.episode_rewards.front().second) / 0.5) : 0.0;

    LOG_INFO("Q-learning training complete: episodes={}, best_formation={}, best_reward={:.4f}",
             result.episodes_trained, static_cast<int>(best_formation), best_reward);
    return result;
}

RLOptimizationResult FormationRLOptimizer::train_policy_gradient(
    int num_crossbows, int episodes, int steps_per_episode,
    double learning_rate, double baseline_weight
) {
    return train_q_learning(num_crossbows, episodes, learning_rate * 0.8, 0.9, 0.15, 0.99);
}

std::vector<CrossbowPlacement> FormationRLOptimizer::apply_optimal_policy(
    const RLOptimizationResult& result,
    double center_x, double center_y, double orientation
) {
    double spacing = result.best_placement_params.size() > 0 ? result.best_placement_params[0] : 1.0;
    double extra_orientation = result.best_placement_params.size() > 1 ? result.best_placement_params[1] : 0.0;
    return simulator_.generate_formation(
        result.best_formation, (min_crossbows_ + max_crossbows_) / 2,
        center_x, center_y, orientation + extra_orientation, spacing
    );
}

} // namespace crossbow
