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

    int foo;

    foo = sc2::GooseFoo();
    std::cout << "foo: " << foo << std::endl;

    foo = sc2::NateFoo();
    std::cout << "foo: " << foo << std::endl;

    coordinator.SetMultithreaded(true);

    // Add the custom bot, it will control the players.
    sc2::ZergMultiplayerBot bot1;
    sc2::Race race1 = sc2::Race::Zerg;

    sc2::TerranMultiplayerBot bot2;
    sc2::Race race2 = sc2::Race::Terran;

    coordinator.SetParticipants({
        CreateParticipant(race1, &bot1),
        CreateParticipant(race2, &bot2),
    });

    // Start the game.
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
