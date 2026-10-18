// =============================================================================
//  DamageChain.cpp  ——  责任链各环节的实现
// =============================================================================

#include "combat/DamageChain.hpp"

#include <sstream>

#include "character/Combatant.hpp"
#include "core/EventBus.hpp"
#include "core/GameConfig.hpp"

namespace gb {

// ---------------------------------------------------------------------------
//  第 1 环：闪避判定
// ---------------------------------------------------------------------------
void DodgeHandler::handle(DamageContext& ctx) {
    GameConfig& config = GameConfig::instance();

    if (config.roll01() < ctx.defender->dodgeRate()) {
        ctx.dodged = true;
        ctx.damage = 0;
        ctx.trace.push_back("闪避判定：命中失败，链条终止");
        return;                                   // 关键：不再向后传递
    }

    ctx.trace.push_back("闪避判定：命中");
    DamageHandler::handle(ctx);                   // 继续向后传递
}

// ---------------------------------------------------------------------------
//  第 2 环：护甲减伤 —— 这里就是策略模式真正发光的地方
//  同一个环节，因为攻击方持有的 DamageStrategy 不同，算法完全不同
// ---------------------------------------------------------------------------
void ArmorHandler::handle(DamageContext& ctx) {
    int effectiveAtk = static_cast<int>(ctx.attacker->atk() * ctx.powerMultiplier);
    const DamageStrategy& strategy = ctx.attacker->damageType();
    ctx.damage = strategy.compute(effectiveAtk, ctx.defender->def());

    std::ostringstream oss;
    oss << "护甲减伤：按【" << strategy.name() << "】规则结算 "
        << effectiveAtk << " vs DEF " << ctx.defender->def()
        << " -> " << ctx.damage;
    ctx.trace.push_back(oss.str());

    DamageHandler::handle(ctx);
}

// ---------------------------------------------------------------------------
//  第 3 环：格挡判定
//  只负责"判定 + 折减 + 置标志位"，不负责对外通知；
//  是否广播 Blocked 事件由链路出口 performAttack() 统一决定（职责分离）。
// ---------------------------------------------------------------------------
void BlockHandler::handle(DamageContext& ctx) {
    GameConfig& config = GameConfig::instance();

    if (config.roll01() < config.blockRate()) {
        ctx.blocked = true;
        ctx.damage = static_cast<int>(ctx.damage * config.blockReduction());
        ctx.trace.push_back("格挡判定：触发格挡，伤害折减至 " + std::to_string(ctx.damage));
    } else {
        ctx.trace.push_back("格挡判定：未触发");
    }

    DamageHandler::handle(ctx);
}

// ---------------------------------------------------------------------------
//  第 4 环：暴击判定（链路末端，处理完直接返回）
// ---------------------------------------------------------------------------
void CritHandler::handle(DamageContext& ctx) {
    GameConfig& config = GameConfig::instance();

    if (config.roll01() < ctx.attacker->critRate()) {
        ctx.crit = true;
        ctx.damage = static_cast<int>(ctx.damage * config.critMultiplier());

        std::ostringstream oss;
        oss << "暴击判定：触发暴击 x" << config.critMultiplier()
            << " -> " << ctx.damage;
        ctx.trace.push_back(oss.str());
    } else {
        ctx.trace.push_back("暴击判定：未触发");
    }

    DamageHandler::handle(ctx);                   // 末端没有 next_，调用基类即结束
}

// ---------------------------------------------------------------------------
//  链的组装：把四个环节按固定顺序串起来
//  想新增环节（如"反伤""护盾吸收"）时，只需在这里插一行
// ---------------------------------------------------------------------------
DamageChain::DamageChain() {
    handlers_.push_back(std::make_unique<DodgeHandler>());
    handlers_.push_back(std::make_unique<ArmorHandler>());
    handlers_.push_back(std::make_unique<BlockHandler>());
    handlers_.push_back(std::make_unique<CritHandler>());

    for (std::size_t i = 0; i + 1 < handlers_.size(); ++i) {
        handlers_[i]->setNext(handlers_[i + 1].get());
    }
}

DamageContext DamageChain::resolve(Combatant& attacker, Combatant& defender, double powerMultiplier) {
    DamageContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.powerMultiplier = powerMultiplier;

    if (!handlers_.empty()) {
        handlers_.front()->handle(ctx);           // 从链头开始
    }
    return ctx;
}

// ---------------------------------------------------------------------------
//  便捷封装：结算 + 广播事件
// ---------------------------------------------------------------------------
DamageContext performAttack(Combatant& caster, Combatant& target,
                            double powerMultiplier, EventBus& bus,
                            const std::string& skillName) {
    DamageChain chain;
    DamageContext ctx = chain.resolve(caster, target, powerMultiplier);

    BattleEvent event;
    event.source = caster.name();
    event.target = target.name();

    if (ctx.dodged) {
        event.type = EventType::Dodged;
        event.value = 0;
        event.message = "  [" + skillName + "] " + target.name()
                        + " 灵巧地闪避了 " + caster.name() + " 的攻击";
        bus.notify(event);
        return ctx;
    }

    // 真正扣血，拿到"被状态放大之后"的实际伤害值
    int applied = target.takeDamage(ctx.damage, caster.name());
    ctx.damage = applied;

    // 若第 3 环触发了格挡，先广播一条独立的 Blocked 事件。
    // 之所以不复用 DamageDealt 而单开一种事件类型：
    //   · 格挡是一个"独立的战斗事实"，它解释了这次伤害为什么偏低；
    //   · 只有独立成事件，观察者才能在不解析文本的前提下统计/响应它
    //     （战斗日志给它配专属前缀，成就系统据此解锁"铜墙铁壁"）。
    // 发出顺序刻意排在伤害事件之前：先解释原因，再给出结果，日志读起来才顺。
    if (ctx.blocked) {
        BattleEvent blockedEvent;
        blockedEvent.type    = EventType::Blocked;
        blockedEvent.source  = caster.name();
        blockedEvent.target  = target.name();
        blockedEvent.value   = applied;
        blockedEvent.message = "[" + skillName + "] " + target.name()
                               + " 格挡了 " + caster.name() + " 的攻击，伤害被折减";
        bus.notify(blockedEvent);
    }

    event.type = ctx.crit ? EventType::CriticalHit : EventType::DamageDealt;
    event.value = applied;

    std::ostringstream oss;
    oss << "  [" << skillName << "] " << caster.name() << " -> " << target.name()
        << "  造成 " << applied << " 点伤害";
    if (ctx.crit)   oss << "（暴击！）";
    oss << "  剩余 HP " << target.hp() << "/" << target.maxHp();
    event.message = oss.str();

    bus.notify(event);
    return ctx;
}

} // namespace gb
