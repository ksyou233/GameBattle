#pragma once
// =============================================================================
//  DamageChain.hpp  ——  【设计模式 7/8：责任链模式 Chain of Responsibility】
//
//  意图：使多个对象都有机会处理请求，从而避免请求的发送者与接收者之间的耦合。
//        将这些对象连成一条链，并沿着这条链传递该请求，直到有一个对象处理它为止。
//
//  在本项目中的落点：
//      一次攻击真正造成多少伤害，要经过一连串判定：
//          闪避判定 → 护甲减伤 → 格挡判定 → 暴击判定
//      每一步都可能改变结果、也可能直接终止流程（闪避成功就到此为止）。
//      把这些环节写成一个巨型函数，后续想插入"反伤""护盾减免"就得动大手术；
//      写成责任链后，新增环节只需实现一个 Handler 并挂到链上，既有环节零改动。
//
//  与其他模式的协作（答辩时的加分点）：
//      · 与【策略模式】：护甲环节调用防守方/攻击方持有的 DamageStrategy 来算基础伤害
//      · 与【单例模式】：暴击倍率、格挡概率、随机数都取自 GameConfig 单例
// =============================================================================

#include <memory>
#include <string>
#include <vector>

namespace gb {

class Combatant;
class EventBus;

/// 伤害结算的上下文：在链上传递，被各个处理环节逐级修改
struct DamageContext {
    Combatant* attacker = nullptr;
    Combatant* defender = nullptr;
    double     powerMultiplier = 1.0;   ///< 技能倍率，乘在攻击力上
    int        damage = 0;              ///< 流转中的伤害值
    bool       dodged  = false;         ///< 是否被闪避
    bool       blocked = false;         ///< 是否被格挡
    bool       crit    = false;         ///< 是否暴击
    std::vector<std::string> trace;     ///< 结算过程记录，可直接写进文档/答辩材料
};

/// 处理者基类（Handler）
class DamageHandler {
public:
    virtual ~DamageHandler() = default;

    /// 把下一个处理者挂到自己后面，形成链条
    void setNext(DamageHandler* next) { next_ = next; }

    /// 处理请求。默认行为是"自己不处理，直接交给下一环"
    virtual void handle(DamageContext& ctx) {
        if (next_ != nullptr) {
            next_->handle(ctx);
        }
    }

    virtual std::string name() const = 0;

protected:
    DamageHandler* next_ = nullptr;
};

/// 第 1 环：闪避判定。命中失败则整条链终止（这是责任链"中途截断"的典型用法）
class DodgeHandler : public DamageHandler {
public:
    void handle(DamageContext& ctx) override;
    std::string name() const override { return "闪避判定"; }
};

/// 第 2 环：护甲减伤。调用攻击方的 DamageStrategy 计算基础伤害（策略模式在此接入）
class ArmorHandler : public DamageHandler {
public:
    void handle(DamageContext& ctx) override;
    std::string name() const override { return "护甲减伤"; }
};

/// 第 3 环：格挡判定。触发则按配置折减伤害
class BlockHandler : public DamageHandler {
public:
    void handle(DamageContext& ctx) override;
    std::string name() const override { return "格挡判定"; }
};

/// 第 4 环：暴击判定。触发则放大伤害
class CritHandler : public DamageHandler {
public:
    void handle(DamageContext& ctx) override;
    std::string name() const override { return "暴击判定"; }
};

/// 组装好的结算链。对外只暴露 resolve()，隐藏内部环节的组装细节
class DamageChain {
public:
    DamageChain();

    /// 沿链结算一次攻击
    DamageContext resolve(Combatant& attacker, Combatant& defender, double powerMultiplier);

private:
    /// 用 vector 持有各环节，保证生命周期并维持链上指针有效
    std::vector<std::unique_ptr<DamageHandler>> handlers_;
};

/// 便捷函数：结算一次攻击，并把结果广播到事件总线（供技能调用）
/// @return 结算上下文，含最终伤害、是否暴击/闪避/格挡，以及结算轨迹
DamageContext performAttack(Combatant& caster, Combatant& target,
                            double powerMultiplier, EventBus& bus,
                            const std::string& skillName);

} // namespace gb
