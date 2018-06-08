#include "sc2api/sc2_api.h"
#include "sc2utils/sc2_manage_process.h"

#include "bot_examples.h"
#include "bot_goose.h"
#include "bot_nate.h"

#include <iostream>

int main(int argc, char* argv[]) {
    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return 1;
    }


    coordinator.SetMultithreaded(true);

    sc2::GooseBot bot1;
    sc2::Race race1 = sc2::Race::Zerg;

    sc2::NateBot bot2;
    sc2::Race race2 = sc2::Race::Terran;

    coordinator.SetParticipants({
        CreateParticipant(race1, &bot1),
        CreateParticipant(race2, &bot2),
    });

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
    
    bot1.Control()->DumpProtoUsage();
    bot2.Control()->DumpProtoUsage();

    return 0;
}
