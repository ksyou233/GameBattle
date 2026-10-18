#pragma once
// =============================================================================
//  Character.hpp  ——  具体的参战角色
//
//  它是若干设计模式的"交汇点"，也是状态模式中的上下文（Context）：
//    · 状态模式：持有 CharacterState*，atk()/def()/takeDamage() 的行为随状态变化
//    · 策略模式：持有 DamageStrategy*（伤害算法）与 AIStrategy*（决策算法）
//    · 观察者模式：持有 EventBus*，状态变化时主动广播
//    · 组合模式：持有技能表 vector<unique_ptr<Skill>>，其中可能含连招
//    · 装饰器模式：作为被装饰的"裸对象"，由 EquipmentDecorator 逐层增强
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include "character/Combatant.hpp"
#include "core/EventBus.hpp"

namespace gb {

class Character : public Combatant {
public:
    Character(std::string name,
              int maxHp,
              int atk,
              int def,
              double critRate,
              double dodgeRate,
              std::unique_ptr<DamageStrategy> damageStrategy,
              EventBus* bus = nullptr);

    // ---------------------- Combatant 接口实现 ----------------------
    std::string name() const override { return name_; }
    int  hp() const override { return hp_; }
    int  maxHp() const override { return maxHp_; }
    int  atk() const override;                       ///< 基础攻击 x 状态系数
    int  def() const override;                       ///< 基础防御 x 状态系数
    double critRate() const override { return critRate_; }
    double dodgeRate() const override { return dodgeRate_; }
    bool alive() const override { return hp_ > 0; }

    const DamageStrategy& damageType() const override { return *damageStrategy_; }
    void setDamageStrategy(std::unique_ptr<DamageStrategy> strategy) override;

    int  takeDamage(int amount, const std::string& source) override;
    void heal(int amount) override;

    void        inflictState(StateKind kind) override;
    StateKind   stateKind() const override;
    std::string stateName() const override;

    /// 本回合能否行动（由当前状态决定）
    bool canAct() const override { return state_->canAct(); }

    /// 回合结束时推进状态计时；到期的状态自动退回"正常"
    void tickState() override;

    /// 是否免疫控制类状态（狂暴中的 BOSS 免疫；刚从控制中恢复的单位也有短暂抗性）
    bool immuneToControl() const override {
        return state_->immuneToControl() || controlImmunityTurns_ > 0;
    }

    const std::vector<std::unique_ptr<Skill>>& skills() const override { return skills_; }
    void addSkill(std::unique_ptr<Skill> skill) override;
    AIStrategy* ai() const override { return ai_.get(); }
    void setAI(std::unique_ptr<AIStrategy> ai) override;

    std::string describe() const override;

    // ---------------------- 角色自身的行为 ----------------------
    /// 直接设定基础属性（升级用）
    void setBaseStats(int atk, int def) { baseAtk_ = atk; baseDef_ = def; }

    /// 附加基础攻击力（职业天赋用）
    /// 用"累加"而不是"绝对赋值"，是为了与装饰器无关：
    /// 无论天赋在 equip() 之前还是之后施加，最终数值都一致。
    void addBaseAtk(int delta) { baseAtk_ += delta; }

    /// 附加基础防御力（职业天赋用）
    void addBaseDef(int delta) { baseDef_ += delta; }

    /// 附加暴击率（天赋用）
    void addCritRate(double delta) { critRate_ += delta; }

    /// 状态预计剩余回合数（INT_MAX 表示不会自然结束）
    int stateTurnsLeft() const { return stateTurnsLeft_; }

private:
    /// 切换到新状态并广播状态变化事件
    void switchState(StateKind kind);

    std::string name_;
    int    hp_;
    int    maxHp_;
    int    baseAtk_;
    int    baseDef_;
    double critRate_;
    double dodgeRate_;

    // —— 策略模式：两个可热插拔的算法 ——
    std::unique_ptr<DamageStrategy> damageStrategy_;
    std::unique_ptr<AIStrategy>     ai_;

    // —— 状态模式：当前状态 + 剩余回合 ——
    std::unique_ptr<CharacterState> state_;
    int stateTurnsLeft_ = INT_MAX;
    /// 控制抗性剩余回合数：从眩晕/冰冻中恢复后短暂免疫控制，防止被无限连控锁死
    int controlImmunityTurns_ = 0;

    // —— 组合模式：技能表（可能包含连招） ——
    std::vector<std::unique_ptr<Skill>> skills_;

    EventBus* bus_;
};

} // namespace gb
