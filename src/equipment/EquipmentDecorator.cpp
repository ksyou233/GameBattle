// =============================================================================
//  EquipmentDecorator.cpp  ——  装饰器实现
// =============================================================================

#include "equipment/EquipmentDecorator.hpp"

#include <sstream>

namespace gb {

EquipmentDecorator::EquipmentDecorator(std::unique_ptr<Combatant> inner, std::string equipmentName)
    : inner_(std::move(inner)), equipmentName_(std::move(equipmentName)) {
    // 若被装饰对象本身就是一个装饰器，则把装备名接着往后拼，
    // 这样最外层拿到的是一整条"装备链"，便于展示与答辩讲解。
    const EquipmentDecorator* previous = dynamic_cast<const EquipmentDecorator*>(inner_.get());
    if (previous != nullptr) {
        chainLabel_ = previous->chainLabel_ + " + " + equipmentName_;
    } else {
        chainLabel_ = equipmentName_;
    }
}

std::string EquipmentDecorator::describe() const {
    // 注意：这里刻意不调用 inner_->describe()，否则打印出来的会是被装饰对象的
    // "裸属性"。装饰器应当以自己对外暴露的属性为准（即已叠加加成后的值）。
    std::ostringstream oss;
    oss << name()
        << " HP " << hp() << "/" << maxHp()
        << "  ATK " << atk()
        << "  DEF " << def()
        << "  暴击率 " << static_cast<int>(critRate() * 100) << "%"
        << "  伤害类型：" << damageType().name()
        << "  | 装备：" << chainLabel_;
    if (stateKind() != StateKind::Normal) {
        oss << "  状态：" << stateName();
    }
    if (ai() != nullptr) {
        oss << "  AI：" << ai()->name();
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
//  具体装饰器
// ---------------------------------------------------------------------------

WeaponDecorator::WeaponDecorator(std::unique_ptr<Combatant> inner,
                                 std::string weaponName,
                                 int atkBonus)
    : EquipmentDecorator(std::move(inner), std::move(weaponName)), atkBonus_(atkBonus) {}

ArmorDecorator::ArmorDecorator(std::unique_ptr<Combatant> inner,
                               std::string armorName,
                               int defBonus)
    : EquipmentDecorator(std::move(inner), std::move(armorName)), defBonus_(defBonus) {}

FireEnchantDecorator::FireEnchantDecorator(std::unique_ptr<Combatant> inner)
    : EquipmentDecorator(std::move(inner), "烈焰附魔") {}

CritGemDecorator::CritGemDecorator(std::unique_ptr<Combatant> inner, double critBonus)
    : EquipmentDecorator(std::move(inner), "暴击宝石"), critBonus_(critBonus) {}

} // namespace gb
