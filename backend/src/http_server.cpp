#include "http_server.h"
#include "logger.h"
#include "metrics.h"
#include "bowstring_material.h"
#include "sight_optics.h"
#include "formation_simulator.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

using json = nlohmann::json;
using namespace crossbow;

struct HttpServer::Impl {
    int port;
    std::shared_ptr<ClickHouseStorage> storage;
    std::shared_ptr<BallisticSimulator> ballistic;
    std::shared_ptr<AccuracyAnalyzerService> accuracy;
    std::shared_ptr<AlarmMqttService> alarm;
    std::shared_ptr<std::map<int, CrossbowType>> crossbow_types;
    std::atomic<bool> running;
    std::thread server_thread;
#ifdef _WIN32
    SOCKET server_fd;
    WSADATA wsa_data;
#else
    int server_fd;
#endif

    Impl(int p, std::shared_ptr<ClickHouseStorage> s,
         std::shared_ptr<BallisticSimulator> b,
         std::shared_ptr<AccuracyAnalyzerService> a,
         std::shared_ptr<AlarmMqttService> al,
         std::shared_ptr<std::map<int, CrossbowType>> ct)
        : port(p), storage(s), ballistic(b), accuracy(a), alarm(al),
          crossbow_types(ct), running(false), server_fd(-1) {}
};

HttpServer::HttpServer(int port,
                       std::shared_ptr<ClickHouseStorage> storage,
                       std::shared_ptr<BallisticSimulator> ballistic,
                       std::shared_ptr<AccuracyAnalyzerService> accuracy,
                       std::shared_ptr<AlarmMqttService> alarm,
                       std::shared_ptr<std::map<int, CrossbowType>> crossbow_types)
    : impl_(std::make_unique<Impl>(port, storage, ballistic, accuracy, alarm, crossbow_types)) {}

HttpServer::~HttpServer() {
    stop();
}

std::string HttpServer::handle_get_crossbow_types(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/crossbow-types");
    auto types = impl_->storage->get_crossbow_types();
    json j = json::array();
    for (const auto& t : types) {
        j.push_back({
            {"id", t.id},
            {"name", t.name},
            {"dynasty", t.dynasty},
            {"draw_weight", t.draw_weight},
            {"bow_length", t.bow_length},
            {"string_length", t.string_length},
            {"arrow_mass", t.arrow_mass},
            {"effective_range", t.effective_range},
            {"max_range", t.max_range}
        });
    }
    return j.dump();
}

std::string HttpServer::handle_get_sensor_data(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/sensor-data");
    json j = json::array();
    auto it = params.find("crossbow_id");
    if (it != params.end() && impl_->accuracy) {
        uint32_t id = std::stoul(it->second);
        auto limit_it = params.find("limit");
        size_t limit = limit_it != params.end() ? std::stoul(limit_it->second) : 100;
        auto data = impl_->accuracy->get_crossbow_data(id, limit);
        for (const auto& d : data) {
            j.push_back({
                {"timestamp", format_timestamp(d.timestamp)},
                {"crossbow_id", d.crossbow_id},
                {"crossbow_name", d.crossbow_name},
                {"bow_string_tension", d.bow_string_tension},
                {"bow_arm_deformation", d.bow_arm_deformation},
                {"arrow_velocity", d.arrow_velocity},
                {"range", d.range},
                {"spread_x", d.spread_x},
                {"spread_y", d.spread_y},
                {"aim_angle", d.aim_angle},
                {"temperature", d.temperature},
                {"humidity", d.humidity},
                {"wind_speed", d.wind_speed},
                {"wind_direction", d.wind_direction}
            });
        }
    }
    return j.dump();
}

std::string HttpServer::handle_get_shot_history(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/shot-history");
    json j = json::array();
    auto it = params.find("crossbow_id");
    if (it != params.end() && impl_->accuracy) {
        uint32_t id = std::stoul(it->second);
        auto limit_it = params.find("limit");
        size_t limit = limit_it != params.end() ? std::stoul(limit_it->second) : 50;
        auto data = impl_->accuracy->get_crossbow_data(id, limit);
        for (const auto& d : data) {
            j.push_back({
                {"timestamp", format_timestamp(d.timestamp)},
                {"crossbow_id", d.crossbow_id},
                {"crossbow_name", d.crossbow_name},
                {"velocity", d.arrow_velocity},
                {"range", d.range},
                {"aim_angle", d.aim_angle},
                {"spread", std::sqrt(d.spread_x * d.spread_x + d.spread_y * d.spread_y)}
            });
        }
    }
    return j.dump();
}

std::string HttpServer::handle_get_alerts(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/alerts");
    json j = json::array();
    if (!impl_->alarm) return j.dump();

    uint32_t crossbow_id = 0;
    auto it = params.find("crossbow_id");
    if (it != params.end()) crossbow_id = std::stoul(it->second);

    auto alerts = impl_->alarm->get_active_alerts(crossbow_id);
    for (const auto& a : alerts) {
        j.push_back({
            {"timestamp", format_timestamp(a.timestamp)},
            {"crossbow_id", a.crossbow_id},
            {"crossbow_name", a.crossbow_name},
            {"alert_type", a.alert_type},
            {"severity", a.severity},
            {"message", a.message},
            {"threshold_value", a.threshold_value},
            {"actual_value", a.actual_value}
        });
    }
    return j.dump();
}

std::string HttpServer::handle_get_accuracy(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/accuracy");
    auto it = params.find("crossbow_id");
    if (it != params.end() && impl_->accuracy) {
        uint32_t id = std::stoul(it->second);
        auto analysis = impl_->accuracy->get_latest_analysis(id);

        json adjustments = json::array();
        for (const auto& adj : analysis.sight_adjustments) {
            adjustments.push_back({
                {"range", adj.first},
                {"adjustment", adj.second}
            });
        }

        json j = {
            {"crossbow_id", analysis.crossbow_id},
            {"crossbow_name", analysis.crossbow_name},
            {"total_shots", analysis.total_shots},
            {"mean_spread_x", analysis.mean_spread_x},
            {"mean_spread_y", analysis.mean_spread_y},
            {"std_spread_x", analysis.std_spread_x},
            {"std_spread_y", analysis.std_spread_y},
            {"circular_error_probable", analysis.circular_error_probable},
            {"mean_velocity", analysis.mean_velocity},
            {"std_velocity", analysis.std_velocity},
            {"mean_range", analysis.mean_range},
            {"optimal_sight_scale", analysis.optimal_sight_scale},
            {"sight_adjustments", adjustments}
        };
        return j.dump();
    }
    return "{}";
}

std::string HttpServer::handle_simulate_shot(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/simulate-shot");
    auto it = params.find("crossbow_id");
    if (it == params.end() || !impl_->ballistic) return "{\"error\":\"missing crossbow_id\"}";

    uint32_t id = std::stoul(it->second);
    double angle = 15.0;
    double wind_speed = 0.0;
    double wind_direction = 0.0;

    auto angle_it = params.find("angle");
    if (angle_it != params.end()) angle = std::stod(angle_it->second);
    auto wind_it = params.find("wind_speed");
    if (wind_it != params.end()) wind_speed = std::stod(wind_it->second);
    auto wd_it = params.find("wind_direction");
    if (wd_it != params.end()) wind_direction = std::stod(wd_it->second);

    auto result = impl_->ballistic->simulate_shot(id, angle, wind_speed, wind_direction);

    json traj = json::array();
    const auto& trajectory = result.full_trajectory;
    size_t step = std::max((size_t)1, trajectory.size() / 200);
    for (size_t i = 0; i < trajectory.size(); i += step) {
        const auto& p = trajectory[i];
        traj.push_back({
            {"t", p.time_step},
            {"x", p.position.x},
            {"y", p.position.y},
            {"z", p.position.z},
            {"vx", p.velocity.x},
            {"vy", p.velocity.y},
            {"vz", p.velocity.z}
        });
    }
    if (trajectory.size() > 0) {
        const auto& p = trajectory.back();
        traj.push_back({
            {"t", p.time_step},
            {"x", p.position.x},
            {"y", p.position.y},
            {"z", p.position.z},
            {"vx", p.velocity.x},
            {"vy", p.velocity.y},
            {"vz", p.velocity.z}
        });
    }

    json j = {
        {"shot_id", result.shot_record.shot_id},
        {"initial_velocity", result.shot_record.initial_velocity},
        {"launch_angle", result.shot_record.launch_angle},
        {"max_height", result.shot_record.max_height},
        {"flight_time", result.shot_record.flight_time},
        {"impact_point", {
            {"x", result.shot_record.impact_point.x},
            {"y", result.shot_record.impact_point.y},
            {"z", result.shot_record.impact_point.z}
        }},
        {"impact_velocity", result.shot_record.impact_velocity},
        {"kinetic_energy", result.shot_record.kinetic_energy},
        {"trajectory", traj},
        {"mach_number", result.mach_number_at_launch},
        {"drag_coefficient", result.drag_coefficient_at_launch},
        {"contact_phase_time_ms", result.launch_phase_contact_time_ms},
        {"max_launch_acceleration_g", result.max_launch_acceleration_g},
        {"dynamics_version", result.dynamics_version}
    };
    return j.dump();
}

std::string HttpServer::handle_run_accuracy_analysis(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/run-analysis");
    auto it = params.find("crossbow_id");
    if (it == params.end() || !impl_->accuracy) return "{\"error\":\"missing crossbow_id\"}";

    uint32_t id = std::stoul(it->second);
    auto data = impl_->accuracy->get_crossbow_data(id, 5000);
    std::string name;
    if (!data.empty()) name = data[0].crossbow_name;
    impl_->accuracy->run_analysis_now(id, name);
    return handle_get_accuracy(params);
}

std::string HttpServer::handle_resolve_alert(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/resolve-alert");
    return "{\"status\":\"ok\"}";
}

std::string HttpServer::handle_get_system_status(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/system-status");
    json j = {
        {"status", "running"},
        {"version", "2.0_refactored"},
        {"architecture", "udp_receiver → message_queue → (ballistic_simulator | accuracy_analyzer | alarm_mqtt)"},
        {"modules", {
            {"udp_receiver", impl_->ballistic != nullptr},
            {"ballistic_simulator", impl_->ballistic != nullptr},
            {"accuracy_analyzer", impl_->accuracy != nullptr},
            {"alarm_mqtt", impl_->alarm != nullptr}
        }}
    };
    return j.dump();
}

std::string HttpServer::handle_get_ballistic_result(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/ballistic-result");
    if (!impl_->ballistic) return "{}";
    auto it = params.find("crossbow_id");
    if (it == params.end()) {
        auto all = impl_->ballistic->get_all_latest_results();
        json j = json::object();
        for (auto& kv : all) {
            j[std::to_string(kv.first)] = {
                {"initial_velocity", kv.second.shot_record.initial_velocity},
                {"range", kv.second.source_data.range},
                {"mach_number", kv.second.mach_number_at_launch},
                {"drag_coefficient", kv.second.drag_coefficient_at_launch},
                {"max_launch_acceleration_g", kv.second.max_launch_acceleration_g},
                {"contact_phase_time_ms", kv.second.launch_phase_contact_time_ms},
                {"dynamics_version", kv.second.dynamics_version}
            };
        }
        return j.dump();
    }
    uint32_t id = std::stoul(it->second);
    auto result = impl_->ballistic->get_latest_result(id);
    json j = {
        {"initial_velocity", result.shot_record.initial_velocity},
        {"range", result.source_data.range},
        {"mach_number", result.mach_number_at_launch},
        {"drag_coefficient", result.drag_coefficient_at_launch},
        {"max_launch_acceleration_g", result.max_launch_acceleration_g},
        {"contact_phase_time_ms", result.launch_phase_contact_time_ms},
        {"dynamics_version", result.dynamics_version}
    };
    return j.dump();
}

static std::map<std::string, std::string> parse_url_params(const std::string& path) {
    std::map<std::string, std::string> params;
    size_t qpos = path.find('?');
    if (qpos == std::string::npos) return params;

    std::string query = path.substr(qpos + 1);
    size_t pos = 0;
    while (pos < query.size()) {
        size_t amp = query.find('&', pos);
        std::string pair = (amp == std::string::npos) ? query.substr(pos) : query.substr(pos, amp - pos);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
            params[key] = val;
        }
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return params;
}

static std::string get_path(const std::string& request) {
    size_t sp1 = request.find(' ');
    if (sp1 == std::string::npos) return "/";
    size_t sp2 = request.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return "/";
    std::string path = request.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) return path.substr(0, qpos);
    return path;
}

static std::string build_response(const std::string& body, const std::string& content_type = "application/json") {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: " << content_type << "; charset=utf-8\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}

void HttpServer::register_routes() {}

bool HttpServer::start() {
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &impl_->wsa_data) != 0) {
        LOG_ERROR("[HTTP] WSAStartup failed");
        return false;
    }
#endif

    impl_->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->server_fd < 0) {
        LOG_ERROR("[HTTP] Failed to create socket");
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(impl_->server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(impl_->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(impl_->port);

    if (bind(impl_->server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("[HTTP] Failed to bind port {}", impl_->port);
        return false;
    }

    if (listen(impl_->server_fd, 10) < 0) {
        LOG_ERROR("[HTTP] Listen failed");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
#ifdef _WIN32
    setsockopt(impl_->server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
    setsockopt(impl_->server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    impl_->running = true;
    impl_->server_thread = std::thread([this]() {
        while (impl_->running) {
            sockaddr_in client_addr;
#ifdef _WIN32
            int addr_len = sizeof(client_addr);
            SOCKET client_fd = accept(impl_->server_fd, (sockaddr*)&client_addr, &addr_len);
#else
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(impl_->server_fd, (sockaddr*)&client_addr, &addr_len);
#endif

            if (client_fd < 0) {
                if (impl_->running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }

            char buffer[8192];
#ifdef _WIN32
            int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#else
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#endif

            std::string response;
            if (bytes > 0) {
                buffer[bytes] = '\0';
                std::string request(buffer);
                std::string path = get_path(request);
                auto params = parse_url_params(buffer);

                std::string body;
                if (path == "/api/crossbow-types") {
                    body = handle_get_crossbow_types(params);
                } else if (path == "/api/sensor-data") {
                    body = handle_get_sensor_data(params);
                } else if (path == "/api/shot-history") {
                    body = handle_get_shot_history(params);
                } else if (path == "/api/alerts") {
                    body = handle_get_alerts(params);
                } else if (path == "/api/accuracy") {
                    body = handle_get_accuracy(params);
                } else if (path == "/api/simulate-shot") {
                    body = handle_simulate_shot(params);
                } else if (path == "/api/run-analysis") {
                    body = handle_run_accuracy_analysis(params);
                } else if (path == "/api/resolve-alert") {
                    body = handle_resolve_alert(params);
                } else if (path == "/api/system-status") {
                    body = handle_get_system_status(params);
                } else if (path == "/api/ballistic-result") {
                    body = handle_get_ballistic_result(params);

                } else if (path == "/api/bowstring-materials") {
                    body = handle_get_bowstring_materials(params);
                } else if (path == "/api/bowstring-compare") {
                    body = handle_compare_bowstring_materials(params);
                } else if (path == "/api/bowstring-efficiency-curve") {
                    body = handle_get_bowstring_efficiency_curve(params);
                } else if (path == "/api/bowstring-optimize") {
                    body = handle_optimize_bowstring(params);

                } else if (path == "/api/sight-designs") {
                    body = handle_get_sight_designs(params);
                } else if (path == "/api/sight-simulate") {
                    body = handle_simulate_sight_optics(params);
                } else if (path == "/api/sight-calibrate") {
                    body = handle_calibrate_sight_scales(params);
                } else if (path == "/api/sight-optimize") {
                    body = handle_optimize_sight_design(params);

                } else if (path == "/api/formation-types") {
                    body = handle_get_formation_types(params);
                } else if (path == "/api/formation-simulate") {
                    body = handle_simulate_formation(params);
                } else if (path == "/api/formation-compare") {
                    body = handle_compare_formations(params);
                } else if (path == "/api/formation-rl-optimize") {
                    body = handle_rl_optimize_formation(params);
                } else if (path == "/api/formation-placement") {
                    body = handle_generate_formation_placement(params);

                } else if (path == "/") {
                    body = handle_get_system_status(params);
                } else {
                    body = "{\"status\":\"running\",\"version\":\"2.0\"}";
                }
                response = build_response(body);
            } else {
                response = build_response("{\"error\":\"bad request\"}");
            }

#ifdef _WIN32
            send(client_fd, response.c_str(), response.size(), 0);
            closesocket(client_fd);
#else
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
#endif
        }
    });

    LOG_INFO("[HTTP] Server started on port {}", impl_->port);
    return true;
}

void HttpServer::stop() {
    impl_->running = false;
    if (impl_->server_thread.joinable()) {
        impl_->server_thread.join();
    }
    if (impl_->server_fd >= 0) {
#ifdef _WIN32
        closesocket(impl_->server_fd);
        WSACleanup();
#else
        close(impl_->server_fd);
#endif
        impl_->server_fd = -1;
    }
    LOG_INFO("[HTTP] Server stopped");
}

bool HttpServer::is_running() const {
    return impl_->running;
}

CrossbowType get_default_crossbow() {
    CrossbowType cb;
    cb.id = 7;
    cb.name = "宋神臂弩";
    cb.dynasty = "宋朝";
    cb.draw_weight = 350;
    cb.bow_length = 1.75;
    cb.string_length = 1.15;
    cb.arrow_mass = 0.080;
    cb.effective_range = 300;
    cb.max_range = 520;
    return cb;
}

// ========== Feature 1: Bowstring Materials ==========

std::string HttpServer::handle_get_bowstring_materials(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/bowstring-materials");
    auto& db = BowstringMaterialDatabase::instance();
    auto materials = db.list_all();

    json arr = json::array();
    for (const auto& m : materials) {
        arr.push_back({
            {"type", static_cast<int>(m.type)},
            {"name", m.name},
            {"description", m.description},
            {"historical_origin", m.historical_origin},
            {"young_modulus_gpa", m.young_modulus / 1e9},
            {"tensile_strength_mpa", m.tensile_strength / 1e6},
            {"density", m.density},
            {"elongation_at_break", m.elongation_at_break},
            {"damping_coefficient", m.damping_coefficient},
            {"energy_efficiency", m.energy_efficiency},
            {"fatigue_life_cycles", m.fatigue_life_cycles},
            {"moisture_sensitivity", m.moisture_sensitivity},
            {"strand_count", m.strand_count},
            {"thickness_mm", m.thickness_mm}
        });
    }
    json j = {{"materials", arr}, {"count", arr.size()}};
    return j.dump();
}

std::string HttpServer::handle_compare_bowstring_materials(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/bowstring-compare");
    CrossbowType cb = get_default_crossbow();
    if (impl_->crossbow_types && !impl_->crossbow_types->empty()) {
        auto it = params.find("crossbow_id");
        if (it != params.end()) {
            int id = std::stoi(it->second);
            auto cit = impl_->crossbow_types->find(id);
            if (cit != impl_->crossbow_types->end()) cb = cit->second;
        } else {
            cb = impl_->crossbow_types->begin()->second;
        }
    }

    double angle = 15.0;
    auto ait = params.find("launch_angle");
    if (ait != params.end()) angle = std::stod(ait->second);

    BowstringPerformanceAnalyzer analyzer(cb);
    auto result = analyzer.compare_all_materials(angle);

    json mats = json::array();
    for (const auto& m : result.materials) {
        mats.push_back({
            {"name", m.material.name},
            {"type", static_cast<int>(m.material.type)},
            {"muzzle_velocity_mps", m.estimated_muzzle_velocity},
            {"energy_joules", m.estimated_energy},
            {"max_tension_newton", m.max_tension_before_break},
            {"string_mass_grams", m.string_mass * 1000},
            {"damping_loss_ratio", m.damping_loss_ratio},
            {"lifespan_days", m.lifespan_days},
            {"accuracy_penalty_factor", m.accuracy_penalty_factor},
            {"velocity_delta_pct",
                (m.estimated_muzzle_velocity - result.reference_velocity) /
                std::max(0.01, result.reference_velocity) * 100.0}
        });
    }
    json j = {
        {"crossbow", result.crossbow_name},
        {"draw_weight_kg", result.crossbow_draw_weight_kg},
        {"launch_angle_deg", angle},
        {"reference_velocity", result.reference_velocity},
        {"timestamp_ms", result.timestamp_ms},
        {"results", mats}
    };
    return j.dump();
}

std::string HttpServer::handle_get_bowstring_efficiency_curve(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/bowstring-efficiency-curve");
    auto& db = BowstringMaterialDatabase::instance();
    std::string mat_name = "silk_thread";
    auto it = params.find("material");
    if (it != params.end()) mat_name = it->second;
    auto material = db.get(mat_name);
    if (material.type == BowstringMaterialType::UNKNOWN) {
        material = db.get(BowstringMaterialType::SILK);
    }

    CrossbowType cb = get_default_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    json curve = json::array();
    auto eff_curve = analyzer.calculate_efficiency_curve(material, 0.1, 1.0, 50);
    auto life_curve = analyzer.calculate_lifespan_curve(material, 365, 200, 0.5);

    json eff_arr = json::array();
    for (const auto& p : eff_curve) eff_arr.push_back({{"pull_ratio", p.first}, {"efficiency", p.second}});

    json life_arr = json::array();
    for (const auto& p : life_curve) life_arr.push_back({{"day", p.first}, {"efficiency_remaining", p.second}});

    json j = {
        {"material", material.name},
        {"efficiency_curve", eff_arr},
        {"lifespan_curve", life_arr}
    };
    return j.dump();
}

std::string HttpServer::handle_optimize_bowstring(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/bowstring-optimize");
    CrossbowType cb = get_default_crossbow();
    BowstringPerformanceAnalyzer analyzer(cb);

    auto& db = BowstringMaterialDatabase::instance();
    auto candidates = db.list_all();

    double acc_w = 0.6, pw_w = 0.25, dur_w = 0.15;
    auto it = params.find("accuracy_weight");
    if (it != params.end()) acc_w = std::stod(it->second);
    it = params.find("power_weight");
    if (it != params.end()) pw_w = std::stod(it->second);
    it = params.find("durability_weight");
    if (it != params.end()) dur_w = std::stod(it->second);

    auto best = analyzer.optimize_material_for_accuracy(candidates, acc_w, pw_w, dur_w);

    json j = {
        {"optimal_material", best.name},
        {"material_type", static_cast<int>(best.type)},
        {"weights", {
            {"accuracy", acc_w}, {"power", pw_w}, {"durability", dur_w}
        }},
        {"properties", {
            {"young_modulus_gpa", best.young_modulus / 1e9},
            {"energy_efficiency", best.energy_efficiency},
            {"damping_coefficient", best.damping_coefficient},
            {"fatigue_life_cycles", best.fatigue_life_cycles}
        }}
    };
    return j.dump();
}

// ========== Feature 2: Sight Optics ==========

std::string HttpServer::handle_get_sight_designs(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/sight-designs");
    auto& db = SightDesignDatabase::instance();
    auto designs = db.list_all();

    json arr = json::array();
    for (const auto& d : designs) {
        arr.push_back({
            {"type", static_cast<int>(d.type)},
            {"name", d.name},
            {"description", d.description},
            {"historical_origin", d.historical_origin},
            {"sight_height_mm", d.sight_height_mm},
            {"sight_radius_mm", d.sight_radius_mm},
            {"notch_width_mm", d.notch_width_mm},
            {"post_diameter_mm", d.post_diameter_mm},
            {"aperture_diameter_mm", d.aperture_diameter_mm},
            {"scale_count", d.scale_count},
            {"scale_range", {d.scale_range_min_m, d.scale_range_max_m}},
            {"total_weight_g", d.total_weight_g},
            {"estimated_cost", d.estimated_cost},
            {"ratings", {
                {"user_experience", d.user_experience_rating},
                {"adjustability", d.adjustability_rating},
                {"durability", d.durability_rating}
            }}
        });
    }
    json j = {{"designs", arr}, {"count", arr.size()}};
    return j.dump();
}

std::string HttpServer::handle_simulate_sight_optics(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/sight-simulate");
    auto& db = SightDesignDatabase::instance();
    SightType st = SightType::RAMP_SIGHT;
    auto it = params.find("sight_type");
    if (it != params.end()) st = static_cast<SightType>(std::stoi(it->second));
    auto sight = db.get(st);

    double dist = 300.0;
    it = params.find("distance_m");
    if (it != params.end()) dist = std::stod(it->second);
    double elev = 15.0;
    it = params.find("elevation_deg");
    if (it != params.end()) elev = std::stod(it->second);
    double light = 10000.0;
    it = params.find("light_lux");
    if (it != params.end()) light = std::stod(it->second);
    double temp = 25.0;
    it = params.find("temperature_c");
    if (it != params.end()) temp = std::stod(it->second);

    OpticalErrorSimulator sim(42);
    sim.set_ambient_conditions(light, temp, 0.5, 2.0);
    sim.set_user_profile(1.0, 0.6);
    int samples = 100;
    auto results = sim.simulate_multiple_aims(sight, elev, 0.0, dist, samples);
    double cep = sim.estimate_sight_cep(sight, dist, samples * 5);

    json results_arr = json::array();
    int limit = std::min(50, (int)results.size());
    for (int i = 0; i < limit; i++) {
        const auto& r = results[i];
        results_arr.push_back({
            {"total_elev_mrad", r.total_error_elevation_mrad},
            {"total_azim_mrad", r.total_error_azimuth_mrad},
            {"impact_x_m", r.predicted_impact_offset_x_m},
            {"impact_y_m", r.predicted_impact_offset_y_m},
            {"confidence", r.aiming_confidence},
            {"optical_elev", r.optical_error_elevation_mrad},
            {"mech_elev", r.mechanical_error_elevation_mrad},
            {"env_elev", r.environmental_error_elevation_mrad}
        });
    }

    auto budget = sim.get_current_error_budget();
    json j = {
        {"sight_type", static_cast<int>(st)},
        {"sight_name", sight.name},
        {"target_distance_m", dist},
        {"cep_meters", cep},
        {"samples_total", (int)results.size()},
        {"samples_returned", limit},
        {"error_budget_mrad", {
            {"visual_acuity_arcmin", budget.visual_acuity_arcmin},
            {"atmospheric_refraction", budget.atmospheric_refraction_mrad},
            {"heat_haze", budget.heat_haze_mrad},
            {"turbulence_strength", budget.turbulence_strength},
            {"mechanical_play_mm", budget.mechanical_play_mm},
            {"glare_intensity", budget.glare_intensity_factor}
        }},
        {"aim_results", results_arr}
    };
    return j.dump();
}

std::string HttpServer::handle_calibrate_sight_scales(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/sight-calibrate");
    auto& db = SightDesignDatabase::instance();
    SightType st = SightType::RAMP_SIGHT;
    auto it = params.find("sight_type");
    if (it != params.end()) st = static_cast<SightType>(std::stoi(it->second));
    auto sight = db.get(st);

    CrossbowType cb = get_default_crossbow();
    SightScaleCalibrator cal;
    cal.set_crossbow(cb);
    cal.set_sight_design(sight);

    double start = 50.0, end = 600.0, step = 50.0;
    it = params.find("start_range_m");
    if (it != params.end()) start = std::stod(it->second);
    it = params.find("end_range_m");
    if (it != params.end()) end = std::stod(it->second);
    it = params.find("step_m");
    if (it != params.end()) step = std::stod(it->second);

    auto scales = cal.calibrate_scales(start, end, step);
    auto table = cal.generate_ballistic_table(start, end, step);

    json scales_arr = json::array();
    for (const auto& s : scales) {
        scales_arr.push_back({
            {"index", s.index},
            {"target_range_m", s.target_range_m},
            {"sight_height_mm", s.scale_height_mm},
            {"calibrated_angle_deg", s.calibrated_angle_deg},
            {"calibration_error_mrad", s.calibration_error_mrad}
        });
    }
    json table_arr = json::array();
    for (const auto& p : table) {
        table_arr.push_back({{"range_m", p.first}, {"elevation_deg", p.second}});
    }
    json j = {
        {"sight_type", static_cast<int>(st)},
        {"crossbow", cb.name},
        {"scales", scales_arr},
        {"ballistic_table", table_arr}
    };
    return j.dump();
}

std::string HttpServer::handle_optimize_sight_design(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/sight-optimize");
    SightType st = SightType::RAMP_SIGHT;
    auto it = params.find("base_type");
    if (it != params.end()) st = static_cast<SightType>(std::stoi(it->second));
    double dist = 300.0;
    it = params.find("target_range_m");
    if (it != params.end()) dist = std::stod(it->second);
    double acc_w = 0.45, usa_w = 0.25, mfg_w = 0.15, his_w = 0.15;
    it = params.find("accuracy_weight");
    if (it != params.end()) acc_w = std::stod(it->second);

    CrossbowType cb = get_default_crossbow();
    SightOptimizer opt;
    opt.set_crossbow(cb);
    opt.set_target_range(dist);
    opt.set_objective_weights(acc_w, usa_w, mfg_w, his_w);
    auto result = opt.optimize(st);

    json tradeoffs = json::array();
    for (const auto& t : result.design_tradeoffs) {
        tradeoffs.push_back({{"factor", t.first}, {"value", t.second}});
    }

    json j = {
        {"base_type", static_cast<int>(st)},
        {"target_range_m", dist},
        {"overall_score", result.overall_score},
        {"scores", {
            {"accuracy", result.accuracy_score},
            {"usability", result.usability_score},
            {"manufacturing", result.manufacturing_score},
            {"historical_authenticity", result.historical_authenticity_score}
        }},
        {"optimal_parameters", {
            {"height_mm", result.optimal_height_mm},
            {"radius_mm", result.optimal_radius_mm},
            {"scale_interval_m", result.optimal_scale_interval_m}
        }},
        {"cep_improvement_pct", result.ceph_improvement_percent},
        {"avg_error_mrad", result.average_total_error_mrad},
        {"tradeoffs", tradeoffs}
    };
    return j.dump();
}

// ========== Feature 3: Formation Simulation & RL ==========

std::string HttpServer::handle_get_formation_types(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/formation-types");
    auto& db = FormationDatabase::instance();
    auto formations = db.list_all();
    json arr = json::array();
    for (const auto& f : formations) {
        arr.push_back({
            {"type", static_cast<int>(f.type)},
            {"name", f.name},
            {"description", f.description},
            {"historical_origin", f.historical_origin},
            {"unit_counts", {
                {"min", f.min_crossbows},
                {"optimal", f.optimal_crossbows},
                {"max", f.max_crossbows}
            }},
            {"spacing_m", {
                {"frontage_per_unit", f.frontage_per_unit_m},
                {"depth_per_unit", f.depth_per_unit_m},
                {"min_safety", f.minimum_safety_spacing_m}
            }},
            {"best_range_m", f.best_suited_range_m},
            {"utility_scores", {
                {"defensive", f.defensive_utility},
                {"offensive", f.offensive_utility},
                {"siege", f.siege_utility},
                {"mobility", f.mobility_score},
                {"command_control", f.command_and_control_score}
            }}
        });
    }
    json j = {{"formations", arr}, {"count", (int)arr.size()}};
    return j.dump();
}

std::string HttpServer::handle_simulate_formation(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/formation-simulate");
    FormationType ft = FormationType::LINE_ABREAST;
    auto it = params.find("formation_type");
    if (it != params.end()) ft = static_cast<FormationType>(std::stoi(it->second));
    int n = 36;
    it = params.find("num_crossbows");
    if (it != params.end()) n = std::stoi(it->second);
    int volleys = 3;
    it = params.find("volleys");
    if (it != params.end()) volleys = std::stoi(it->second);

    TargetSpecification target;
    target.type = TargetType::AREA_RECTANGLE;
    target.name = "敌方阵地";
    target.center_x_m = 0;
    target.center_y_m = 250;
    target.width_m = 40;
    target.depth_m = 30;
    target.priority_factor = 1.0;
    target.hardness_factor = 0.5;
    target.personnel_estimate = 200;

    FormationSimulator sim(12345);
    auto types = impl_->crossbow_types ? *impl_->crossbow_types : std::map<int, CrossbowType>{};
    if (types.empty()) {
        CrossbowType cb = get_default_crossbow();
        types[cb.id] = cb;
    }
    sim.set_crossbow_types(types);
    sim.set_target(target);

    auto placements = sim.generate_formation(ft, n, 0, 0, 180.0, 1.0);
    auto eng = sim.simulate_engagement(placements, target, volleys, 2.0, 0.0, 10.0);
    eng.formation_type = ft;
    eng.formation_name = FormationDatabase::instance().get(ft).name;
    auto salvo = sim.optimize_salvo_timing(placements, target, 0.3);

    json place_arr = json::array();
    int p_limit = std::min(50, (int)placements.size());
    for (int i = 0; i < p_limit; i++) {
        const auto& p = placements[i];
        place_arr.push_back({
            {"id", p.id},
            {"crossbow_type_id", p.crossbow_type_id},
            {"x_m", p.x_m}, {"y_m", p.y_m},
            {"facing_deg", p.facing_deg},
            {"salvo_group", p.salvo_group},
            {"firing_delay_ms", p.firing_delay_ms}
        });
    }

    json hits_arr = json::array();
    int h_limit = std::min(200, (int)eng.hit_pattern.size());
    for (int i = 0; i < h_limit; i++) {
        hits_arr.push_back({eng.hit_pattern[i].first, eng.hit_pattern[i].second});
    }

    json j = {
        {"formation_type", static_cast<int>(ft)},
        {"formation_name", eng.formation_name},
        {"num_crossbows", eng.num_crossbows},
        {"volleys", volleys},
        {"results", {
            {"total_shots", eng.total_shots},
            {"hits", eng.total_hits},
            {"hit_probability", eng.overall_hit_probability},
            {"target_coverage_pct", eng.target_coverage_percent},
            {"saturation_score", eng.saturation_score},
            {"suppression_effect", eng.suppression_effect},
            {"logistical_efficiency", eng.logistical_efficiency},
            {"friendly_risk_counterfire_m", eng.friendly_risk_radius_m}
        }},
        {"firing_plan", {
            {"total_projectiles", eng.firing_plan.total_projectiles},
            {"density_projectiles_per_m2", eng.firing_plan.density_at_target_projectiles_per_m2},
            {"estimated_kill_prob", eng.firing_plan.estimated_kill_probability},
            {"estimated_casualties", eng.firing_plan.estimated_casualties},
            {"ammo_consumed_kg", eng.firing_plan.supply_burden_kg}
        }},
        {"salvo_optimization", {
            {"optimal_salvo_size", salvo.optimal_salvo_size},
            {"interval_ms", salvo.optimal_interval_ms},
            {"saturation_time_s", salvo.saturation_time_s},
            {"time_over_target_s", salvo.time_over_target_seconds}
        }},
        {"placements_count", (int)placements.size()},
        {"placements_sample", place_arr},
        {"hit_pattern_points", (int)eng.hit_pattern.size()},
        {"hit_pattern_sample", hits_arr},
        {"timestamp_ms", eng.timestamp_ms}
    };
    return j.dump();
}

std::string HttpServer::handle_compare_formations(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/formation-compare");
    int n = 36;
    auto it = params.find("num_crossbows");
    if (it != params.end()) n = std::stoi(it->second);
    int volleys = 2;
    it = params.find("volleys");
    if (it != params.end()) volleys = std::stoi(it->second);

    TargetSpecification target;
    target.type = TargetType::AREA_CIRCLE;
    target.name = "敌军集群";
    target.center_x_m = 0; target.center_y_m = 250;
    target.radius_m = 20; target.priority_factor = 1.0;
    target.hardness_factor = 0.3; target.personnel_estimate = 300;

    FormationSimulator sim(54321);
    auto types = impl_->crossbow_types ? *impl_->crossbow_types : std::map<int, CrossbowType>{};
    if (types.empty()) { types[get_default_crossbow().id] = get_default_crossbow(); }
    sim.set_crossbow_types(types);
    sim.set_target(target);

    auto all = {
        FormationType::LINE_ABREAST, FormationType::WEDGE, FormationType::VEE,
        FormationType::ARCHER_PARABOLA, FormationType::CRANE_WING,
        FormationType::FISH_SCALE, FormationType::SQUARE_GRID,
        FormationType::COILED_BOW, FormationType::ECHELON_LEFT
    };
    std::vector<FormationType> compare_list;
    auto f_it = params.find("formations");
    if (f_it != params.end()) {
        std::stringstream ss(f_it->second);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) compare_list.push_back(static_cast<FormationType>(std::stoi(token)));
        }
    } else {
        compare_list.assign(all.begin(), all.end());
    }

    auto results = sim.compare_formations(compare_list, n, target, volleys);

    json arr = json::array();
    for (const auto& r : results) {
        arr.push_back({
            {"formation_type", static_cast<int>(r.formation_type)},
            {"formation_name", r.formation_name},
            {"hit_probability", r.overall_hit_probability},
            {"coverage_pct", r.target_coverage_percent},
            {"saturation_score", r.saturation_score},
            {"suppression_effect", r.suppression_effect},
            {"logistical_efficiency", r.logistical_efficiency},
            {"estimated_casualties", r.firing_plan.estimated_casualties},
            {"ammo_kg", r.firing_plan.supply_burden_kg}
        });
    }
    json j = {
        {"num_crossbows", n}, {"volleys", volleys},
        {"target", target.name},
        {"comparisons", arr}
    };
    return j.dump();
}

std::string HttpServer::handle_rl_optimize_formation(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/formation-rl-optimize");
    int n = 36;
    auto it = params.find("num_crossbows");
    if (it != params.end()) n = std::stoi(it->second);
    int episodes = 200;
    it = params.find("episodes");
    if (it != params.end()) episodes = std::stoi(it->second);
    double lr = 0.1;
    it = params.find("learning_rate");
    if (it != params.end()) lr = std::stod(it->second);
    double gamma = 0.95;
    it = params.find("discount_factor");
    if (it != params.end()) gamma = std::stod(it->second);
    double eps = 0.2;
    it = params.find("exploration_rate");
    if (it != params.end()) eps = std::stod(it->second);

    TargetSpecification target;
    target.type = TargetType::AREA_RECTANGLE;
    target.name = "RL优化目标";
    target.center_x_m = 0; target.center_y_m = 280;
    target.width_m = 50; target.depth_m = 25;
    target.priority_factor = 1.0; target.personnel_estimate = 250;

    FormationRLOptimizer rl(98765);
    auto types = impl_->crossbow_types ? *impl_->crossbow_types : std::map<int, CrossbowType>{};
    if (types.empty()) { types[get_default_crossbow().id] = get_default_crossbow(); }
    rl.set_crossbow_types(types);
    rl.set_target(target);
    rl.set_training_conditions(std::max(6, n / 3), std::min(80, n * 2), true, true, true);

    auto result = rl.train_q_learning(n, episodes, lr, gamma, eps, 0.995);

    json rew = json::array();
    int skip = std::max(1, (int)result.episode_rewards.size() / 200);
    for (size_t i = 0; i < result.episode_rewards.size(); i += skip) {
        const auto& p = result.episode_rewards[i];
        rew.push_back({{"episode", p.first}, {"reward", p.second}});
    }
    json vals = json::array();
    for (const auto& fv : result.formation_value_estimates) {
        vals.push_back({
            {"formation_type", static_cast<int>(fv.first)},
            {"formation_name", FormationDatabase::instance().get(fv.first).name},
            {"estimated_value", fv.second}
        });
    }

    json j = {
        {"best_formation_type", static_cast<int>(result.best_formation)},
        {"best_formation_name", FormationDatabase::instance().get(result.best_formation).name},
        {"best_reward", result.best_reward},
        {"best_placement_params", {
            {"spacing_multiplier", result.best_placement_params.size() > 0 ? result.best_placement_params[0] : 1.0},
            {"orientation_offset_deg", result.best_placement_params.size() > 1 ? result.best_placement_params[1] : 0.0}
        }},
        {"episodes_trained", result.episodes_trained},
        {"convergence_rate", result.convergence_rate},
        {"training_rewards_sampled", rew},
        {"formation_value_estimates", vals},
        {"timestamp_ms", result.timestamp_ms}
    };
    return j.dump();
}

std::string HttpServer::handle_generate_formation_placement(const std::map<std::string, std::string>& params) {
    METRICS_HTTP_REQ("/api/formation-placement");
    FormationType ft = FormationType::LINE_ABREAST;
    auto it = params.find("formation_type");
    if (it != params.end()) ft = static_cast<FormationType>(std::stoi(it->second));
    int n = 24;
    it = params.find("num_crossbows");
    if (it != params.end()) n = std::stoi(it->second);
    double cx = 0, cy = 0;
    it = params.find("center_x"); if (it != params.end()) cx = std::stod(it->second);
    it = params.find("center_y"); if (it != params.end()) cy = std::stod(it->second);
    double orient = 0;
    it = params.find("orientation_deg"); if (it != params.end()) orient = std::stod(it->second);
    double spacing = 1.0;
    it = params.find("spacing_multiplier"); if (it != params.end()) spacing = std::stod(it->second);

    TargetSpecification target;
    target.type = TargetType::SINGLE_POINT;
    target.center_x_m = 0; target.center_y_m = 300;
    FormationSimulator sim(11111);
    sim.set_target(target);
    auto types = impl_->crossbow_types ? *impl_->crossbow_types : std::map<int, CrossbowType>{};
    if (types.empty()) { types[get_default_crossbow().id] = get_default_crossbow(); }
    sim.set_crossbow_types(types);
    auto placements = sim.generate_formation(ft, n, cx, cy, orient, spacing);

    json arr = json::array();
    for (const auto& p : placements) {
        arr.push_back({
            {"id", p.id},
            {"type_id", p.crossbow_type_id},
            {"name", p.name},
            {"x", p.x_m}, {"y", p.y_m}, {"z", p.z_m},
            {"facing_deg", p.facing_deg},
            {"elevation_deg", p.elevation_deg},
            {"firing_delay_ms", p.firing_delay_ms},
            {"salvo_group", p.salvo_group}
        });
    }
    json j = {
        {"formation_type", static_cast<int>(ft)},
        {"formation_name", FormationDatabase::instance().get(ft).name},
        {"center", {cx, cy}},
        {"orientation_deg", orient},
        {"spacing_multiplier", spacing},
        {"count", (int)placements.size()},
        {"placements", arr}
    };
    return j.dump();
}
