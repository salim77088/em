#include "core/Engine.h"
#include "core/Logger.h"
#include <cstdio>

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    using namespace nexus;

    NX_INFO("Main", "NEXUS Engine starting...");
    auto& engine = core::Engine::get();
    if (!engine.init("NEXUS Engine v1.0", 1280, 720)) {
        NX_FATAL("Main", "Engine init failed");
        return 1;
    }

    while (engine.run()) {
        // main loop
    }

    engine.shutdown();
    NX_INFO("Main", "NEXUS Engine exited");
    return 0;
}
