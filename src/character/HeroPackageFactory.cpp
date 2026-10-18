// =============================================================================
//  HeroPackageFactory.cpp  ——  三个具体工厂的产品族定义
//
//  一个职业的产品（武器、防具、技能）在这里配套生成，保证风格一致。
//  新增职业只需再加一个具体工厂类，既有代码无需改动（开闭原则）。
// =============================================================================

#include "character/HeroPackageFactory.hpp"

#include "character/Character.hpp"          // applyTalent 需要真实角色定义
#include "combat/DamageStrategy.hpp"        // MagicDamage
#include "equipment/EquipmentDecorator.hpp"

namespace gb {

// ---------------------------------------------------------------------------
//  战士职业包：高攻高防、带控制的重装近战
// ---------------------------------------------------------------------------
void WarriorPackageFactory::applyTalent(Character& c) const {
    // 铁壁：基础防御 +4。作用在裸角色上，随后装备（板甲 +6）再叠加。
    // 亚瑟最终 DEF = 14(基础) + 4(天赋) + 6(板甲) = 24
    c.addBaseDef(4);
}

std::unique_ptr<Combatant> WarriorPackageFactory::equip(std::unique_ptr<Combatant> base) const {
    // 先套武器，再套防具 —— 装饰顺序决定了 self 的表达方式，但不影响叠加结果
    base = std::make_unique<WeaponDecorator>(std::move(base), "巨剑", 9);
    base = std::make_unique<ArmorDecorator>(std::move(base), "板甲", 6);
    return base;
}

std::unique_ptr<Skill> WarriorPackageFactory::signatureSkill() const {
    // 招牌技：旋风斩，1.4 倍伤害，25% 概率把目标打晕
    return std::make_unique<AttackSkill>("旋风斩", 1.4, StateKind::Stunned, 0.25);
}

std::unique_ptr<Skill> WarriorPackageFactory::ultimateSkill() const {
    // 大招：裂地三连斩 —— 组合模式的容器节点，内含三段单体技能
    std::vector<std::unique_ptr<Skill>> parts;
    parts.push_back(std::make_unique<AttackSkill>("斩击·壹", 1.0));
    parts.push_back(std::make_unique<AttackSkill>("斩击·贰", 1.1));
    parts.push_back(std::make_unique<AttackSkill>("斩击·叁", 1.3, StateKind::Stunned, 0.5));
    return std::make_unique<ComboSkill>("裂地三连斩", std::move(parts));
}

// ---------------------------------------------------------------------------
//  法师职业包：靠魔法规则绕过护甲、附带控制与自我治疗
// ---------------------------------------------------------------------------
void MagePackageFactory::applyTalent(Character& c) const {
    // 奥术亲和：伤害按魔法规则结算 —— 把伤害策略换成 MagicDamage
    // （物理 max(1, ATK-DEF)，魔法 max(1, ATK-DEF/2)，对高防目标收益明显）
    // 这里把"天赋文案"落成真实行为：调用方即使建号时没指定策略，
    // 只要挂了法师职业包，也一定会走魔法结算。
    c.setDamageStrategy(std::make_unique<MagicDamage>());
}

std::unique_ptr<Combatant> MagePackageFactory::equip(std::unique_ptr<Combatant> base) const {
    base = std::make_unique<WeaponDecorator>(std::move(base), "秘法法杖", 7);
    base = std::make_unique<ArmorDecorator>(std::move(base), "法师长袍", 2);
    return base;
}

std::unique_ptr<Skill> MagePackageFactory::signatureSkill() const {
    return std::make_unique<AttackSkill>("火球术", 1.6);
}

std::unique_ptr<Skill> MagePackageFactory::ultimateSkill() const {
    // 大招：冰火两重天 —— 先烧再冻，冻结必定命中，用来演示状态模式
    std::vector<std::unique_ptr<Skill>> parts;
    parts.push_back(std::make_unique<AttackSkill>("烈焰冲击", 1.3));
    parts.push_back(std::make_unique<AttackSkill>("寒冰禁锢", 1.0, StateKind::Frozen, 1.0));
    return std::make_unique<ComboSkill>("冰火两重天", std::move(parts));
}

// ---------------------------------------------------------------------------
//  射手职业包：高暴击、稳定输出，附带自我恢复
// ---------------------------------------------------------------------------
void ArcherPackageFactory::applyTalent(Character& c) const {
    // 鹰眼：基础暴击率 +10%。作用在裸角色上，装备（暴击宝石）再叠加。
    // 莉娜最终暴击 = 30%(基础) + 10%(天赋) = 40%
    c.addCritRate(0.10);
}

std::unique_ptr<Combatant> ArcherPackageFactory::equip(std::unique_ptr<Combatant> base) const {
    base = std::make_unique<WeaponDecorator>(std::move(base), "穿云长弓", 6);
    base = std::make_unique<ArmorDecorator>(std::move(base), "皮甲", 3);
    return base;
}

std::unique_ptr<Skill> ArcherPackageFactory::signatureSkill() const {
    return std::make_unique<AttackSkill>("穿透射击", 1.35);
}

std::unique_ptr<Skill> ArcherPackageFactory::ultimateSkill() const {
    // 大招：疾风三连射
    std::vector<std::unique_ptr<Skill>> parts;
    parts.push_back(std::make_unique<AttackSkill>("第一箭", 1.0));
    parts.push_back(std::make_unique<AttackSkill>("第二箭", 1.1));
    parts.push_back(std::make_unique<AttackSkill>("第三箭", 1.25));
    return std::make_unique<ComboSkill>("疾风三连射", std::move(parts));
}

} // namespace gb
