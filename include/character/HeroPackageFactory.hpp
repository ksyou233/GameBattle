#pragma once
// =============================================================================
//  HeroPackageFactory.hpp  ——  【设计模式 2/8：抽象工厂模式 Abstract Factory】
//
//  意图：提供一个创建一系列相关或相互依赖对象的接口，而无需指定它们具体的类。
//
//  在本项目中的落点：
//      一个"职业"从来不是单个对象，而是一整套配套的产品：
//          战士  = 巨剑 + 板甲  + 旋风斩 + 裂地三连斩
//          法师  = 法杖 + 长袍  + 火球术 + 冰火两重天
//          射手  = 长弓 + 皮甲  + 穿透射击 + 疾风三连射
//      如果让调用方自己一个个 new 出来，那么"职业"这个概念就会散落在各处，
//      想新增一个职业要改很多地方。抽象工厂把"一整套产品"打包成一个工厂，
//      调用方只需要换工厂对象，整个职业的产品族就跟着换掉。
//
//  与工厂方法（Factory Method）的区别（答辩常见追问）：
//      工厂方法解决"一个产品的创建延迟到子类"；
//      抽象工厂解决"一整个产品族的一致性创建"。本项目中武器、技能、防具
//      必须配套，所以用抽象工厂。
//
//  与装饰器模式的协作：
//      equip() 返回的不是裸角色，而是已经套好武器与防具装饰器的 Combatant，
//      调用方拿到手就是"武装形态"，无需关心内部套了几层。
//
//  关于第四个产品 applyTalent()：
//      一个职业除了"装备 + 技能"之外，还带一条"天赋"。天赋改的是角色的
//      基础数值或伤害规则（基础防御 +4 / 改走魔法结算 / 基础暴击 +10%），
//      它与装备加成是两回事：装备走装饰器的 addAtk()/addDef()/addCrit() 加算，
//      天赋直接改裸角色的基础属性。因此 applyTalent() 必须在 equip() 之前
//      对"裸 Character"调用，两者互不干扰、可叠加。
//      把天赋也纳入工厂，才算真正兑现了"抽象工厂创建一整套产品族"的承诺 ——
//      职业之间的差异全部由工厂封装，调用方只换工厂对象。
// =============================================================================

#include <memory>
#include <string>

#include "character/Combatant.hpp"
#include "skill/Skill.hpp"

namespace gb {

// 前向声明：applyTalent 只用到 Character 的引用，无需在此包含其定义
class Character;

/// 抽象工厂：定义创建各"产品"的接口
class HeroPackageFactory {
public:
    virtual ~HeroPackageFactory() = default;

    /// 职业名
    virtual std::string career() const = 0;

    /// 职业天赋说明
    virtual std::string talent() const = 0;

    /// 产品 D：把职业天赋真正施加到"裸角色"上（数值 / 伤害规则）
    /// 必须在 equip() 之前调用 —— 天赋作用于基础属性，装备加成在其之上叠加。
    virtual void applyTalent(Character& c) const = 0;

    /// 产品 A：把裸角色装配成该职业的武装形态（内部使用装饰器叠加武器与防具）
    virtual std::unique_ptr<Combatant> equip(std::unique_ptr<Combatant> base) const = 0;

    /// 产品 B：招牌技能
    virtual std::unique_ptr<Skill> signatureSkill() const = 0;

    /// 产品 C：大招（连招，组合模式的容器节点）
    virtual std::unique_ptr<Skill> ultimateSkill() const = 0;
};

/// 具体工厂：战士职业包
class WarriorPackageFactory : public HeroPackageFactory {
public:
    std::string career() const override { return "战士"; }
    std::string talent() const override { return "铁壁：基础防御 +4"; }
    void applyTalent(Character& c) const override;          ///< 基础防御 +4
    std::unique_ptr<Combatant> equip(std::unique_ptr<Combatant> base) const override;
    std::unique_ptr<Skill> signatureSkill() const override;
    std::unique_ptr<Skill> ultimateSkill() const override;
};

/// 具体工厂：法师职业包
class MagePackageFactory : public HeroPackageFactory {
public:
    std::string career() const override { return "法师"; }
    std::string talent() const override { return "奥术亲和：伤害按魔法规则结算"; }
    void applyTalent(Character& c) const override;          ///< 伤害策略换成魔法规则
    std::unique_ptr<Combatant> equip(std::unique_ptr<Combatant> base) const override;
    std::unique_ptr<Skill> signatureSkill() const override;
    std::unique_ptr<Skill> ultimateSkill() const override;
};

/// 具体工厂：射手职业包
class ArcherPackageFactory : public HeroPackageFactory {
public:
    std::string career() const override { return "射手"; }
    std::string talent() const override { return "鹰眼：基础暴击率 +10%"; }
    void applyTalent(Character& c) const override;          ///< 基础暴击率 +10%
    std::unique_ptr<Combatant> equip(std::unique_ptr<Combatant> base) const override;
    std::unique_ptr<Skill> signatureSkill() const override;
    std::unique_ptr<Skill> ultimateSkill() const override;
};

} // namespace gb
