#include "modules/dynamics_solver.h"
#include "dynamics_model.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <chrono>
#include <memory>

namespace crossbow {
namespace modules {

DynamicsSolver::DynamicsSolver(size_t num_threads)
    : num_threads_(num_threads)
    , running_(false)
    , next_task_id_(1)
    , pending_count_(0)
    , completed_count_(0) {}

DynamicsSolver::~DynamicsSolver() {
    stop();
}

void DynamicsSolver::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_.load()) {
        return;
    }
    running_.store(true);
    workers_.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
        workers_.emplace_back(&DynamicsSolver::worker_loop, this);
    }
}

void DynamicsSolver::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

bool DynamicsSolver::is_running() const {
    return running_.load();
}

uint64_t DynamicsSolver::submit_task(const SolverTask& task, ResultCallback callback) {
    uint64_t task_id = next_task_id_.fetch_add(1);
    SolverTask queued_task = task;
    queued_task.task_id = task_id;

    QueuedTask qt;
    qt.task = queued_task;
    qt.callback = std::move(callback);
    qt.promise = nullptr;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.push(std::move(qt));
        pending_count_.fetch_add(1);
    }
    cv_.notify_one();
    return task_id;
}

SolverResult DynamicsSolver::submit_task_sync(const SolverTask& task) {
    uint64_t task_id = next_task_id_.fetch_add(1);
    SolverTask queued_task = task;
    queued_task.task_id = task_id;

    auto promise = std::make_shared<std::promise<SolverResult>>();
    auto future = promise->get_future();

    QueuedTask qt;
    qt.task = queued_task;
    qt.callback = nullptr;
    qt.promise = promise;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.push(std::move(qt));
        pending_count_.fetch_add(1);
    }
    cv_.notify_one();

    return future.get();
}

bool DynamicsSolver::cancel_task(uint64_t task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cancelled_tasks_.find(task_id);
    if (it != cancelled_tasks_.end()) {
        return false;
    }
    cancelled_tasks_.insert(task_id);
    return true;
}

size_t DynamicsSolver::get_pending_count() const {
    return pending_count_.load();
}

size_t DynamicsSolver::get_completed_count() const {
    return completed_count_.load();
}

void DynamicsSolver::worker_loop() {
    while (running_.load()) {
        QueuedTask qt;
        bool has_task = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return !task_queue_.empty() || !running_.load();
            });
            if (!running_.load() && task_queue_.empty()) {
                return;
            }
            if (!task_queue_.empty()) {
                qt = std::move(const_cast<QueuedTask&>(task_queue_.top()));
                task_queue_.pop();

                auto cancelled_it = cancelled_tasks_.find(qt.task.task_id);
                if (cancelled_it != cancelled_tasks_.end()) {
                    cancelled_tasks_.erase(cancelled_it);
                    pending_count_.fetch_sub(1);
                    completed_count_.fetch_add(1);
                    continue;
                }

                has_task = true;
            }
        }

        if (has_task) {
            SolverResult result = execute_task(qt.task);
            pending_count_.fetch_sub(1);
            completed_count_.fetch_add(1);

            if (qt.callback) {
                qt.callback(result);
            }
            if (qt.promise) {
                qt.promise->set_value(std::move(result));
            }
        }
    }
}

SolverResult DynamicsSolver::execute_task(const SolverTask& task) {
    auto start_time = std::chrono::steady_clock::now();
    SolverResult result;
    result.task_id = task.task_id;
    result.success = false;

    try {
        DynamicsModel model(task.crossbow);

        auto trajectory = model.simulate_trajectory(
            task.initial_velocity,
            task.launch_angle,
            task.wind_speed_ms,
            task.wind_direction_deg,
            task.time_step
        );

        ShotRecord shot_record = model.calculate_shot_results(trajectory);

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(end_time - start_time);

        result.task_id = task.task_id;
        result.success = true;
        result.error_message = "";
        result.trajectory = std::move(trajectory);
        result.shot_record = shot_record;
        result.elapsed_ms = elapsed.count();
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(end_time - start_time);
        result.error_message = e.what();
        result.elapsed_ms = elapsed.count();
    } catch (...) {
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(end_time - start_time);
        result.error_message = "Unknown exception occurred";
        result.elapsed_ms = elapsed.count();
    }

    return result;
}

} // namespace modules
} // namespace crossbow
