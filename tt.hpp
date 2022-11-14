#ifndef TT_HPP
#define TT_HPP

#include "primitives/common.hpp"
#include <cstddef>

class Board;

enum Bound {
    BOUND_NONE = 0,
    BOUND_ALPHA = 1,
    BOUND_BETA = 2,
    BOUND_EXACT = 3,
};

struct TTEntry {
    uint64_t key;
    union {
        uint64_t data;
        struct {
            uint16_t move16;
            int16_t score16;
            uint8_t depth8;
            uint8_t bound8;
            bool avoid_null;
            uint8_t age;
        };
    };

    [[nodiscard]] int score(int ply) const;

    TTEntry() = default;
    TTEntry(uint64_t key, int score, Bound b, int depth, 
            Move m, int ply, bool avoid_null);
};

class TranspositionTable {
    struct Bucket {
        static constexpr int N = 4;
        TTEntry entries[N];
    };
public:
    TranspositionTable() = default;

    void resize(size_t mbs);
    void clear() const;

    void new_search();

    bool probe(uint64_t key, TTEntry &e) const;
    void store(TTEntry entry) const;

    int extract_pv(Board b, Move *pv, int len) const;

    void prefetch(uint64_t key) const;

    [[nodiscard]] uint64_t hashfull() const;

    ~TranspositionTable();

private:
    Bucket* buckets_{};
    size_t size_{};
    uint8_t age_{};
};

extern TranspositionTable g_tt;

#endif
