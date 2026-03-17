#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace TestFramework {

struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class TestSuite {
public:
    std::string suiteName;
    std::vector<TestResult> results;
    int passed = 0;
    int failed = 0;

    TestSuite(std::string name) : suiteName(name) {}

    void addResult(const std::string& testName, bool passed, const std::string& message = "") {
        results.push_back({testName, passed, message});
        if (passed) {
            this->passed++;
        } else {
            this->failed++;
        }
    }

    void printReport() {
        std::cout << "\n========================================\n";
        std::cout << "测试套件: " << suiteName << "\n";
        std::cout << "========================================\n";
        
        for (const auto& result : results) {
            std::string status = result.passed ? "[PASS]" : "[FAIL]";
            std::cout << status << " " << result.testName;
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << "\n";
        }
        
        std::cout << "----------------------------------------\n";
        std::cout << "总计: " << passed << " 通过, " << failed << " 失败\n";
        std::cout << "========================================\n";
    }

    bool allPassed() {
        return failed == 0;
    }
};

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(message); \
    }

#define TEST_ASSERT_EQ(actual, expected, message) \
    if ((actual) != (expected)) { \
        std::ostringstream oss; \
        oss << message << " (期望: " << expected << ", 实际: " << actual << ")"; \
        throw std::runtime_error(oss.str()); \
    }

#define TEST_ASSERT_NEAR(actual, expected, epsilon, message) \
    if (std::abs((actual) - (expected)) > (epsilon)) { \
        std::ostringstream oss; \
        oss << message << " (期望: " << expected << ", 实际: " << actual << ")"; \
        throw std::runtime_error(oss.str()); \
    }

#define RUN_TEST(suite, testFunc) \
    try { \
        testFunc(); \
        suite.addResult(#testFunc, true); \
    } catch (const std::exception& e) { \
        suite.addResult(#testFunc, false, e.what()); \
    }

}
