#include <iostream>
#include "Engine.h"
//#include "setup.h"

int main() {
    ProgramOptions options{
            "data.dump",
            100
    };
//    setup_experiment(0.8);
    Engine engine("initial.data", options);
    engine.set_baseplate(2e-4, 0.02);
    for (int s{0}; s < 100000; s++) {
        engine.step();
    }
}