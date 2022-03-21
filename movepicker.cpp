#include "movepicker.hpp"

MovePicker::MovePicker(const Board &board, Move ttm)
    : board_(board), moves_{{ttm, 0}}, excluded_{ttm}, 
      stage_(Stage::TT_MOVE) {}

MovePicker::MovePicker(const Board &board)
    : board_(board), stage_(Stage::INIT_TACTICAL) {}

Move MovePicker::next() {
    Move m;
    switch (stage_) {
    case Stage::TT_MOVE:
        stage_ = Stage::INIT_TACTICAL;
        return moves_[0];
    case Stage::INIT_TACTICAL:
        stage_ = Stage::TACTICAL;
        cur_ = moves_;
        end_ = generate<TACTICAL>(board_, cur_);
        score_tactical();

        [[fallthrough]];
    case Stage::TACTICAL:
        if ((m = select()) != MOVE_NONE)
            return m;
        stage_ = Stage::INIT_NONTACTICAL;

        [[fallthrough]];
    case Stage::INIT_NONTACTICAL:
        stage_ = Stage::NON_TACTICAL;
        cur_ = moves_;
        end_ = generate<NON_TACTICAL>(board_, cur_);
        score_nontactical();

        [[fallthrough]];
    case Stage::NON_TACTICAL:
        return select();
    };

    //unreachable
    return MOVE_NONE;
}


Stage MovePicker::stage() const { return stage_; }

void MovePicker::score_tactical() {
    for (auto it = cur_; it != end_; ++it) {
        for (Move ex: excluded_)
            if (it->move == ex)
                *it = *--end_;
    }
}

void MovePicker::score_nontactical() {
    for (auto it = cur_; it != end_; ++it) {
        for (Move ex: excluded_)
            if (it->move == ex)
                *it = *--end_;
    }
}

Move MovePicker::select() {
    return cur_ == end_ ? MOVE_NONE : *cur_++;
}

