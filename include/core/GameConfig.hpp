#pragma once
// =============================================================================
//  GameConfig.hpp  ——  【设计模式 1/8：单例模式 Singleton】
//
//  意图：保证一个类仅有一个实例，并提供一个访问它的全局访问点。
//
//  在本项目中的落点：
//      整个战斗系统共享同一份"规则参数"（最大回合数、暴击倍率……）以及
//      同一台随机数发生器。若各处各自 new 一份，会出现：
//        · 参数不一致（一处改了暴击倍率，另一处还是旧值）
//        · 随机源互相独立，战斗日志无法复现，调试困难
//      因此把它做成单例。
//
//  实现要点：
//      · 构造函数私有 —— 单例最核心的约束
//      · 拷贝构造 / 赋值运算符 delete —— 防止被复制出第二个实例
//      · Meyers Singleton（C++11 静态局部变量）—— 天然线程安全、无需手写锁
// =============================================================================

#include <random>
#include <string>

namespace gb {

class GameConfig {
public:
    /// 全局唯一访问点。C++11 保证函数内静态局部变量的初始化是线程安全的。
    static GameConfig& instance();

    /// 禁用拷贝 —— 否则可以从既有实例"克隆"出第二个单例，单例就失效了
    GameConfig(const GameConfig&) = delete;
    GameConfig& operator=(const GameConfig&) = delete;

    // ---------------------- 战斗规则参数 ----------------------
    int    maxTurns()        const { return maxTurns_; }        ///< 一场战斗的回合上限
    double critMultiplier()  const { return critMultiplier_; }  ///< 暴击伤害倍率
    double blockRate()       const { return blockRate_; }       ///< 基础格挡概率
    double blockReduction()  const { return blockReduction_; }  ///< 格挡后的伤害折减
    bool   verbose()         const { return verbose_; }         ///< 是否输出详细日志

    void setVerbose(bool v)      { verbose_ = v; }
    void setCritMultiplier(double m) { critMultiplier_ = m; }

    // ---------------------- 全局随机源 ----------------------
    /// 设定随机种子：同一颗种子 + 同一套操作 = 完全相同的战斗日志，便于写文档与答辩演示
    void setSeed(unsigned int seed);

    /// 闭区间 [low, high] 上的均匀整数
    int roll(int low, int high);

    /// [0.0, 1.0) 上的均匀浮点数，用于概率判定（暴击 / 闪避 / 格挡）
    double roll01();

    /// 打印当前生效的规则参数
    void banner() const;

private:
    GameConfig();                       // 私有构造：只有 instance() 能创建它

    int    maxTurns_        = 30;       // 30 回合足够打完演示局，也防止死循环
    double critMultiplier_  = 2.0;      // 暴击造成 2 倍伤害
    double blockRate_       = 0.18;     // 18% 概率触发格挡
    double blockReduction_  = 0.70;     // 格挡后只吃 70% 伤害
    bool   verbose_         = true;

    std::mt19937 rng_;                  // 梅森旋转随机数引擎
};

} // namespace gb
