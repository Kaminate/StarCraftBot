
#include "bot_goose.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>

#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"


namespace sc2 {


bool GooseBot::TryBuildDrone() {
    const ObservationInterface* observation = Observation();
    size_t larva_count = CountUnitType( observation, UNIT_TYPEID::ZERG_LARVA );
    Units bases = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );
    size_t worker_count = CountUnitType( observation, UNIT_TYPEID::ZERG_DRONE );
    Units eggs = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::ZERG_EGG ) );
    for( const auto& egg : eggs ) {
        if( !egg->orders.empty() ) {
            if( egg->orders.front().ability_id == ABILITY_ID::TRAIN_DRONE ) {
                worker_count++;
            }
        }
    }
    if( worker_count >= max_worker_count_ ) {
        return false;
    }

    if( worker_count > GetExpectedWorkers( UNIT_TYPEID::ZERG_EXTRACTOR ) ) {
        return false;
    }

    if( observation->GetFoodUsed() >= observation->GetFoodCap() ) {
        return false;
    }

    for( const auto& base : bases ) {
        //if there is a base with less than ideal workers
        if( base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1 ) {
            if( observation->GetMinerals() >= 50 && larva_count > 0 ) {
                return TryBuildUnit( ABILITY_ID::TRAIN_DRONE, UNIT_TYPEID::ZERG_LARVA );
            }
        }
    }
    return false;
}

bool GooseBot::TryBuildOnCreep( AbilityID ability_type_for_structure, UnitTypeID unit_type ) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    const ObservationInterface* observation = Observation();
    Point2D build_location = Point2D( startLocation_.x + rx * 15, startLocation_.y + ry * 15 );

    if( observation->HasCreep( build_location ) ) {
        return TryBuildStructure( ability_type_for_structure, unit_type, build_location );
    }
    return false;
}

void GooseBot::BuildOrder() {
    const ObservationInterface* observation = Observation();
    bool hive_tech = CountUnitType( observation, UNIT_TYPEID::ZERG_HIVE ) > 0;
    bool lair_tech = CountUnitType( observation, UNIT_TYPEID::ZERG_LAIR ) > 0 || hive_tech;
    size_t base_count = observation->GetUnits( Unit::Self, IsTownHall() ).size();
    size_t evolution_chanber_target = 1;
    size_t morphing_lair = 0;
    size_t morphing_hive = 0;
    Units hatcherys = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::ZERG_HATCHERY ) );
    Units lairs = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::ZERG_LAIR ) );
    for( const auto& hatchery : hatcherys ) {
        if( !hatchery->orders.empty() ) {
            if( hatchery->orders.front().ability_id == ABILITY_ID::MORPH_LAIR ) {
                ++morphing_lair;
            }
        }
    }
    for( const auto& lair : lairs ) {
        if( !lair->orders.empty() ) {
            if( lair->orders.front().ability_id == ABILITY_ID::MORPH_HIVE ) {
                ++morphing_hive;
            }
        }
    }

    if( !mutalisk_build_ ) {
        evolution_chanber_target++;
    }
    //Priority to spawning pool
    if( CountUnitType( observation, UNIT_TYPEID::ZERG_SPAWNINGPOOL ) < 1 ) {
        TryBuildOnCreep( ABILITY_ID::BUILD_SPAWNINGPOOL, UNIT_TYPEID::ZERG_DRONE );
    }
    else {
        if( base_count < 1 ) {
            TryBuildExpansionHatch();
            return;
        }

        if( CountUnitType( observation, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER ) < evolution_chanber_target ) {
            TryBuildOnCreep( ABILITY_ID::BUILD_EVOLUTIONCHAMBER, UNIT_TYPEID::ZERG_DRONE );
        }

        if( !mutalisk_build_ && CountUnitType( observation, UNIT_TYPEID::ZERG_ROACHWARREN ) < 1 ) {
            TryBuildOnCreep( ABILITY_ID::BUILD_ROACHWARREN, UNIT_TYPEID::ZERG_DRONE );
        }

        if( !lair_tech ) {
            if( CountUnitType( observation, UNIT_TYPEID::ZERG_HIVE ) + CountUnitType( observation, UNIT_TYPEID::ZERG_LAIR ) < 1 && CountUnitType( observation, UNIT_TYPEID::ZERG_QUEEN ) > 0 ) {
                TryBuildUnit( ABILITY_ID::MORPH_LAIR, UNIT_TYPEID::ZERG_HATCHERY );
            }
        }
        else {
            if( !mutalisk_build_ ) {
                if( CountUnitType( observation, UNIT_TYPEID::ZERG_HYDRALISKDEN ) + CountUnitType( observation, UNIT_TYPEID::ZERG_LURKERDENMP ) < 1 ) {
                    TryBuildOnCreep( ABILITY_ID::BUILD_HYDRALISKDEN, UNIT_TYPEID::ZERG_DRONE );
                }
                if( CountUnitType( observation, UNIT_TYPEID::ZERG_HYDRALISKDEN ) > 0 ) {
                    TryBuildUnit( ABILITY_ID::MORPH_LURKERDEN, UNIT_TYPEID::ZERG_HYDRALISKDEN );
                }
            }
            else {
                if( CountUnitType( observation, UNIT_TYPEID::ZERG_SPIRE ) + CountUnitType( observation, UNIT_TYPEID::ZERG_GREATERSPIRE ) < 1 ) {
                    TryBuildOnCreep( ABILITY_ID::BUILD_SPIRE, UNIT_TYPEID::ZERG_DRONE );
                }
            }

            if( base_count < 3 ) {
                TryBuildExpansionHatch();
                return;
            }
            if( CountUnitType( observation, UNIT_TYPEID::ZERG_INFESTATIONPIT ) > 0 && CountUnitType( observation, UNIT_TYPEID::ZERG_HIVE ) < 1 ) {
                TryBuildUnit( ABILITY_ID::MORPH_HIVE, UNIT_TYPEID::ZERG_LAIR );
                return;
            }

            if( CountUnitType( observation, UNIT_TYPEID::ZERG_BANELINGNEST ) < 1 ) {
                TryBuildOnCreep( ABILITY_ID::BUILD_BANELINGNEST, UNIT_TYPEID::ZERG_DRONE );
            }

            if( observation->GetUnits( Unit::Self, IsTownHall() ).size() > 2 ) {
                if( CountUnitType( observation, UNIT_TYPEID::ZERG_INFESTATIONPIT ) < 1 ) {
                    TryBuildOnCreep( ABILITY_ID::BUILD_INFESTATIONPIT, UNIT_TYPEID::ZERG_DRONE );
                }
            }

        }

        if( hive_tech ) {

            if( !mutalisk_build_ && CountUnitType( observation, UNIT_TYPEID::ZERG_ULTRALISKCAVERN ) < 1 ) {
                TryBuildOnCreep( ABILITY_ID::BUILD_ULTRALISKCAVERN, UNIT_TYPEID::ZERG_DRONE );
            }

            if( CountUnitType( observation, UNIT_TYPEID::ZERG_GREATERSPIRE ) < 1 ) {
                TryBuildUnit( ABILITY_ID::MORPH_GREATERSPIRE, UNIT_TYPEID::ZERG_SPIRE );
            }
        }

    }
}

void GooseBot::ManageArmy() {
    const ObservationInterface* observation = Observation();

    Units enemy_units = observation->GetUnits( Unit::Alliance::Enemy );
    Units army = observation->GetUnits( Unit::Alliance::Self, IsArmy( observation ) );
    int wait_til_supply = 100;

    if( enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply ) {
        for( const auto& unit : army ) {
            switch( unit->unit_type.ToType() ) {
            case( UNIT_TYPEID::ZERG_LURKERMPBURROWED ): {
                Actions()->UnitCommand( unit, ABILITY_ID::BURROWUP );
            }
            default:
                RetreatWithUnit( unit, staging_location_ );
                break;
            }
        }
    }
    else if( !enemy_units.empty() ) {
        for( const auto& unit : army ) {
            switch( unit->unit_type.ToType() ) {
            case( UNIT_TYPEID::ZERG_CORRUPTOR ): {
                Tag closest_unit;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u->tag;
                    }
                }
                const Unit* enemy_unit = observation->GetUnit( closest_unit );

                auto attributes = observation->GetUnitTypeData().at( enemy_unit->unit_type ).attributes;
                for( const auto& attribute : attributes ) {
                    if( attribute == Attribute::Structure ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_CAUSTICSPRAY, enemy_unit );
                    }
                }
                if( !unit->orders.empty() ) {
                    if( unit->orders.front().ability_id == ABILITY_ID::EFFECT_CAUSTICSPRAY ) {
                        break;
                    }
                }
                AttackWithUnit( unit, observation );
                break;
            }
            case( UNIT_TYPEID::ZERG_OVERSEER ): {
                Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, enemy_units.front() );
                break;
            }
            case( UNIT_TYPEID::ZERG_RAVAGER ): {
                Point2D closest_unit;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u->pos;
                    }
                }
                Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_CORROSIVEBILE, closest_unit );
                AttackWithUnit( unit, observation );
            }
            case( UNIT_TYPEID::ZERG_LURKERMP ): {
                Point2D closest_unit;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u->pos;
                    }
                }
                if( distance < 7 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::BURROWDOWN );
                }
                else {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, closest_unit );
                }
                break;
            }
            case( UNIT_TYPEID::ZERG_LURKERMPBURROWED ): {
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                    }
                }
                if( distance > 9 ) {
                    Actions()->UnitCommand( unit, ABILITY_ID::BURROWUP );
                }
                break;
            }
            case( UNIT_TYPEID::ZERG_SWARMHOSTMP ): {
                Point2D closest_unit;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u->pos;
                    }
                }
                if( distance < 15 ) {
                    const auto abilities = Query()->GetAbilitiesForUnit( unit ).abilities;
                    bool ability_available = false;
                    for( const auto& ability : abilities ) {
                        if( ability.ability_id == ABILITY_ID::EFFECT_SPAWNLOCUSTS ) {
                            ability_available = true;
                        }
                    }
                    if( ability_available ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_SPAWNLOCUSTS, closest_unit );
                    }
                    else {
                        RetreatWithUnit( unit, staging_location_ );
                    }
                }
                else {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, closest_unit );
                }
                break;
            }
            case( UNIT_TYPEID::ZERG_INFESTOR ): {
                Point2D closest_unit;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u->pos;
                    }
                }
                if( distance < 9 ) {
                    const auto abilities = Query()->GetAbilitiesForUnit( unit ).abilities;
                    if( unit->energy > 75 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_FUNGALGROWTH, closest_unit );
                    }
                    else {
                        RetreatWithUnit( unit, staging_location_ );
                    }
                }
                else {
                    Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, closest_unit );
                }
                break;
            }
            case( UNIT_TYPEID::ZERG_VIPER ): {
                const Unit* closest_unit;
                bool is_flying = false;
                float distance = std::numeric_limits<float>::max();
                for( const auto& u : enemy_units ) {
                    auto attributes = observation->GetUnitTypeData().at( u->unit_type ).attributes;
                    bool is_structure = false;
                    for( const auto& attribute : attributes ) {
                        if( attribute == Attribute::Structure ) {
                            is_structure = true;
                        }
                    }
                    if( is_structure ) {
                        continue;
                    }
                    float d = Distance2D( u->pos, unit->pos );
                    if( d < distance ) {
                        distance = d;
                        closest_unit = u;
                        is_flying = u->is_flying;
                    }
                }
                if( distance < 10 ) {
                    if( is_flying && unit->energy > 124 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_PARASITICBOMB, closest_unit );
                    }
                    else if( !is_flying && unit->energy > 100 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_BLINDINGCLOUD, closest_unit );
                    }
                    else {
                        RetreatWithUnit( unit, startLocation_ );
                    }

                }
                else {
                    if( unit->energy > 124 ) {
                        Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, enemy_units.front() );
                    }
                    else {
                        if( !unit->orders.empty() ) {
                            if( unit->orders.front().ability_id != ABILITY_ID::EFFECT_VIPERCONSUME ) {
                                Units extractors = observation->GetUnits( Unit::Self, IsUnit( UNIT_TYPEID::ZERG_EXTRACTOR ) );
                                for( const auto& extractor : extractors ) {
                                    if( extractor->health > 200 ) {
                                        Actions()->UnitCommand( unit, ABILITY_ID::EFFECT_VIPERCONSUME, extractor );
                                    }
                                    else {
                                        continue;
                                    }
                                }
                            }
                            else {
                                if( observation->GetUnit( unit->orders.front().target_unit_tag )->health < 100 ) {
                                    Actions()->UnitCommand( unit, ABILITY_ID::STOP );
                                }
                            }
                        }
                        else {
                            Actions()->UnitCommand( unit, ABILITY_ID::ATTACK, closest_unit );
                        }
                    }
                }
                break;
            }
            default: {
                AttackWithUnit( unit, observation );
            }
            }
        }
    }
    else {
        for( const auto& unit : army ) {
            switch( unit->unit_type.ToType() ) {
            case( UNIT_TYPEID::ZERG_LURKERMPBURROWED ): {
                Actions()->UnitCommand( unit, ABILITY_ID::BURROWUP );
            }
            default:
                ScoutWithUnit( unit, observation );
                break;
            }
        }
    }
}

void GooseBot::BuildArmy() {
    const ObservationInterface* observation = Observation();
    size_t larva_count = CountUnitType( observation, UNIT_TYPEID::ZERG_LARVA );
    size_t base_count = observation->GetUnits( Unit::Self, IsTownHall() ).size();

    size_t queen_Count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_QUEEN, UNIT_TYPEID::ZERG_HATCHERY, ABILITY_ID::TRAIN_QUEEN );
    queen_Count += CountUnitTypeBuilding( observation, UNIT_TYPEID::ZERG_LAIR, ABILITY_ID::TRAIN_QUEEN );
    queen_Count += CountUnitTypeBuilding( observation, UNIT_TYPEID::ZERG_HIVE, ABILITY_ID::TRAIN_QUEEN );
    size_t hydralisk_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_HYDRALISK, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_HYDRALISK );
    size_t roach_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_ROACH, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_ROACH );
    size_t corruptor_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_CORRUPTOR, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_CORRUPTOR );
    size_t swarmhost_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_SWARMHOSTMP, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_SWARMHOST );
    size_t viper_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_VIPER, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_VIPER );
    size_t ultralisk_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_ULTRALISK, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_ULTRALISK );
    size_t infestor_count = CountUnitTypeTotal( observation, UNIT_TYPEID::ZERG_INFESTOR, UNIT_TYPEID::ZERG_EGG, ABILITY_ID::TRAIN_INFESTOR );

    if( queen_Count < base_count && CountUnitType( observation, UNIT_TYPEID::ZERG_SPAWNINGPOOL ) > 0 ) {
        if( observation->GetMinerals() >= 150 ) {
            if( !TryBuildUnit( ABILITY_ID::TRAIN_QUEEN, UNIT_TYPEID::ZERG_HATCHERY ) ) {
                if( !TryBuildUnit( ABILITY_ID::TRAIN_QUEEN, UNIT_TYPEID::ZERG_LAIR ) ) {
                    TryBuildUnit( ABILITY_ID::TRAIN_QUEEN, UNIT_TYPEID::ZERG_HIVE );
                }
            }
        }
    }
    if( CountUnitType( observation, UNIT_TYPEID::ZERG_OVERSEER ) + CountUnitType( observation, UNIT_TYPEID::ZERG_OVERLORDCOCOON ) < 1 ) {
        TryBuildUnit( ABILITY_ID::MORPH_OVERSEER, UNIT_TYPEID::ZERG_OVERLORD );
    }

    if( larva_count > 0 ) {
        if( viper_count < 2 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_VIPER, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
        }
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_ULTRALISKCAVERN ) > 0 && !mutalisk_build_ && ultralisk_count < 4 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_ULTRALISK, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
            //Try to build at least one ultralisk
            if( ultralisk_count < 1 ) {
                return;
            }
        }
    }

    if( !mutalisk_build_ ) {
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_RAVAGER ) + CountUnitType( observation, UNIT_TYPEID::ZERG_RAVAGERCOCOON ) < 3 ) {
            TryBuildUnit( ABILITY_ID::MORPH_RAVAGER, UNIT_TYPEID::ZERG_ROACH );
        }
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_LURKERMP ) + CountUnitType( observation, UNIT_TYPEID::ZERG_LURKERMPEGG ) + CountUnitType( observation, UNIT_TYPEID::ZERG_LURKERMPBURROWED ) < 6 ) {
            TryBuildUnit( ABILITY_ID::MORPH_LURKER, UNIT_TYPEID::ZERG_HYDRALISK );
        }
        if( larva_count > 0 ) {
            if( CountUnitType( observation, UNIT_TYPEID::ZERG_HYDRALISKDEN ) + CountUnitType( observation, UNIT_TYPEID::ZERG_LURKERDENMP ) > 0 && hydralisk_count < 15 ) {
                if( TryBuildUnit( ABILITY_ID::TRAIN_HYDRALISK, UNIT_TYPEID::ZERG_LARVA ) ) {
                    --larva_count;
                }
            }
        }

        if( larva_count > 0 ) {
            if( swarmhost_count < 1 ) {
                if( TryBuildUnit( ABILITY_ID::TRAIN_SWARMHOST, UNIT_TYPEID::ZERG_LARVA ) ) {
                    --larva_count;
                }
            }
        }

        if( larva_count > 0 ) {
            if( infestor_count < 1 ) {
                if( TryBuildUnit( ABILITY_ID::TRAIN_INFESTOR, UNIT_TYPEID::ZERG_LARVA ) ) {
                    --larva_count;
                }
            }
        }
    }
    else {
        if( larva_count > 0 ) {
            if( CountUnitType( observation, UNIT_TYPEID::ZERG_SPIRE ) + CountUnitType( observation, UNIT_TYPEID::ZERG_GREATERSPIRE ) > 0 ) {
                if( corruptor_count < 7 ) {
                    if( TryBuildUnit( ABILITY_ID::TRAIN_CORRUPTOR, UNIT_TYPEID::ZERG_LARVA ) ) {
                        --larva_count;
                    }
                }
                if( TryBuildUnit( ABILITY_ID::TRAIN_MUTALISK, UNIT_TYPEID::ZERG_LARVA ) ) {
                    --larva_count;
                }
            }
        }
    }

    if( larva_count > 0 ) {
        if( ++viper_count < 1 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_VIPER, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
        }
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_ULTRALISKCAVERN ) > 0 && !mutalisk_build_ && ultralisk_count < 4 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_ULTRALISK, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
            //Try to build at least one ultralisk
            if( ultralisk_count < 1 ) {
                return;
            }
        }
    }
    if( CountUnitType( observation, UNIT_TYPEID::ZERG_GREATERSPIRE ) > 0 ) {
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_BROODLORD ) + CountUnitType( observation, UNIT_TYPEID::ZERG_BROODLORDCOCOON ) < 4 )
            TryBuildUnit( ABILITY_ID::MORPH_BROODLORD, UNIT_TYPEID::ZERG_CORRUPTOR );
    }

    if( !mutalisk_build_ && larva_count > 0 ) {
        if( roach_count < 10 && CountUnitType( observation, UNIT_TYPEID::ZERG_ROACHWARREN ) > 0 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_ROACH, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
        }
    }
    size_t baneling_count = CountUnitType( observation, UNIT_TYPEID::ZERG_BANELING ) + CountUnitType( observation, UNIT_TYPEID::ZERG_BANELINGCOCOON );
    if( larva_count > 0 ) {
        if( CountUnitType( observation, UNIT_TYPEID::ZERG_ZERGLING ) < 20 && CountUnitType( observation, UNIT_TYPEID::ZERG_SPAWNINGPOOL ) > 0 ) {
            if( TryBuildUnit( ABILITY_ID::TRAIN_ZERGLING, UNIT_TYPEID::ZERG_LARVA ) ) {
                --larva_count;
            }
        }
    }

    size_t baneling_target = 5;
    if( mutalisk_build_ ) {
        baneling_target = baneling_target * 2;
    }
    if( baneling_count < baneling_target ) {
        TryBuildUnit( ABILITY_ID::TRAIN_BANELING, UNIT_TYPEID::ZERG_ZERGLING );
    }

}

void GooseBot::ManageUpgrades() {
    const ObservationInterface* observation = Observation();
    auto upgrades = observation->GetUpgrades();
    size_t base_count = observation->GetUnits( Unit::Alliance::Self, IsTownHall() ).size();
    bool hive_tech = CountUnitType( observation, UNIT_TYPEID::ZERG_HIVE ) > 0;
    bool lair_tech = CountUnitType( observation, UNIT_TYPEID::ZERG_LAIR ) > 0 || hive_tech;

    if( upgrades.empty() ) {
        TryBuildUnit( ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST, UNIT_TYPEID::ZERG_SPAWNINGPOOL );
    }
    else {
        for( const auto& upgrade : upgrades ) {
            if( mutalisk_build_ ) {
                if( upgrade == UPGRADE_ID::ZERGFLYERWEAPONSLEVEL1 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_GREATERSPIRE );
                }
                else if( upgrade == UPGRADE_ID::ZERGFLYERARMORSLEVEL1 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_GREATERSPIRE );
                }
                else if( upgrade == UPGRADE_ID::ZERGFLYERWEAPONSLEVEL2 && base_count > 4 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_GREATERSPIRE );
                }
                else if( upgrade == UPGRADE_ID::ZERGFLYERARMORSLEVEL2 && base_count > 4 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_GREATERSPIRE );
                }
                else {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERATTACK, UNIT_TYPEID::ZERG_GREATERSPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_SPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGFLYERARMOR, UNIT_TYPEID::ZERG_GREATERSPIRE );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGGROUNDARMOR, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGMELEEWEAPONS, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                }
            }//Not Mutalisk build only
            else {
                if( upgrade == UPGRADE_ID::ZERGMISSILEWEAPONSLEVEL1 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGMISSILEWEAPONS, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                }
                else if( upgrade == UPGRADE_ID::ZERGMISSILEWEAPONSLEVEL2 && base_count > 4 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGMISSILEWEAPONS, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                }
                if( upgrade == UPGRADE_ID::ZERGGROUNDARMORSLEVEL1 && base_count > 3 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGGROUNDARMOR, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                }
                else if( upgrade == UPGRADE_ID::ZERGGROUNDARMORSLEVEL2 && base_count > 4 ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGGROUNDARMOR, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                }

                if( hive_tech ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_CHITINOUSPLATING, UNIT_TYPEID::ZERG_ULTRALISKCAVERN );
                }

                if( lair_tech ) {
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGMISSILEWEAPONS, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                    TryBuildUnit( ABILITY_ID::RESEARCH_ZERGGROUNDARMOR, UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER );
                    TryBuildUnit( ABILITY_ID::RESEARCH_CENTRIFUGALHOOKS, UNIT_TYPEID::ZERG_BANELINGNEST );
                    TryBuildUnit( ABILITY_ID::RESEARCH_MUSCULARAUGMENTS, UNIT_TYPEID::ZERG_HYDRALISKDEN );
                    TryBuildUnit( ABILITY_ID::RESEARCH_MUSCULARAUGMENTS, UNIT_TYPEID::ZERG_LURKERDENMP );
                    TryBuildUnit( ABILITY_ID::RESEARCH_GLIALREGENERATION, UNIT_TYPEID::ZERG_ROACHWARREN );
                }
            }
            //research regardless of build
            if( hive_tech ) {
                TryBuildUnit( ABILITY_ID::RESEARCH_ZERGLINGADRENALGLANDS, UNIT_TYPEID::ZERG_SPAWNINGPOOL );
            }
            else if( lair_tech ) {
                TryBuildUnit( ABILITY_ID::RESEARCH_PNEUMATIZEDCARAPACE, UNIT_TYPEID::ZERG_HIVE );
                TryBuildUnit( ABILITY_ID::RESEARCH_PNEUMATIZEDCARAPACE, UNIT_TYPEID::ZERG_LAIR );
                TryBuildUnit( ABILITY_ID::RESEARCH_PNEUMATIZEDCARAPACE, UNIT_TYPEID::ZERG_HATCHERY );
            }
        }
    }
}

bool GooseBot::TryBuildOverlord() {
    const ObservationInterface* observation = Observation();
    size_t larva_count = CountUnitType( observation, UNIT_TYPEID::ZERG_LARVA );
    if( observation->GetFoodCap() == 200 ) {
        return false;
    }
    if( observation->GetFoodUsed() < observation->GetFoodCap() - 4 ) {
        return false;
    }
    if( observation->GetMinerals() < 100 ) {
        return false;
    }

    //Slow overlord development in the beginning
    if( observation->GetFoodUsed() < 30 ) {
        Units units = observation->GetUnits( Unit::Alliance::Self, IsUnit( UNIT_TYPEID::ZERG_EGG ) );
        for( const auto& unit : units ) {
            if( unit->orders.empty() ) {
                return false;
            }
            if( unit->orders.front().ability_id == ABILITY_ID::TRAIN_OVERLORD ) {
                return false;
            }
        }
    }
    if( larva_count > 0 ) {
        return TryBuildUnit( ABILITY_ID::TRAIN_OVERLORD, UNIT_TYPEID::ZERG_LARVA );
    }
    return false;
}

void GooseBot::TryInjectLarva() {
    const ObservationInterface* observation = Observation();
    Units queens = observation->GetUnits( Unit::Alliance::Self, IsUnit( UNIT_TYPEID::ZERG_QUEEN ) );
    Units hatcheries = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );

    //if we don't have queens or hatcheries don't do anything
    if( queens.empty() || hatcheries.empty() )
        return;

    for( size_t i = 0; i < queens.size(); ++i ) {
        for( size_t j = 0; j < hatcheries.size(); ++j ) {

            //if hatchery isn't complete ignore it
            if( hatcheries.at( j )->build_progress != 1 ) {
                continue;
            }
            else {

                //Inject larva and move onto next available queen
                if( i < queens.size() ) {
                    if( queens.at( i )->energy >= 25 && queens.at( i )->orders.empty() ) {
                        Actions()->UnitCommand( queens.at( i ), ABILITY_ID::EFFECT_INJECTLARVA, hatcheries.at( j ) );
                    }
                    ++i;
                }
            }
        }
    }
}

bool GooseBot::TryBuildExpansionHatch() {
    const ObservationInterface* observation = Observation();

    //Don't have more active bases than we can provide workers for
    if( GetExpectedWorkers( UNIT_TYPEID::ZERG_EXTRACTOR ) > max_worker_count_ ) {
        return false;
    }
    // If we have extra workers around, try and build another Hatch.
    if( GetExpectedWorkers( UNIT_TYPEID::ZERG_EXTRACTOR ) < observation->GetFoodWorkers() - 10 ) {
        return TryExpand( ABILITY_ID::BUILD_HATCHERY, UNIT_TYPEID::ZERG_DRONE );
    }
    //Only build another Hatch if we are floating extra minerals
    if( observation->GetMinerals() > std::min<size_t>( ( CountUnitType( observation, UNIT_TYPEID::ZERG_HATCHERY ) * 300 ), 1200 ) ) {
        return TryExpand( ABILITY_ID::BUILD_HATCHERY, UNIT_TYPEID::ZERG_DRONE );
    }
    return false;
}

bool GooseBot::BuildExtractor() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );

    if( CountUnitType( observation, UNIT_TYPEID::ZERG_EXTRACTOR ) >= observation->GetUnits( Unit::Alliance::Self, IsTownHall() ).size() * 2 ) {
        return false;
    }

    for( const auto& base : bases ) {
        if( base->assigned_harvesters >= base->ideal_harvesters ) {
            if( base->build_progress == 1 ) {
                if( TryBuildGas( ABILITY_ID::BUILD_EXTRACTOR, UNIT_TYPEID::ZERG_DRONE, base->pos ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

void GooseBot::OnStep() {

    const ObservationInterface* observation = Observation();
    Units base = observation->GetUnits( Unit::Alliance::Self, IsTownHall() );

    //Throttle some behavior that can wait to avoid duplicate orders.
    int frames_to_skip = 4;
    if( observation->GetFoodUsed() >= observation->GetFoodCap() ) {
        frames_to_skip = 6;
    }

    if( observation->GetGameLoop() % frames_to_skip != 0 ) {
        return;
    }

    if( !nuke_detected ) {
        ManageArmy();
    }
    else {
        if( nuke_detected_frame + 400 < observation->GetGameLoop() ) {
            nuke_detected = false;
        }
        Units units = observation->GetUnits( Unit::Self, IsArmy( observation ) );
        for( const auto& unit : units ) {
            RetreatWithUnit( unit, startLocation_ );
        }
    }

    BuildOrder();

    TryInjectLarva();

    ManageWorkers( UNIT_TYPEID::ZERG_DRONE, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::ZERG_EXTRACTOR );

    ManageUpgrades();

    if( TryBuildDrone() )
        return;

    if( TryBuildOverlord() )
        return;

    if( observation->GetFoodArmy() < base.size() * 25 ) {
        BuildArmy();
    }

    if( BuildExtractor() ) {
        return;
    }

    if( TryBuildExpansionHatch() ) {
        return;
    }
}

void GooseBot::OnUnitIdle( const Unit* unit ) {
    switch( unit->unit_type.ToType() ) {
    case UNIT_TYPEID::ZERG_DRONE: {
        MineIdleWorkers( unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::ZERG_EXTRACTOR );
        break;
    }
    default:
        break;
    }
}



}
