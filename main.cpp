#include <iostream>
#include "Engine.h"
#include "setup.h"

int main() {
    ProgramOptions options{
        "data.dump",
        1000
    };
    setup_experiment(0.8);
    Engine engine("initial.data", options);
    engine.set_baseplate(1e-15, 0.02);
    for (int s{0}; s<10000; s++){
        engine.step();
    }

}
