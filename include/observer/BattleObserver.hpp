#pragma once
// =============================================================================
//  BattleObserver.hpp  ——  【设计模式 5/8：观察者模式 Observer（观察者一侧）】
//
//  这里放四个具体的观察者。它们的共同点是：都只依赖 EventBus 推过来的
//  BattleEvent，彼此之间毫无耦合，也不被战斗逻辑反向依赖。
//
//  这一点正是观察者模式的价值所在 —— 想加"伤害统计面板""战斗回放""录像导出"
//  等新功能，只需新增一个类并 attach()，战斗结算代码一行都不用改。
// =============================================================================

#include <string>
#include <vector>

#include "character/Combatant.hpp"
#include "core/EventBus.hpp"

namespace gb {

/// 观察者 A：战斗日志。把每条事件按缩进打印成人类可读的日志
class BattleLogObserver : public IBattleObserver {
public:
    void onEvent(const BattleEvent& event) override;
    std::string observerName() const override { return "战斗日志"; }
};

/// 观察者 B：成就统计。跨回合累计数据，达到条件就解锁成就
class AchievementObserver : public IBattleObserver {
public:
    void onEvent(const BattleEvent& event) override;
    std::string observerName() const override { return "成就系统"; }

    int criticalCount() const { return criticalCount_; }
    int blockCount() const { return blockCount_; }
    int killCount() const { return killCount_; }

private:
    int criticalCount_ = 0;
    int blockCount_ = 0;
    int killCount_ = 0;
    int maxSingleHit_ = 0;
    std::vector<std::string> unlocked_;
    std::size_t reported_ = 0;      ///< 已播报过的成就条数，避免重复打印
};

/// 观察者 C：BOSS 阶段监听。
/// 与状态模式联动：监听到被观察单位血量跌破阈值时，给它挂上狂暴状态。
/// 战斗结算完全不需要知道"BOSS 有第二阶段"这回事。
class BossPhaseObserver : public IBattleObserver {
public:
    BossPhaseObserver(Combatant* watched, double hpThreshold, StateKind triggerState);

    void onEvent(const BattleEvent& event) override;
    std::string observerName() const override { return "BOSS 阶段监听"; }

private:
    Combatant* watched_;
    double     hpThreshold_;
    StateKind  triggerState_;
    bool       triggered_ = false;      ///< 只触发一次，防止反复进入狂暴
};

/// 观察者 D：战斗回放录制。把所有事件按时间顺序存下来，战斗结束后可导出一份战报
class BattleRecorderObserver : public IBattleObserver {
public:
    void onEvent(const BattleEvent& event) override;
    std::string observerName() const override { return "战斗回放"; }

    void dump() const;
    std::size_t recordedCount() const { return records_.size(); }

private:
    std::vector<BattleEvent> records_;
};

} // namespace gb
