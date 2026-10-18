// =============================================================================
//  BattleObserver.cpp  ——  各观察者的回调实现
// =============================================================================

#include "observer/BattleObserver.hpp"

#include <algorithm>
#include <iostream>

#include "core/GameConfig.hpp"

namespace gb {

// ---------------------------------------------------------------------------
//  A. 战斗日志
// ---------------------------------------------------------------------------
void BattleLogObserver::onEvent(const BattleEvent& event) {
    switch (event.type) {
        case EventType::BattleStart:
        case EventType::TurnStart:
        case EventType::BattleEnd:
            std::cout << event.message << "\n";
            break;

        case EventType::StateChanged:
            std::cout << "  ★ " << event.message << "\n";
            break;

        case EventType::CharacterDied:
            std::cout << "  ✖ " << event.message << "\n";
            break;

        case EventType::NoAction:
            std::cout << "  - " << event.message << "\n";
            break;

        case EventType::Blocked:
            std::cout << "  ⊘ " << event.message << "\n";
            break;

        default:
            std::cout << event.message << "\n";
            break;
    }
}

// ---------------------------------------------------------------------------
//  B. 成就系统
// ---------------------------------------------------------------------------
void AchievementObserver::onEvent(const BattleEvent& event) {
    switch (event.type) {
        case EventType::CriticalHit:
            ++criticalCount_;
            maxSingleHit_ = std::max(maxSingleHit_, event.value);
            if (criticalCount_ == 3) {
                unlocked_.push_back("【致命节奏】单场战斗打出 3 次以上暴击");
            }
            break;

        case EventType::Blocked:
            ++blockCount_;
            if (blockCount_ == 3) {
                unlocked_.push_back("【铜墙铁壁】单场战斗成功格挡 3 次");
            }
            break;

        case EventType::CharacterDied:
            ++killCount_;
            if (killCount_ == 2) {
                unlocked_.push_back("【初露锋芒】击倒 2 个敌人");
            } else if (killCount_ == 3) {
                unlocked_.push_back("【清道夫】击倒 3 个敌人");
            }
            break;

        default:
            break;
    }

    // 成就解锁后立即提示
    while (reported_ < unlocked_.size()) {
        std::cout << "  ◆ 成就解锁：" << unlocked_[reported_] << "\n";
        ++reported_;
    }
}

// ---------------------------------------------------------------------------
//  C. BOSS 阶段监听
// ---------------------------------------------------------------------------
BossPhaseObserver::BossPhaseObserver(Combatant* watched, double hpThreshold, StateKind triggerState)
    : watched_(watched), hpThreshold_(hpThreshold), triggerState_(triggerState) {}

void BossPhaseObserver::onEvent(const BattleEvent& event) {
    if (triggered_ || watched_ == nullptr) return;
    // 只关心"被打"这件事，且必须打在被观察单位身上
    if (event.type != EventType::DamageDealt && event.type != EventType::CriticalHit) return;
    if (event.target != watched_->name()) return;

    double ratio = static_cast<double>(watched_->hp()) / static_cast<double>(watched_->maxHp());
    if (ratio > 0.0 && ratio < hpThreshold_) {
        triggered_ = true;                       // 先置位，避免状态变化事件回调进来再次触发
        std::cout << "  ⚠ " << watched_->name() << " 血量跌破 "
                  << static_cast<int>(hpThreshold_ * 100) << "%，进入第二阶段！\n";
        // 与状态模式联动：观察者不修改战斗逻辑，只是给被观察者换了个状态
        watched_->inflictState(triggerState_);
    }
}

// ---------------------------------------------------------------------------
//  D. 战斗回放录制
// ---------------------------------------------------------------------------
void BattleRecorderObserver::onEvent(const BattleEvent& event) {
    // 只录"战斗内容"，跳过开场/回合/结束这些框架性事件，
    // 这样导出的战报全部是实质性的战斗动作。
    switch (event.type) {
        case EventType::BattleStart:
        case EventType::TurnStart:
        case EventType::BattleEnd:
            return;
        default:
            break;
    }
    records_.push_back(event);
}

void BattleRecorderObserver::dump() const {
    std::cout << "  本次战斗共录制 " << records_.size() << " 条事件，最后 6 条如下：\n";
    std::size_t start = records_.size() > 6 ? records_.size() - 6 : 0;
    for (std::size_t i = start; i < records_.size(); ++i) {
        std::cout << "    回放#" << (i + 1) << "：" << records_[i].message << "\n";
    }
}

} // namespace gb
