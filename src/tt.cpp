#include "tt.hpp"
#include <cstring>
#include "board/board.hpp"
#include <algorithm>
#include <xmmintrin.h>

#ifdef _MSC_VER
#else
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wshadow"
#endif

TranspositionTable g_tt;

int TTEntry::score(const int ply) const {
    int s = score16;
    if (s > MATE_BOUND)
        s -= ply;
    else if (s < -MATE_BOUND)
        s += ply;
    return s;
}

TTEntry::TTEntry(const uint64_t key, int s, const Bound b,
                 const int depth, const Move m, const int ply, bool null) : key(key)
{
    move16 = static_cast<uint16_t>(m);
    depth8 = static_cast<uint8_t>(depth);
    bound8 = static_cast<uint8_t>(b);
    avoid_null = null;

    if (s > MATE_BOUND)
        s += ply;
    else if (s < -MATE_BOUND)
        s -= ply;
    score16 = static_cast<int16_t>(s);
}

void TranspositionTable::resize(const size_t mbs) {
    if (buckets_)
        delete[] buckets_;
    size_ = mbs * 1024 * 1024 / sizeof(Bucket);
    buckets_ = new Bucket[size_];
    memset(buckets_, 0, size_ * sizeof(Bucket));
}

void TranspositionTable::clear() const
{
    if (buckets_)
        memset(buckets_, 0, size_ * sizeof(Bucket));
}

void TranspositionTable::new_search() {
    ++age_;
}

bool TranspositionTable::probe(const uint64_t key, 
                               TTEntry &e) const 
{
	const Bucket &b = buckets_[key % size_];
    for (const auto entrie : b.entries)
    {
        e = entrie;
        if ((e.key ^ e.data) == key) {
            e.age = age_;
            return true;
        }
    }

    return false;
}

void TranspositionTable::store(TTEntry entry) const
{
    Bucket &b = buckets_[entry.key % size_];
    TTEntry *replace = nullptr;
    for (auto& e : b.entries)
    {
	    if ((e.key ^ e.data) == entry.key) {
            replace = &e;
            break;
        }
    }

    if (!replace) {
        int replace_depth = 9999;
        for (auto& e : b.entries)
        {
	        if (e.age != age_ && e.depth8 < replace_depth) {
                replace = &e;
                replace_depth = e.depth8;
            }
        }

        if (!replace) {
            for (auto& e : b.entries)
            {
	            if (e.depth8 < replace_depth) {
                    replace = &e;
                    replace_depth = e.depth8;
                }
            }
        }
    }

    entry.age = age_;

    replace->key = entry.key ^ entry.data;
    replace->data = entry.data;
}

void TranspositionTable::prefetch(const uint64_t key) const {
    _mm_prefetch(reinterpret_cast<const char*>(&buckets_[key % size_]), 
            _MM_HINT_NTA);
}

uint64_t TranspositionTable::hashfull() const {
    uint64_t cnt = 0;
    for (size_t i = 0; i < 1000; ++i) {
        for (const auto &e: buckets_[i].entries)
            cnt += e.depth8 && e.age == age_;
    }

    return cnt / Bucket::N;
}

TranspositionTable::~TranspositionTable() {
    if (buckets_) {
        delete[] buckets_;
    }
}

int TranspositionTable::extract_pv(Board b, Move *pv, const int len) const
{
    int n = 0;
    TTEntry tte{};
    while (probe(b.key(), tte) && n < len) {
	    const auto m = static_cast<Move>(tte.move16);
        if (!b.is_valid_move(m))
            break;
        b = b.do_move(m);
        *pv++ = m;
        ++n;
    }
    return n;
}

