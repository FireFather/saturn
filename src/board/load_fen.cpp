#include "board.hpp"
#include <cstring>
#include "../parse_helpers.hpp"
#include "../primitives/utility.hpp"
#include "../zobrist.hpp"

bool Board::load_fen(std::string_view fen) {
    trim_front(fen);
    memset(this, 0, sizeof(Board));

    for (int r = RANK_8; r >= RANK_1; --r) {
        for (int f = FILE_A; f <= FILE_H; ++f) {
            if (fen.empty())
                return false;

            const char ch = fen.front();

            const Square s = make_square(static_cast<File>(f), static_cast<Rank>(r));
            if (is_digit(fen.front())) {
                f = static_cast<File>(f + ch - '1');
                fen = fen.substr(1);
                continue;
            }

            const Piece p = piece_from_str(fen);
            if (!is_ok(p))
                return false;
            put_piece(p, s);

            fen = fen.substr(1);
        }

        if (r != RANK_1) {
            if (fen.empty())
                return false;
            if (fen.front() != '/')
                return false;
            fen = fen.substr(1);
        }
    }

    trim_front(fen);
    if (fen.empty())
        return false;

    switch (to_lower(fen.front())) {
    case 'w': side_to_move_ = WHITE; break;
    case 'b': side_to_move_ = BLACK; break;
    default: return false;
    }

    if (side_to_move_ == BLACK)
        key_ ^= ZOBRIST.side;

    next_word(fen);
    castling_ = castling_from_str(fen);
    key_ ^= ZOBRIST.castling[castling_];

    next_word(fen);
    en_passant_ = square_from_str(fen);
    if (en_passant_ != SQ_NONE)
        key_ ^= ZOBRIST.enpassant[file_of(en_passant_)];

    update_pin_info();

    validate();

    return true;
}

