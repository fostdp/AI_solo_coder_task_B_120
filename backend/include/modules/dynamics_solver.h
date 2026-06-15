#pragma once

#include "common.h"
#include "dynamics_model.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <vector>
#include <memory>
#include <unordered_set>

namespace crossbow {
namespace modules {

struct SolverTask {
    uint64_t task_id;
    CrossbowType crossbow;
    double initial_velocity;
    double launch_angle;
    double wind_speed_ms;
    double wind_direction_deg;
    double time_step;
    uint32_t priority;
};

struct SolverResult {
    uint64_t task_id;
    bool success;
    std::string error_message;
    std::vector<TrajectoryPoint> trajectory;
    ShotRecord shot_record;
    double elapsed_ms;
};

using ResultCallback = std::function<void(const SolverResult&)>;

class DynamicsSolver {
public:
    explicit DynamicsSolver(size_t num_threads = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 2);
    ~DynamicsSolver();

    DynamicsSolver(const DynamicsSolver&) = delete;
    DynamicsSolver& operator=(const DynamicsSolver&) = delete;

    void start();
    void stop();
    bool is_running() const;

    uint64_t submit_task(const SolverTask& task, ResultCallback callback = nullptr);
    SolverResult submit_task_sync(const SolverTask& task);

    bool cancel_task(uint64_t task_id);

    size_t get_pending_count() const;
    size_t get_completed_count() const;

private:
    struct QueuedTask {
        SolverTask task;
        ResultCallback callback;
        std::shared_ptr<std::promise<SolverResult>> promise;

        bool operator<(const QueuedTask& other) const {
            return task.priority < other.task.priority;
        }
    };

    void worker_loop();
    SolverResult execute_task(const SolverTask& task);

    size_t num_threads_;
    std::vector<std::thread> workers_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::priority_queue<QueuedTask> task_queue_;
    std::unordered_set<uint64_t> cancelled_tasks_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> next_task_id_;
    std::atomic<size_t> pending_count_;
    std::atomic<size_t> completed_count_;
};

} // namespace modules
} // namespace crossbow
