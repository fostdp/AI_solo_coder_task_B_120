-- 弩机动力学仿真系统 - ClickHouse 初始化脚本
-- 包含 TTL 数据生命周期管理

CREATE DATABASE IF NOT EXISTS crossbow_sim;

USE crossbow_sim;

-- ==================== 弩机类型表 ====================
CREATE TABLE IF NOT EXISTS crossbow_types (
    id UInt32,
    name String,
    dynasty String,
    draw_weight Float64,
    bow_length Float64,
    string_length Float64,
    arrow_mass Float64,
    effective_range Float64,
    max_range Float64,
    created_at DateTime DEFAULT now()
) ENGINE = ReplacingMergeTree(created_at)
ORDER BY id
SETTINGS index_granularity = 8192;

-- ==================== 传感器数据表（TTL: 30天） ====================
CREATE TABLE IF NOT EXISTS sensor_data (
    timestamp DateTime64(3),
    crossbow_id UInt32,
    crossbow_name String,
    bow_string_tension Float64,
    bow_arm_deformation Float64,
    arrow_velocity Float64,
    range Float64,
    spread_x Float64,
    spread_y Float64,
    aim_angle Float64,
    temperature Float64,
    humidity Float64,
    wind_speed Float64,
    wind_direction Float64,
    mach_number Float64,
    drag_coefficient Float64,
    max_launch_acceleration_g Float64,
    contact_phase_time_ms Float64
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, timestamp)
TTL timestamp + INTERVAL 30 DAY
SETTINGS index_granularity = 8192;

-- ==================== 弹道详情表（TTL: 7天） ====================
CREATE TABLE IF NOT EXISTS trajectory_data (
    timestamp DateTime64(3),
    crossbow_id UInt32,
    shot_id UUID,
    time_step Float64,
    position_x Float64,
    position_y Float64,
    position_z Float64,
    velocity_x Float64,
    velocity_y Float64,
    velocity_z Float64,
    acceleration_x Float64,
    acceleration_y Float64,
    acceleration_z Float64,
    drag_force Float64,
    lift_force Float64,
    mach_number Float64,
    drag_coefficient Float64
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, shot_id, time_step)
TTL timestamp + INTERVAL 7 DAY
SETTINGS index_granularity = 8192;

-- ==================== 射击记录表（TTL: 90天） ====================
CREATE TABLE IF NOT EXISTS shot_records (
    timestamp DateTime64(3),
    crossbow_id UInt32,
    shot_id UUID,
    initial_velocity Float64,
    launch_angle Float64,
    max_height Float64,
    flight_time Float64,
    impact_point_x Float64,
    impact_point_y Float64,
    impact_point_z Float64,
    impact_velocity Float64,
    kinetic_energy Float64,
    mach_number Float64,
    drag_coefficient Float64
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, timestamp)
TTL timestamp + INTERVAL 90 DAY
SETTINGS index_granularity = 8192;

-- ==================== 精度分析表（TTL: 1年） ====================
CREATE TABLE IF NOT EXISTS accuracy_analysis (
    id UUID DEFAULT generateUUIDv4(),
    crossbow_id UInt32,
    crossbow_name String,
    analysis_date Date,
    total_shots UInt32,
    mean_spread_x Float64,
    mean_spread_y Float64,
    std_spread_x Float64,
    std_spread_y Float64,
    circular_error_probable Float64,
    mean_velocity Float64,
    std_velocity Float64,
    mean_range Float64,
    optimal_sight_scale Float64,
    sight_adjustments Array(Tuple(Float64, Float64)),
    created_at DateTime DEFAULT now()
) ENGINE = ReplacingMergeTree(created_at)
ORDER BY (crossbow_id, analysis_date)
TTL created_at + INTERVAL 365 DAY
SETTINGS index_granularity = 8192;

-- ==================== 告警表（TTL: 30天） ====================
CREATE TABLE IF NOT EXISTS alerts (
    id UUID DEFAULT generateUUIDv4(),
    timestamp DateTime64(3),
    crossbow_id UInt32,
    crossbow_name String,
    alert_type String,
    severity String,
    message String,
    threshold_value Float64,
    actual_value Float64,
    resolved UInt8 DEFAULT 0,
    resolved_at DateTime64(3)
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, timestamp)
TTL timestamp + INTERVAL 30 DAY
SETTINGS index_granularity = 8192;

-- ==================== 物化视图：每分钟传感器聚合 ====================
CREATE MATERIALIZED VIEW IF NOT EXISTS sensor_data_1min_mv
ENGINE = SummingMergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, timestamp)
AS SELECT
    toStartOfMinute(timestamp) AS timestamp,
    crossbow_id,
    count() AS shot_count,
    avg(arrow_velocity) AS avg_velocity,
    stddevPop(arrow_velocity) AS std_velocity,
    avg(`range`) AS avg_range,
    avg(bow_string_tension) AS avg_tension,
    avg(bow_arm_deformation) AS avg_deformation
FROM sensor_data
GROUP BY crossbow_id, timestamp;

-- ==================== 初始数据：弩机类型 ====================
INSERT INTO crossbow_types (id, name, dynasty, draw_weight, bow_length, string_length, arrow_mass, effective_range, max_range) VALUES
(1, '秦弩', '秦朝', 150.0, 1.38, 1.42, 0.065, 150.0, 300.0),
(2, '汉弩', '汉朝', 180.0, 1.45, 1.50, 0.068, 180.0, 350.0),
(3, '魏武卒弩', '三国', 200.0, 1.50, 1.55, 0.070, 200.0, 380.0),
(4, '诸葛连弩', '三国蜀', 120.0, 1.20, 1.25, 0.055, 100.0, 200.0),
(5, '隋大弩', '隋朝', 220.0, 1.55, 1.60, 0.072, 220.0, 400.0),
(6, '唐伏远弩', '唐朝', 250.0, 1.60, 1.65, 0.075, 250.0, 450.0),
(7, '宋神臂弩', '宋朝', 350.0, 1.75, 1.80, 0.080, 300.0, 550.0),
(8, '金铁鹞子弩', '金朝', 280.0, 1.65, 1.70, 0.077, 280.0, 480.0),
(9, '元神风弩', '元朝', 300.0, 1.68, 1.73, 0.078, 290.0, 500.0),
(10, '明三眼弩', '明朝', 260.0, 1.60, 1.65, 0.076, 260.0, 460.0);

-- ==================== Feature 1: 弓弦材质对比分析表（TTL: 365天） ====================
CREATE TABLE IF NOT EXISTS bowstring_comparison (
    id UUID DEFAULT generateUUIDv4(),
    timestamp DateTime64(3),
    crossbow_id UInt32,
    crossbow_name String,
    launch_angle Float64,
    material_type UInt8,
    material_name String,
    muzzle_velocity Float64,
    energy_joules Float64,
    max_tension_N Float64,
    string_mass_g Float64,
    damping_loss_ratio Float64,
    lifespan_days Float64,
    accuracy_penalty_factor Float64,
    velocity_delta_pct Float64,
    user_weights Tuple(Float64, Float64, Float64),
    is_optimal UInt8 DEFAULT 0,
    created_at DateTime DEFAULT now()
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (crossbow_id, material_type, timestamp)
TTL created_at + INTERVAL 365 DAY
SETTINGS index_granularity = 8192;

-- ==================== Feature 2: 望山设计与光学误差仿真表（TTL: 365天） ====================
CREATE TABLE IF NOT EXISTS sight_optics_analysis (
    id UUID DEFAULT generateUUIDv4(),
    timestamp DateTime64(3),
    sight_type UInt8,
    sight_name String,
    crossbow_id UInt32,
    target_distance_m Float64,
    elevation_deg Float64,
    cep_meters Float64,
    sample_count UInt32,
    avg_elev_error_mrad Float64,
    avg_azim_error_mrad Float64,
    error_budget Tuple(
        visual_acuity_arcmin Float64,
        atmospheric_refraction Float64,
        heat_haze Float64,
        turbulence Float64,
        mechanical_play_mm Float64,
        glare_intensity Float64
    ),
    ambient_lux Float64,
    ambient_temp_c Float64,
    optimal_height_mm Float64,
    optimal_radius_mm Float64,
    optimal_scale_interval_m Float64,
    cep_improvement_pct Float64,
    created_at DateTime DEFAULT now()
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (sight_type, crossbow_id, target_distance_m, timestamp)
TTL created_at + INTERVAL 365 DAY
SETTINGS index_granularity = 8192;

-- ==================== Feature 3: 阵型交火模拟结果表（TTL: 180天） ====================
CREATE TABLE IF NOT EXISTS formation_engagement (
    id UUID DEFAULT generateUUIDv4(),
    timestamp DateTime64(3),
    formation_type UInt8,
    formation_name String,
    num_crossbows UInt32,
    volleys UInt32,
    target_name String,
    target_type UInt8,
    target_area_m2 Float64,
    total_shots UInt32,
    total_hits UInt32,
    hit_probability Float64,
    target_coverage_pct Float64,
    saturation_score Float64,
    suppression_effect Float64,
    logistical_efficiency Float64,
    friendly_risk_m Float64,
    firing_plan Tuple(
        total_projectiles UInt32,
        density_projectiles_per_m2 Float64,
        estimated_kill_prob Float64,
        estimated_casualties Float64,
        ammo_supply_kg Float64
    ),
    salvo_optimization Tuple(
        optimal_salvo_size UInt32,
        interval_ms Float64,
        saturation_time_s Float64,
        time_over_target_s Float64
    ),
    rl_trained UInt8 DEFAULT 0,
    rl_reward Float64 DEFAULT 0,
    rl_episodes UInt32 DEFAULT 0,
    created_at DateTime DEFAULT now()
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (formation_type, num_crossbows, timestamp)
TTL created_at + INTERVAL 180 DAY
SETTINGS index_granularity = 8192;

-- ==================== Feature 3.2: 强化学习训练记录表（TTL: 365天） ====================
CREATE TABLE IF NOT EXISTS rl_training_records (
    id UUID DEFAULT generateUUIDv4(),
    timestamp DateTime64(3),
    target_name String,
    num_crossbows UInt32,
    episodes UInt32,
    learning_rate Float64,
    discount_factor Float64,
    exploration_final Float64,
    best_formation_type UInt8,
    best_formation_name String,
    best_reward Float64,
    convergence_rate Float64,
    best_spacing_multiplier Float64,
    best_orientation_offset_deg Float64,
    formation_value_estimates Array(Tuple(UInt8, String, Float64)),
    episode_rewards_sampled Array(Tuple(Int32, Float64)),
    training_duration_ms Float64,
    created_at DateTime DEFAULT now()
) ENGINE = ReplacingMergeTree(created_at)
ORDER BY (id)
TTL created_at + INTERVAL 365 DAY
SETTINGS index_granularity = 8192;

-- ==================== Feature 4: 虚拟射击体验记录表（TTL: 90天） ====================
CREATE TABLE IF NOT EXISTS virtual_shooting_sessions (
    id UUID DEFAULT generateUUIDv4(),
    session_start DateTime64(3),
    session_end DateTime64(3) DEFAULT toDateTime64(now(), 3),
    user_id String DEFAULT 'anonymous',
    crossbow_id UInt32,
    crossbow_name String,
    crossbow_material UInt8 DEFAULT 3,
    total_shots UInt32,
    total_hits UInt32,
    accuracy Float64,
    total_score UInt32,
    avg_points_per_hit Float64,
    best_single_hit UInt32,
    longest_hit_m Float64,
    shots Array(Tuple(
        shot_idx UInt32,
        elevation_deg Float64,
        azimuth_deg Float64,
        draw_power_pct Float64,
        velocity_mps Float64,
        impact_x Float64,
        impact_y Float64,
        impact_z Float64,
        target_range_m Float64,
        hit UInt8,
        ring_hit UInt8,
        points UInt32
    )),
    targets_destroyed UInt32,
    targets_remaining UInt32,
    session_duration_s Float64,
    client_info String DEFAULT 'web',
    created_at DateTime DEFAULT now()
) ENGINE = MergeTree()
PARTITION BY toYYYYMM(session_start)
ORDER BY (user_id, crossbow_id, session_start)
TTL created_at + INTERVAL 90 DAY
SETTINGS index_granularity = 8192;
