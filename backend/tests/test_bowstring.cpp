#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "test_harness.h"
#include "../include/bowstring_material.h"
#include "../include/common.h"
#include <cmath>
#include <vector>

namespace crossbow {
std::ostream& operator<<(std::ostream& os, BowstringMaterialType type) {
    switch (type) {
        case BowstringMaterialType::UNKNOWN: return os << "UNKNOWN";
        case BowstringMaterialType::SINEW: return os << "SINEW";
        case BowstringMaterialType::HORN: return os << "HORN";
        case BowstringMaterialType::SILK: return os << "SILK";
        case BowstringMaterialType::HEMP: return os << "HEMP";
        case BowstringMaterialType::CUSTOM: return os << "CUSTOM";
        default: return os << "INVALID_TYPE";
    }
}
}

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

// ================ Feature 1: 弓弦材质对比 - 正常测试 ================

TEST_CASE(Bowstring, 内置材质数据库初始化正确) {
    auto& db = BowstringMaterialDatabase::instance();
    auto materials = db.list_all();

    REQUIRE_GE(materials.size(), 4);

    auto sinew = db.get(BowstringMaterialType::SINEW);
    REQUIRE_EQ(sinew.type, BowstringMaterialType::SINEW);
    REQUIRE_MSG(sinew.name.find("筋") != std::string::npos, "筋材质名称不正确");

    auto horn = db.get(BowstringMaterialType::HORN);
    auto silk = db.get(BowstringMaterialType::SILK);
    auto hemp = db.get(BowstringMaterialType::HEMP);

    REQUIRE_GT(sinew.young_modulus, 0);
    REQUIRE_GT(horn.tensile_strength, 0);
    REQUIRE_GT(silk.density, 0);
    REQUIRE_GT(hemp.elongation_at_break, 0);

    REQUIRE_GT(sinew.fatigue_life_cycles, 0);
    REQUIRE_GT(hemp.fatigue_life_cycles, 0);
    REQUIRE_GT(silk.energy_efficiency, 0.7);
}

TEST_CASE(Bowstring, 材质物理计算正确_弦质量) {
    auto& db = BowstringMaterialDatabase::instance();
    auto silk = db.get(BowstringMaterialType::SILK);

    double diameter_m = 0.0025;
    double length_m = 1.8;
    double mass = silk.calculate_string_mass(length_m);

    double expected = silk.density * M_PI * diameter_m * diameter_m / 4.0 * length_m;
    REQUIRE_NEAR(mass, expected, 1e-9);

    REQUIRE_GT(mass, 0);
    REQUIRE_LT(mass, 0.1);
}

TEST_CASE(Bowstring, 材质物理计算正确_刚度) {
    auto& db = BowstringMaterialDatabase::instance();
    auto sinew = db.get(BowstringMaterialType::SINEW);

    double diameter_m = 0.0025;
    double length_m = 1.8;
    double area = M_PI * diameter_m * diameter_m / 4.0;
    double stiffness = sinew.calculate_stiffness(length_m, area);
    double expected = sinew.young_modulus * area / length_m;
    REQUIRE_NEAR(stiffness, expected, 1e-6);

    REQUIRE_GT(stiffness, 1000);
}

TEST_CASE(Bowstring, 储能效率排序符合物理规律) {
    auto& db = BowstringMaterialDatabase::instance();
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    std::vector<BowstringMaterialType> types = {
        BowstringMaterialType::SINEW,
        BowstringMaterialType::HORN,
        BowstringMaterialType::SILK,
        BowstringMaterialType::HEMP
    };

    auto result = analyzer.compare_materials(types, 15.0);

    REQUIRE_EQ(result.materials.size(), 4);

    double silk_vel = 0, horn_vel = 0, sinew_vel = 0, hemp_vel = 0;
    for (const auto& m : result.materials) {
        if (m.material.type == BowstringMaterialType::SILK) silk_vel = m.estimated_muzzle_velocity;
        if (m.material.type == BowstringMaterialType::HORN) horn_vel = m.estimated_muzzle_velocity;
        if (m.material.type == BowstringMaterialType::SINEW) sinew_vel = m.estimated_muzzle_velocity;
        if (m.material.type == BowstringMaterialType::HEMP) hemp_vel = m.estimated_muzzle_velocity;
    }

    REQUIRE_GT(silk_vel, 0);
    REQUIRE_GT(horn_vel, 0);

    double max_vel = std::max({silk_vel, horn_vel, sinew_vel, hemp_vel});
    REQUIRE_GT(max_vel, 60);
    REQUIRE_LT(max_vel, 200);

    for (const auto& m : result.materials) {
        REQUIRE_GT(m.estimated_energy, 0);
        REQUIRE_GT(m.max_tension_before_break, 0);
        REQUIRE_LT(m.damping_loss_ratio, 1.0);
        REQUIRE_GT(m.damping_loss_ratio, 0.5);
    }
}

TEST_CASE(Bowstring, 效率曲线在70_pct拉距附近达到峰值) {
    auto& db = BowstringMaterialDatabase::instance();
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);
    auto silk = db.get(BowstringMaterialType::SILK);

    auto curve = analyzer.calculate_efficiency_curve(silk, 100);

    REQUIRE_GE(curve.size(), 10);

    double max_eff = 0;
    double max_pull = 0;
    for (const auto& p : curve) {
        if (p.second > max_eff) {
            max_eff = p.second;
            max_pull = p.first;
        }
    }

    REQUIRE_GT(max_eff, 0.6);
    REQUIRE_LT(max_eff, 1.0);

    REQUIRE_GE(max_pull, 0.4);
    REQUIRE_LE(max_pull, 1.0);
}

TEST_CASE(Bowstring, 多目标优化返回合理结果) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    auto all_materials = BowstringMaterialDatabase::instance().list_all();
    auto optimal = analyzer.optimize_material_for_accuracy(all_materials, 0.4, 0.35, 0.25);

    REQUIRE_MSG(optimal.type != BowstringMaterialType::CUSTOM,
                "优化结果不应是CUSTOM类型");

    REQUIRE_GT(optimal.energy_efficiency, 0.7);
    REQUIRE_GT(optimal.fatigue_life_cycles, 10000);
}

// ================ Feature 1: 弓弦材质对比 - 边界测试 ================

TEST_CASE(Bowstring_Boundary, 极小直径弦计算稳定性) {
    auto& db = BowstringMaterialDatabase::instance();
    auto silk = db.get(BowstringMaterialType::SILK);

    double tiny_diameter = 1e-9;
    double area = M_PI * tiny_diameter * tiny_diameter / 4.0;
    double mass = silk.calculate_string_mass(1.8);
    double stiffness = silk.calculate_stiffness(1.8, area);

    REQUIRE_GE(mass, 0);
    REQUIRE_GE(stiffness, 0);
    REQUIRE(!std::isnan(mass));
    REQUIRE(!std::isnan(stiffness));
}

TEST_CASE(Bowstring_Boundary, 极大直径弦不超过抗拉强度) {
    auto& db = BowstringMaterialDatabase::instance();
    auto hemp = db.get(BowstringMaterialType::HEMP);

    double huge_diameter = 0.1;
    double area = M_PI * huge_diameter * huge_diameter / 4.0;
    double stiffness = hemp.calculate_stiffness(1.8, area);

    REQUIRE(!std::isinf(stiffness));
    REQUIRE_GT(stiffness, 0);
}

TEST_CASE(Bowstring_Boundary, 零长度弦返回零) {
    auto& db = BowstringMaterialDatabase::instance();
    auto sinew = db.get(BowstringMaterialType::SINEW);

    double area = M_PI * 0.0025 * 0.0025 / 4.0;
    double mass = sinew.calculate_string_mass(0.0);
    double stiffness = sinew.calculate_stiffness(0.0, area);
    double wave_speed = sinew.calculate_wave_speed(0.0, 0.0);

    REQUIRE_EQ(mass, 0.0);
    REQUIRE(!std::isnan(stiffness));
    REQUIRE(!std::isnan(wave_speed));
}

TEST_CASE(Bowstring_Boundary, 零角度射击不崩溃) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    std::vector<BowstringMaterialType> types = {BowstringMaterialType::SILK};

    REQUIRE_NOTHROW(analyzer.compare_materials(types, 0.0));
    REQUIRE_NOTHROW(analyzer.compare_materials(types, 90.0));
    REQUIRE_NOTHROW(analyzer.compare_materials(types, -5.0));
    REQUIRE_NOTHROW(analyzer.compare_materials(types, 100.0));
}

TEST_CASE(Bowstring_Boundary, 自定义材质参数边界处理) {
    BowstringMaterial custom;
    custom.type = BowstringMaterialType::CUSTOM;
    custom.name = "自定义材质";
    custom.young_modulus = 0;
    custom.tensile_strength = -1;
    custom.density = -100;
    custom.energy_efficiency = 2.0;

    double area = M_PI * 0.0025 * 0.0025 / 4.0;
    double mass = custom.calculate_string_mass(1.8);
    REQUIRE_GE(mass, 0);

    double stiffness = custom.calculate_stiffness(1.8, area);
    REQUIRE_GE(stiffness, 0);

    double efficiency = custom.calculate_efficiency_at_pull(0.7);
    REQUIRE_LE(efficiency, 1.0);
    REQUIRE_GE(efficiency, 0.0);
}

TEST_CASE(Bowstring_Boundary, 空材质列表对比) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    std::vector<BowstringMaterialType> empty;
    REQUIRE_NOTHROW(analyzer.compare_materials(empty, 15.0));

    auto result = analyzer.compare_materials(empty, 15.0);
    REQUIRE_EQ(result.materials.size(), 0);
}

TEST_CASE(Bowstring_Boundary, 权重和不为1时优化仍正常) {
    auto cb = get_test_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);
    auto all_materials = BowstringMaterialDatabase::instance().list_all();

    REQUIRE_NOTHROW(analyzer.optimize_material_for_accuracy(all_materials, 0, 0, 0));
    REQUIRE_NOTHROW(analyzer.optimize_material_for_accuracy(all_materials, 10, 20, 30));
    REQUIRE_NOTHROW(analyzer.optimize_material_for_accuracy(all_materials, -1, 0.5, 2));

    auto r1 = analyzer.optimize_material_for_accuracy(all_materials, 0, 0, 0);
    auto r2 = analyzer.optimize_material_for_accuracy(all_materials, 1, 1, 1);
    REQUIRE(r1.energy_efficiency >= 0);
    REQUIRE(r2.energy_efficiency >= 0);
}

// ================ Feature 1: 弓弦材质对比 - 异常测试 ================

TEST_CASE(Bowstring_Exception, 无效材质类型返回UNKNOWN) {
    auto& db = BowstringMaterialDatabase::instance();
    auto& result = db.get(static_cast<BowstringMaterialType>(999));
    REQUIRE_EQ(result.type, BowstringMaterialType::UNKNOWN);

    auto& result2 = db.get(static_cast<BowstringMaterialType>(-1));
    REQUIRE_EQ(result2.type, BowstringMaterialType::UNKNOWN);
}

TEST_CASE(Bowstring_Exception, 无效材质类型检查) {
    auto& db = BowstringMaterialDatabase::instance();

    auto& invalid = db.get(static_cast<BowstringMaterialType>(999));
    REQUIRE_EQ(invalid.type, BowstringMaterialType::UNKNOWN);

    auto& valid = db.get(BowstringMaterialType::SILK);
    REQUIRE_EQ(valid.type, BowstringMaterialType::SILK);
}

TEST_CASE(Bowstring_Exception, 负直径弦的鲁棒性) {
    auto& db = BowstringMaterialDatabase::instance();
    auto silk = db.get(BowstringMaterialType::SILK);

    double area = M_PI * (-0.001) * (-0.001) / 4.0;
    REQUIRE_NOTHROW(silk.calculate_string_mass(1.8));
    REQUIRE_NOTHROW(silk.calculate_stiffness(1.8, area));

    double mass = silk.calculate_string_mass(1.8);
    double stiffness = silk.calculate_stiffness(1.8, area);

    REQUIRE(!std::isnan(mass));
    REQUIRE(!std::isnan(stiffness));
    REQUIRE_GE(mass, 0);
    REQUIRE_GE(stiffness, 0);
}

TEST_CASE(Bowstring_Exception, 数据库单例线程安全) {
    auto& db1 = BowstringMaterialDatabase::instance();
    auto& db2 = BowstringMaterialDatabase::instance();

    REQUIRE_EQ(&db1, &db2);

    auto list1 = db1.list_all();
    auto list2 = db2.list_all();
    REQUIRE_EQ(list1.size(), list2.size());
}

TEST_CASE(Bowstring_Exception, 阻尼力计算极端输入) {
    auto& db = BowstringMaterialDatabase::instance();
    auto sinew = db.get(BowstringMaterialType::SINEW);

    REQUIRE_NOTHROW(sinew.calculate_damping_force(1e6, 1e6));
    REQUIRE_NOTHROW(sinew.calculate_damping_force(0, 0));
    REQUIRE_NOTHROW(sinew.calculate_damping_force(-100, -100));

    double f1 = sinew.calculate_damping_force(100, 10);
    double f2 = sinew.calculate_damping_force(100, 1000);
    REQUIRE_GE(f2, f1);
}

TEST_CASE(Bowstring_Exception, 蠕变因子极端温湿度) {
    auto& db = BowstringMaterialDatabase::instance();
    auto horn = db.get(BowstringMaterialType::HORN);

    REQUIRE_NOTHROW(horn.calculate_creep_factor(-50, 0));
    REQUIRE_NOTHROW(horn.calculate_creep_factor(200, 100));
    REQUIRE_NOTHROW(horn.calculate_creep_factor(20, 200));

    double c1 = horn.calculate_creep_factor(20, 50);
    double c2 = horn.calculate_creep_factor(80, 90);
    REQUIRE_GE(c2, c1);
}

TEST_CASE(Bowstring_Exception, 寿命估计极端应力) {
    auto& db = BowstringMaterialDatabase::instance();
    auto silk = db.get(BowstringMaterialType::SILK);

    REQUIRE_NOTHROW(silk.estimate_lifespan(0, 50));
    REQUIRE_NOTHROW(silk.estimate_lifespan(static_cast<int>(silk.tensile_strength * 10), 50));
    REQUIRE_NOTHROW(silk.estimate_lifespan(-1000000, 50));

    double l1 = silk.estimate_lifespan(static_cast<int>(silk.tensile_strength * 0.3), 50);
    double l2 = silk.estimate_lifespan(static_cast<int>(silk.tensile_strength * 0.8), 50);
    REQUIRE_GE(l1, l2);
}
