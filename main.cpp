#include "zobrist.hpp"
#include "movgen/attack.hpp"
#include "tt.hpp"
#include "core/eval.hpp"
#include "cli.hpp"
#include "nnue/nnue.h"

using namespace std;

int main(const int argc, char **argv) {
    init_zobrist();
    init_attack_tables();
    init_ps_tables();
    init_reduction_tables();
    g_tt.resize(128);
    nnue_init("saturn.bin");
    return enter_cli(argc, argv);
}

