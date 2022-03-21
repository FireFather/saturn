#include "cli.hpp"
#include "primitives/utility.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include "tt.hpp"

std::ostream& operator<<(std::ostream &os, const UciSpin &spin) {
    return os << spin.value << " min " 
        << spin.min << " max " << spin.max;
}

std::istream& operator>>(std::istream &is, UciSpin &spin) {
    return is >> spin.value;
}

std::ostream& operator<<(std::ostream &os, const UciOption &opt) {
    auto flags = os.flags();

    os << std::boolalpha;
    std::visit([&os](auto &&arg) { os << arg; }, opt);

    os.flags(flags);

    return os;
}

std::istream& operator>>(std::istream& is, UciOption &opt) {
    auto flags = is.flags();
    is >> std::boolalpha;

    std::visit([&is](auto &&arg) { is >> arg; }, opt);

    is.flags(flags);
    return is;
}

UCIContext::UCIContext() {
    options_["hash"] = UciSpin { 4, 1024, 128 };
}

void UCIContext::enter_loop() {
    board_ = Board::start_pos();
    st_.reset();

    std::string s, cmd;
    std::istringstream is;

    do {
        if (!std::getline(std::cin, s))
            s = "quit";

        is.str(s);
        is.clear();
        is >> cmd;

        if (cmd == "isready") sync_cout() << "readyok\n";
        else if (cmd == "uci") print_info();
        else if (cmd == "position") parse_position(is);
        else if (cmd == "go") parse_go(is);
        else if (cmd == "setoption") parse_setopt(is);
        else if (cmd == "stop") {}
        else if (cmd == "d") {}
        else if (cmd == "quit") break;

    } while (s != "quit");
}

void UCIContext::parse_position(std::istream &is) {
    std::string s, fen;
    is >> s;
    st_.reset();

    if (s == "fen") {
        while (is >> s && s != "moves")
            fen += s + ' ';
        bool result = board_.load_fen(fen);
        assert(result);
    } else if (s == "startpos") {
        board_ = Board::start_pos();
        is >> s; //consume "moves"
    } else {
        return;
    }

    if (s != "moves")
        return;

    while (is >> s) {
        Move m = move_from_str(board_, s);
        if (m == MOVE_NONE)
            break;
        st_.push(board_.key(), m);
        board_ = board_.do_move(m);
    }
    st_.set_start(st_.height());
}

void UCIContext::parse_go(std::istream &is) {
    std::string token;
    SearchLimits limits;
    limits.start = timer::now();

    while (is >> token) {
        if (token == "wtime") is >> limits.time[WHITE];
        else if (token == "btime") is >> limits.time[BLACK];
        else if (token == "winc") is >> limits.inc[WHITE];
        else if (token == "binc") is >> limits.inc[BLACK];
        else if (token == "movetime") is >> limits.move_time;
        else if (token == "infinite") limits.infinite = true;
        else if (token == "depth") is >> limits.max_depth;
    }

    search_.go(board_, st_, limits);
}

void UCIContext::parse_setopt(std::istream &is) {
    std::string name, dummy;
    is >> name >> dummy;
    std::transform(name.begin(), name.end(), name.begin(),
        [](char ch) { return std::tolower(ch); });
    if (auto it = options_.find(name); it != options_.end()) {
        if (dummy == "value") {
            is >> it->second;
            update_option(name, it->second);
        }
    }
}

void UCIContext::update_option(std::string_view name, 
        const UciOption &opt)
{
    if (name == "hash") {
        if (auto spin = std::get_if<UciSpin>(&opt); spin 
                && spin->value >= spin->min
                && spin->value <= spin->max) 
        {
            search_.stop();
            search_.wait_for_completion();
            g_tt.resize(spin->value);
        }
    }
}

void UCIContext::print_info() {
    sync_cout() << "id name gm_bit\nid author asdf\n"
        << "uciok\n";

    static const char *opt_type[] = { "check", "spin", "string" };
    for (auto &[name, opt]: options_) {
        sync_cout() << "option name " << name
            << " type " << opt_type[opt.index()]
            << " default " << opt << '\n';
    }
}

int enter_cli(int argc, char **argv) {
    (void)(argc);
    (void)(argv);

    UCIContext uci;
    uci.enter_loop();

    return 0;
}

