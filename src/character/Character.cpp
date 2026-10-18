// =============================================================================
//  Character.cpp  ——  参战角色的实现
//  在这里可以最直观地看到"状态模式"与"策略模式"如何分工：
//    · 状态影响的是【数值与能不能动】—— atk()/def()/takeDamage()/canAct()
//    · 策略影响的是【怎么打、打多少】—— AI 决策 + 伤害计算规则
// =============================================================================

#include "character/Character.hpp"

#include <cmath>
#include <sstream>

namespace gb {

Character::Character(std::string name,
                     int maxHp,
                     int atk,
                     int def,
                     double critRate,
                     double dodgeRate,
                     std::unique_ptr<DamageStrategy> damageStrategy,
                     EventBus* bus)
    : name_(std::move(name)),
      hp_(maxHp),
      maxHp_(maxHp),
      baseAtk_(atk),
      baseDef_(def),
      critRate_(critRate),
      dodgeRate_(dodgeRate),
      damageStrategy_(std::move(damageStrategy)),
      state_(new NormalState()),
      bus_(bus) {}

int Character::atk() const {
    // 状态对攻击力的影响在这里统一体现（例如狂暴 x1.5）
    return static_cast<int>(std::lround(baseAtk_ * state_->atkMultiplier()));
}

int Character::def() const {
    // 状态对防御力的影响在这里统一体现（例如狂暴 x0.5）
    return static_cast<int>(std::lround(baseDef_ * state_->defMultiplier()));
}

void Character::setDamageStrategy(std::unique_ptr<DamageStrategy> strategy) {
    if (strategy != nullptr) {
        damageStrategy_ = std::move(strategy);
    }
}

int Character::takeDamage(int amount, const std::string& /*source*/) {
    if (amount <= 0) return 0;

    // 冰冻等易伤状态在这里生效：先放大伤害，再扣血
    int real = static_cast<int>(std::lround(amount * state_->incomingMultiplier()));
    if (real < 1) real = 1;

    hp_ -= real;
    if (hp_ < 0) hp_ = 0;
    return real;
}

void Character::heal(int amount) {
    if (amount <= 0) return;
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
}

void Character::inflictState(StateKind kind) {
    switchState(kind);
}

StateKind Character::stateKind() const {
    return state_->kind();
}

std::string Character::stateName() const {
    return state_->name();
}

void Character::switchState(StateKind kind) {
    StateKind oldKind = state_->kind();
    if (oldKind == kind) return;

    state_.reset(makeState(kind));          // 换掉状态对象 —— 行为随之整体改变
    stateTurnsLeft_ = state_->duration();

    // 状态变化属于"值得被广播的事"，通知所有观察者（日志 / UI / BOSS 阶段监听……）
    if (bus_ != nullptr) {
        BattleEvent event;
        event.type   = EventType::StateChanged;
        event.source = name_;
        event.target = name_;
        event.value  = static_cast<int>(kind);
        event.message = name_ + " 进入【" + state_->name() + "】状态";
        bus_->notify(event);
    }
}

void Character::tickState() {
    // 控制抗性计时先递减（它代表"本回合仍然免控"）
    if (controlImmunityTurns_ > 0) {
        --controlImmunityTurns_;
    }

    if (stateTurnsLeft_ == INT_MAX) return;   // 不会自然结束的状态（正常 / 狂暴）

    if (stateTurnsLeft_ > 0) {
        --stateTurnsLeft_;
    }
    if (stateTurnsLeft_ <= 0) {
        const StateKind old = state_->kind();
        // 从控制类状态中恢复后获得一回合控制抗性。
        // 这条规则属于"状态的退出行为"，因此放在状态模式内部处理，
        // 而不是散落成技能代码里的 if (被冻过)。
        if (old == StateKind::Stunned || old == StateKind::Frozen) {
            controlImmunityTurns_ = 1;
        }
        switchState(StateKind::Normal);       // 状态到期，自动回到正常
        stateTurnsLeft_ = INT_MAX;
    }
}

void Character::addSkill(std::unique_ptr<Skill> skill) {
    if (skill != nullptr) {
        skills_.push_back(std::move(skill));
    }
}

void Character::setAI(std::unique_ptr<AIStrategy> ai) {
    ai_ = std::move(ai);
}

std::string Character::describe() const {
    std::ostringstream oss;
    oss << name_
        << " HP " << hp_ << "/" << maxHp_
        << "  ATK " << atk()
        << "  DEF " << def()
        << "  伤害类型：" << damageStrategy_->name();
    if (state_->kind() != StateKind::Normal) {
        oss << "  状态：" << state_->name();
    }
    if (ai_ != nullptr) {
        oss << "  AI：" << ai_->name();
    }
    return oss.str();
}

} // namespace gb
