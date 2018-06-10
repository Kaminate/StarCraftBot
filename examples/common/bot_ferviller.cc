
#include "bot_ferviller.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>

#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"


namespace sc2 {

//Manages attack and retreat patterns, as well as unit micro
void FervillerBot::ManageArmy() {
    const ObservationInterface* observation = Observation();
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
    Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
    int wait_til_supply = 100;

    //There are no enemies yet, and we don't have a big army
    if (enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply) {
        for (const auto& unit : army) {
            RetreatWithUnit(unit, staging_location_);
        }
    }
    else if(!enemy_units.empty()){
        for (const auto& unit : army) {
            AttackWithUnit(unit, observation);
            switch (unit->unit_type.ToType()) {
                //Stalker blink micro, blinks back towards your base
                case(UNIT_TYPEID::PROTOSS_OBSERVER): {
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
                    break;
                }
                case(UNIT_TYPEID::PROTOSS_STALKER): {
                    if (blink_reasearched_) {
                        /*const Unit* old_unit = observation->GetPreviousUnit(unit.tag);
                        const Unit* target_unit = observation->GetUnit(unit.engaged_target_tag);
                        if (old_unit == nullptr) {
                            break;
                        }
                        Point2D blink_location = startLocation_;
                        if (old_unit->shield > 0 && unit.shield < 1) {
                            if (!unit.orders.empty()) {
                                if (target_unit != nullptr) {
                                    Vector2D diff = unit.pos - target_unit->pos;
                                    Normalize2D(diff);
                                    blink_location = unit.pos + diff * 7.0f;
                                }
                                else {
                                    Vector2D diff = unit.pos - startLocation_;
                                    Normalize2D(diff);
                                    blink_location = unit.pos - diff * 7.0f;
                                }
                                Actions()->UnitCommand(unit.tag, ABILITY_ID::EFFECT_BLINK, blink_location);
                            }
                        }*/
                    }
                    break;
                }
                //Turns on guardian shell when close to an enemy
                case (UNIT_TYPEID::PROTOSS_SENTRY): {
                    if (!unit->orders.empty()) {
                        if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                            float distance = std::numeric_limits<float>::max();
                            for (const auto& u : enemy_units) {
                                 float d = Distance2D(u->pos, unit->pos);
                                 if (d < distance) {
                                     distance = d;
                                 }
                            }
                            if (distance < 6 && unit->energy >= 75) {
                                Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_GUARDIANSHIELD);
                            }
                        }
                    }
                    break;
                }
                //Turns on Damage boost when close to an enemy
                case (UNIT_TYPEID::PROTOSS_VOIDRAY): {
                    if (!unit->orders.empty()) {
                        if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                            float distance = std::numeric_limits<float>::max();
                            for (const auto& u : enemy_units) {
                                float d = Distance2D(u->pos, unit->pos);
                                if (d < distance) {
                                    distance = d;
                                }
                            }
                            if (distance < 8) {
                                Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT);
                            }
                        }
                    }
                    break;
                }
                //Turns on oracle weapon when close to an enemy
                case (UNIT_TYPEID::PROTOSS_ORACLE): {
                    if (!unit->orders.empty()) {
                         float distance = std::numeric_limits<float>::max();
                         for (const auto& u : enemy_units) {
                             float d = Distance2D(u->pos, unit->pos);
                             if (d < distance) {
                                 distance = d;
                             }
                         }
                         if (distance < 6 && unit->energy >= 25) {
                             Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
                         }
                    }
                    break;
                }
                //fires a disruptor nova when in range
                case (UNIT_TYPEID::PROTOSS_DISRUPTOR): {
                    float distance = std::numeric_limits<float>::max();
                    Point2D closest_unit;
                    for (const auto& u : enemy_units) {
                        float d = Distance2D(u->pos, unit->pos);
                        if (d < distance) {
                            distance = d;
                            closest_unit = u->pos;
                        }
                    }
                    if (distance < 7) {
                        Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_PURIFICATIONNOVA, closest_unit);
                    }
                    else {
                        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, closest_unit);
                    }
                    break;
                }
                //controls disruptor novas.
                case (UNIT_TYPEID::PROTOSS_DISRUPTORPHASED): {
                            float distance = std::numeric_limits<float>::max();
                            Point2D closest_unit;
                            for (const auto& u : enemy_units) {
                                if (u->is_flying) {
                                    continue;
                                }
                                float d = DistanceSquared2D(u->pos, unit->pos);
                                if (d < distance) {
                                    distance = d;
                                    closest_unit = u->pos;
                                }
                            }
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, closest_unit);
                    break;
                }
                default:
                    break;
            }
        }
    }
    else {
        for (const auto& unit : army) {
            ScoutWithUnit(unit, observation);
        }
    }
}

//Manages when to build and how many to build of units
bool FervillerBot::TryBuildArmy() {
    const ObservationInterface* observation = Observation();
    if (observation->GetFoodWorkers() <= target_worker_count_) {
        return false;
    }
    //Until we have 2 bases, hold off on building too many units
    if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) < 2 && observation->GetFoodArmy() > 10) {
        return false;
    }

    //If we have a decent army already, try hold until we expand again
    if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) < 3 && observation->GetFoodArmy() > 40) {
        return false;
    }
    size_t mothership_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE);
    mothership_count += CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIP);

    if (observation->GetFoodWorkers() > target_worker_count_ && mothership_count < 1) {
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE) > 0) {
            TryBuildUnit(ABILITY_ID::TRAIN_MOTHERSHIPCORE, UNIT_TYPEID::PROTOSS_NEXUS);
        }
    }
    size_t colossus_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_COLOSSUS);
    size_t carrier_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER);
    Units templar = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_HIGHTEMPLAR));
    if (templar.size() > 1) {
        Units templar_merge;
        for (int i = 0; i < 2; ++i) {
            templar_merge.push_back(templar.at(i));
        }
        Actions()->UnitCommand(templar_merge, ABILITY_ID::MORPH_ARCHON);
    }

    if (air_build_) {
        //If we have a fleet beacon, and haven't hit our carrier count, build more carriers
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON) > 0 && carrier_count < max_colossus_count_) {
            //After the first carrier try and make a Mothership
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIP) < 1 && mothership_count > 0) {
                if (TryBuildUnit(ABILITY_ID::MORPH_MOTHERSHIP, UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE)) {
                    return true;
                }
                return false;
            }

            if (observation->GetMinerals() > 350 && observation->GetVespene() > 250) {
                if (TryBuildUnit(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE)) {
                    return true;
                }
            }
            else if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPEST) < 1) {
                TryBuildUnit(ABILITY_ID::TRAIN_TEMPEST, UNIT_TYPEID::PROTOSS_STARGATE);
            }
            else if (carrier_count < 1){ //Try to build at least 1
                return false;
            }
        }
        else {
            // If we can't build Carrier, try to build voidrays
            if (observation->GetMinerals() > 250 && observation->GetVespene() > 150) {
                TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE);

            }
            //Have at least 1 void ray before we build the other units
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_VOIDRAY) > 0) {
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ORACLE) < 1) {
                    TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE);
                }
                else if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOENIX) < 2) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PHOENIX, UNIT_TYPEID::PROTOSS_STARGATE);
                }
            }
        }
    }
    else {
        //If we have a robotics bay, and haven't hit our colossus count, build more colossus
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_OBSERVER) < 1) {
            TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY) > 0 && colossus_count < max_colossus_count_) {
            if (observation->GetMinerals() > 300 && observation->GetVespene() > 200) {
                if (TryBuildUnit(ABILITY_ID::TRAIN_COLOSSUS, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
                    return true;
                }
            }
            else if (CountUnitTypeTotal(observation, UNIT_TYPEID::PROTOSS_DISRUPTOR, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, ABILITY_ID::TRAIN_DISRUPTOR) < 2) {
                TryBuildUnit(ABILITY_ID::TRAIN_DISRUPTOR, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
            }
        }
        else {
            // If we can't build Colossus, try to build immortals
            if (observation->GetMinerals() > 250 && observation->GetVespene() > 100) {
                if (TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
                    return true;
                }
            }
        }
    }

    if (warpgate_reasearched_ &&  CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE) > 0) {
        if (observation->GetMinerals() > 1000 && observation->GetVespene() < 200) {
            return TryWarpInUnit(ABILITY_ID::TRAINWARP_ZEALOT);
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER) > max_stalker_count_){
            return false;
        }

        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT) > max_stalker_count_) {
            return false;
        }

        if (!air_build_) {
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_SENTRY) < max_sentry_count_) {
                return TryWarpInUnit(ABILITY_ID::TRAINWARP_SENTRY);
            }

            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_HIGHTEMPLAR) < 2 && CountUnitType(observation, UNIT_TYPEID::PROTOSS_ARCHON) < 2) {
                return TryWarpInUnit(ABILITY_ID::TRAINWARP_HIGHTEMPLAR);
            }
        }
        //build adepts until we have robotics facility, then switch to stalkers.
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY) > 0) {
            return TryWarpInUnit(ABILITY_ID::TRAINWARP_STALKER);
        }
        else {
            return TryWarpInUnit(ABILITY_ID::TRAINWARP_ADEPT);
        }
    }
    else {
        //Train Adepts if we have a core otherwise build Zealots
        if (observation->GetMinerals() > 120 && observation->GetVespene() > 25) {
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE) > 0) {
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        else if (observation->GetMinerals() > 100) {
            return TryBuildUnit(ABILITY_ID::TRAIN_ZEALOT, UNIT_TYPEID::PROTOSS_GATEWAY);
        }
        return false;
    }
}

//Manages when to build your buildings
void FervillerBot::BuildOrder() {
    const ObservationInterface* observation = Observation();
    size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation,UNIT_TYPEID::PROTOSS_WARPGATE);
    size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
    size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
    size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    size_t templar_archive_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);
    size_t base_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS);
    size_t robotics_facility_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    size_t robotics_bay_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
    size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
    size_t fleet_beacon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON);

    // 3 Gateway per expansion
    if (gateway_count < std::min<size_t>(2 * base_count, 7)) {

        //If we have 1 gateway, prioritize building CyberCore
        if (cybernetics_count < 1 && gateway_count > 0) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
            return;
        }
        else {
            //If we have 1 gateway Prioritize getting another expansion out before building more gateways
            if (base_count < 2 && gateway_count > 0) {
                TryBuildExpansionNexus();
                return;
            }

            if (observation->GetFoodWorkers() >= target_worker_count_  && observation->GetMinerals() > 150 + (100 * gateway_count)) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
            }
        }
    }

    if (cybernetics_count > 0 && forge_count < 2) {
        TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_PROBE);
    }

    //go stargate or robo depending on build
    if (air_build_) {
        if (gateway_count > 1 && cybernetics_count > 0) {
            if (stargate_count < std::min<size_t>(base_count, 5)) {
                if (observation->GetMinerals() > 150 && observation->GetVespene() > 150) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
            else if (stargate_count > 0 && fleet_beacon_count < 1) {
                if (observation->GetMinerals() > 300 && observation->GetVespene() > 200) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }
    }
    else {
        if (gateway_count > 2 && cybernetics_count > 0) {
            if (robotics_facility_count < std::min<size_t>(base_count, 4)) {
                if (observation->GetMinerals() > 200 && observation->GetVespene() > 100) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
            else if (robotics_facility_count > 0 && robotics_bay_count < 1) {
                if (observation->GetMinerals() > 200 && observation->GetVespene() > 200) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }
    }

    if (forge_count > 0 && twilight_council_count < 1 && base_count > 1) {
        TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
    }

    if (twilight_council_count > 0 && templar_archive_count < 1 && base_count > 2) {
        TryBuildStructureNearPylon(ABILITY_ID::BUILD_TEMPLARARCHIVE, UNIT_TYPEID::PROTOSS_PROBE);
    }

}

//Try to get upgrades depending on build
void FervillerBot::ManageUpgrades() {
    const ObservationInterface* observation = Observation();
    auto upgrades = observation->GetUpgrades();
    size_t base_count = observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size();
    if (upgrades.empty()) {
        TryBuildUnit(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
    }
    else {
        for (const auto& upgrade : upgrades) {
            if (air_build_) {
                if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL1 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL1 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL2 && base_count > 3) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL2 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL2 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                }
            }
            if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1 && base_count > 2) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
            }
            else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1 && base_count > 2) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
            }
            else if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2 && base_count > 3) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
            }
            else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL2 && base_count > 3) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
            }
            else {
                if (air_build_) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                    TryBuildUnit(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON);
                }
                else {
                    TryBuildUnit(ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
                    TryBuildUnit(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                    TryBuildUnit(ABILITY_ID::RESEARCH_CHARGE, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
                }

            }
        }
    }
}

bool FervillerBot::TryWarpInUnit(ABILITY_ID ability_type_for_unit) {
    const ObservationInterface* observation = Observation();
    std::vector<PowerSource> power_sources = observation->GetPowerSources();
    Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));

    if (power_sources.empty()) {
        return false;
    }

    const PowerSource& random_power_source = GetRandomEntry(power_sources);
    float radius = random_power_source.radius;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);

    // If the warp location is walled off, don't warp there.
    // We check this to see if there is pathing from the build location to the center of the map
    if (Query()->PathingDistance(build_location, Point2D(game_info_.playable_max.x / 2, game_info_.playable_max.y / 2)) < .01f) {
        return false;
    }

    for (const auto& warpgate : warpgates) {
        if (warpgate->build_progress == 1) {
            AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
            for (const auto& ability : abilities.abilities) {
                if (ability.ability_id == ability_type_for_unit) {
                    Actions()->UnitCommand(warpgate, ability_type_for_unit, build_location);
                    return true;
                }
            }
        }
    }
    return false;
}

void FervillerBot::ConvertGateWayToWarpGate() {
    const ObservationInterface* observation = Observation();
    Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));

    if (warpgate_reasearched_) {
        for (const auto& gateway : gateways) {
            if (gateway->build_progress == 1) {
                Actions()->UnitCommand(gateway, ABILITY_ID::MORPH_WARPGATE);
            }
        }
    }
}

bool FervillerBot::TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID) {
    const ObservationInterface* observation = Observation();

    //Need to check to make sure its a pylon instead of a warp prism
    std::vector<PowerSource> power_sources = observation->GetPowerSources();
    if (power_sources.empty()) {
        return false;
    }

    const PowerSource& random_power_source = GetRandomEntry(power_sources);
    if (observation->GetUnit(random_power_source.tag) != nullptr) {
        if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISM) {
            return false;
        }
    }
    else {
        return false;
    }
    float radius = random_power_source.radius;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);
    return TryBuildStructure(ability_type_for_structure, UNIT_TYPEID::PROTOSS_PROBE, build_location);
}

bool FervillerBot::TryBuildPylon() {
    const ObservationInterface* observation = Observation();

    // If we are not supply capped, don't build a supply depot.
    if (observation->GetFoodUsed() < observation->GetFoodCap() - 6) {
        return false;
    }

    if (observation->GetMinerals() < 100) {
        return false;
    }

    //check to see if there is already on building
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
    if (observation->GetFoodUsed() < 40) {
        for (const auto& unit : units) {
            if (unit->build_progress != 1) {
                    return false;
            }
        }
    }

    // Try and build a pylon. Find a random Probe and give it the order.
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
    return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
}

//Separated per race due to gas timings
bool FervillerBot::TryBuildAssimilator() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

    if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
        return false;
    }

    for (const auto& base : bases) {
        if (base->assigned_harvesters >= base->ideal_harvesters) {
            if (base->build_progress == 1) {
                if (TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Same as above with expansion timings
bool FervillerBot::TryBuildExpansionNexus() {
    const ObservationInterface* observation = Observation();

    //Don't have more active bases than we can provide workers for
    if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) > max_worker_count_) {
        return false;
    }
    // If we have extra workers around, try and build another nexus.
    if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) < observation->GetFoodWorkers() - 16) {
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    }
    //Only build another nexus if we are floating extra minerals
    if (observation->GetMinerals() > CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) * 400) {
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    }
    return false;
}

bool FervillerBot::TryBuildProbe() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    if (observation->GetFoodWorkers() >= max_worker_count_) {
        return false;
    }

    if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
        return false;
    }

    if (observation->GetFoodWorkers() > GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR)) {
        return false;
    }

    for (const auto& base : bases) {
        //if there is a base with less than ideal workers
        if (base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1) {
            if (observation->GetMinerals() >= 50) {
                return TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
            }
        }
    }
    return false;
}

void FervillerBot::OnStep() {

    const ObservationInterface* observation = Observation();

    //Throttle some behavior that can wait to avoid duplicate orders.
    int frames_to_skip = 4;
    if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
        frames_to_skip = 6;
    }

    if (observation->GetGameLoop() % frames_to_skip != 0) {
        return;
    }

    if (!nuke_detected) {
        ManageArmy();
    }
    else if (nuke_detected) {
        if (nuke_detected_frame + 400 < observation->GetGameLoop()) {
            nuke_detected = false;
        }
        Units units = observation->GetUnits(Unit::Self, IsArmy(observation));
        for (const auto& unit : units) {
            RetreatWithUnit(unit, startLocation_);
        }
    }

    ConvertGateWayToWarpGate();

    ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);

    ManageUpgrades();

    BuildOrder();

    if (TryBuildPylon()) {
        return;
    }

    if (TryBuildAssimilator()) {
        return;
    }

    if (TryBuildProbe()) {
        return;
    }

    if (TryBuildArmy()) {
        return;
    }

    if (TryBuildExpansionNexus()) {
        return;
    }
}

void FervillerBot::OnGameEnd() {
    std::cout << "Game Ended for: " << std::to_string(Control()->Proto().GetAssignedPort()) << std::endl;
}

void FervillerBot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::PROTOSS_PROBE: {
            MineIdleWorkers(unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
            break;
        }
        case UNIT_TYPEID::PROTOSS_CYBERNETICSCORE: {
            const ObservationInterface* observation = Observation();
            Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

            if (!warpgate_reasearched_) {
                Actions()->UnitCommand(unit, ABILITY_ID::RESEARCH_WARPGATE);
                if (!nexus.empty()) {
                    Actions()->UnitCommand(nexus.front(), ABILITY_ID::EFFECT_TIMEWARP, unit);
                }
            }
        }
        default:
            break;
    }
}

void FervillerBot::OnUpgradeCompleted(UpgradeID upgrade) {
    switch (upgrade.ToType()) {
        case UPGRADE_ID::WARPGATERESEARCH: {
            warpgate_reasearched_ = true;
        }
        case UPGRADE_ID::BLINKTECH: {
            blink_reasearched_ = true;
        }
        default:
            break;
    }
}


}
