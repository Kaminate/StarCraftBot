#include "sc2api/sc2_api.h"
#include "sc2utils/sc2_manage_process.h"

#include "bot_examples.h"
#include "bot_goose.h"
#include "bot_nate.h"
#include "bot_ferviller.h"

#include <iostream>
#include <cassert>

bool useAIEnemy = true;
bool useBotAlly = true;

int main(int argc, char* argv[]) {

    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return 1;
    }
    coordinator.SetMultithreaded(true);
    std::vector<sc2::PlayerSetup> participants;
    
    sc2::FervillerBot fervillerBot;
    sc2::NateBot nateBot;
    sc2::GooseBot gooseBot;

    participants.push_back( CreateComputer( sc2::Race::Protoss, sc2::Difficulty::VeryHard ) );
    //participants.push_back( CreateComputer( sc2::Race::Terran, sc2::Difficulty::VeryHard ) );
    //participants.push_back( CreateComputer( sc2::Race::Zerg, sc2::Difficulty::VeryHard ) );
    //participants.push_back( CreateParticipant( sc2::Race::Protoss, &fervillerBot ) );
    //participants.push_back( CreateParticipant( sc2::Race::Terran, &nateBot ) );
    participants.push_back( CreateParticipant( sc2::Race::Zerg, &gooseBot ) );

    assert( participants.size() == 2 );

    coordinator.SetParticipants( participants );
    coordinator.LaunchStarcraft();

    bool do_break = false;
    while (!do_break) {
        if (!coordinator.StartGame(sc2::kMapBelShirVestigeLE)) {
            break;
        }
        while (coordinator.Update() && !do_break) {
            if (sc2::PollKeyPress()) {
                do_break = true;
            }
        }
    }
    
    for( sc2::Agent* agent : {
        ( sc2::Agent* )&nateBot,
        ( sc2::Agent* )&gooseBot,
        ( sc2::Agent* )&fervillerBot,
        } )
        agent->Control()->DumpProtoUsage();

    return 0;
}
