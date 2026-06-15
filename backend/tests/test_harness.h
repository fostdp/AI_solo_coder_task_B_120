#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace crossbow {
namespace test {

namespace detail {
    template<typename T>
    struct Printable {
        const T& val;
        friend std::ostream& operator<<(std::ostream& os, const Printable& p) {
            if constexpr (std::is_enum_v<T>) {
                return os << static_cast<int>(p.val);
            } else {
                return os << p.val;
            }
        }
    };

    template<typename T>
    Printable<T> printable(const T& val) { return Printable<T>{val}; }
}

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

struct TestStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    double total_time_ms = 0;
};

class TestHarness {
public:
    static TestHarness& instance() {
        static TestHarness inst;
        return inst;
    }

    void register_test(const std::string& suite, const std::string& name, std::function<void()> fn) {
        tests_.push_back({suite, name, fn});
    }

    int run_all() {
        std::cout << "============================================================" << std::endl;
        std::cout << "  弩机动力学仿真系统 - 单元测试" << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::endl;

        TestStats stats;
        stats.total = (int)tests_.size();

        std::string current_suite;
        for (const auto& tc : tests_) {
            if (tc.suite != current_suite) {
                std::cout << std::endl;
                std::cout << "[测试套件] " << tc.suite << std::endl;
                std::cout << "------------------------------------------------------------" << std::endl;
                current_suite = tc.suite;
            }

            std::cout << "  测试: " << std::left << std::setw(50) << tc.name << " ... ";
            std::cout.flush();

            auto start = std::chrono::high_resolution_clock::now();
            try {
                tc.fn();
                auto end = std::chrono::high_resolution_clock::now();
                double dur = std::chrono::duration<double, std::milli>(end - start).count();
                std::cout << "\033[32mPASS\033[0m  (" << std::fixed << std::setprecision(2) << dur << "ms)" << std::endl;
                stats.passed++;
                stats.total_time_ms += dur;
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                double dur = std::chrono::duration<double, std::milli>(end - start).count();
                std::cout << "\033[31mFAIL\033[0m  (" << std::fixed << std::setprecision(2) << dur << "ms)" << std::endl;
                std::cout << "       错误: " << e.what() << std::endl;
                stats.failed++;
                stats.total_time_ms += dur;
            } catch (...) {
                auto end = std::chrono::high_resolution_clock::now();
                double dur = std::chrono::duration<double, std::milli>(end - start).count();
                std::cout << "\033[31mFAIL\033[0m  (" << std::fixed << std::setprecision(2) << dur << "ms)" << std::endl;
                std::cout << "       错误: 未知异常" << std::endl;
                stats.failed++;
                stats.total_time_ms += dur;
            }
        }

        std::cout << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << "  测试汇总: 总共 " << stats.total
                  << " | 通过 " << stats.passed
                  << " | 失败 " << stats.failed
                  << " | 耗时 " << std::fixed << std::setprecision(2) << stats.total_time_ms << "ms" << std::endl;
        std::cout << "============================================================" << std::endl;

        return stats.failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;
    TestHarness() = default;
};

// ============== 断言宏 ==============

#define TEST_CASE(suite, name) \
    static void suite##_##name(); \
    namespace { \
        bool _reg_##suite##_##name = []() { \
            crossbow::test::TestHarness::instance().register_test(#suite, #name, suite##_##name); \
            return true; \
        }(); \
    } \
    static void suite##_##name()

#define REQUIRE(cond) \
    do { if (!(cond)) { \
        std::ostringstream oss; \
        oss << "REQUIRE(" #cond ") 失败"; \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_MSG(cond, msg) \
    do { if (!(cond)) { \
        std::ostringstream oss; \
        oss << msg; \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_EQ(a, b) \
    do { if (!((a) == (b))) { \
        std::ostringstream oss; \
        oss << "REQUIRE_EQ(" #a ", " #b ") 失败: " << crossbow::test::detail::printable(a) << " != " << crossbow::test::detail::printable(b); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_NEAR(a, b, eps) \
    do { if (std::abs((a) - (b)) > (eps)) { \
        std::ostringstream oss; \
        oss << "REQUIRE_NEAR(" #a ", " #b ", " #eps ") 失败: " \
            << (a) << " ≉ " << (b) << " (误差=" << std::abs((a)-(b)) << ")"; \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_GT(a, b) \
    do { if (!((a) > (b))) { \
        std::ostringstream oss; \
        oss << "REQUIRE_GT(" #a ", " #b ") 失败: " << crossbow::test::detail::printable(a) << " <= " << crossbow::test::detail::printable(b); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_LT(a, b) \
    do { if (!((a) < (b))) { \
        std::ostringstream oss; \
        oss << "REQUIRE_LT(" #a ", " #b ") 失败: " << crossbow::test::detail::printable(a) << " >= " << crossbow::test::detail::printable(b); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_GE(a, b) \
    do { if (!((a) >= (b))) { \
        std::ostringstream oss; \
        oss << "REQUIRE_GE(" #a ", " #b ") 失败: " << crossbow::test::detail::printable(a) << " < " << crossbow::test::detail::printable(b); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_LE(a, b) \
    do { if (!((a) <= (b))) { \
        std::ostringstream oss; \
        oss << "REQUIRE_LE(" #a ", " #b ") 失败: " << crossbow::test::detail::printable(a) << " > " << crossbow::test::detail::printable(b); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_NOTHROW(expr) \
    do { try { (expr); } catch (const std::exception& e) { \
        std::ostringstream oss; \
        oss << "REQUIRE_NOTHROW(" #expr ") 抛出异常: " << e.what(); \
        throw std::runtime_error(oss.str()); \
    } } while(0)

#define REQUIRE_THROWS(expr) \
    do { bool _thrown = false; \
         try { (expr); } catch (...) { _thrown = true; } \
         if (!_thrown) { \
             std::ostringstream oss; \
             oss << "REQUIRE_THROWS(" #expr ") 未抛出异常"; \
             throw std::runtime_error(oss.str()); \
         } } while(0)

} // namespace test
} // namespace crossbow
