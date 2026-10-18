// =============================================================================
//  Battle.cpp  ——  回合制战斗主循环
//
//  注意这份代码里几乎看不到任何具体类名（没有 Character、没有 WarriorPackageFactory、
//  没有 EquipmentDecorator），全部面向 Combatant / Skill / AIStrategy 这些抽象。
//  把八个设计模式组合起来之后，战斗流程本身反而变得非常"干净"。
// =============================================================================

#include "battle/Battle.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "core/GameConfig.hpp"

namespace gb {

Battle::Battle(std::string title, EventBus& bus)
    : title_(std::move(title)), bus_(bus) {}

void Battle::addHero(std::unique_ptr<Combatant> unit) {
    heroes_.push_back(std::move(unit));
}

void Battle::addMonster(std::unique_ptr<Combatant> unit) {
    monsters_.push_back(std::move(unit));
}

std::vector<Combatant*> Battle::aliveHeroes() const {
    std::vector<Combatant*> result;
    for (const auto& unit : heroes_) {
        if (unit->alive()) result.push_back(unit.get());
    }
    return result;
}

std::vector<Combatant*> Battle::aliveMonsters() const {
    std::vector<Combatant*> result;
    for (const auto& unit : monsters_) {
        if (unit->alive()) result.push_back(unit.get());
    }
    return result;
}

Combatant* Battle::pickTarget(const std::vector<Combatant*>& candidates) {
    // 简化策略：集火第一个还活着的敌人
    return candidates.empty() ? nullptr : candidates.front();
}

bool Battle::battleOver() const {
    return aliveHeroes().empty() || aliveMonsters().empty();
}

void Battle::tickAllStates() {
    // 状态推进由各单位自己完成（状态模式把"到期回到正常"这件事封装在上下文里），
    // 战斗循环只负责统一触发时机。已阵亡单位不再推进状态。
    for (const auto& unit : heroes_) {
        if (unit->alive()) unit->tickState();
    }
    for (const auto& unit : monsters_) {
        if (unit->alive()) unit->tickState();
    }
}

void Battle::actOnce(Combatant& actor) {
    const bool isHero = std::any_of(heroes_.begin(), heroes_.end(),
                                    [&actor](const std::unique_ptr<Combatant>& u) {
                                        return u.get() == &actor;
                                    });
    std::vector<Combatant*> enemies = isHero ? aliveMonsters() : aliveHeroes();

    Combatant* target = pickTarget(enemies);
    if (target == nullptr) return;

    // 决策交给该单位自己的 AI 策略（策略模式），战斗循环不关心它怎么想
    AIStrategy* ai = actor.ai();
    Skill* skill = (ai != nullptr) ? ai->chooseSkill(actor, enemies) : nullptr;

    if (skill == nullptr) {
        BattleEvent event;
        event.type    = EventType::NoAction;
        event.source  = actor.name();
        event.target  = actor.name();
        event.message = actor.name() + " 选择防御，本回合不出手";
        bus_.notify(event);
        return;
    }

    // 出手。技能内部会走责任链算伤害、按组合结构依次施放、并广播全部中间事件
    skill->execute(actor, *target, bus_);

    // 目标是否被这一击打死，由战斗循环统一宣告
    if (!target->alive()) {
        BattleEvent event;
        event.type    = EventType::CharacterDied;
        event.source  = actor.name();
        event.target  = target->name();
        event.message = target->name() + " 被 " + actor.name() + " 击倒！";
        bus_.notify(event);
    }
}

BattleResult Battle::run() {
    GameConfig& config = GameConfig::instance();

    // ---------------------------- 战斗开始 ----------------------------
    {
        BattleEvent event;
        event.type    = EventType::BattleStart;
        event.message = "\n===== " + title_ + " =====";
        bus_.notify(event);

        for (const auto& unit : heroes_)   std::cout << "  我方：" << unit->describe() << "\n";
        for (const auto& unit : monsters_) std::cout << "  敌方：" << unit->describe() << "\n";
    }

    BattleResult result = BattleResult::Draw;

    for (int turn = 1; turn <= config.maxTurns(); ++turn) {
        turnsTaken_ = turn;

        {
            BattleEvent event;
            event.type    = EventType::TurnStart;
            event.message = "── 第 " + std::to_string(turn) + " 回合 ──";
            bus_.notify(event);
        }

        // 行动顺序：英雄先手，怪物后手（回合制游戏的常见设定）
        std::vector<Combatant*> order;
        for (Combatant* hero : aliveHeroes())      order.push_back(hero);
        for (Combatant* monster : aliveMonsters()) order.push_back(monster);

        for (Combatant* actor : order) {
            if (!actor->alive())  continue;      // 可能在本次循环中已被打倒
            if (battleOver())     break;

            // 状态模式的价值在此体现：眩晕/冰冻不让行动，但仍会照常推进状态计时
            if (!actor->canAct()) {
                BattleEvent event;
                event.type    = EventType::NoAction;
                event.source  = actor->name();
                event.target  = actor->name();
                event.message = actor->name() + " 处于【" + actor->stateName() + "】状态，无法行动";
                bus_.notify(event);
                continue;
            }

            actOnce(*actor);
        }

        // 回合结束：统一推进状态计时
        tickAllStates();

        if (aliveMonsters().empty()) { result = BattleResult::HeroesWin;    break; }
        if (aliveHeroes().empty())   { result = BattleResult::MonstersWin;  break; }
    }

    // ---------------------------- 战斗结束 ----------------------------
    {
        std::string text;
        switch (result) {
            case BattleResult::HeroesWin:   text = "★ 英雄阵营取得胜利！"; break;
            case BattleResult::MonstersWin: text = "✖ 英雄阵营全军覆没……"; break;
            case BattleResult::Draw:        text = "= 回合耗尽，双方不分胜负。"; break;
        }
        BattleEvent event;
        event.type    = EventType::BattleEnd;
        event.message = "\n===== 战斗结束（共 " + std::to_string(turnsTaken_)
                        + " 回合）=====\n" + text + "\n";
        bus_.notify(event);
    }

    return result;
}

} // namespace gb
