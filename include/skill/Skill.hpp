#pragma once
// =============================================================================
//  Skill.hpp  ——  【设计模式 8/8：组合模式 Composite】
//
//  意图：将对象组合成树形结构以表示"部分-整体"的层次结构。
//        Composite 使得用户对单个对象和组合对象的使用具有一致性。
//
//  在本项目中的落点：
//      战斗系统里既有"单体技能"（火球术、旋风斩），也有"连招"
//      （烈焰三连 = 三段单体技能依次打出）。
//      若为连招单独设计一套调用方式，战斗循环里就得判断
//      "这一招是普通技能还是连招" —— 又是一堆分支。
//      组合模式让 ComboSkill 与 AttackSkill 实现同一个 Skill 接口，
//      战斗循环只认 Skill::execute()，单体与连招一视同仁；
//      而且连招里还能再嵌连招，自然形成技能树。
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include "combat/CharacterState.hpp"   // 技能命中后可能附加状态（StateKind）

namespace gb {

class Combatant;    // 前向声明：技能要作用于参战单位
class EventBus;     // 前向声明：技能释放要向事件总线广播

/// 技能抽象基类 —— 叶子(单体技能)与容器(连招)的共同接口
class Skill {
public:
    virtual ~Skill() = default;

    /// 技能名
    virtual std::string name() const = 0;

    /// 技能说明，用于角色面板展示
    virtual std::string describe() const { return name(); }

    /// 释放技能：由 caster 对 target 施放，过程通过 bus 广播战斗事件
    virtual void execute(Combatant& caster, Combatant& target, EventBus& bus) = 0;
};

// ---------------------------------------------------------------------------
//  叶子节点：单体攻击技能
// ---------------------------------------------------------------------------
class AttackSkill : public Skill {
public:
    /// @param name         技能名
    /// @param powerScale   伤害倍率（乘在攻击力上），1.0 表示一次普攻
    /// @param inflict      命中后附加的状态
    /// @param inflictRate  附加状态的概率（0 表示不附加）
    AttackSkill(std::string name,
                double powerScale,
                StateKind inflict = StateKind::Normal,
                double inflictRate = 0.0);

    std::string name() const override { return name_; }
    std::string describe() const override;
    void execute(Combatant& caster, Combatant& target, EventBus& bus) override;

private:
    std::string name_;
    double      powerScale_;
    StateKind   inflict_;
    double      inflictRate_;
};

// ---------------------------------------------------------------------------
//  叶子节点：治疗技能
// ---------------------------------------------------------------------------
class HealSkill : public Skill {
public:
    HealSkill(std::string name, int healAmount);

    std::string name() const override { return name_; }
    std::string describe() const override;
    void execute(Combatant& caster, Combatant& target, EventBus& bus) override;

    /// 供 AI 识别"我手里有没有奶"
    int healAmount() const { return healAmount_; }

private:
    std::string name_;
    int         healAmount_;
};

// ---------------------------------------------------------------------------
//  容器节点：连招（组合模式的核心）
//  它本身是一个 Skill，同时又持有若干 Skill —— 递归结构
// ---------------------------------------------------------------------------
class ComboSkill : public Skill {
public:
    ComboSkill(std::string name, std::vector<std::unique_ptr<Skill>> parts);

    std::string name() const override { return name_; }
    std::string describe() const override;
    void execute(Combatant& caster, Combatant& target, EventBus& bus) override;

    /// 连招段数
    std::size_t partCount() const { return parts_.size(); }

    /// 添加一段（可继续嵌套连招）
    void add(std::unique_ptr<Skill> part) { parts_.push_back(std::move(part)); }

private:
    std::string                            name_;
    std::vector<std::unique_ptr<Skill>>    parts_;   ///< 子技能列表（树形结构的"子节点"）
};

} // namespace gb
