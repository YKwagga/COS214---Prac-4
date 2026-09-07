#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

// Minimal, dependency-free unit test framework.
// No internet access was available to fetch Catch2/GoogleTest, so this
// self-registering header keeps the whole test project buildable with
// nothing but g++. Behaviour it supports:
//   TEST(name) { ... CHECK(expr); ... }
//   CHECK(expr)      - records pass/fail, keeps running (does not abort)
//   REQUIRE(expr)    - records failure and returns from the test immediately
// run_all_tests() prints a summary and returns the number of failures
// (0 == success), suitable for use as the test binary's exit code.

#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

inline int& failure_count() {
    static int count = 0;
    return count;
}

inline int& check_count() {
    static int count = 0;
    return count;
}

inline void record_check(bool passed, const char* expr, const char* file, int line) {
    ++check_count();
    if (!passed) {
        ++failure_count();
        std::cout << "    FAILED: " << expr << " (" << file << ":" << line << ")\n";
    }
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back(TestCase{name, fn});
    }
};

inline int run_all_tests() {
    int total = static_cast<int>(registry().size());
    int failedTests = 0;
    for (std::vector<TestCase>::iterator it = registry().begin(); it != registry().end(); ++it) {
        int before = failure_count();
        std::cout << "[ RUN  ] " << it->name << "\n";
        it->fn();
        if (failure_count() > before) {
            ++failedTests;
            std::cout << "[ FAIL ] " << it->name << "\n";
        } else {
            std::cout << "[  OK  ] " << it->name << "\n";
        }
    }
    std::cout << "\n" << (total - failedTests) << "/" << total << " test cases passed, "
               << check_count() << " assertion(s) checked, " << failure_count() << " assertion failure(s)\n";
    return failure_count();
}

} // namespace testfw

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

#define TEST(test_name) \
    static void TEST_CONCAT(test_fn_, __LINE__)(); \
    static testfw::Registrar TEST_CONCAT(test_registrar_, __LINE__)(test_name, TEST_CONCAT(test_fn_, __LINE__)); \
    static void TEST_CONCAT(test_fn_, __LINE__)()

#define CHECK(expr) testfw::record_check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

// NOTE: expr is captured into a bool exactly once. Evaluating it twice
// (e.g. once to log, once in the "if") would silently double-fire any
// side effect the expression has -- which matters a lot here since most
// REQUIREs wrap state-changing calls like film.startShooting().
#define REQUIRE(expr) \
    do { \
        bool testfw_result_ = static_cast<bool>(expr); \
        testfw::record_check(testfw_result_, #expr, __FILE__, __LINE__); \
        if (!testfw_result_) return; \
    } while (0)

#endif
