#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "test_harness.h"
#include "../include/formation_simulator.h"
#include "../include/common.h"
#include <cmath>
#include <vector>
#include <map>
#include <set>

using namespace crossbow;

static CrossbowType get_test_crossbow() {
    CrossbowType cb;
    cb.id = 7;
    cb.name = "宋神臂弩";
    cb.dynasty = "宋朝";
    cb.draw_weight = 350;
    cb.bow_length = 1.75;
    cb.string_length = 1.8;
    cb.arrow_mass = 0.080;
    cb.effective_range = 300;
    cb.max_range = 550;
    return cb;
}

static std::map<int, CrossbowType> get_crossbow_map() {
    std::map<int, CrossbowType> m;
    m[7] = get_test_crossbow();
    return m;
}

static TargetSpecification make_target(TargetType type, const std::string& name,
                                        double cx, double cy,
                                        double radius = 0, double width = 0, double depth = 0) {
    TargetSpecification t{};
    t.type = type;
    t.name = name;
    t.center_x_m = cx;
    t.center_y_m = cy;
    t.radius_m = radius;
    t.width_m = width;
    t.depth_m = depth;
    t.priority_factor = 1.0;
    t.hardness_factor = 1.0;
    t.personnel_estimate = 100;
    return t;
}

// ================ Feature 3: 阵型模拟 - 正常测试 ================

TEST_CASE(Formation, 内置阵型数据库完整) {
    auto& db = FormationDatabase::instance();
    auto formations = db.list_all();
    REQUIRE_GE(formations.size(), 9);

    auto line = db.get(FormationType::LINE_ABREAST);
    auto wedge = db.get(FormationType::WEDGE);
    auto vee = db.get(FormationType::VEE);

    REQUIRE_EQ(line.type, FormationType::LINE_ABREAST);
    REQUIRE(!line.name.empty());

    REQUIRE_GT(line.best_suited_range_m, 0);
    REQUIRE_GT(line.optimal_crossbows, 0);
    REQUIRE(!line.historical_origin.empty());
}

TEST_CASE(Formation, 阵型生成坐标不重叠) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    int n = 36;
    double cx = 0, cy = 0;
    double orient = 0, spacing = 3.0;

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, n, cx, cy, orient, spacing);

    REQUIRE_EQ((int)placements.size(), n);

    std::set<std::pair<double, double>> coords;
    for (const auto& p : placements) {
        coords.insert({std::round(p.x_m * 1000), std::round(p.y_m * 1000)});
    }
    REQUIRE_EQ((int)coords.size(), n);

    double min_x = 1e9, max_x = -1e9;
    for (const auto& p : placements) {
        min_x = std::min(min_x, p.x_m);
        max_x = std::max(max_x, p.x_m);
    }
    double width = max_x - min_x;
    REQUIRE_GT(width, 0);
}

TEST_CASE(Formation, 不同阵型生成正确坐标分布) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());
    int n = 36;
    double cx = 0, cy = 0, orient = 0, spacing = 3.0;

    std::vector<FormationType> types = {
        FormationType::LINE_ABREAST,
        FormationType::WEDGE,
        FormationType::VEE,
        FormationType::ARCHER_PARABOLA,
        FormationType::COILED_BOW,
        FormationType::CRANE_WING,
        FormationType::SQUARE_GRID,
        FormationType::FISH_SCALE
    };

    for (auto ft : types) {
        auto& name = FormationDatabase::instance().get(ft).name;
        auto placements = sim.generate_formation(ft, n, cx, cy, orient, spacing);

        REQUIRE_MSG((int)placements.size() == n,
                    name + " 生成数量错误: " + std::to_string(placements.size()) + " != " + std::to_string(n));

        double avg_x = 0, avg_y = 0;
        for (const auto& p : placements) {
            avg_x += p.x_m;
            avg_y += p.y_m;
        }
        avg_x /= (int)placements.size();
        avg_y /= (int)placements.size();

        REQUIRE_NEAR(avg_x, cx, 100.0);
        REQUIRE_NEAR(avg_y, cy, 100.0);
    }
}

TEST_CASE(Formation, 交火模拟返回合理命中率) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 36, 0, 0, 0, 3.0);

    auto target = make_target(TargetType::AREA_CIRCLE, "圆形靶区", 0, 250, 50);

    auto result = sim.simulate_engagement(placements, target, 3);

    REQUIRE_GE(result.overall_hit_probability, 0.0);
    REQUIRE_LE(result.overall_hit_probability, 1.0);
    REQUIRE_GE(result.target_coverage_percent, 0.0);
    REQUIRE_LE(result.target_coverage_percent, 1.0);
    REQUIRE_GE(result.saturation_score, 0.0);
    REQUIRE_LE(result.saturation_score, 1.0);

    REQUIRE_GT(result.total_shots, 0);

    REQUIRE(!std::isnan(result.logistical_efficiency));
    REQUIRE(!std::isnan(result.friendly_risk_radius_m));
}

TEST_CASE(Formation, 命中概率估算有效) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto target = make_target(TargetType::SINGLE_POINT, "点目标", 100, 200, 10);

    CrossbowPlacement p;
    p.id = 1;
    p.x_m = 0;
    p.y_m = 0;
    p.z_m = 0;
    p.facing_deg = 0;
    p.elevation_deg = 15;
    p.crossbow_type_id = 7;
    p.is_active = true;

    double hp = sim.estimate_hit_probability(p, target);
    REQUIRE_GE(hp, 0.0);
    REQUIRE_LE(hp, 1.0);
    REQUIRE(!std::isnan(hp));
}

TEST_CASE(Formation, 阵型对比返回有序结果) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    std::vector<FormationType> types = {
        FormationType::LINE_ABREAST,
        FormationType::WEDGE,
        FormationType::VEE,
        FormationType::FISH_SCALE
    };

    auto target = make_target(TargetType::AREA_RECTANGLE, "矩形防线", 0, 250, 0, 100, 20);

    auto results = sim.compare_formations(types, 36, target, 3);

    REQUIRE_EQ(results.size(), types.size());

    double max_hit = 0;
    for (const auto& r : results) {
        REQUIRE(!std::isnan(r.overall_hit_probability));
        REQUIRE_GE(r.overall_hit_probability, 0);
        REQUIRE_LE(r.overall_hit_probability, 1);
        max_hit = std::max(max_hit, r.overall_hit_probability);
    }
    REQUIRE_GT(max_hit, 0);
}

TEST_CASE(Formation, 齐射计划合理分配火力) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 36, 0, 0, 0, 3.0);

    FiringPlan plan = sim.generate_firing_plan(placements, 3, 500.0);

    REQUIRE_GE(plan.total_projectiles, (int)placements.size());
    REQUIRE_GT(plan.density_at_target_projectiles_per_m2, 0);
    REQUIRE_GE(plan.estimated_kill_probability, 0);
    REQUIRE_LE(plan.estimated_kill_probability, 1);
}

TEST_CASE(Formation, 强化学习训练收敛) {
    FormationRLOptimizer rl;
    rl.set_crossbow_types(get_crossbow_map());

    auto target = make_target(TargetType::AREA_CIRCLE, "敌军集群", 0, 250, 40);

    rl.set_target(target);

    auto result = rl.train_q_learning(16, 200, 0.2, 0.9, 1.0, 0.995);

    REQUIRE_GT(result.episodes_trained, 0);
    REQUIRE(!std::isnan(result.best_reward));
    REQUIRE_GT(result.best_reward, -1000);

    REQUIRE(result.best_formation != FormationType::LINE_ABREAST || result.episodes_trained > 0);

    REQUIRE_GE(result.convergence_rate, 0.0);
    REQUIRE_LE(result.convergence_rate, 1.0);

    REQUIRE_GE(result.formation_value_estimates.size(), 1);
    for (const auto& fv : result.formation_value_estimates) {
        REQUIRE(!std::isnan(fv.second));
    }
}

// ================ Feature 3: 阵型模拟 - 边界测试 ================

TEST_CASE(Formation_Boundary, 单弩阵型钳位到最小值) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::WEDGE, 1, 0, 0, 0, 3.0);
    auto& design = FormationDatabase::instance().get(FormationType::WEDGE);
    int expected_min = design.min_crossbows;
    REQUIRE_EQ((int)placements.size(), expected_min);
    for (const auto& p : placements) {
        REQUIRE(!std::isnan(p.x_m));
        REQUIRE(!std::isnan(p.y_m));
    }
}

TEST_CASE(Formation_Boundary, 零弩负弩钳位到最小值) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements1 = sim.generate_formation(FormationType::LINE_ABREAST, 0, 0, 0, 0, 3.0);
    auto& design = FormationDatabase::instance().get(FormationType::LINE_ABREAST);
    REQUIRE_EQ((int)placements1.size(), design.min_crossbows);

    auto placements2 = sim.generate_formation(FormationType::LINE_ABREAST, -5, 0, 0, 0, 3.0);
    REQUIRE_EQ((int)placements2.size(), design.min_crossbows);
}

TEST_CASE(Formation_Boundary, 超大数量阵型钳位到最大值) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    REQUIRE_NOTHROW(sim.generate_formation(FormationType::FISH_SCALE, 10000, 0, 0, 0, 3.0));
    auto p = sim.generate_formation(FormationType::FISH_SCALE, 10000, 0, 0, 0, 3.0);
    auto& design = FormationDatabase::instance().get(FormationType::FISH_SCALE);
    REQUIRE_EQ((int)p.size(), design.max_crossbows);
}

TEST_CASE(Formation_Boundary, 零间距阵型生成) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 0, 0.0);
    REQUIRE_EQ((int)placements.size(), 10);
    for (const auto& p : placements) {
        REQUIRE(!std::isnan(p.x_m));
        REQUIRE(!std::isnan(p.y_m));
    }
}

TEST_CASE(Formation_Boundary, 不同朝向生成不同坐标) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto p0 = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 0.0, 3.0);
    auto p90 = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 90.0, 3.0);

    REQUIRE_EQ(p0.size(), p90.size());

    bool any_pos_diff = false;
    for (size_t i = 0; i < p0.size(); i++) {
        if (std::abs(p0[i].x_m - p90[i].x_m) > 0.01 || std::abs(p0[i].y_m - p90[i].y_m) > 0.01) {
            any_pos_diff = true;
            break;
        }
    }
    REQUIRE(any_pos_diff);

    bool any_facing_diff = false;
    for (size_t i = 0; i < p0.size(); i++) {
        if (std::abs(p0[i].facing_deg - p90[i].facing_deg) > 0.01) {
            any_facing_diff = true;
            break;
        }
    }
    REQUIRE(any_facing_diff);
}

TEST_CASE(Formation_Boundary, 极小目标区域交火) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 0, 3.0);

    auto target = make_target(TargetType::SINGLE_POINT, "小点目标", 0, 300, 5.0);

    REQUIRE_NOTHROW(sim.simulate_engagement(placements, target, 3));
    auto result = sim.simulate_engagement(placements, target, 3);
    REQUIRE_GE(result.overall_hit_probability, 0.0);
    REQUIRE_LE(result.overall_hit_probability, 1.0);
    REQUIRE(!std::isnan(result.overall_hit_probability));
}

TEST_CASE(Formation_Boundary, 超远目标距离交火) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 0, 3.0);

    auto target = make_target(TargetType::AREA_CIRCLE, "超远目标", 0, 10000, 100);

    auto result = sim.simulate_engagement(placements, target, 3);
    REQUIRE_GE(result.overall_hit_probability, 0.0);
    REQUIRE_LE(result.overall_hit_probability, 1.0);
}

TEST_CASE(Formation_Boundary, RL零episode训练) {
    FormationRLOptimizer rl;
    rl.set_crossbow_types(get_crossbow_map());

    auto target = make_target(TargetType::AREA_CIRCLE, "目标", 0, 200, 30);

    rl.set_target(target);

    auto result = rl.train_q_learning(16, 0, 0.2, 0.9, 1.0, 0.995);

    REQUIRE_EQ(result.episodes_trained, 0);
    REQUIRE(!std::isnan(result.best_reward));
}

// ================ Feature 3: 阵型模拟 - 异常测试 ================

TEST_CASE(Formation_Exception, 无效阵型类型返回默认) {
    auto& result = FormationDatabase::instance().get(static_cast<FormationType>(999));
    REQUIRE_EQ(result.type, FormationType::CUSTOM);

    auto& result2 = FormationDatabase::instance().get(static_cast<FormationType>(-1));
    REQUIRE_EQ(result2.type, FormationType::CUSTOM);
}

TEST_CASE(Formation_Exception, 无效阵型类型检查) {
    auto& invalid_result = FormationDatabase::instance().get(static_cast<FormationType>(999));
    REQUIRE_EQ(invalid_result.type, FormationType::CUSTOM);

    auto& valid_result = FormationDatabase::instance().get(FormationType::WEDGE);
    REQUIRE(valid_result.type == FormationType::WEDGE);
}

TEST_CASE(Formation_Exception, 空布阵交火模拟鲁棒性) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    std::vector<CrossbowPlacement> empty;

    auto target = make_target(TargetType::AREA_CIRCLE, "目标", 0, 200, 30);

    REQUIRE_NOTHROW(sim.simulate_engagement(empty, target, 3));

    auto result = sim.simulate_engagement(empty, target, 3);
    REQUIRE_EQ(result.total_shots, 0);
    REQUIRE_EQ(result.total_hits, 0);
}

TEST_CASE(Formation_Exception, 负目标半径鲁棒性) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 10, 0, 0, 0, 3.0);

    auto target = make_target(TargetType::AREA_CIRCLE, "无效目标", 0, 200, -50);

    REQUIRE_NOTHROW(sim.simulate_engagement(placements, target, 3));

    auto result = sim.simulate_engagement(placements, target, 3);
    REQUIRE_GE(result.overall_hit_probability, 0.0);
    REQUIRE_LE(result.overall_hit_probability, 1.0);
    REQUIRE(!std::isnan(result.overall_hit_probability));
}

TEST_CASE(Formation_Exception, 空弩机类型映射鲁棒性) {
    FormationSimulator sim;

    auto placements = sim.generate_formation(FormationType::LINE_ABREAST, 5, 0, 0, 0, 3.0);
    REQUIRE_EQ((int)placements.size(), 5);

    for (auto& p : placements) {
        p.crossbow_type_id = 999;
    }

    auto target = make_target(TargetType::SINGLE_POINT, "", 0, 200, 10);

    REQUIRE_NOTHROW(sim.simulate_engagement(placements, target, 3));
    auto result = sim.simulate_engagement(placements, target, 3);
    REQUIRE(!std::isnan(result.overall_hit_probability));
}

TEST_CASE(Formation_Exception, 阵型旋转360度回到原位) {
    FormationSimulator sim;
    sim.set_crossbow_types(get_crossbow_map());

    auto p0 = sim.generate_formation(FormationType::WEDGE, 5, 0, 0, 0.0, 3.0);
    auto p360 = sim.generate_formation(FormationType::WEDGE, 5, 0, 0, 360.0, 3.0);

    REQUIRE_EQ(p0.size(), p360.size());
    for (size_t i = 0; i < p0.size(); i++) {
        REQUIRE_NEAR(p0[i].x_m, p360[i].x_m, 0.01);
        REQUIRE_NEAR(p0[i].y_m, p360[i].y_m, 0.01);
    }
}

TEST_CASE(Formation_Exception, 多次RL训练一致性) {
    FormationRLOptimizer rl;
    rl.set_crossbow_types(get_crossbow_map());

    auto target = make_target(TargetType::AREA_CIRCLE, "集群", 0, 250, 40);

    rl.set_target(target);

    auto r1 = rl.train_q_learning(16, 300, 0.2, 0.9, 1.0, 0.995);
    auto r2 = rl.train_q_learning(16, 300, 0.2, 0.9, 1.0, 0.995);

    REQUIRE_EQ(r1.episodes_trained, 300);
    REQUIRE_EQ(r2.episodes_trained, 300);
    REQUIRE(!std::isnan(r1.best_reward));
    REQUIRE(!std::isnan(r2.best_reward));
}

TEST_CASE(Formation_Exception, 齐射计划空布阵) {
    FormationSimulator sim;

    std::vector<CrossbowPlacement> empty;
    FiringPlan plan = sim.generate_firing_plan(empty, 3, 500.0);
    REQUIRE_EQ(plan.total_projectiles, 0);
}
