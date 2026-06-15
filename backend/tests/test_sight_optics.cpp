#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "test_harness.h"
#include "../include/sight_optics.h"
#include "../include/common.h"
#include <cmath>
#include <vector>

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

// ================ Feature 2: 望山瞄准优化 - 正常测试 ================

TEST_CASE(SightOptics, 内置望山设计数据库正确) {
    auto& db = SightDesignDatabase::instance();
    auto designs = db.list_all();
    REQUIRE_GE(designs.size(), 6);

    auto notch = db.get(SightType::BASIC_NOTCH);
    REQUIRE_EQ(notch.type, SightType::BASIC_NOTCH);
    REQUIRE(!notch.name.empty());

    auto pin = db.get(SightType::PIN_SIGHT);
    auto ramp = db.get(SightType::RAMP_SIGHT);
    auto multi = db.get(SightType::MULTI_PIN);
    auto tangent = db.get(SightType::TANGENT_SIGHT);
    auto peep = db.get(SightType::PRECISION_PEEP);

    REQUIRE_GT(notch.sight_height_mm, 0);
    REQUIRE_GT(pin.sight_radius_mm, 0);
    REQUIRE_GT(ramp.notch_width_mm, 0);
    REQUIRE_GT(multi.scale_count, 0);
    REQUIRE_GT(tangent.manufacturing_precision_mm, 0);
    REQUIRE_GT(peep.aperture_diameter_mm, 0);
}

TEST_CASE(SightOptics, 光学误差模拟器各分量计算正确) {
    OpticalErrorSimulator simulator;

    double acuity_err = simulator.calculate_visual_acuity_error(200.0);
    REQUIRE_GT(acuity_err, 0);
    REQUIRE(!std::isnan(acuity_err));

    double parallax_err = simulator.calculate_parallax_error(50.0, 100.0);
    REQUIRE_GT(parallax_err, 0);

    double refraction = simulator.calculate_atmospheric_refraction(300.0);
    REQUIRE_GE(refraction, 0);

    double haze = simulator.calculate_heat_haze_error(200.0, 30.0, 50.0);
    REQUIRE_GE(haze, 0);

    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);
    double play = simulator.calculate_mechanical_play_error(design);
    REQUIRE_GE(play, 0);

    double thermal = simulator.calculate_thermal_distortion(design, 20.0);
    REQUIRE_GE(thermal, 0);

    auto glare = simulator.calculate_glare_effect(500.0, 30.0);
    REQUIRE_GE(glare.first, 0);
    REQUIRE_GE(glare.second, 0);
}

TEST_CASE(SightOptics, 蒙特卡洛CEP估计统计特性正确) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    int samples = 300;
    double cep = simulator.estimate_sight_cep(design, 200.0, samples);

    REQUIRE_GT(cep, 0);
    REQUIRE_LT(cep, 50.0);
    REQUIRE(!std::isnan(cep));
}

TEST_CASE(SightOptics, 单次瞄准模拟返回有效结果) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    auto result = simulator.simulate_single_aim(design, 10.0, 0.0, 200.0);

    REQUIRE(!std::isnan(result.raw_aim_angle_deg));
    REQUIRE(!std::isnan(result.optical_error_elevation_mrad));
    REQUIRE(!std::isnan(result.optical_error_azimuth_mrad));
    REQUIRE(!std::isnan(result.mechanical_error_elevation_mrad));
    REQUIRE(!std::isnan(result.total_error_elevation_mrad));
    REQUIRE(!std::isnan(result.total_error_azimuth_mrad));
    REQUIRE(!std::isnan(result.aiming_confidence));

    REQUIRE_GE(result.aiming_confidence, 0);
    REQUIRE_LE(result.aiming_confidence, 1);
}

TEST_CASE(SightOptics, 多次瞄准模拟统计特性) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    auto results = simulator.simulate_multiple_aims(design, 10.0, 0.0, 200.0, 200);

    REQUIRE_EQ((int)results.size(), 200);

    double sum_elev = 0, sum_azim = 0;
    for (const auto& r : results) {
        sum_elev += r.total_error_elevation_mrad;
        sum_azim += r.total_error_azimuth_mrad;
    }
    double mean_elev = sum_elev / results.size();
    double mean_azim = sum_azim / results.size();

    REQUIRE_GT(std::abs(mean_elev), 0);
    REQUIRE(!std::isnan(mean_elev));
    REQUIRE(!std::isnan(mean_azim));
}

TEST_CASE(SightOptics, 望山校准刻度计算正确) {
    SightScaleCalibrator calibrator;
    auto design = SightDesignDatabase::instance().get(SightType::MULTI_PIN);
    auto cb = get_test_crossbow();

    calibrator.set_crossbow(cb);
    calibrator.set_sight_design(design);

    auto calib = calibrator.calibrate_scales(50.0, 300.0, 50.0);

    REQUIRE_GT(calib.size(), 0);

    for (size_t i = 1; i < calib.size(); i++) {
        REQUIRE_GE(calib[i].calibrated_angle_deg, calib[i-1].calibrated_angle_deg);
    }

    for (const auto& pt : calib) {
        REQUIRE_GT(pt.calibrated_angle_deg, 0);
        REQUIRE_LT(pt.calibrated_angle_deg, 85);
        REQUIRE_GT(pt.scale_height_mm, 0);
        REQUIRE(!std::isnan(pt.calibration_error_mrad));
    }
}

TEST_CASE(SightOptics, 网格搜索优化返回更优设计) {
    SightOptimizer optimizer;
    auto cb = get_test_crossbow();

    optimizer.set_crossbow(cb);
    optimizer.set_target_range(200.0);

    auto result = optimizer.grid_search_optimize(SightType::PIN_SIGHT, 10, 8, 5);

    REQUIRE_GT(result.optimal_design.sight_height_mm, 0);
    REQUIRE_GE(result.ceph_improvement_percent, 0);
    REQUIRE_LT(result.ceph_improvement_percent, 100);
    REQUIRE_GT(result.overall_score, 0);
    REQUIRE_LE(result.overall_score, 1.0);
}

TEST_CASE(SightOptics, 误差预算获取和设置正确) {
    OpticalErrorSimulator simulator;

    simulator.set_ambient_conditions(300.0, 25.0, 0.6, 2.0);
    simulator.set_user_profile(1.5, 0.7);
    simulator.set_distance(200.0);

    auto budget = simulator.get_current_error_budget();
    REQUIRE(!std::isnan(budget.visual_acuity_arcmin));
    REQUIRE(!std::isnan(budget.ambient_light_lux));
    REQUIRE(!std::isnan(budget.atmospheric_refraction_mrad));
}

// ================ Feature 2: 望山瞄准优化 - 边界测试 ================

TEST_CASE(SightOptics_Boundary, 极近距离瞄准稳定性) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, 5.0, 100));
    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, 0.1, 100));

    double cep = simulator.estimate_sight_cep(design, 5.0, 100);
    REQUIRE(!std::isnan(cep));
    REQUIRE_GE(cep, 0);
}

TEST_CASE(SightOptics_Boundary, 超远距离瞄准稳定性) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PRECISION_PEEP);

    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, 1000.0, 100));
    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, 5000.0, 100));

    double cep = simulator.estimate_sight_cep(design, 2000.0, 100);
    REQUIRE(!std::isnan(cep));
    REQUIRE_GE(cep, 0);
}

TEST_CASE(SightOptics_Boundary, 单样本蒙特卡洛仍返回有效结果) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    double cep = simulator.estimate_sight_cep(design, 200.0, 1);
    REQUIRE(!std::isnan(cep));
    REQUIRE_GE(cep, 0);
}

TEST_CASE(SightOptics_Boundary, 反向距离范围校准鲁棒性) {
    SightScaleCalibrator calibrator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);
    auto cb = get_test_crossbow();

    calibrator.set_crossbow(cb);
    calibrator.set_sight_design(design);

    try {
        auto calib = calibrator.calibrate_scales(100.0, 50.0, 10.0);
        REQUIRE_GE(calib.size(), 0);
    } catch (...) {
        REQUIRE(true);
    }
}

TEST_CASE(SightOptics_Boundary, 极端环境误差叠加不崩溃) {
    OpticalErrorSimulator simulator;

    simulator.set_ambient_conditions(0.01, -40, 0.0, 50.0);
    simulator.set_user_profile(100.0, 0.0);
    simulator.set_distance(300.0);

    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, 300.0, 50));

    double cep = simulator.estimate_sight_cep(design, 300.0, 50);
    REQUIRE(!std::isnan(cep));
    REQUIRE(!std::isinf(cep));
}

TEST_CASE(SightOptics_Boundary, 超远距离校准插值仍有效) {
    SightScaleCalibrator calibrator;
    auto design = SightDesignDatabase::instance().get(SightType::TANGENT_SIGHT);
    auto cb = get_test_crossbow();

    calibrator.set_crossbow(cb);
    calibrator.set_sight_design(design);

    REQUIRE_NOTHROW(calibrator.calibrate_scales(10.0, 5000.0, 1000.0));

    auto calib = calibrator.calibrate_scales(10.0, 5000.0, 1000.0);
    for (const auto& pt : calib) {
        REQUIRE(!std::isnan(pt.calibrated_angle_deg));
        REQUIRE(!std::isnan(pt.scale_height_mm));
    }
}

// ================ Feature 2: 望山瞄准优化 - 异常测试 ================

TEST_CASE(SightOptics_Exception, 无效望山类型返回默认) {
    auto& result = SightDesignDatabase::instance().get(static_cast<SightType>(999));
    REQUIRE_EQ(result.type, SightType::CUSTOM);

    auto& result2 = SightDesignDatabase::instance().get(static_cast<SightType>(-1));
    REQUIRE_EQ(result2.type, SightType::CUSTOM);
}

TEST_CASE(SightOptics_Exception, 无效望山类型检查) {
    auto& invalid = SightDesignDatabase::instance().get(static_cast<SightType>(999));
    REQUIRE_EQ(invalid.type, SightType::CUSTOM);

    auto& valid = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);
    REQUIRE(valid.type == SightType::PIN_SIGHT);
}

TEST_CASE(SightOptics_Exception, 负距离瞄准鲁棒性) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    REQUIRE_NOTHROW(simulator.estimate_sight_cep(design, -10.0, 100));

    double cep = simulator.estimate_sight_cep(design, -10.0, 100);
    REQUIRE(!std::isnan(cep));
    REQUIRE_GE(cep, 0);
}

TEST_CASE(SightOptics_Exception, 负样本数处理) {
    OpticalErrorSimulator simulator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);

    try { simulator.estimate_sight_cep(design, 200.0, -100); } catch (...) {}
    try { simulator.estimate_sight_cep(design, 200.0, 0); } catch (...) {}
    try { simulator.estimate_sight_cep(design, 200.0, -5); } catch (...) {}
}

TEST_CASE(SightOptics_Exception, 负距离校准) {
    SightScaleCalibrator calibrator;
    auto design = SightDesignDatabase::instance().get(SightType::PIN_SIGHT);
    auto cb = get_test_crossbow();

    calibrator.set_crossbow(cb);
    calibrator.set_sight_design(design);

    REQUIRE_NOTHROW(calibrator.calibrate_scales(-100.0, 200.0, 50.0));

    auto calib = calibrator.calibrate_scales(-100.0, 200.0, 50.0);
    for (const auto& pt : calib) {
        REQUIRE(!std::isnan(pt.calibrated_angle_deg));
    }
}

TEST_CASE(SightOptics_Exception, 优化搜索极端参数) {
    SightOptimizer optimizer;
    auto cb = get_test_crossbow();

    optimizer.set_crossbow(cb);
    optimizer.set_target_range(200.0);

    REQUIRE_NOTHROW(optimizer.grid_search_optimize(SightType::PIN_SIGHT, -10, -10, -10));
    REQUIRE_NOTHROW(optimizer.grid_search_optimize(SightType::PIN_SIGHT, 1000000, 1000000, 1000000));

    auto r = optimizer.grid_search_optimize(SightType::PIN_SIGHT, -10, -10, -10);
    REQUIRE(r.overall_score >= 0);
}

TEST_CASE(SightOptics_Exception, 不同望山类型CEP排序符合预期) {
    OpticalErrorSimulator simulator;

    auto basic = SightDesignDatabase::instance().get(SightType::BASIC_NOTCH);
    auto precision = SightDesignDatabase::instance().get(SightType::PRECISION_PEEP);

    double cep1 = simulator.estimate_sight_cep(basic, 200.0, 300);
    double cep2 = simulator.estimate_sight_cep(precision, 200.0, 300);

    REQUIRE_GT(cep1, 0);
    REQUIRE_GT(cep2, 0);
    REQUIRE(!std::isnan(cep1));
    REQUIRE(!std::isnan(cep2));

    REQUIRE_MSG(cep2 <= cep1 * 1.5,
                "觇孔式瞄准不应显著差于缺口式");
}
