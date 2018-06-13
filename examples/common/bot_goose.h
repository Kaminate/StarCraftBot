#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "bot_examples.h"

namespace sc2 {

    enum NodeStatus {
        True = 1,
        False = 2,
        Running = 3,
        New = 4,
    };

    struct BehaviorTreeNode {
    public:
        virtual void Execute() = 0;
        void DeleteChildren();
        void ResetChildren();
        void AssignBot(class GooseBot*);
        NodeStatus Status = NodeStatus::New;
        std::vector<BehaviorTreeNode*> Children;
        GooseBot* bot;
    };
#pragma region composite_nodes
    struct NodeRepeatWhileTrue : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeRepeat : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeAny : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeEvery : public BehaviorTreeNode {
        void Execute();
    };
#pragma endregion Loops and conditionals
#pragma region decorator_nodes
    // Decorator nodes
    struct NodeNot : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeTrue : public BehaviorTreeNode {
        void Execute();
    };
#pragma endregion Basically a not nad a true, hahah
#pragma region leaf_nodes
    // Leaf nodes
    struct NodeAccrueEconomy : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeAccrueProduction : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeAccrueInformation : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeAccrueArmy : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeAccrueForce : public BehaviorTreeNode {
        void Execute();
    };

    struct NodeMoveRandomly : public BehaviorTreeNode {
        void Execute();
    };
#pragma endregion Perform actions

    class GooseBot : public MultiplayerBot {
    public:
        bool mutalisk_build_ = false;

        bool TryBuildDrone();

        bool TryBuildOverlord();

        void BuildArmy();

        bool TryBuildOnCreep(AbilityID ability_type_for_structure, UnitTypeID unit_type);

        void BuildOrder();

        void ManageUpgrades();

        void ManageArmy();

        void TryInjectLarva();

        bool TryBuildExpansionHatch();

        bool BuildExtractor();

        virtual void OnStep() final;

        virtual void OnUnitIdle(const Unit* unit) override;

        void Heartbeat();

        void Initialize();

        bool CanPathToLocation(const sc2::Unit*, sc2::Point2D&);

        BehaviorTreeNode* BuildOrderMoveRandomly();

    private:
        std::vector<UNIT_TYPEID> hatchery_types = { UNIT_TYPEID::ZERG_HATCHERY, UNIT_TYPEID::ZERG_HIVE, UNIT_TYPEID::ZERG_LAIR };

        bool Initialized = false;

        BehaviorTreeNode* Root;
    };



}