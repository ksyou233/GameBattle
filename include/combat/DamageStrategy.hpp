#pragma once
// =============================================================================
//  DamageStrategy.hpp  ——  【设计模式 3/8：策略模式 Strategy（其一）】
//
//  意图：定义一系列算法，把它们一个个封装起来，并且使它们可相互替换。
//        本模式使得算法可独立于使用它的客户而变化。
//
//  在本项目中的落点：
//      "一次攻击打出多少伤害"有多种算规则 —— 物理伤害直接减防御、魔法伤害只吃
//      一半防御、真实伤害无视防御。若写成
//          if (type == PHYSICAL) {...} else if (type == MAGIC) {...}
//      那么每加一种伤害类型都要改动角色类，且角色类会越来越臃肿。
//      于是把"伤害计算规则"抽象成 DamageStrategy，由角色持有一个策略对象，
//      运行期可以整个换掉（法师被打上"沉默"后切换为普通物理伤害）。
// =============================================================================

#include <algorithm>
#include <string>

namespace gb {

/// 伤害计算策略（抽象策略角色 Strategy）
class DamageStrategy {
public:
    virtual ~DamageStrategy() = default;

    /// 策略名称，用于日志与 UML 说明
    virtual std::string name() const = 0;

    /// 具体的伤害算法
    /// @param baseAtk 攻击方攻击力（已包含装备与状态的加成）
    /// @param def     防御方防御力
    /// @return 计算出的伤害，至少为 1（避免出现 0 伤害的挫败感）
    virtual int compute(int baseAtk, int def) const = 0;
};

/// 物理伤害：直接减去对方防御力
class PhysicalDamage : public DamageStrategy {
public:
    std::string name() const override { return "物理"; }
    int compute(int baseAtk, int def) const override {
        return std::max(1, baseAtk - def);
    }
};

/// 魔法伤害：只受一半防御力减免，对高防目标更有效
class MagicDamage : public DamageStrategy {
public:
    std::string name() const override { return "魔法"; }
    int compute(int baseAtk, int def) const override {
        return std::max(1, baseAtk - def / 2);
    }
};

/// 真实伤害：无视一切防御，用于 BOSS 的斩杀技能
class TrueDamage : public DamageStrategy {
public:
    std::string name() const override { return "真实"; }
    int compute(int baseAtk, int /*def*/) const override {
        return std::max(1, baseAtk);
    }
};

} // namespace gb
