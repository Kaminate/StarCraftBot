#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "bot_examples.h"

namespace sc2 {




class GooseBot : public MultiplayerBot {
public:
    bool mutalisk_build_ = false;

    bool TryBuildDrone();

    bool TryBuildOverlord();

    void BuildArmy();

    bool TryBuildOnCreep( AbilityID ability_type_for_structure, UnitTypeID unit_type );

    void BuildOrder();

    void ManageUpgrades();

    void ManageArmy();

    void TryInjectLarva();

    bool TryBuildExpansionHatch();

    bool BuildExtractor();

    virtual void OnStep() final;

    virtual void OnUnitIdle( const Unit* unit ) override;

private:
    std::vector<UNIT_TYPEID> hatchery_types = { UNIT_TYPEID::ZERG_HATCHERY, UNIT_TYPEID::ZERG_HIVE, UNIT_TYPEID::ZERG_LAIR };

};



}