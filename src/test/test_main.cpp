#include "src/test/TestFramework.h"
#include "src/test/TestCardSelection.cpp"
#include "src/test/TestVulnerableRelic.cpp"
#include "src/test/TestDamagePipeline.cpp"
#include "src/test/TestMonster.cpp"
#include "src/test/TestRandomDamageAction.cpp"

int main() {
    std::cout << "========================================\n";
    std::cout << "  STS_CPP 单元测试套件\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    CardSelectionTests::runAllTests();
    std::cout << "\n";
    VulnerableRelicTests::runAllTests();
    std::cout << "\n";
    DamagePipelineTests::runAllTests();
    std::cout << "\n";
    BlockPipelineTests::runAllTests();
    std::cout << "\n";
    MonsterTests::runAllTests();
    std::cout << "\n";
    RandomDamageActionTests::runAllTests();
    std::cout << "\n";

    std::cout << "\n========================================\n";
    std::cout << "  所有测试完成\n";
    std::cout << "========================================\n";

    return 0;
}
