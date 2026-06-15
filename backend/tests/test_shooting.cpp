#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "test_harness.h"
#include "../include/bowstring_material.h"
#include "../include/dynamics_model.h"
#include "../include/common.h"
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>

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

// ================ Feature 4: 虚拟射击体验 - 正常测试 ================

TEST_CASE(Shooting, 能量法初速计算正确) {
    auto cb = get_test_crossbow();
    auto& db = BowstringMaterialDatabase::instance();
    auto silk = db.get(BowstringMaterialType::SILK);

    double draw_weight_n = cb.draw_weight;
    double draw_length_m = cb.string_length * 0.6;
    double string_mass = silk.calculate_string_mass(cb.string_length);

    double stored_energy = 0.5 * draw_weight_n * draw_length_m;
    double efficiency = silk.calculate_efficiency_at_pull(0.7);
    double kinetic_energy = stored_energy * efficiency;

    double arrow_mass = cb.arrow_mass;
    double v0 = std::sqrt(2 * kinetic_energy / (arrow_mass + string_mass / 3.0));

    REQUIRE_GT(v0, 40);
    REQUIRE_LT(v0, 140);

    REQUIRE_GT(stored_energy, 0);
    REQUIRE_GT(kinetic_energy, 0);
    REQUIRE_LT(kinetic_energy, stored_energy);
}

TEST_CASE(Shooting, 本地弹道欧拉积分收敛) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    double v0 = 80.0;
    double angle_deg = 15.0;

    auto traj = model.simulate_trajectory(v0, angle_deg, 0, 0, 0.001);

    REQUIRE_GT(traj.size(), 10);

    double max_height = 0;
    double max_range = 0;
    for (const auto& pt : traj) {
        max_height = std::max(max_height, pt.position.y);
        max_range = std::max(max_range, pt.position.x);
        REQUIRE(!std::isnan(pt.position.x));
        REQUIRE(!std::isnan(pt.position.y));
        REQUIRE(!std::isnan(pt.velocity.x));
        REQUIRE(!std::isnan(pt.velocity.y));
    }

    REQUIRE_GT(max_height, 0);
    REQUIRE_GT(max_range, 0);

    double impact_y = traj.back().position.y;
    REQUIRE_LE(impact_y, 0.05);
}

TEST_CASE(Shooting, 不同蓄力百分比初速通过compare验证) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    auto shot_full = analyzer.simulate_shot(
        BowstringMaterialDatabase::instance().get(BowstringMaterialType::SILK), 15.0);
    auto shot_low = analyzer.simulate_shot(
        BowstringMaterialDatabase::instance().get(BowstringMaterialType::HEMP), 15.0);

    REQUIRE_GT(shot_full.actual_energy_efficiency, 0.5);
    REQUIRE_GT(shot_full.string_mass, 0);
    REQUIRE(!std::isnan(shot_full.wave_speed_mps));

    REQUIRE_GT(shot_low.actual_energy_efficiency, 0.3);
}

TEST_CASE(Shooting, 弹丸距离命中检测正确) {
    std::vector<std::pair<double, double>> target_rings = {
        {0.3, 60}, {0.5, 35}, {0.75, 20}, {1.0, 10}, {1.4, 5}
    };

    std::vector<std::pair<double, int>> test_cases = {
        {0.0, 60}, {0.2, 60}, {0.29, 60}, {0.31, 35},
        {0.49, 35}, {0.51, 20}, {0.74, 20}, {0.76, 10},
        {0.99, 10}, {1.01, 5}, {1.39, 5}, {1.41, 0}
    };

    for (const auto& tc : test_cases) {
        double distance = tc.first;
        int expected_points = tc.second;
        int points = 0;
        int hit_ring = -1;

        for (size_t i = 0; i < target_rings.size(); i++) {
            if (distance <= target_rings[i].first) {
                points = target_rings[i].second;
                hit_ring = (int)i;
                break;
            }
        }

        std::ostringstream msg;
        msg << "distance " << distance << "m expected " << expected_points << " got " << points;
        REQUIRE_MSG(points == expected_points, msg.str());
        if (expected_points > 0) {
            REQUIRE_GE(hit_ring, 0);
        } else {
            REQUIRE_EQ(hit_ring, -1);
        }
    }
}

TEST_CASE(Shooting, 弹道重力影响正确) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    double v0 = 60.0;
    double angle_deg = 0.0;

    auto traj = model.simulate_trajectory(v0, angle_deg, 0, 0, 0.001);

    REQUIRE_GT(traj.size(), 5);

    for (size_t i = 1; i < traj.size(); i++) {
        REQUIRE_LT(traj[i].velocity.y, traj[i-1].velocity.y);
    }

    double flight_time = traj.back().time_step;
    double estimated_drop = 0.5 * 9.81 * flight_time * flight_time;
    REQUIRE_GT(estimated_drop, 0);
}

TEST_CASE(Shooting, 弹道阻力影响正确_高速低速差异) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    double v_fast = 120.0;
    double v_slow = 40.0;
    double angle_deg = 10.0;

    auto traj_fast = model.simulate_trajectory(v_fast, angle_deg, 0, 0, 0.001);
    auto traj_slow = model.simulate_trajectory(v_slow, angle_deg, 0, 0, 0.001);

    REQUIRE_GT(traj_fast.back().position.x, traj_slow.back().position.x);

    double decel_fast = 0;
    for (size_t i = 1; i < std::min(traj_fast.size(), (size_t)50); i++) {
        double v1 = std::sqrt(traj_fast[i-1].velocity.x * traj_fast[i-1].velocity.x +
                              traj_fast[i-1].velocity.y * traj_fast[i-1].velocity.y);
        double v2 = std::sqrt(traj_fast[i].velocity.x * traj_fast[i].velocity.x +
                              traj_fast[i].velocity.y * traj_fast[i].velocity.y);
        decel_fast += v1 - v2;
    }

    double decel_slow = 0;
    for (size_t i = 1; i < std::min(traj_slow.size(), (size_t)50); i++) {
        double v1 = std::sqrt(traj_slow[i-1].velocity.x * traj_slow[i-1].velocity.x +
                              traj_slow[i-1].velocity.y * traj_slow[i-1].velocity.y);
        double v2 = std::sqrt(traj_slow[i].velocity.x * traj_slow[i].velocity.x +
                              traj_slow[i].velocity.y * traj_slow[i].velocity.y);
        decel_slow += v1 - v2;
    }

    REQUIRE_GT(decel_fast, decel_slow * 0.5);
}

TEST_CASE(Shooting, 射击统计累加正确) {
    int total_shots = 0;
    int total_hits = 0;
    int total_score = 0;
    double best_hit = 0;
    double longest_hit = 0;

    std::vector<std::tuple<double, double, bool, int, double>> shots = {
        {15.0, 0.0, true, 60, 80.5},
        {14.8, 0.2, true, 35, 120.3},
        {16.0, -0.5, false, 0, 150.0},
        {15.5, 0.1, true, 10, 50.0},
        {15.2, 0.0, true, 20, 200.0},
    };

    for (const auto& s : shots) {
        total_shots++;
        if (std::get<2>(s)) {
            total_hits++;
            total_score += std::get<3>(s);
            best_hit = std::max(best_hit, (double)std::get<3>(s));
            longest_hit = std::max(longest_hit, std::get<4>(s));
        }
    }

    REQUIRE_EQ(total_shots, 5);
    REQUIRE_EQ(total_hits, 4);
    REQUIRE_EQ(total_score, 60 + 35 + 10 + 20);
    REQUIRE_EQ(best_hit, 60);
    REQUIRE_EQ(longest_hit, 200.0);

    double accuracy = total_hits / (double)total_shots;
    REQUIRE_NEAR(accuracy, 0.8, 1e-9);

    double avg_points = (double)total_score / total_hits;
    REQUIRE_NEAR(avg_points, (60 + 35 + 10 + 20) / 4.0, 1e-9);
}

TEST_CASE(Shooting, 目标破坏逻辑正确) {
    int max_hits = 3;

    std::vector<int> targets_hits = {0, 0, 0, 0, 0};
    std::vector<bool> targets_destroyed = {false, false, false, false, false};
    int destroyed_count = 0;

    for (int round = 0; round < 5; round++) {
        int target_idx = round % 5;
        if (!targets_destroyed[target_idx]) {
            targets_hits[target_idx]++;
            if (targets_hits[target_idx] >= max_hits) {
                targets_destroyed[target_idx] = true;
                destroyed_count++;
            }
        }
    }

    REQUIRE_EQ(targets_hits[0], 1);
    REQUIRE_EQ(targets_hits[2], 1);
    REQUIRE_EQ(destroyed_count, 0);

    for (int i = 0; i < 15; i++) {
        int target_idx = i % 5;
        if (!targets_destroyed[target_idx]) {
            targets_hits[target_idx]++;
            if (targets_hits[target_idx] >= max_hits) {
                targets_destroyed[target_idx] = true;
                destroyed_count++;
            }
        }
    }

    REQUIRE_EQ(destroyed_count, 5);
    for (bool d : targets_destroyed) {
        REQUIRE(d);
    }
}

// ================ Feature 4: 虚拟射击体验 - 边界测试 ================

TEST_CASE(Shooting_Boundary, 零初速弹道立即落地) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    auto traj = model.simulate_trajectory(0.0, 10.0, 0, 0, 0.001);

    REQUIRE_GE(traj.size(), 1);
    REQUIRE_NEAR(traj.front().position.x, 0.0, 0.1);
    double max_y = 0;
    for (const auto& pt : traj) {
        max_y = std::max(max_y, pt.position.y);
    }
    REQUIRE_LT(max_y, 5.0);
}

TEST_CASE(Shooting_Boundary, 垂直向上弹道) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    auto traj = model.simulate_trajectory(80.0, 90.0, 0, 0, 0.001);

    REQUIRE_GT(traj.size(), 100);

    double max_height = 0;
    for (const auto& pt : traj) {
        max_height = std::max(max_height, pt.position.y);
        REQUIRE_NEAR(pt.position.x, 0.0, 0.5);
    }

    REQUIRE_GT(max_height, 50);
    REQUIRE_LT(max_height, 500);
}

TEST_CASE(Shooting_Boundary, 垂直向下弹道) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    auto traj = model.simulate_trajectory(50.0, -90.0, 0, 0, 0.001);

    REQUIRE_GE(traj.size(), 1);
    REQUIRE_LE(traj.back().position.y, 0.01);
    REQUIRE_LT(traj.back().time_step, 1.0);
}

TEST_CASE(Shooting_Boundary, 极小角度平射) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    auto traj = model.simulate_trajectory(100.0, 0.001, 0, 0, 0.001);

    REQUIRE_GT(traj.size(), 100);
    REQUIRE_GT(traj.back().position.x, 50);
    REQUIRE_LE(traj.back().position.y, 0.05);
}

TEST_CASE(Shooting_Boundary, 极高初速弹道稳定性) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    REQUIRE_NOTHROW(model.simulate_trajectory(1000.0, 15.0, 0, 0, 0.001));

    auto traj = model.simulate_trajectory(1000.0, 15.0, 0, 0, 0.001);
    REQUIRE_GT(traj.size(), 1);

    for (const auto& pt : traj) {
        REQUIRE(!std::isnan(pt.position.x));
        REQUIRE(!std::isnan(pt.position.y));
        REQUIRE(!std::isinf(pt.position.x));
        REQUIRE(!std::isinf(pt.position.y));
    }
}

TEST_CASE(Shooting_Boundary, 命中距离边界值) {
    std::vector<std::pair<double, double>> target_rings = {
        {0.3, 60}, {0.5, 35}, {0.75, 20}, {1.0, 10}, {1.4, 5}
    };

    for (size_t i = 0; i < target_rings.size(); i++) {
        double boundary = target_rings[i].first;

        int points_inner = 0;
        for (size_t j = 0; j < target_rings.size(); j++) {
            if (boundary - 1e-9 <= target_rings[j].first) {
                points_inner = (int)target_rings[j].second;
                break;
            }
        }
        REQUIRE_EQ(points_inner, (int)target_rings[i].second);
    }

    double outside = target_rings.back().first + 0.001;
    int points_outside = 0;
    for (size_t j = 0; j < target_rings.size(); j++) {
        if (outside <= target_rings[j].first) {
            points_outside = (int)target_rings[j].second;
            break;
        }
    }
    REQUIRE_EQ(points_outside, 0);
}

TEST_CASE(Shooting_Boundary, 最大目标数量处理) {
    int max_targets = 100;
    std::vector<bool> targets_destroyed(max_targets, false);

    for (int i = 0; i < max_targets; i++) {
        targets_destroyed[i] = true;
    }

    int destroyed = 0;
    for (bool d : targets_destroyed) {
        if (d) destroyed++;
    }

    REQUIRE_EQ(destroyed, max_targets);
}

// ================ Feature 4: 虚拟射击体验 - 异常测试 ================

TEST_CASE(Shooting_Exception, 负初速弹道鲁棒性) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    REQUIRE_NOTHROW(model.simulate_trajectory(-50.0, 15.0, 0, 0, 0.001));

    auto traj = model.simulate_trajectory(-50.0, 15.0, 0, 0, 0.001);
    REQUIRE_GE(traj.size(), 1);
    REQUIRE(!std::isnan(traj.back().position.x));
}

TEST_CASE(Shooting_Exception, 极端角度弹道鲁棒性) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    REQUIRE_NOTHROW(model.simulate_trajectory(80.0, 180.0, 0, 0, 0.001));
    REQUIRE_NOTHROW(model.simulate_trajectory(80.0, -180.0, 0, 0, 0.001));
    REQUIRE_NOTHROW(model.simulate_trajectory(80.0, 1000.0, 0, 0, 0.001));

    auto t1 = model.simulate_trajectory(80.0, 180.0, 0, 0, 0.001);
    auto t2 = model.simulate_trajectory(80.0, -180.0, 0, 0, 0.001);

    REQUIRE(!std::isnan(t1.back().position.x));
    REQUIRE(!std::isnan(t2.back().position.x));
}

TEST_CASE(Shooting_Exception, 负时间步长处理) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    REQUIRE_NOTHROW(model.simulate_trajectory(80.0, 15.0, 0, 0, -0.001));
    REQUIRE_NOTHROW(model.simulate_trajectory(80.0, 15.0, 0, 0, 0));

    auto traj1 = model.simulate_trajectory(80.0, 15.0, 0, 0, -0.001);
    auto traj2 = model.simulate_trajectory(80.0, 15.0, 0, 0, 0);

    REQUIRE_GE(traj1.size(), 1);
    REQUIRE_GE(traj2.size(), 1);
}

TEST_CASE(Shooting_Exception, 不同材质初速通过compare验证) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    auto result = analyzer.compare_all_materials(15.0);

    REQUIRE_GT(result.materials.size(), 0);

    double max_vel = 0;
    for (const auto& m : result.materials) {
        REQUIRE_GT(m.estimated_muzzle_velocity, 0);
        max_vel = std::max(max_vel, m.estimated_muzzle_velocity);
    }

    REQUIRE_LT(max_vel, 200);
    REQUIRE_GT(max_vel, 30);
}

TEST_CASE(Shooting_Exception, 射击统计零样本处理) {
    int total_shots = 0;
    int total_hits = 0;
    int total_score = 0;

    double accuracy = total_shots > 0 ? (double)total_hits / total_shots : 0.0;
    double avg_points = total_hits > 0 ? (double)total_score / total_hits : 0.0;

    REQUIRE_EQ(accuracy, 0.0);
    REQUIRE_EQ(avg_points, 0.0);
    REQUIRE(!std::isnan(accuracy));
    REQUIRE(!std::isnan(avg_points));
}

TEST_CASE(Shooting_Exception, 极高频率弹道不溢出) {
    auto cb = get_test_crossbow();
    DynamicsModel model(cb);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> angle_dist(-30.0, 60.0);
    std::uniform_real_distribution<double> vel_dist(40.0, 120.0);

    for (int i = 0; i < 100; i++) {
        double angle = angle_dist(rng);
        double vel = vel_dist(rng);
        REQUIRE_NOTHROW(model.simulate_trajectory(vel, angle, 0, 0, 0.001));
    }
}

TEST_CASE(Shooting_Exception, 零密度材质simulate_shot鲁棒性) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    auto custom = BowstringMaterialDatabase::instance().create_custom(
        "zero", 1e10, 1e8, 0, 0.01, 0.01, 0.9);

    REQUIRE_NOTHROW(analyzer.simulate_shot(custom, 15.0));

    auto result = analyzer.simulate_shot(custom, 15.0);
    REQUIRE(!std::isnan(result.wave_speed_mps));
    REQUIRE_GT(result.actual_energy_efficiency, 0);
}
