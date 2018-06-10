#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "bot_examples.h"

namespace sc2 {




class FervillerBot : public MultiplayerBot {
public:
    bool air_build_ = false;

    bool TryBuildArmy();

    void BuildOrder();

    void ManageUpgrades();

    void ManageArmy();

    bool TryWarpInUnit(ABILITY_ID ability_type_for_unit);

    void ConvertGateWayToWarpGate();

    bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID unit_type);

    bool TryBuildPylon();

    //Separated per race due to gas timings
    bool TryBuildAssimilator();

    // Same as above with expansion timings
    bool TryBuildExpansionNexus();

    bool TryBuildProbe();

    virtual void OnStep() final;

    virtual void OnGameEnd() final;

    virtual void OnUnitIdle(const Unit* unit) override;

    virtual void OnUpgradeCompleted(UpgradeID upgrade) final;

private:
    bool warpgate_reasearched_ = false;
    bool blink_reasearched_ = false;
    int max_colossus_count_ = 5;
    int max_sentry_count_ = 2;
    int max_stalker_count_ = 20;
};



}
