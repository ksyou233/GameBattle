// =============================================================================
//  Skill.cpp  ——  技能实现（含组合模式的容器节点）
// =============================================================================

#include "skill/Skill.hpp"

#include <sstream>

#include "character/Combatant.hpp"
#include "combat/DamageChain.hpp"
#include "core/EventBus.hpp"
#include "core/GameConfig.hpp"

namespace gb {

// ---------------------------------------------------------------------------
//  AttackSkill —— 单体攻击技能（组合模式中的"叶子"）
// ---------------------------------------------------------------------------
AttackSkill::AttackSkill(std::string name, double powerScale,
                         StateKind inflict, double inflictRate)
    : name_(std::move(name)),
      powerScale_(powerScale),
      inflict_(inflict),
      inflictRate_(inflictRate) {}

std::string AttackSkill::describe() const {
    std::ostringstream oss;
    oss << name_ << "（伤害倍率 x" << powerScale_;
    if (inflict_ != StateKind::Normal) {
        oss << "，命中后有 " << static_cast<int>(inflictRate_ * 100) << "% 概率附加负面状态";
    }
    oss << "）";
    return oss.str();
}

void AttackSkill::execute(Combatant& caster, Combatant& target, EventBus& bus) {
    // 1) 走责任链算伤害并广播事件（闪避 / 护甲 / 格挡 / 暴击 全在链上）
    DamageContext ctx = performAttack(caster, target, powerScale_, bus, name_);

    // 2) 附加状态：闪避了就不附加；目标免疫控制时，控制类状态同样无效
    if (ctx.dodged || inflict_ == StateKind::Normal || !target.alive()) {
        return;
    }
    const bool isControl = (inflict_ == StateKind::Stunned || inflict_ == StateKind::Frozen);
    if (isControl && target.immuneToControl()) {
        BattleEvent immune;
        immune.type    = EventType::DamageDealt;
        immune.source  = caster.name();
        immune.target  = target.name();
        immune.message = "  ↳ " + target.name() + " 免疫了控制效果（控制抗性生效）";
        bus.notify(immune);
        return;
    }
    if (GameConfig::instance().roll01() < inflictRate_) {
        // inflictState 内部会广播 StateChanged 事件，无需在此重复广播
        target.inflictState(inflict_);
    }
}

// ---------------------------------------------------------------------------
//  HealSkill —— 治疗技能（组合模式中的"叶子"）
// ---------------------------------------------------------------------------
HealSkill::HealSkill(std::string name, int healAmount)
    : name_(std::move(name)), healAmount_(healAmount) {}

std::string HealSkill::describe() const {
    return name_ + "（恢复 " + std::to_string(healAmount_) + " 点生命）";
}

void HealSkill::execute(Combatant& caster, Combatant& /*target*/, EventBus& bus) {
    // 治疗以自己为目标
    caster.heal(healAmount_);

    BattleEvent event;
    event.type    = EventType::Healed;
    event.source  = caster.name();
    event.target  = caster.name();
    event.value   = healAmount_;
    event.message = "  [" + name_ + "] " + caster.name() + " 恢复 "
                    + std::to_string(healAmount_) + " 点生命  当前 HP "
                    + std::to_string(caster.hp()) + "/" + std::to_string(caster.maxHp());
    bus.notify(event);
}

// ---------------------------------------------------------------------------
//  ComboSkill —— 连招（组合模式中的"容器"）
//  注意它自己也是 Skill，因此连招里可以再嵌连招，形成技能树
// ---------------------------------------------------------------------------
ComboSkill::ComboSkill(std::string name, std::vector<std::unique_ptr<Skill>> parts)
    : name_(std::move(name)), parts_(std::move(parts)) {}

std::string ComboSkill::describe() const {
    std::ostringstream oss;
    oss << name_ << "（" << parts_.size() << " 段连招：";
    for (std::size_t i = 0; i < parts_.size(); ++i) {
        if (i > 0) oss << " -> ";
        oss << parts_[i]->name();
    }
    oss << "）";
    return oss.str();
}

void ComboSkill::execute(Combatant& caster, Combatant& target, EventBus& bus) {
    BattleEvent begin;
    begin.type    = EventType::DamageDealt;
    begin.source  = caster.name();
    begin.target  = target.name();
    begin.message = "  ▶ " + caster.name() + " 施展连招【" + name_ + "】";
    bus.notify(begin);

    // 组合模式的价值：这里不需要区分"子技能是单体还是连招"，
    // 一律调 execute() —— 对单个对象和组合对象的使用具有一致性。
    for (const auto& part : parts_) {
        if (!target.alive()) break;      // 目标已阵亡，连招中断
        part->execute(caster, target, bus);
    }
}

} // namespace gb
