#pragma once
// =============================================================================
//  EquipmentDecorator.hpp  ——  【设计模式 4/8：装饰器模式 Decorator】
//
//  意图：动态地给一个对象添加一些额外的职责。就增加功能而言，装饰器比生成
//        子类更为灵活。
//
//  在本项目中的落点：
//      如果"武器 + 护甲 + 附魔 + 宝石"都靠继承实现，组合数是 2^n 级的
//      （长剑角色、长剑+重甲角色、长剑+重甲+附魔角色……）。装饰器把每种
//      加成都做成一个薄薄的包装层，需要什么就套什么，顺序还可以自由调整。
//
//      演示效果：一个 34 攻击力的战士，套上「巨剑 → 烈焰附魔 → 暴击宝石」后，
//      攻击力变成 34+8+7 = 49，暴击率 +12%，而且伤害类型被烈焰附魔改写为魔法，
//      连责任链里的护甲减伤规则都随之改变 —— 装饰器与策略模式的联动。
//
//  关键约束：装饰器自身必须实现被装饰对象的同一接口（Combatant），
//            这样才能"套娃"式地层层包裹，并对使用者保持透明。
// =============================================================================

#include <memory>
#include <string>

#include "character/Combatant.hpp"

namespace gb {

/// 装备装饰器基类。它继承 Combatant，因此可以装饰任意 Combatant，
/// 也可以被别的装饰器装饰。
class EquipmentDecorator : public Combatant {
public:
    EquipmentDecorator(std::unique_ptr<Combatant> inner, std::string equipmentName);

    // ---------- 生命值相关：原样透传给被装饰对象 ----------
    std::string name() const override { return inner_->name(); }
    int  hp() const override { return inner_->hp(); }
    int  maxHp() const override { return inner_->maxHp(); }
    bool alive() const override { return inner_->alive(); }
    int  takeDamage(int amount, const std::string& source) override {
        return inner_->takeDamage(amount, source);
    }
    void heal(int amount) override { inner_->heal(amount); }

    // ---------- 属性：在被装饰对象的基础上叠加加成 ----------
    int    atk() const override { return inner_->atk() + bonusAtk(); }
    int    def() const override { return inner_->def() + bonusDef(); }
    double critRate() const override { return inner_->critRate() + bonusCrit(); }
    double dodgeRate() const override { return inner_->dodgeRate(); }

    // ---------- 策略：默认透传；子类可覆写以改写伤害类型 ----------
    const DamageStrategy& damageType() const override { return inner_->damageType(); }
    void setDamageStrategy(std::unique_ptr<DamageStrategy> strategy) override {
        inner_->setDamageStrategy(std::move(strategy));
    }

    // ---------- 状态：原样透传 ----------
    void        inflictState(StateKind kind) override { inner_->inflictState(kind); }
    StateKind   stateKind() const override { return inner_->stateKind(); }
    std::string stateName() const override { return inner_->stateName(); }
    bool        canAct() const override { return inner_->canAct(); }
    void        tickState() override { inner_->tickState(); }
    bool        immuneToControl() const override { return inner_->immuneToControl(); }

    // ---------- 技能与 AI：原样透传 ----------
    const std::vector<std::unique_ptr<Skill>>& skills() const override { return inner_->skills(); }
    void addSkill(std::unique_ptr<Skill> skill) override { inner_->addSkill(std::move(skill)); }
    AIStrategy* ai() const override { return inner_->ai(); }
    void setAI(std::unique_ptr<AIStrategy> ai) override { inner_->setAI(std::move(ai)); }

    std::string describe() const override;

    /// 装备链名称，例如 "巨剑 + 烈焰附魔 + 暴击宝石"
    const std::string& chainLabel() const { return chainLabel_; }

protected:
    /// 三个加成钩子：子类只覆写自己关心的那一个
    virtual int    bonusAtk()  const { return 0; }
    virtual int    bonusDef()  const { return 0; }
    virtual double bonusCrit() const { return 0.0; }

    const std::string& equipmentName() const { return equipmentName_; }

private:
    std::unique_ptr<Combatant> inner_;        ///< 被装饰对象（可能是另一个装饰器）
    std::string equipmentName_;               ///< 本层装备名
    std::string chainLabel_;                  ///< 累积的装备链名称
};

// ---------------------------------------------------------------------------
//  具体装饰器
// ---------------------------------------------------------------------------

/// 武器：提升攻击力
class WeaponDecorator : public EquipmentDecorator {
public:
    WeaponDecorator(std::unique_ptr<Combatant> inner, std::string weaponName, int atkBonus);
protected:
    int bonusAtk() const override { return atkBonus_; }
private:
    int atkBonus_;
};

/// 护甲：提升防御力
class ArmorDecorator : public EquipmentDecorator {
public:
    ArmorDecorator(std::unique_ptr<Combatant> inner, std::string armorName, int defBonus);
protected:
    int bonusDef() const override { return defBonus_; }
private:
    int defBonus_;
};

/// 烈焰附魔：额外攻击力，并把伤害类型改写为【魔法】
/// —— 这是装饰器与策略模式的交叉点：装饰一层，伤害算法整套换掉
class FireEnchantDecorator : public EquipmentDecorator {
public:
    explicit FireEnchantDecorator(std::unique_ptr<Combatant> inner);
protected:
    int bonusAtk() const override { return 7; }
    const DamageStrategy& damageType() const override { return fireStrategy_; }
private:
    MagicDamage fireStrategy_;                ///< 附魔后统一按魔法伤害结算
};

/// 暴击宝石：提升暴击率
class CritGemDecorator : public EquipmentDecorator {
public:
    CritGemDecorator(std::unique_ptr<Combatant> inner, double critBonus);
protected:
    double bonusCrit() const override { return critBonus_; }
private:
    double critBonus_;
};

} // namespace gb
