#pragma once

#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_agent.h"
#include "sc2api/sc2_map_info.h"
#include "bot_examples.h"

namespace sc2 {




class NateBot : public MultiplayerBot {
public:
    bool mech_build_ = false;

    bool TryBuildSCV();

    bool TryBuildSupplyDepot();

    void BuildArmy();

    bool TryBuildAddOn( AbilityID ability_type_for_structure, Tag base_structure );

    bool TryBuildStructureRandom( AbilityID ability_type_for_structure, UnitTypeID unit_type );

    void BuildOrder();

    void ManageUpgrades();

    void ManageArmy();

    bool TryBuildExpansionCom();

    bool BuildRefinery();

    virtual void OnStep() final;

    virtual void OnUnitIdle( const Unit* unit ) override;

    virtual void OnUpgradeCompleted( UpgradeID upgrade ) override;

private:
    std::vector<UNIT_TYPEID> barrack_types = { UNIT_TYPEID::TERRAN_BARRACKSFLYING, UNIT_TYPEID::TERRAN_BARRACKS };
    std::vector<UNIT_TYPEID> factory_types = { UNIT_TYPEID::TERRAN_FACTORYFLYING, UNIT_TYPEID::TERRAN_FACTORY };
    std::vector<UNIT_TYPEID> starport_types = { UNIT_TYPEID::TERRAN_STARPORTFLYING, UNIT_TYPEID::TERRAN_STARPORT };
    std::vector<UNIT_TYPEID> supply_depot_types = { UNIT_TYPEID::TERRAN_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED };
    std::vector<UNIT_TYPEID> bio_types = { UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_MARAUDER, UNIT_TYPEID::TERRAN_GHOST, UNIT_TYPEID::TERRAN_REAPER /*reaper*/ };
    std::vector<UNIT_TYPEID> widow_mine_types = { UNIT_TYPEID::TERRAN_WIDOWMINE, UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED };
    std::vector<UNIT_TYPEID> siege_tank_types = { UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED };
    std::vector<UNIT_TYPEID> viking_types = { UNIT_TYPEID::TERRAN_VIKINGASSAULT, UNIT_TYPEID::TERRAN_VIKINGFIGHTER };
    std::vector<UNIT_TYPEID> hellion_types = { UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK };

    bool nuke_built = false;
    bool stim_researched_ = false;
    bool ghost_cloak_researched_ = true;
    bool banshee_cloak_researched_ = true;
};



}
