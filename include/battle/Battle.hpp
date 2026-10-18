#pragma once
// =============================================================================
//  Battle.hpp  ——  回合制战斗流程（设计模式的"装配车间"）
//
//  这个类本身不是某个设计模式的实现，而是把各模式串起来的地方：
//      · 通过 Combatant 抽象接口统一操作"英雄"与"怪物"
//      · 让每个单位用自己的 AIStrategy 决策、用自己的 Skill 出手
//      · 所有结果通过 EventBus 广播给若干观察者
//  代码里看不到 EquipmentDecorator、ConcreteSkill 等具体类型，
//  全部面向抽象 —— 这正是多个设计模式协同之后的效果。
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include "character/Combatant.hpp"
#include "core/EventBus.hpp"

namespace gb {

enum class BattleResult {
    HeroesWin,
    MonstersWin,
    Draw
};

class Battle {
public:
    Battle(std::string title, EventBus& bus);

    void addHero(std::unique_ptr<Combatant> unit);
    void addMonster(std::unique_ptr<Combatant> unit);

    /// 跑完整场战斗
    BattleResult run();

    int turnsTaken() const { return turnsTaken_; }

private:
    /// 回合开始的统一处理：推进状态计时
    void tickAllStates();

    /// 某个单位行动一次
    void actOnce(Combatant& actor);

    std::vector<Combatant*> aliveHeroes() const;
    std::vector<Combatant*> aliveMonsters() const;

    /// 从候选列表中挑第一个存活单位作为目标
    static Combatant* pickTarget(const std::vector<Combatant*>& candidates);

    bool battleOver() const;

    std::string title_;
    EventBus&   bus_;

    std::vector<std::unique_ptr<Combatant>> heroes_;
    std::vector<std::unique_ptr<Combatant>> monsters_;

    int turnsTaken_ = 0;
};

} // namespace gb
