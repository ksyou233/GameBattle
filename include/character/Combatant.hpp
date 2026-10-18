#pragma once
// =============================================================================
//  Combatant.hpp  ——  参战单位的抽象接口
//
//  这个文件本身不承担某个设计模式，但它是整个系统能够"协同"的关键接缝：
//    · 装饰器模式（EquipmentDecorator）需要它作为被装饰对象的统一类型
//    · 战斗循环（Battle）只面向它编程，因此"裸角色"和"穿了装备的角色"
//      在使用上完全一致
//    · 责任链模式（DamageChain）通过它读取闪避率、暴击率、防御策略
//
//  设计说明：
//      为了让装饰器在包裹角色后仍然能透明转发"技能表、AI 策略、状态"等信息，
//      这些能力被一并提升到接口层。接口偏宽，换来的是装饰器可以完全透传，
//      战斗循环无需做任何向下转型（dynamic_cast），这是刻意的取舍。
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include "combat/AIStrategy.hpp"
#include "combat/CharacterState.hpp"
#include "combat/DamageStrategy.hpp"
#include "skill/Skill.hpp"

namespace gb {

class Combatant {
public:
    virtual ~Combatant() = default;

    // ---------------------- 基础属性 ----------------------
    virtual std::string name() const = 0;
    virtual int  hp() const = 0;
    virtual int  maxHp() const = 0;
    virtual int  atk() const = 0;              ///< 已包含装备与状态的加成
    virtual int  def() const = 0;              ///< 已包含装备与状态的加成
    virtual double critRate() const = 0;       ///< 暴击率 [0,1]
    virtual double dodgeRate() const = 0;      ///< 闪避率 [0,1]
    virtual bool alive() const = 0;

    /// 该单位使用哪种伤害计算规则（策略模式：运行期可替换）
    virtual const DamageStrategy& damageType() const = 0;
    virtual void setDamageStrategy(std::unique_ptr<DamageStrategy> strategy) = 0;

    // ---------------------- 生命值变更 ----------------------
    /// 承受伤害
    /// @return 实际扣减的血量（可能因状态易伤而被放大）
    virtual int  takeDamage(int amount, const std::string& source) = 0;

    /// 恢复生命值
    virtual void heal(int amount) = 0;

    // ---------------------- 状态（状态模式） ----------------------
    virtual void        inflictState(StateKind kind) = 0;
    virtual StateKind   stateKind() const = 0;
    virtual std::string stateName() const = 0;
    /// 本回合能否行动（由当前状态决定：眩晕/冰冻时不能行动）
    virtual bool        canAct() const = 0;
    /// 回合结束时推进状态计时，到期的状态自动退回"正常"
    virtual void        tickState() = 0;
    /// 是否免疫控制类状态（眩晕 / 冰冻），由状态模式决定
    virtual bool        immuneToControl() const = 0;

    // ---------------------- 技能与 AI ----------------------
    virtual const std::vector<std::unique_ptr<Skill>>& skills() const = 0;
    virtual void addSkill(std::unique_ptr<Skill> skill) = 0;
    virtual AIStrategy* ai() const = 0;
    virtual void setAI(std::unique_ptr<AIStrategy> ai) = 0;

    // ---------------------- 展示 ----------------------
    virtual std::string describe() const = 0;
};

} // namespace gb
