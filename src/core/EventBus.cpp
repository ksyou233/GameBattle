// =============================================================================
//  EventBus.cpp  ——  观察者模式中"主题"侧的实现
// =============================================================================

#include "core/EventBus.hpp"

#include <algorithm>

namespace gb {

void EventBus::attach(IBattleObserver* observer) {
    if (observer == nullptr) return;
    // 避免重复注册同一个观察者
    if (std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void EventBus::detach(IBattleObserver* observer) {
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
}

void EventBus::notify(const BattleEvent& event) {
    // 遍历订阅者名单逐个推送。
    // 这里故意不关心观察者是谁、有几个 —— 这正是观察者模式"解耦"的体现。
    for (IBattleObserver* observer : observers_) {
        if (observer != nullptr) {
            observer->onEvent(event);
        }
    }
}

} // namespace gb
