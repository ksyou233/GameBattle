#pragma once
// =============================================================================
//  EventBus.hpp  ——  【设计模式 5/8：观察者模式 Observer】
//
//  意图：定义对象间的一对多依赖。当一个对象（主题 Subject）状态改变时，
//        所有依赖它的对象（观察者 Observer）都得到通知并自动更新。
//
//  在本项目中的落点：
//      战斗过程中会发生大量事件 —— 掉血、暴击、闪避、治疗、状态变化、阵亡。
//      如果让"结算伤害"的代码直接去调用 日志打印 / 成就统计 / BOSS 阶段切换，
//      战斗逻辑就会被 UI 与业务无关的杂事污染，且每加一个功能都要改结算代码。
//      因此引入 EventBus 作为主题，各功能模块作为观察者按需订阅。
//
//  收益：
//      · 战斗结算只管广播事件，不知道谁在听 —— 解耦
//      · 新增"战斗回放""伤害统计面板"等功能，只需再写一个观察者并注册，零改动既有代码
// =============================================================================

#include <string>
#include <vector>

namespace gb {

/// 战斗事件类型
enum class EventType {
    BattleStart,     ///< 战斗开始
    TurnStart,       ///< 某回合开始
    DamageDealt,     ///< 造成普通伤害
    CriticalHit,     ///< 造成暴击伤害
    Dodged,          ///< 攻击被闪避
    Blocked,         ///< 攻击被格挡
    Healed,          ///< 被治疗
    StateChanged,    ///< 状态变化（眩晕 / 冰冻 / 狂暴……）
    CharacterDied,   ///< 单位阵亡
    NoAction,        ///< 因状态无法行动
    BattleEnd        ///< 战斗结束
};

/// 战斗事件数据包：观察者收到它就能知道"发生了什么"
struct BattleEvent {
    EventType   type{EventType::DamageDealt};
    std::string source;        ///< 事件发起者（攻击方 / 施法方）
    std::string target;        ///< 事件承受者
    int         value{0};      ///< 数值：伤害量或治疗量
    std::string message;       ///< 人类可读的完整描述，便于日志直接输出
};

/// 观察者接口：任何想"听战斗广播"的模块都实现它
class IBattleObserver {
public:
    virtual ~IBattleObserver() = default;

    /// 收到一条事件后的回调
    virtual void onEvent(const BattleEvent& event) = 0;

    /// 观察者名字，便于调试与文档说明
    virtual std::string observerName() const = 0;
};

/// 主题（Subject）：维护观察者名单，并在事件发生时逐一通知
class EventBus {
public:
    /// 注册观察者。注意：EventBus 不持有观察者的生命周期，
    /// 观察者需保证在 EventBus 存活期间一直有效。
    void attach(IBattleObserver* observer);

    /// 取消注册
    void detach(IBattleObserver* observer);

    /// 广播事件：通知名单上的每一个观察者
    void notify(const BattleEvent& event);

    /// 当前订阅者数量
    std::size_t observerCount() const { return observers_.size(); }

private:
    std::vector<IBattleObserver*> observers_;
};

} // namespace gb
