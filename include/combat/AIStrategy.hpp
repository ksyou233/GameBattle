#pragma once
// =============================================================================
//  AIStrategy.hpp  ——  【设计模式 3/8：策略模式 Strategy（其二）】
//
//  意图：同 DamageStrategy —— 封装可互换的算法族。
//
//  在本项目中的落点：
//      "这个单位本回合该干什么"有不同的决策算法：
//        · 莽夫 AI：永远选伤害最高的技能硬刚
//        · 保守 AI：血量低于三成时转入防御，否则反击
//        · 辅助 AI：自己残血就先给自己奶一口，否则攻击
//      把决策算法抽成策略对象后，同一个 Character 只要换掉手里的 AI 策略，
//      行为立刻改变 —— 精英怪和杂兵可以共用同一个角色类。
// =============================================================================

#include <string>
#include <vector>

#include "skill/Skill.hpp"

namespace gb {

class Combatant;

/// AI 决策策略（抽象策略角色）
class AIStrategy {
public:
    virtual ~AIStrategy() = default;

    virtual std::string name() const = 0;

    /// 决策：返回本回合要释放的技能，返回 nullptr 表示"本回合转入防御"
    /// @param self    做出决策的单位
    /// @param enemies 敌对单位列表（均为存活单位）
    virtual Skill* chooseSkill(Combatant& self, const std::vector<Combatant*>& enemies) = 0;
};

/// 莽夫 AI：完全不考虑防御与自身血量，从攻击类技能里随机挑一个硬刚
class AggressiveAI : public AIStrategy {
public:
    std::string name() const override { return "莽夫"; }
    Skill* chooseSkill(Combatant& self, const std::vector<Combatant*>& enemies) override;
};

/// 保守 AI：血量低于 30% 时转入防御，否则反击
class DefensiveAI : public AIStrategy {
public:
    std::string name() const override { return "保守"; }
    Skill* chooseSkill(Combatant& self, const std::vector<Combatant*>& enemies) override;
};

/// 辅助 AI：血量低于一半且手上有治疗技能时先自救；否则释放手中最强的攻击手段
class SupportAI : public AIStrategy {
public:
    std::string name() const override { return "辅助"; }
    Skill* chooseSkill(Combatant& self, const std::vector<Combatant*>& enemies) override;
};

} // namespace gb
