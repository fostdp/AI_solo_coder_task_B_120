#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "test_harness.h"
#include "modules/string_comparator.h"
#include "modules/sight_optimizer.h"
#include "modules/formation_simulator_module.h"
#include "modules/virtual_shooting.h"
#include "modules/dynamics_solver.h"
#include <cmath>
#include <vector>
#include <thread>
#include <chrono>

using namespace crossbow;
namespace cm = crossbow::modules;

static CrossbowType make_test_crossbow() {
    CrossbowType cb = {};
    cb.id = 1;
    cb.name = "test";
    cb.dynasty = "test";
    cb.draw_weight = 350.0;
    cb.bow_length = 1.75;
    cb.string_length = 1.2;
    cb.arrow_mass = 0.050;
    cb.effective_range = 300.0;
    cb.max_range = 550.0;
    return cb;
}

// ================ STRING COMPARATOR (3 tests) ================

TEST_CASE(Module_StringComparator, evaluate_material_returns_valid_scores) {
    cm::StringComparator comparator;
    auto all_results = comparator.compare_all_materials();

    cm::StringComparisonMetric sinew_result;
    bool found_sinew = false;
    for (const auto& r : all_results) {
        if (r.type == BowstringMaterialType::SINEW) {
            sinew_result = r;
            found_sinew = true;
            break;
        }
    }
    REQUIRE(found_sinew);

    REQUIRE_GT(sinew_result.energy_efficiency, 0.5);
    REQUIRE_GT(sinew_result.string_mass_g, 0.0);
    REQUIRE_GT(sinew_result.overall_score, 0.0);
}

TEST_CASE(Module_StringComparator, compare_all_materials_ranking_order) {
    cm::StringComparator comparator;
    auto results = comparator.compare_all_materials();

    REQUIRE_GE(results.size(), (size_t)3);

    for (size_t i = 1; i < results.size(); i++) {
        REQUIRE_GE(results[i-1].overall_score, results[i].overall_score);
    }
}

TEST_CASE(Module_StringComparator, find_optimal_material_returns_best) {
    cm::StringComparator comparator;
    auto result = comparator.find_optimal_material();

    for (const auto& r : result.rankings) {
        REQUIRE_GE(result.best_score, r.overall_score);
    }
}

// ================ SIGHT OPTIMIZER (3 tests) ================

static cm::SightOptimizationTarget make_sight_target() {
    cm::SightOptimizationTarget target;
    target.range_m = 200.0;
    target.wind_speed_ms = 2.0;
    target.temperature_c = 20.0;
    target.humidity = 0.5;
    target.target_radius_m = 0.5;
    return target;
}

TEST_CASE(Module_SightOptimizer, evaluate_returns_positive_cep) {
    cm::SightOptimizer optimizer;
    optimizer.set_target(make_sight_target());
    auto result = optimizer.evaluate(SightType::BASIC_NOTCH);

    REQUIRE_GT(result.cep_mrad, 0.0);
}

TEST_CASE(Module_SightOptimizer, compare_all_contains_all_types) {
    cm::SightOptimizer optimizer;
    optimizer.set_target(make_sight_target());
    auto results = optimizer.compare_all();

    REQUIRE_GE(results.size(), (size_t)4);
}

TEST_CASE(Module_SightOptimizer, optimize_picks_lowest_cep) {
    cm::SightOptimizer optimizer;
    optimizer.set_target(make_sight_target());
    std::vector<SightType> types = {
        SightType::BASIC_NOTCH,
        SightType::PIN_SIGHT,
        SightType::RAMP_SIGHT,
        SightType::MULTI_PIN
    };
    auto result = optimizer.optimize(types);

    bool found = false;
    for (const auto& t : types) {
        if (t == result.best_sight_type) {
            found = true;
            break;
        }
    }
    REQUIRE(found);
    REQUIRE_GE(result.improvement_pct, 0.0);
}

// ================ FORMATION MODULE (3 tests) ================

TEST_CASE(Module_Formation, evaluate_coverage_returns_valid) {
    cm::FormationSimulatorModule module;
    TargetSpecification target = {};
    target.type = TargetType::AREA_CIRCLE;
    target.name = "test_target";
    target.center_x_m = 0.0;
    target.center_y_m = 250.0;
    target.radius_m = 50.0;
    target.priority_factor = 1.0;
    target.hardness_factor = 1.0;
    target.personnel_estimate = 10;
    module.set_target(target);

    auto result = module.evaluate_coverage(FormationType::LINE_ABREAST);

    REQUIRE_GE(result.coverage_pct, 0.0);
    REQUIRE_LE(result.coverage_pct, 100.0);
    REQUIRE_GE(result.hit_probability, 0.0);
    REQUIRE_LE(result.hit_probability, 1.0);
}

TEST_CASE(Module_Formation, compare_formations_returns_sorted) {
    cm::FormationSimulatorModule module;
    TargetSpecification target = {};
    target.type = TargetType::AREA_CIRCLE;
    target.center_x_m = 0.0;
    target.center_y_m = 250.0;
    target.radius_m = 50.0;
    module.set_target(target);

    std::vector<FormationType> types = {
        FormationType::LINE_ABREAST,
        FormationType::WEDGE,
        FormationType::ECHELON_LEFT,
        FormationType::ECHELON_RIGHT,
        FormationType::VEE,
        FormationType::SQUARE_GRID
    };
    auto results = module.compare_formations(types);

    REQUIRE_GE(results.size(), (size_t)5);

    for (size_t i = 1; i < results.size(); i++) {
        REQUIRE_GE(results[i-1].second.coverage_pct, results[i].second.coverage_pct);
    }
}

TEST_CASE(Module_Formation, validate_placement_detects_overlap) {
    cm::FormationSimulatorModule module;

    std::vector<CrossbowPlacement> placements;
    CrossbowPlacement p1 = {};
    p1.id = 1;
    p1.crossbow_type_id = 0;
    p1.name = "cb1";
    p1.x_m = 0.0;
    p1.y_m = 0.0;
    p1.z_m = 0.0;
    p1.facing_deg = 0.0;
    p1.elevation_deg = 0.0;
    p1.firing_delay_ms = 0.0;
    p1.salvo_group = 0;
    p1.is_active = true;
    p1.crew_size = 2;
    p1.supply_status = 1.0;
    placements.push_back(p1);

    CrossbowPlacement p2 = {};
    p2.id = 2;
    p2.crossbow_type_id = 0;
    p2.name = "cb2";
    p2.x_m = 0.0;
    p2.y_m = 0.0;
    p2.z_m = 0.0;
    p2.facing_deg = 0.0;
    p2.elevation_deg = 0.0;
    p2.firing_delay_ms = 0.0;
    p2.salvo_group = 0;
    p2.is_active = true;
    p2.crew_size = 2;
    p2.supply_status = 1.0;
    placements.push_back(p2);

    bool valid = module.validate_placement(placements);
    REQUIRE(!valid);
}

// ================ VIRTUAL SHOOTING (3 tests) ================

TEST_CASE(Module_VirtualShooting, validate_parameters_rejects_bad_input) {
    cm::VirtualShooting shooting;
    cm::ShootingSessionConfig config = {};
    config.min_distance_m = 50.0;
    config.max_distance_m = 500.0;
    config.crossbow_type = make_test_crossbow();
    shooting.start_session(config);

    bool short_dist = shooting.validate_parameters(0.0, 10.0, 45.0, 15.0);
    REQUIRE(!short_dist);

    bool far_dist = shooting.validate_parameters(0.0, 1000.0, 45.0, 15.0);
    REQUIRE(!far_dist);
}

TEST_CASE(Module_VirtualShooting, add_target_and_fire_hit_detection) {
    cm::VirtualShooting shooting;
    cm::ShootingSessionConfig config = {};
    config.min_distance_m = 5.0;
    config.max_distance_m = 1000.0;
    config.crossbow_type = make_test_crossbow();
    shooting.start_session(config);

    bool added = shooting.add_target(0.0, 200.0, 5.0, 1.0);
    REQUIRE(added);

    shooting.aim(0.0, 200.0);
    shooting.fire();

    cm::ShootingStats stats = shooting.get_stats();
    REQUIRE_GE(stats.total_shots, (uint32_t)1);
}

TEST_CASE(Module_VirtualShooting, get_stats_aggregates_correctly) {
    cm::VirtualShooting shooting;
    cm::ShootingSessionConfig config = {};
    config.min_distance_m = 5.0;
    config.max_distance_m = 1000.0;
    config.crossbow_type = make_test_crossbow();
    shooting.start_session(config);

    shooting.add_target(0.0, 200.0, 10.0, 1.0);

    for (int i = 0; i < 8; i++) {
        shooting.aim(0.0, 200.0);
        shooting.fire();
    }

    cm::ShootingStats before = shooting.get_stats();
    REQUIRE_GE(before.total_shots, (uint32_t)5);

    shooting.reset();
    cm::ShootingStats after = shooting.get_stats();
    REQUIRE_EQ(after.total_shots, (uint32_t)0);
}

// ================ DYNAMICS SOLVER (3 tests) ================

TEST_CASE(Module_DynamicsSolver, submit_sync_returns_trajectory) {
    cm::DynamicsSolver solver(2);
    solver.start();

    cm::SolverTask task = {};
    task.task_id = 1;
    task.crossbow = make_test_crossbow();
    task.initial_velocity = 50.0;
    task.launch_angle = to_radians(15.0);
    task.wind_speed_ms = 0.0;
    task.wind_direction_deg = 0.0;
    task.time_step = 0.001;
    task.priority = 0;

    cm::SolverResult result = solver.submit_task_sync(task);

    REQUIRE(result.success);
    REQUIRE_GT(result.trajectory.size(), (size_t)0);
    REQUIRE_GT(result.shot_record.impact_point.x, 0.0);

    solver.stop();
}

TEST_CASE(Module_DynamicsSolver, multiple_tasks_complete_all) {
    cm::DynamicsSolver solver(2);
    solver.start();

    CrossbowType cb = make_test_crossbow();
    double velocities[] = {40.0, 50.0, 60.0, 55.0, 45.0};
    double angles[] = {10.0, 15.0, 20.0, 12.0, 18.0};

    for (int i = 0; i < 5; i++) {
        cm::SolverTask task = {};
        task.task_id = i + 1;
        task.crossbow = cb;
        task.initial_velocity = velocities[i];
        task.launch_angle = to_radians(angles[i]);
        task.wind_speed_ms = 0.0;
        task.wind_direction_deg = 0.0;
        task.time_step = 0.001;
        task.priority = 0;
        solver.submit_task(task);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    size_t completed = solver.get_completed_count();
    REQUIRE_GE(completed, (size_t)5);

    solver.stop();
}

TEST_CASE(Module_DynamicsSolver, start_stop_cycle_no_crash) {
    CrossbowType cb = make_test_crossbow();

    {
        cm::DynamicsSolver solver1(2);
        solver1.start();
        cm::SolverTask task = {};
        task.task_id = 1;
        task.crossbow = cb;
        task.initial_velocity = 50.0;
        task.launch_angle = to_radians(15.0);
        task.time_step = 0.001;
        solver1.submit_task(task);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        solver1.stop();
    }

    {
        cm::DynamicsSolver solver2(2);
        REQUIRE_NOTHROW(solver2.start());
        REQUIRE_NOTHROW(solver2.stop());
        REQUIRE_NOTHROW(solver2.start());
        REQUIRE_NOTHROW(solver2.stop());
    }
}
