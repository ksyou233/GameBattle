#include "combat/AIStrategy.hpp"

#include <vector>

#include "character/Combatant.hpp"
#include "core/GameConfig.hpp"

namespace gb {

namespace {

/// 收集单位技能表里所有"能造成伤害"的技能
std::vector<Skill*> pickAttackCandidates(Combatant& self) {
    std::vector<Skill*> candidates;
    for (const auto& skill : self.skills()) {
        if (dynamic_cast<AttackSkill*>(skill.get()) != nullptr ||
            dynamic_cast<ComboSkill*>(skill.get()) != nullptr) {
            candidates.push_back(skill.get());
        }
    }
    return candidates;
}

/// 找一个治疗技能
Skill* pickHeal(Combatant& self) {
    for (const auto& skill : self.skills()) {
        if (dynamic_cast<HealSkill*>(skill.get()) != nullptr) {
            return skill.get();
        }
    }
    return nullptr;
}

} // namespace

Skill* AggressiveAI::chooseSkill(Combatant& self, const std::vector<Combatant*>& /*enemies*/) {
    // 莽夫不看血量、不看局势，从手里的攻击技能随机挑一个就上。
    // 随机源同样来自 GameConfig 单例，因此设好种子后战斗过程完全可复现。
    std::vector<Skill*> candidates = pickAttackCandidates(self);
    if (candidates.empty()) return nullptr;

    std::size_t index = static_cast<std::size_t>(
        GameConfig::instance().roll(0, static_cast<int>(candidates.size()) - 1));
    return candidates[index];
}

Skill* DefensiveAI::chooseSkill(Combatant& self, const std::vector<Combatant*>& /*enemies*/) {
    // 残血就缩起来（返回 nullptr → 战斗循环走"防御"分支）
    if (self.hp() * 100 / self.maxHp() < 30) {
        return nullptr;
    }
    std::vector<Skill*> candidates = pickAttackCandidates(self);
    return candidates.empty() ? nullptr : candidates.front();
}

Skill* SupportAI::chooseSkill(Combatant& self, const std::vector<Combatant*>& /*enemies*/) {
    // 血量低于一半且有治疗技能：先自救
    Skill* heal = pickHeal(self);
    if (heal != nullptr && self.hp() * 2 < self.maxHp()) {
        return heal;
    }
    // 血量健康：释放手中最强的攻击手段（技能表末尾通常是大招）
    std::vector<Skill*> candidates = pickAttackCandidates(self);
    if (!candidates.empty()) return candidates.back();
    return heal;
}

} // namespace gb
