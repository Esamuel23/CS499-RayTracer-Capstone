//=============================================================================
// File:     test_framework.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Minimal self-registering unit test harness.
//
// Design notes:
//   A small harness keeps the build free of downloads, so it works offline
//   and on any compiler. CTest runs the executable. A nonzero exit code
//   marks the run as failed in CI.
//=============================================================================
#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace rt_test {

struct TestCase {
    const char* name;
    void (*function)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failure_count() {
    static int failures = 0;
    return failures;
}

struct Registrar {
    Registrar(const char* name, void (*function)()) { registry().push_back({name, function}); }
};

inline void report_failure(const char* file, int line, const std::string& message) {
    ++failure_count();
    std::cerr << "  FAILED " << file << ":" << line << "  " << message << "\n";
}

}  // namespace rt_test

// Declares and registers a test function.
#define RT_TEST(name)                                                  \
    static void name();                                                \
    static const rt_test::Registrar name##_registrar(#name, &name);    \
    static void name()

#define CHECK(condition)                                               \
    do {                                                               \
        if (!(condition)) {                                            \
            rt_test::report_failure(__FILE__, __LINE__, #condition);   \
        }                                                              \
    } while (false)

// Floating-point values are compared with a tolerance, never with ==.
#define CHECK_NEAR(actual, expected, tolerance)                                    \
    do {                                                                           \
        const double rt_actual = (actual);                                         \
        const double rt_expected = (expected);                                     \
        if (!(std::fabs(rt_actual - rt_expected) <= (tolerance))) {                \
            rt_test::report_failure(__FILE__, __LINE__,                            \
                                    std::string(#actual) + " = " +                 \
                                        std::to_string(rt_actual) + ", expected " + \
                                        std::to_string(rt_expected));              \
        }                                                                          \
    } while (false)

// Passes only if the expression throws the named exception type.
#define CHECK_THROWS_AS(expression, exception_type)                              \
    do {                                                                         \
        bool rt_threw = false;                                                   \
        try {                                                                    \
            (void)(expression);                                                  \
        } catch (const exception_type&) {                                        \
            rt_threw = true;                                                     \
        } catch (...) {                                                          \
        }                                                                        \
        if (!rt_threw) {                                                         \
            rt_test::report_failure(__FILE__, __LINE__,                          \
                                    std::string(#expression) +                   \
                                        " did not throw " #exception_type);      \
        }                                                                        \
    } while (false)
