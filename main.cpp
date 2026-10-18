// =============================================================================
//  main.cpp  ——  设计模式综合演示入口
//
//  演示分三幕：
//    第一幕 装饰器模式渐进式演示：一个裸角色如何被武器、附魔、宝石逐层包装，
//           每包一层就打印一次面板，直观看到"动态添加职责"。
//    第二幕 责任链模式结算轨迹：把一次攻击经过的四道判定逐步打印出来，
//           证明伤害确实是被一条链逐环节加工出来的。
//    第三幕 完整回合制战斗：三名英雄（由抽象工厂装配）对抗四只怪物，
//           过程中自动触发策略决策、责任链结算、状态切换、事件广播与组合技能。
//
//  编译运行见 README.md。设定了随机种子，因此每次运行的战斗过程完全一致。
//
//  命令行参数：
//    （无参数）    双击运行时，演示结束后停住窗口等一次回车，避免输出一闪而过
//    --pause       强制停留（现场演示用）
//    --no-pause    强制不停留（脚本、管道、输出重定向调用）
// =============================================================================

#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX          // 避免 windows.h 的 min/max 宏污染标准库
#  endif
#  include <windows.h>        // GetConsoleProcessList
#endif

#include "battle/Battle.hpp"
#include "character/Character.hpp"
#include "character/HeroPackageFactory.hpp"
#include "combat/AIStrategy.hpp"
#include "combat/DamageChain.hpp"
#include "combat/DamageStrategy.hpp"
#include "core/EventBus.hpp"
#include "core/GameConfig.hpp"
#include "equipment/EquipmentDecorator.hpp"
#include "observer/BattleObserver.hpp"
#include "skill/Skill.hpp"

using namespace gb;

namespace {

// -----------------------------------------------------------------------------
//  演示结束后是否保持控制台窗口
//
//  直接双击 exe 运行时，Windows 会在进程退出的瞬间销毁它的控制台，
//  上百行演示输出还没看清就随窗口一起没了。这里决定要不要停下来等一次回车。
//
//  优先级：命令行参数 > 自动判断
//    --pause     强制停留
//    --no-pause  强制不停留（脚本 / 管道 / 重定向场景）
//    默认        只有"双击启动"才停留
//
//  自动判断的依据是 GetConsoleProcessList()：它返回与当前控制台关联的进程数。
//  双击 exe 时 Explorer 会新建一个控制台，且其中只有本进程（计数 == 1）；
//  而从已有的 cmd / PowerShell / 终端里启动时，控制台里至少还挂着父 shell
//  （计数 >= 2），窗口本来就不会消失，无需再停。
// -----------------------------------------------------------------------------
bool holdWindow(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--no-pause") return false;
        if (arg == "--pause")    return true;
    }
#ifdef _WIN32
    DWORD attached[4] = {0};
    return ::GetConsoleProcessList(attached, 4) == 1;
#else
    return false;   // 仅针对 Windows 双击启动的场景
#endif
}

/// 打印一条分节标题
void section(const std::string& title) {
    std::cout << "\n";
    std::cout << "################################################################\n";
    std::cout << "#  " << title << "\n";
    std::cout << "################################################################\n";
}

// -----------------------------------------------------------------------------
//  第一幕：装饰器模式渐进式演示
// -----------------------------------------------------------------------------
void demoDecorator() {
    section("第一幕  装饰器模式：角色的动态武装");

    // 一个裸角色：只有基础属性，没有任何装备
    std::unique_ptr<Combatant> unit = std::make_unique<Character>(
        "示范兵", 200, 30, 10, 0.15, 0.10,
        std::make_unique<PhysicalDamage>(), nullptr);

    std::cout << "  ① 裸装：" << unit->describe() << "\n";

    // 装上一把巨剑
    unit = std::make_unique<WeaponDecorator>(std::move(unit), "巨剑", 9);
    std::cout << "  ② 装上巨剑后：" << unit->describe() << "\n";

    // 再套一层烈焰附魔：注意伤害类型从"物理"变成了"魔法"
    unit = std::make_unique<FireEnchantDecorator>(std::move(unit));
    std::cout << "  ③ 附加烈焰附魔后：" << unit->describe() << "\n";

    // 最后镶一颗暴击宝石
    unit = std::make_unique<CritGemDecorator>(std::move(unit), 0.12);
    std::cout << "  ④ 镶嵌暴击宝石后：" << unit->describe() << "\n";

    std::cout << "\n  [说明] 全程只做了一件事：把对象包进新的装饰器。\n";
    std::cout << "         没有新增任何子类，属性与伤害类型却都变了。\n";
}

// -----------------------------------------------------------------------------
//  第二幕：责任链模式 —— 把一次攻击的结算轨迹完整打印出来
//  这是责任链模式最直观的证据：一次伤害到底过了哪几道关、每道关改了什么
// -----------------------------------------------------------------------------
void demoDamageChain() {
    section("第二幕  责任链模式：一次攻击的结算轨迹");

    EventBus quietBus;      // 本幕只关心结算轨迹，不挂任何观察者

    // 攻击方：攻击力 40，暴击率 100%（保证轨迹里一定出现暴击环节）
    auto attacker = std::make_unique<Character>(
        "演示剑士", 200, 40, 10, 1.0, 0.0,
        std::make_unique<PhysicalDamage>(), &quietBus);
    // 防守方：防御力 30 的"训练木桩"，闪避率 0，不会反击
    auto defender = std::make_unique<Character>(
        "训练木桩", 500, 0, 30, 0.0, 0.0,
        std::make_unique<PhysicalDamage>(), &quietBus);

    std::cout << "  攻击方：ATK " << attacker->atk()
              << "，技能倍率 1.5，暴击率 100%\n";
    std::cout << "  防守方：DEF " << defender->def()
              << "（物理伤害按 max(1, ATK - DEF) 结算）\n\n";

    DamageChain chain;
    DamageContext ctx = chain.resolve(*attacker, *defender, 1.5);

    std::cout << "  ── 责任链逐环节轨迹 ──\n";
    for (std::size_t i = 0; i < ctx.trace.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << ctx.trace[i] << "\n";
    }
    std::cout << "\n  链的最终产出：伤害 " << ctx.damage
              << "，暴击=" << (ctx.crit ? "是" : "否")
              << "，格挡=" << (ctx.blocked ? "是" : "否")
              << "，闪避=" << (ctx.dodged ? "是" : "否") << "\n";
    std::cout << "  （以上轨迹由 DamageContext::trace 逐环节记录，不是手工构造的示例）\n";
}

// -----------------------------------------------------------------------------
//  第三幕：完整战斗
// -----------------------------------------------------------------------------
void demoBattle() {
    section("第三幕  回合制战斗：八个设计模式的协同");

    // ---------- 全局唯一的配置中心（单例模式） ----------
    GameConfig& config = GameConfig::instance();
    config.setVerbose(true);

    // ---------- 事件总线 + 观察者（观察者模式） ----------
    EventBus bus;

    BattleLogObserver        logObserver;
    AchievementObserver      achievementObserver;
    BattleRecorderObserver   recorderObserver;

    bus.attach(&logObserver);
    bus.attach(&achievementObserver);
    bus.attach(&recorderObserver);

    std::cout << "  已注册观察者 " << bus.observerCount() << " 个："
              << logObserver.observerName() << " / "
              << achievementObserver.observerName() << " / "
              << recorderObserver.observerName() << "\n";

    // ---------- 抽象工厂（抽象工厂模式）：三个职业包 ----------
    WarriorPackageFactory warriorFactory;
    MagePackageFactory    mageFactory;
    ArcherPackageFactory  archerFactory;

    Battle battle("英雄小队 vs 暗影魔龙", bus);

    // ---- 英雄 1：亚瑟王（战士，莽夫 AI） ----
    {
        auto body = std::make_unique<Character>(
            "亚瑟", 300, 34, 14, 0.22, 0.08,
            std::make_unique<PhysicalDamage>(), &bus);
        Character* raw = body.get();                       // 先拿到裸角色以便装配技能
        warriorFactory.applyTalent(*raw);                  // 产品D：天赋作用于裸角色
        raw->addSkill(warriorFactory.signatureSkill());    // 旋风斩（单体 + 眩晕）
        raw->addSkill(warriorFactory.ultimateSkill());     // 裂地三连斩（组合模式）
        raw->addSkill(std::make_unique<HealSkill>("绷带包扎", 25));
        raw->setAI(std::make_unique<AggressiveAI>());      // 策略模式

        // 交给工厂装配武器与防具（返回的是套好装饰器的 Combatant）
        // 天赋已先于装备生效，因此"基础14 + 铁壁4 + 板甲6"三层加成互不干扰
        std::unique_ptr<Combatant> unit = warriorFactory.equip(std::move(body));
        std::cout << "  ▶ 战士天赋：" << warriorFactory.talent() << "\n";
        battle.addHero(std::move(unit));
    }

    // ---- 英雄 2：梅林（法师，辅助 AI，魔法伤害策略） ----
    {
        auto body = std::make_unique<Character>(
            "梅林", 180, 30, 6, 0.15, 0.10,
            std::make_unique<MagicDamage>(), &bus);        // 策略模式：魔法伤害规则
        Character* raw = body.get();
        mageFactory.applyTalent(*raw);                     // 产品D：天赋换伤害策略
        raw->addSkill(mageFactory.signatureSkill());       // 火球术
        raw->addSkill(mageFactory.ultimateSkill());        // 冰火两重天（连招 + 冻结）
        raw->addSkill(std::make_unique<HealSkill>("奥术回响", 30));
        raw->setAI(std::make_unique<SupportAI>());

        // 天赋把伤害策略设为 MagicDamage，与建号时指定的策略一致（此处即"兑现天赋文案"）
        std::unique_ptr<Combatant> unit = mageFactory.equip(std::move(body));
        std::cout << "  ▶ 法师天赋：" << mageFactory.talent() << "\n";
        battle.addHero(std::move(unit));
    }

    // ---- 英雄 3：莉娜（射手，莽夫 AI，高暴击） ----
    {
        auto body = std::make_unique<Character>(
            "莉娜", 200, 28, 8, 0.30, 0.18,
            std::make_unique<PhysicalDamage>(), &bus);
        Character* raw = body.get();
        archerFactory.applyTalent(*raw);                   // 产品D：天赋 +10% 暴击率
        raw->addSkill(archerFactory.signatureSkill());     // 穿透射击
        raw->addSkill(archerFactory.ultimateSkill());      // 疾风三连射
        raw->setAI(std::make_unique<AggressiveAI>());

        // 天赋已先于装备生效，因此莉娜暴击 = 30%(基础) + 10%(鹰眼) = 40%
        std::unique_ptr<Combatant> unit = archerFactory.equip(std::move(body));
        std::cout << "  ▶ 射手天赋：" << archerFactory.talent() << "\n";
        battle.addHero(std::move(unit));
    }

    // ---- 怪物 1：哥布林斥候（保守 AI） ----
    {
        auto body = std::make_unique<Character>(
            "哥布林斥候", 150, 24, 8, 0.08, 0.12,
            std::make_unique<PhysicalDamage>(), &bus);
        body->addSkill(std::make_unique<AttackSkill>("短刀突刺", 1.1));
        body->setAI(std::make_unique<DefensiveAI>());
        battle.addMonster(std::move(body));
    }

    // ---- 怪物 2：哥布林祭司（辅助 AI，会自愈） ----
    {
        auto body = std::make_unique<Character>(
            "哥布林祭司", 130, 20, 6, 0.05, 0.08,
            std::make_unique<MagicDamage>(), &bus);
        body->addSkill(std::make_unique<AttackSkill>("腐化之箭", 1.2));
        body->addSkill(std::make_unique<HealSkill>("邪能治疗", 22));
        body->setAI(std::make_unique<SupportAI>());
        battle.addMonster(std::move(body));
    }

    // ---- 怪物 3：哥布林战士（莽夫 AI，纯物理输出） ----
    {
        auto body = std::make_unique<Character>(
            "哥布林战士", 170, 26, 10, 0.10, 0.05,
            std::make_unique<PhysicalDamage>(), &bus);
        body->addSkill(std::make_unique<AttackSkill>("战斧劈砍", 1.25));
        body->setAI(std::make_unique<AggressiveAI>());
        battle.addMonster(std::move(body));
    }

    // ---- 怪物 4：暗影魔龙（BOSS，真实伤害策略 + 观察者监听的第二阶段） ----
    Combatant* boss = nullptr;
    {
        auto body = std::make_unique<Character>(
            "暗影魔龙", 900, 24, 20, 0.18, 0.05,
            std::make_unique<TrueDamage>(), &bus);         // 策略模式：无视防御的真实伤害
        body->addSkill(std::make_unique<AttackSkill>("龙爪撕裂", 1.35));

        std::vector<std::unique_ptr<Skill>> breath;
        breath.push_back(std::make_unique<AttackSkill>("暗影冲击", 1.2));
        breath.push_back(std::make_unique<AttackSkill>("龙息", 1.3));
        body->addSkill(std::make_unique<ComboSkill>("暗影吐息", std::move(breath)));

        body->setAI(std::make_unique<AggressiveAI>());
        boss = body.get();
        battle.addMonster(std::move(body));
    }

    // 为 BOSS 动态追加一个观察者：血量跌破一半就进入狂暴（观察者 + 状态模式联动）。
    // 这一个 attach() 就是新增功能的全部代价 —— 战斗逻辑一行没改。
    BossPhaseObserver bossPhase(boss, 0.5, StateKind::Berserk);
    bus.attach(&bossPhase);
    std::cout << "  ▶ 追加观察者：" << bossPhase.observerName()
              << "（监听 " << boss->name() << "，阈值 50%）\n";

    // ---------- 开打 ----------
    battle.run();

    section("战斗统计");
    // 格挡次数由 AchievementObserver 统计：它监听责任链第 3 环发出的 Blocked 事件，
    // 与暴击、击杀一样是"观察者独立采集"的数据，战斗结算本身并不统计任何东西。
    std::cout << "  暴击次数：" << achievementObserver.criticalCount()
              << "    格挡次数：" << achievementObserver.blockCount()
              << "    击杀数：" << achievementObserver.killCount() << "\n";
    std::cout << "  录制事件：" << recorderObserver.recordedCount() << " 条\n\n";
    recorderObserver.dump();
}

} // namespace

int main(int argc, char** argv) {
    // 固定随机种子：同一份种子 + 同一套代码 = 完全相同的战斗过程，
    // 便于写实验报告、截图与答辩复现。
    GameConfig::instance().setSeed(20261010u);
    GameConfig::instance().banner();

    demoDecorator();
    demoDamageChain();

    // 第二幕消耗了一部分随机数，这里把种子重置回初始值，
    // 保证第三幕的战斗过程与文档中记录的输出完全一致。
    GameConfig::instance().setSeed(20261010u);
    demoBattle();

    std::cout << "\n演示结束。\n";

    // 双击启动时把窗口留住，否则输出会随控制台一起消失。
    if (holdWindow(argc, argv)) {
        std::cout << "\n按 Enter 键关闭窗口 ...";
        std::cout.flush();
        std::cin.get();
    }
    return 0;
}
