#pragma once
// =============================================================================
//  CharacterState.hpp  ——  【设计模式 6/8：状态模式 State】
//
//  意图：允许一个对象在其内部状态改变时改变它的行为，对象看起来似乎修改了它的类。
//
//  在本项目中的落点：
//      角色会处于"正常 / 眩晕 / 冰冻 / 狂暴"等状态，不同状态下：
//        · 能不能行动？        （眩晕、冰冻 → 本回合跳过）
//        · 攻击力有没有加成？  （狂暴 → 攻击 x1.5）
//        · 防御力有没有变化？  （狂暴 → 防御 x0.5，高风险高收益）
//        · 受到的伤害有没有变化？（冰冻 → 受到伤害 x1.5，破冰增伤）
//      如果把这些差异全部写成 if/switch 塞进角色类，角色类会迅速失控。
//      状态模式把每种状态封装成一个类，把"行为随状态变化"这件事交给多态。
//
//  设计取舍：
//      本实现采用"状态对象只提供系数与能力，由上下文(Context, 即 Character)
//      负责应用"的轻量写法，避免状态类反向依赖角色类造成循环引用，
//      同时保留了状态模式最核心的价值 —— 消除状态判断的 if-else 树。
// =============================================================================

#include <climits>
#include <string>

namespace gb {

/// 状态种类：同时被战斗事件与角色接口复用，故放在此处统一定义
enum class StateKind {
    Normal,     ///< 正常
    Stunned,    ///< 眩晕：无法行动
    Frozen,     ///< 冰冻：无法行动，且受到伤害提高
    Berserk     ///< 狂暴：攻击大幅提高、防御下降
};

/// 状态基类（抽象状态角色 State）
class CharacterState {
public:
    virtual ~CharacterState() = default;

    virtual StateKind   kind() const = 0;
    virtual std::string name() const = 0;

    /// 本回合能否行动
    virtual bool   canAct() const { return true; }

    /// 攻击力系数
    virtual double atkMultiplier() const { return 1.0; }

    /// 防御力系数
    virtual double defMultiplier() const { return 1.0; }

    /// 受到伤害的系数（>1 表示易伤）
    virtual double incomingMultiplier() const { return 1.0; }

    /// 是否免疫控制类状态（眩晕 / 冰冻）。
    /// 状态模式的一个好处在这里体现：把"谁免控"这条规则封装进状态类本身，
    /// 而不是散落在技能代码里做 if (target == boss) 的硬编码判断。
    virtual bool immuneToControl() const { return false; }

    /// 持续回合数；正常状态用 INT_MAX 表示"不会自然结束"
    virtual int duration() const { return INT_MAX; }
};

/// 正常状态：所有系数均为 1，是可行动的中性状态
class NormalState : public CharacterState {
public:
    StateKind kind() const override { return StateKind::Normal; }
    std::string name() const override { return "正常"; }
};

/// 眩晕：无法行动，持续 2 回合
class StunnedState : public CharacterState {
public:
    StateKind kind() const override { return StateKind::Stunned; }
    std::string name() const override { return "眩晕"; }
    bool canAct() const override { return false; }
    int  duration() const override { return 2; }
};

/// 冰冻：无法行动，且受到伤害 x1.5（破冰机制），持续 2 回合
class FrozenState : public CharacterState {
public:
    StateKind kind() const override { return StateKind::Frozen; }
    std::string name() const override { return "冰冻"; }
    bool   canAct() const override { return false; }
    double incomingMultiplier() const override { return 1.5; }
    int    duration() const override { return 2; }
};

/// 狂暴：BOSS 血量低于一半时触发。攻击 x1.5、防御 x0.5，持续到战斗结束。
/// 同时免疫眩晕与冰冻 —— 否则玩家一个控制技能就能把 BOSS 的爆发期白白拖过去。
class BerserkState : public CharacterState {
public:
    StateKind kind() const override { return StateKind::Berserk; }
    std::string name() const override { return "狂暴"; }
    double atkMultiplier() const override { return 1.5; }
    double defMultiplier() const override { return 0.5; }
    bool   immuneToControl() const override { return true; }
    int    duration() const override { return INT_MAX; }
};

/// 根据状态种类创建状态对象（简单工厂，服务于状态模式的上下文切换）
CharacterState* makeState(StateKind kind);

} // namespace gb
