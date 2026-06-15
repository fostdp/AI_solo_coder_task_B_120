#pragma once

#include "common.h"
#include "clickhouse_storage.h"
#include "ballistic_simulator.h"
#include "accuracy_analyzer_service.h"
#include "alarm_mqtt_service.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>

class HttpServer {
public:
    using RequestHandler = std::function<std::string(const std::map<std::string, std::string>& params)>;

    HttpServer(int port,
               std::shared_ptr<ClickHouseStorage> storage,
               std::shared_ptr<BallisticSimulator> ballistic,
               std::shared_ptr<AccuracyAnalyzerService> accuracy,
               std::shared_ptr<AlarmMqttService> alarm,
               std::shared_ptr<std::map<int, CrossbowType>> crossbow_types = nullptr);
    ~HttpServer();

    bool start();
    void stop();
    bool is_running() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void register_routes();
    std::string handle_get_crossbow_types(const std::map<std::string, std::string>& params);
    std::string handle_get_sensor_data(const std::map<std::string, std::string>& params);
    std::string handle_get_shot_history(const std::map<std::string, std::string>& params);
    std::string handle_get_alerts(const std::map<std::string, std::string>& params);
    std::string handle_get_accuracy(const std::map<std::string, std::string>& params);
    std::string handle_simulate_shot(const std::map<std::string, std::string>& params);
    std::string handle_run_accuracy_analysis(const std::map<std::string, std::string>& params);
    std::string handle_resolve_alert(const std::map<std::string, std::string>& params);
    std::string handle_get_system_status(const std::map<std::string, std::string>& params);
    std::string handle_get_ballistic_result(const std::map<std::string, std::string>& params);

    // ===== New Feature APIs =====
    std::string handle_get_bowstring_materials(const std::map<std::string, std::string>& params);
    std::string handle_compare_bowstring_materials(const std::map<std::string, std::string>& params);
    std::string handle_get_bowstring_efficiency_curve(const std::map<std::string, std::string>& params);
    std::string handle_optimize_bowstring(const std::map<std::string, std::string>& params);

    std::string handle_get_sight_designs(const std::map<std::string, std::string>& params);
    std::string handle_simulate_sight_optics(const std::map<std::string, std::string>& params);
    std::string handle_calibrate_sight_scales(const std::map<std::string, std::string>& params);
    std::string handle_optimize_sight_design(const std::map<std::string, std::string>& params);

    std::string handle_get_formation_types(const std::map<std::string, std::string>& params);
    std::string handle_simulate_formation(const std::map<std::string, std::string>& params);
    std::string handle_compare_formations(const std::map<std::string, std::string>& params);
    std::string handle_rl_optimize_formation(const std::map<std::string, std::string>& params);
    std::string handle_generate_formation_placement(const std::map<std::string, std::string>& params);
};
