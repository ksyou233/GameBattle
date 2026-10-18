// =============================================================================
//  GameConfig.cpp  ——  单例模式的实现
// =============================================================================

#include "core/GameConfig.hpp"

#include <iostream>

namespace gb {

GameConfig::GameConfig() : rng_(std::random_device{}()) {
    // 构造函数私有，只可能被 instance() 内的静态局部变量调用一次。
    // 默认用真随机数做种子，保证每次运行战斗过程不同。
}

GameConfig& GameConfig::instance() {
    // Meyers Singleton：函数内静态局部变量
    //   · C++11 起保证"首次控制流到达声明处时初始化"，且该初始化是线程安全的
    //   · 程序结束时自动析构，不需要手动释放，也不会有内存泄漏
    static GameConfig theInstance;
    return theInstance;
}

void GameConfig::setSeed(unsigned int seed) {
    rng_.seed(seed);
}

int GameConfig::roll(int low, int high) {
    if (low > high) std::swap(low, high);
    std::uniform_int_distribution<int> dist(low, high);
    return dist(rng_);
}

double GameConfig::roll01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_);
}

void GameConfig::banner() const {
    std::cout << "========================================================\n";
    std::cout << "   游戏战斗与技能系统 —— 设计模式综合演示\n";
    std::cout << "   采用设计模式：单例 / 抽象工厂 / 策略 / 装饰器 /\n";
    std::cout << "                 观察者 / 状态 / 责任链 / 组合\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << "   回合上限：" << maxTurns_
              << "    暴击倍率：x" << critMultiplier_
              << "    格挡概率：" << static_cast<int>(blockRate_ * 100) << "%\n";
    std::cout << "========================================================\n";
}

} // namespace gb
