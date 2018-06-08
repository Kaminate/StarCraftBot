
#include "bot_nate.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>

#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"


namespace sc2 {



bool NateBot::TryBuildSCV() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );

    for( const auto& base : bases ) {
        if( base->unit_type == UNIT_TYPEID::TERRAN_ORBITALCOMMAND && base->energy > 50 ) {
            if( FindNearestMineralPatch( base->pos ) ) {
                Actions()->UnitCommand( base, ABILITY_ID::EFFECT_CALLDOWNMULE );
            }
        }
    }

    if( observation->GetFoodWorkers() >= max_worker_count_ ) {
        return false;
    }

    if( observation->GetFoodUsed() >= observation->GetFoodCap() ) {
        return false;
    }

    if( observation->GetFoodWorkers() > GetExpectedWorkers( UNIT_TYPEID::TERRAN_REFINERY ) ) {
        return false;
    }

    for( const auto& base : bases ) {
        //if there is a base with less than ideal workers
        if( base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1 ) {
            if( observation->GetMinerals() >= 50 ) {
                return TryBuildUnit( ABILITY_ID::TRAIN_SCV, base->unit_type );
            }
        }
    }
    return false;
}

bool NateBot::TryBuildSupplyDepot() {
    const ObservationInterface* observation = Observation();

    // If we are not supply capped, don't build a supply depot.
    if( observation->GetFoodUsed() < observation->GetFoodCap() - 6 ) {
        return false;
    }

    if( observation->GetMinerals() < 100 ) {
        return false;
    }

    //check to see if there is already on building
    Units units = observation->GetUnits( Unit::Alliance::Self, IsUnits( supply_depot_types ) );
    if( observation->GetFoodUsed() < 40 ) {
        for( const auto& unit : units ) {
            if( unit->build_progress != 1 ) {
                return false;
            }
        }
    }

    // Try and build a supply depot. Find a random SCV and give it the order.
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D( staging_location_.x + rx * 15, staging_location_.y + ry * 15 );
    return TryBuildStructure( ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV, build_location );
}

void NateBot::BuildArmy() {
    const ObservationInterface* observation = Observation();
    //grab army and building counts
    Units barracks = observation->GetUnits( Unit::Alliance::Self, IsUnit( UNIT_TYPEID::TERRAN_BARRACKS ) );
    Units factorys = observation->GetUnits( Unit::Alliance::Self, IsUnit( UNIT_TYPEID::TERRAN_FACTORY ) );
    Units starports = observation->GetUnits( Unit::Alliance::Self, IsUnit( UNIT_TYPEID::TERRAN_STARPORT ) );

    size_t widowmine_count = CountUnitTypeTotal( observation, widow_mine_types, UNIT_TYPEID::TERRAN_FACTORY, ABILITY_ID::TRAIN_WIDOWMINE );

    size_t hellbat_count = CountUnitTypeTotal( observation, hellion_types, UNIT_TYPEID::TERRAN_FACTORY, ABILITY_ID::TRAIN_HELLION );
    hellbat_count += CountUnitTypeBuilding( observation, UNIT_TYPEID::TERRAN_FACTORY, ABILITY_ID::TRAIN_HELLBAT );

    size_t siege_tank_count = CountUnitTypeTotal( observation, siege_tank_types, UNIT_TYPEID::TERRAN_FACTORY, ABILITY_ID::TRAIN_SIEGETANK );
    size_t viking_count = CountUnitTypeTotal( observation, viking_types, UNIT_TYPEID::TERRAN_FACTORY, ABILITY_ID::TRAIN_VIKINGFIGHTER );
    size_t marine_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_BARRACKS, ABILITY_ID::TRAIN_MARINE );
    size_t marauder_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_MARAUDER, UNIT_TYPEID::TERRAN_BARRACKS, ABILITY_ID::TRAIN_MARAUDER );
    size_t reaper_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_REAPER, UNIT_TYPEID::TERRAN_BARRACKS, ABILITY_ID::TRAIN_REAPER );
    size_t ghost_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_GHOST, UNIT_TYPEID::TERRAN_BARRACKS, ABILITY_ID::TRAIN_GHOST );
    size_t medivac_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_MEDIVAC, UNIT_TYPEID::TERRAN_STARPORT, ABILITY_ID::TRAIN_MEDIVAC );
    size_t raven_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_RAVEN, UNIT_TYPEID::TERRAN_STARPORT, ABILITY_ID::TRAIN_RAVEN );
    size_t battlecruiser_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_MEDIVAC, UNIT_TYPEID::TERRAN_STARPORT, ABILITY_ID::TRAIN_BATTLECRUISER );
    size_t banshee_count = CountUnitTypeTotal( observation, UNIT_TYPEID::TERRAN_MEDIVAC, UNIT_TYPEID::TERRAN_STARPORT, ABILITY_ID::TRAIN_BANSHEE );

    if( !mech_build_ && CountUnitType( observation, UNIT_TYPEID::TERRAN_GHOSTACADEMY ) + CountUnitType( observation, UNIT_TYPEID::TERRAN_FACTORY ) > 0 ) {
        if( !nuke_built ) {
            Units ghosts = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_GHOST ) );
            if( observation->GetMinerals() > 100 && observation->GetVespene() > 100 ) {
                TryBuildUnit( ABILITY_ID::BUILD_NUKE, UNIT_TYPEID::TERRAN_GHOSTACADEMY );
            }
            if( !ghosts.empty() ) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit( ghosts.front() );
                for( const auto& ability : abilities.abilities ) {
                    if( ability.ability_id == ABILITY_ID::EFFECT_NUKECALLDOWN ) {
                        nuke_built = true;
                    }
                }
            }

        }
    }


    if( !starports.empty() ) {
        for( const auto& starport : starports ) {
            if( observation->GetUnit( starport->add_on_tag ) == nullptr ) {
                if( mech_build_ ) {
                    if( starport->orders.empty() && viking_count < 5 ) {
                        Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_VIKINGFIGHTER );
                    }
                }
                else {
                    if( starport->orders.empty() && medivac_count < 5 ) {
                        Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_MEDIVAC );
                    }
                }
                continue;
            }
            else {
                if( observation->GetUnit( starport->add_on_tag )->unit_type == UNIT_TYPEID::TERRAN_STARPORTREACTOR ) {
                    if( mech_build_ ) {
                        if( starport->orders.size() < 2 && viking_count < 5 ) {
                            Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_VIKINGFIGHTER );
                        }
                    }
                    else {
                        if( starport->orders.size() < 2 && medivac_count < 5 ) {
                            Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_MEDIVAC );
                        }
                        if( starport->orders.size() < 2 && medivac_count < 3 ) {
                            Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_MEDIVAC );
                        }
                    }

                }
                else {
                    if( starport->orders.empty() && raven_count < 2 ) {
                        Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_RAVEN );
                    }
                    if( !mech_build_ ) {
                        if( CountUnitType( observation, UNIT_TYPEID::TERRAN_FUSIONCORE ) > 0 ) {
                            if( starport->orders.empty() && battlecruiser_count < 2 ) {
                                Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_BATTLECRUISER );
                                if( battlecruiser_count < 1 ) {
                                    return;
                                }
                            }
                        }
                        if( starport->orders.empty() && banshee_count < 2 ) {
                            Actions()->UnitCommand( starport, ABILITY_ID::TRAIN_BANSHEE );
                        }
                    }
                }
            }
        }
    }

    if( !barracks.empty() ) {
        for( const auto& barrack : barracks ) {
            if( observation->GetUnit( barrack->add_on_tag ) == nullptr ) {
                if( !mech_build_ ) {
                    if( barrack->orders.empty() && marine_count < 20 ) {
                        Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                    }
                    else if( barrack->orders.empty() && observation->GetMinerals() > 1000 && observation->GetVespene() < 300 ) {
                        Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                    }
                }
            }
            else {
                if( observation->GetUnit( barrack->add_on_tag )->unit_type == UNIT_TYPEID::TERRAN_BARRACKSREACTOR ) {
                    if( mech_build_ ) {
                        if( barrack->orders.size() < 2 && marine_count < 5 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                        }
                    }
                    else {
                        if( barrack->orders.size() < 2 && marine_count < 20 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                        }
                        else if( observation->GetMinerals() > 1000 && observation->GetVespene() < 300 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                        }
                    }
                }
                else {
                    if( !mech_build_ && barrack->orders.empty() ) {
                        if( reaper_count < 2 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_REAPER );
                        }
                        else if( ghost_count < 4 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_GHOST );
                        }
                        else if( marauder_count < 10 ) {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARAUDER );
                        }
                        else {
                            Actions()->UnitCommand( barrack, ABILITY_ID::TRAIN_MARINE );
                        }
                    }
                }
            }
        }
    }

    if( !factorys.empty() ) {
        for( const auto& factory : factorys ) {
            if( observation->GetUnit( factory->add_on_tag ) == nullptr ) {
                if( mech_build_ ) {
                    if( factory->orders.empty() && hellbat_count < 20 ) {
                        if( CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) > 0 ) {
                            Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_HELLBAT );
                        }
                        else {
                            Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_HELLION );
                        }
                    }
                }
            }
            else {
                if( observation->GetUnit( factory->add_on_tag )->unit_type == UNIT_TYPEID::TERRAN_FACTORYREACTOR ) {
                    if( factory->orders.size() < 2 && widowmine_count < 4 ) {
                        Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_WIDOWMINE );
                    }
                    if( mech_build_ && factory->orders.size() < 2 ) {
                        if( observation->GetMinerals() > 1000 && observation->GetVespene() < 300 ) {
                            Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_HELLBAT );
                        }
                        if( hellbat_count < 20 ) {
                            if( CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) > 0 ) {
                                Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_HELLBAT );
                            }
                            else {
                                Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_HELLION );
                            }
                        }
                        if( CountUnitType( observation, UNIT_TYPEID::TERRAN_CYCLONE ) < 6 ) {
                            Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_CYCLONE );
                        }
                    }
                }
                else {
                    if( mech_build_ && factory->orders.empty() && CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) > 0 ) {
                        if( CountUnitType( observation, UNIT_TYPEID::TERRAN_THOR ) < 4 ) {
                            Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_THOR );
                            return;
                        }
                    }
                    if( !mech_build_ && factory->orders.empty() && siege_tank_count < 7 ) {
                        Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_SIEGETANK );
                    }
                    if( mech_build_ && factory->orders.empty() && siege_tank_count < 10 ) {
                        Actions()->UnitCommand( factory, ABILITY_ID::TRAIN_SIEGETANK );
                    }
                }
            }
        }
    }
}

void NateBot::BuildOrder() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits( Unit::Self, IsTownHall() );
    Units barracks = observation->GetUnits( Unit::Self, IsUnits( barrack_types ) );
    Units factorys = observation->GetUnits( Unit::Self, IsUnits( factory_types ) );
    Units starports = observation->GetUnits( Unit::Self, IsUnits( starport_types ) );
    Units barracks_tech = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_BARRACKSTECHLAB ) );
    Units factorys_tech = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_FACTORYTECHLAB ) );
    Units starports_tech = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_STARPORTTECHLAB ) );

    Units supply_depots = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_SUPPLYDEPOT ) );
    if( bases.size() < 3 && CountUnitType( observation, UNIT_TYPEID::TERRAN_FUSIONCORE ) ) {
        TryBuildExpansionCom();
        return;
    }

    for( const auto& supply_depot : supply_depots ) {
        Actions()->UnitCommand( supply_depot, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER );
    }
    if( !barracks.empty() ) {
        for( const auto& base : bases ) {
            if( base->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER && observation->GetMinerals() > 150 ) {
                Actions()->UnitCommand( base, ABILITY_ID::MORPH_ORBITALCOMMAND );
            }
        }
    }

    for( const auto& barrack : barracks ) {
        if( !barrack->orders.empty() || barrack->build_progress != 1 ) {
            continue;
        }
        if( observation->GetUnit( barrack->add_on_tag ) == nullptr ) {
            if( barracks_tech.size() < barracks.size() / 2 || barracks_tech.empty() ) {
                TryBuildAddOn( ABILITY_ID::BUILD_TECHLAB_BARRACKS, barrack->tag );
            }
            else {
                TryBuildAddOn( ABILITY_ID::BUILD_REACTOR_BARRACKS, barrack->tag );
            }
        }
    }

    for( const auto& factory : factorys ) {
        if( !factory->orders.empty() ) {
            continue;
        }

        if( observation->GetUnit( factory->add_on_tag ) == nullptr ) {
            if( mech_build_ ) {
                if( factorys_tech.size() < factorys.size() / 2 ) {
                    TryBuildAddOn( ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag );
                }
                else {
                    TryBuildAddOn( ABILITY_ID::BUILD_REACTOR_FACTORY, factory->tag );
                }
            }
            else {
                if( CountUnitType( observation, UNIT_TYPEID::TERRAN_BARRACKSREACTOR ) < 1 ) {
                    TryBuildAddOn( ABILITY_ID::BUILD_REACTOR_FACTORY, factory->tag );
                }
                else {
                    TryBuildAddOn( ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag );
                }
            }

        }
    }

    for( const auto& starport : starports ) {
        if( !starport->orders.empty() ) {
            continue;
        }
        if( observation->GetUnit( starport->add_on_tag ) == nullptr ) {
            if( mech_build_ ) {
                if( CountUnitType( observation, UNIT_TYPEID::TERRAN_STARPORTREACTOR ) < 2 ) {
                    TryBuildAddOn( ABILITY_ID::BUILD_REACTOR_STARPORT, starport->tag );
                }
                else {
                    TryBuildAddOn( ABILITY_ID::BUILD_TECHLAB_STARPORT, starport->tag );
                }
            }
            else {
                if( CountUnitType( observation, UNIT_TYPEID::TERRAN_STARPORTREACTOR ) < 1 ) {
                    TryBuildAddOn( ABILITY_ID::BUILD_REACTOR_STARPORT, starport->tag );
                }
                else {
                    TryBuildAddOn( ABILITY_ID::BUILD_TECHLAB_STARPORT, starport->tag );
                }

            }
        }
    }

    size_t barracks_count_target = std::min<size_t>( 3 * bases.size(), 8 );
    size_t factory_count_target = 1;
    size_t starport_count_target = 2;
    size_t armory_count_target = 1;
    if( mech_build_ ) {
        barracks_count_target = 1;
        armory_count_target = 2;
        factory_count_target = std::min<size_t>( 2 * bases.size(), 7 );
        starport_count_target = std::min<size_t>( 1 * bases.size(), 4 );
    }

    if( !factorys.empty() && starports.size() < std::min<size_t>( 1 * bases.size(), 4 ) ) {
        if( observation->GetMinerals() > 150 && observation->GetVespene() > 100 ) {
            TryBuildStructureRandom( ABILITY_ID::BUILD_STARPORT, UNIT_TYPEID::TERRAN_SCV );
        }
    }

    if( !barracks.empty() && factorys.size() < std::min<size_t>( 2 * bases.size(), 7 ) ) {
        if( observation->GetMinerals() > 150 && observation->GetVespene() > 100 ) {
            TryBuildStructureRandom( ABILITY_ID::BUILD_FACTORY, UNIT_TYPEID::TERRAN_SCV );
        }
    }

    if( barracks.size() < barracks_count_target ) {
        if( observation->GetFoodWorkers() >= target_worker_count_ ) {
            TryBuildStructureRandom( ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV );
        }
    }

    if( !mech_build_ ) {
        if( CountUnitType( observation, UNIT_TYPEID::TERRAN_ENGINEERINGBAY ) < std::min<size_t>( bases.size(), 2 ) ) {
            if( observation->GetMinerals() > 150 && observation->GetVespene() > 100 ) {
                TryBuildStructureRandom( ABILITY_ID::BUILD_ENGINEERINGBAY, UNIT_TYPEID::TERRAN_SCV );
            }
        }
        if( !barracks.empty() && CountUnitType( observation, UNIT_TYPEID::TERRAN_GHOSTACADEMY ) < 1 ) {
            if( observation->GetMinerals() > 150 && observation->GetVespene() > 50 ) {
                TryBuildStructureRandom( ABILITY_ID::BUILD_GHOSTACADEMY, UNIT_TYPEID::TERRAN_SCV );
            }
        }
        if( !factorys.empty() && CountUnitType( observation, UNIT_TYPEID::TERRAN_FUSIONCORE ) < 1 ) {
            if( observation->GetMinerals() > 150 && observation->GetVespene() > 150 ) {
                TryBuildStructureRandom( ABILITY_ID::BUILD_FUSIONCORE, UNIT_TYPEID::TERRAN_SCV );
            }
        }
    }

    if( !barracks.empty() && CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) < armory_count_target ) {
        if( observation->GetMinerals() > 150 && observation->GetVespene() > 100 ) {
            TryBuildStructureRandom( ABILITY_ID::BUILD_ARMORY, UNIT_TYPEID::TERRAN_SCV );
        }
    }
}

bool NateBot::TryBuildAddOn( AbilityID ability_type_for_structure, Tag base_structure ) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    const Unit* unit = Observation()->GetUnit( base_structure );

    if( unit->build_progress != 1 ) {
        return false;
    }

    Point2D build_location = Point2D( unit->pos.x + rx * 15, unit->pos.y + ry * 15 );

    Units units = Observation()->GetUnits( Unit::Self, IsStructure( Observation() ) );

    if( Query()->Placement( ability_type_for_structure, unit->pos, unit ) ) {
        Actions()->UnitCommand( unit, ability_type_for_structure );
        return true;
    }

    float distance = std::numeric_limits<float>::max();
    for( const auto& u : units ) {
        float d = Distance2D( u->pos, build_location );
        if( d < distance ) {
            distance = d;
        }
    }
    if( distance < 6 ) {
        return false;
    }

    if( Query()->Placement( ability_type_for_structure, build_location, unit ) ) {
        Actions()->UnitCommand( unit, ability_type_for_structure, build_location );
        return true;
    }
    return false;

}

bool NateBot::TryBuildStructureRandom( AbilityID ability_type_for_structure, UnitTypeID unit_type ) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D( staging_location_.x + rx * 15, staging_location_.y + ry * 15 );

    Units units = Observation()->GetUnits( Unit::Self, IsStructure( Observation() ) );
    float distance = std::numeric_limits<float>::max();
    for( const auto& u : units ) {
        if( u->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED ) {
            continue;
        }
        float d = Distance2D( u->pos, build_location );
        if( d < distance ) {
            distance = d;
        }
    }
    if( distance < 6 ) {
        return false;
    }
    return TryBuildStructure( ability_type_for_structure, unit_type, build_location );
}

void NateBot::ManageUpgrades() {
    const ObservationInterface* observation = Observation();
    auto upgrades = observation->GetUpgrades();
    size_t base_count = observation->GetUnits( Unit::Alliance::Self, IsTownHall() ).size();


    if( upgrades.empty() ) {
        if( mech_build_ ) {
            TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
        }
        else {
            TryBuildUnit( ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB );
        }
    }
    else {
        for( const auto& upgrade : upgrades ) {
            if( mech_build_ ) {
                if( upgrade == UPGRADE_ID::TERRANSHIPWEAPONSLEVEL1 && base_count > 2 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANSHIPWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
                }
                else if( upgrade == UPGRADE_ID::TERRANVEHICLEWEAPONSLEVEL1 && base_count > 2 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
                }
                else if( upgrade == UPGRADE_ID::TERRANVEHICLEANDSHIPARMORSLEVEL1 && base_count > 2 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::TERRAN_ARMORY );
                }
                else if( upgrade == UPGRADE_ID::TERRANVEHICLEWEAPONSLEVEL2 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
                }
                else if( upgrade == UPGRADE_ID::TERRANVEHICLEANDSHIPARMORSLEVEL2 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::ZERG_SPIRE );
                }
                else {
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANSHIPWEAPONS, UNIT_TYPEID::TERRAN_ARMORY );
                    TryBuildUnit( ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING, UNIT_TYPEID::TERRAN_ARMORY );
                    TryBuildUnit( ABILITY_ID::RESEARCH_INFERNALPREIGNITER, UNIT_TYPEID::TERRAN_FACTORYTECHLAB );
                }
            }//Not mech build only
            else {
                if( CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) > 0 ) {
                    if( upgrade == UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1 && base_count > 2 ) {
                        TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                    }
                    else if( upgrade == UPGRADE_ID::TERRANINFANTRYARMORSLEVEL1 && base_count > 2 ) {
                        TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                    }
                    if( upgrade == UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL2 && base_count > 4 ) {
                        TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                    }
                    else if( upgrade == UPGRADE_ID::TERRANINFANTRYARMORSLEVEL2 && base_count > 4 ) {
                        TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                    }
                }
                TryBuildUnit( ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB );
                TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONS, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                TryBuildUnit( ABILITY_ID::RESEARCH_TERRANINFANTRYARMOR, UNIT_TYPEID::TERRAN_ENGINEERINGBAY );
                TryBuildUnit( ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB );
                TryBuildUnit( ABILITY_ID::RESEARCH_COMBATSHIELD, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB );
                TryBuildUnit( ABILITY_ID::RESEARCH_CONCUSSIVESHELLS, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB );
                TryBuildUnit( ABILITY_ID::RESEARCH_PERSONALCLOAKING, UNIT_TYPEID::TERRAN_GHOSTACADEMY );
                TryBuildUnit( ABILITY_ID::RESEARCH_BANSHEECLOAKINGFIELD, UNIT_TYPEID::TERRAN_STARPORTTECHLAB );
            }
        }
    }
}

void NateBot::ManageArmy() {

    const ObservationInterface* observation = Observation();

    Units enemy_units = observation->GetUnits( Unit::Alliance::Enemy );

    Units army = observation->GetUnits( Unit::Alliance::Self, IsArmy( observation ) );
    int wait_til_supply = 100;
    if( mech_build_ ) {
        wait_til_supply = 110;
    }

    Units nuke = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_NUKE ) );
    for( const auto& unit : army ) {
        if( enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply ) {
            switch( unit->unit_type.ToType() ) {
            case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                Actions()->UnitCommand( unit, ABILITY_ID::MORPH_UNSIEGE );
                break;
            }
            default:
                RetreatWithUnit( unit, staging_location_ );
                break;
            }
        }
        else if( !enemy_units.empty() ) {
            switch( unit->unit_type.ToType() ) {
            case UNIT_TYPEID::TERRAN_WIDOWMINE: {
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                    }
                }
                if( distance < 6 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::BURROWDOWN );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_MARINE: {
                if( stim_researched_ && !unit->orders.empty() ) {
                    if( unit->orders.front().ability_id == ABILITY_ID::ATTACK ) {
                        float distance = std::numeric_limits<float>::max();
                        for( const auto& u : enemy_units ) {
                            float d = Distance2D( u->pos, unit->pos );
                            if( d < distance ) {
                                distance = d;
                            }
                        }
                        bool has_stimmed = false;
                        for( const auto& buff : unit->buffs ) {
                            if( buff == BUFF_ID::STIMPACK ) {
                                has_stimmed = true;
                            }
                        }
                        if( distance < 6 && !has_stimmed ) {
                            Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_STIM );
                            break;
                        }
                    }

                }
                AttackWithUnit( unit, observation );
                break;
            }
            case UNIT_TYPEID::TERRAN_MARAUDER: {
                if( stim_researched_ && !unit->orders.empty() ) {
                    if( unit->orders.front().ability_id == ABILITY_ID::ATTACK ) {
                        float distance = std::numeric_limits<float>::max();
                        for( const auto& u : enemy_units ) {
                            float d = Distance2D( u->pos, unit->pos );
                            if( d < distance ) {
                                distance = d;
                            }
                        }
                        bool has_stimmed = false;
                        for( const auto& buff : unit->buffs ) {
                            if( buff == BUFF_ID::STIMPACK ) {
                                has_stimmed = true;
                            }
                        }
                        if( distance < 7 && !has_stimmed ) {
                            Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_STIM );
                            break;
                        }
                    }
                }
                AttackWithUnit( unit, observation );
                break;
            }
            case UNIT_TYPEID::TERRAN_GHOST: {
                float distance = std::numeric_limits<float>::max();
                const Unit* closest_unit;
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u;
                    }
                }
                if( ghost_cloak_researched_ ) {
                    if( distance < 7 && unit->energy > 50 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::BEHAVIOR_CLOAKON );
                        break;
                    }
                }
                if( nuke_built ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_NUKECALLDOWN, closest_unit->pos );
                }
                else if( unit->energy > 50 && !unit->orders.empty() ) {
                    if( unit->orders.front().ability_id == ABILITY_ID::ATTACK )
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_GHOSTSNIPE, unit );
                    break;
                }
                AttackWithUnit( unit, observation );
                break;
            }
            case UNIT_TYPEID::TERRAN_SIEGETANK: {
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                    }
                }
                if( distance < 11 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::MORPH_SIEGEMODE );
                }
                else {
                    AttackWithUnit( unit, observation );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                    }
                }
                if( distance > 13 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::MORPH_UNSIEGE );
                }
                else {
                    AttackWithUnit( unit, observation );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_MEDIVAC: {
                Units bio_units = observation->GetUnits( Unit::Self, IsUnits( bio_types ) );
                if( unit->orders.empty() ) {
                    for( const auto& bio_unit : bio_units ) {
                        if( bio_unit->health < bio_unit->health_max ) {
                            Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_HEAL, bio_unit );
                            break;
                        }
                    }
                    if( !bio_units.empty() ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, bio_units.front() );
                    }
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_VIKINGFIGHTER: {
                Units flying_units = observation->GetUnits( Unit::Enemy, IsFlying() );
                if( flying_units.empty() ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::MORPH_VIKINGASSAULTMODE );
                }
                else {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, flying_units.front() );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_VIKINGASSAULT: {
                Units flying_units = observation->GetUnits( Unit::Enemy, IsFlying() );
                if( !flying_units.empty() ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::MORPH_VIKINGFIGHTERMODE );
                }
                else {
                    AttackWithUnit( unit, observation );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_CYCLONE: {
                Units flying_units = observation->GetUnits( Unit::Enemy, IsFlying() );
                if( !flying_units.empty() && unit->orders.empty() ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_LOCKON, flying_units.front() );
                }
                else if( !flying_units.empty() && !unit->orders.empty() ) {
                    if( unit->orders.front().ability_id != ABILITY_ID::EFFECT_LOCKON ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_LOCKON, flying_units.front() );
                    }
                }
                else {
                    AttackWithUnit( unit, observation );
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_HELLION: {
                if( CountUnitType( observation, UNIT_TYPEID::TERRAN_ARMORY ) > 0 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::MORPH_HELLBAT );
                }
                AttackWithUnit( unit, observation );
                break;
            }
            case UNIT_TYPEID::TERRAN_BANSHEE: {
                if( banshee_cloak_researched_ ) {
                    float distance = std::numeric_limits<float>::max();
                    for( const auto& u : enemy_units ) {
                        float d = Distance2D( u->pos, unit->pos );
                        if( d < distance ) {
                            distance = d;
                        }
                    }
                    if( distance < 7 && unit->energy > 50 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::BEHAVIOR_CLOAKON );
                    }
                }
                AttackWithUnit( unit, observation );
                break;
            }
            case UNIT_TYPEID::TERRAN_RAVEN: {
                if( unit->energy > 125 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_HUNTERSEEKERMISSILE, enemy_units.front() );
                    break;
                }
                if( unit->orders.empty() ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, army.front()->pos );
                }
                break;
            }
            default: {
                AttackWithUnit( unit, observation );
            }
            }
        }
        else {
            switch( unit->unit_type.ToType() ) {
            case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                Actions()->UnitCommand( unit, ABILITY_ID::MORPH_UNSIEGE );
                break;
            }
            case UNIT_TYPEID::TERRAN_MEDIVAC: {
                Units bio_units = observation->GetUnits( Unit::Self, IsUnits( bio_types ) );
                if( unit->orders.empty() ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, bio_units.front()->pos );
                }
                break;
            }
            default:
                ScoutWithUnit( unit, observation );
                break;
            }
        }
    }
}

bool NateBot::TryBuildExpansionCom() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );
    //Don't have more active bases than we can provide workers for
    if( GetExpectedWorkers( UNIT_TYPEID::TERRAN_REFINERY ) > max_worker_count_ ) {
        return false;
    }
    // If we have extra workers around, try and build another Hatch.
    if( GetExpectedWorkers( UNIT_TYPEID::TERRAN_REFINERY ) < observation->GetFoodWorkers() - 10 ) {
        return TryExpand( ABILITY_ID::BUILD_COMMANDCENTER, UNIT_TYPEID::TERRAN_SCV );
    }
    //Only build another Hatch if we are floating extra minerals
    if( observation->GetMinerals() > std::min<size_t>( bases.size() * 400, 1200 ) ) {
        return TryExpand( ABILITY_ID::BUILD_COMMANDCENTER, UNIT_TYPEID::TERRAN_SCV );
    }
    return false;
}

bool NateBot::BuildRefinery() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );

    if( CountUnitType( observation, UNIT_TYPEID::TERRAN_REFINERY ) >= observation->GetUnits( Unit::Alliance::Self, IsTownHall() ).size() * 2 ) {
        return false;
    }

    for( const auto& base : bases ) {
        if( base->assigned_harvesters >= base->ideal_harvesters ) {
            if( base->build_progress == 1 ) {
                if( TryBuildGas( ABILITY_ID::BUILD_REFINERY, UNIT_TYPEID::TERRAN_SCV, base->pos ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

void NateBot::OnStep() {


    const ObservationInterface* observation = Observation();
    Units units = observation->GetUnits( Unit::Self, IsArmy( observation ) );
    Units nukes = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::TERRAN_NUKE ) );

    //Throttle some behavior that can wait to avoid duplicate orders.
    int frames_to_skip = 4;
    if( observation->GetFoodUsed() >= observation->GetFoodCap() ) {
        frames_to_skip = 6;
    }

    if( observation->GetGameLoop() % frames_to_skip != 0 ) {
        return;
    }

    if( !nuke_detected && nukes.empty() ) {
        ManageArmy();
    }
    else {
        if( nuke_detected_frame + 400 < observation->GetGameLoop() ) {
            nuke_detected = false;
        }
        for( const auto& unit : units ) {
            RetreatWithUnit( unit, startLocation_ );
        }
    }

    BuildOrder();

    ManageWorkers( UNIT_TYPEID::TERRAN_SCV, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY );

    ManageUpgrades();

    if( TryBuildSCV() )
        return;

    if( TryBuildSupplyDepot() )
        return;

    BuildArmy();

    if( BuildRefinery() ) {
        return;
    }

    if( TryBuildExpansionCom() ) {
        return;
    }
}

void NateBot::OnUnitIdle( const Unit* unit ) {
    switch( unit->unit_type.ToType() ) {
    case UNIT_TYPEID::TERRAN_SCV: {
        MineIdleWorkers( unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY );
        break;
    }
    default:
        break;
    }
}

void NateBot::OnUpgradeCompleted( UpgradeID upgrade ) {
    switch( upgrade.ToType() ) {
    case UPGRADE_ID::STIMPACK: {
        stim_researched_ = true;
    }
    case UPGRADE_ID::PERSONALCLOAKING: {
        ghost_cloak_researched_ = true;
    }
    case UPGRADE_ID::BANSHEECLOAK: {
        banshee_cloak_researched_ = true;
    }
    default:
        break;
    }
}



}
