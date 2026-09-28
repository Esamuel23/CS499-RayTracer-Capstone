//=============================================================================
// File:     test_main.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Runs every registered test and reports the result.
//=============================================================================
#include <exception>
#include <iostream>

#include "test_framework.h"

int main() {
    int crashed = 0;
    for (const rt_test::TestCase& test : rt_test::registry()) {
        const int failures_before = rt_test::failure_count();
        bool threw = false;
        try {
            test.function();
        } catch (const std::exception& error) {
            threw = true;
            ++crashed;
            std::cerr << "  EXCEPTION " << error.what() << "\n";
        }
        const bool passed = !threw && rt_test::failure_count() == failures_before;
        std::cout << (passed ? "[PASS] " : "[FAIL] ") << test.name << "\n";
    }

    const int total_failures = rt_test::failure_count() + crashed;
    std::cout << "\n" << rt_test::registry().size() << " tests, " << total_failures
              << " failures\n";
    return total_failures == 0 ? 0 : 1;
}
