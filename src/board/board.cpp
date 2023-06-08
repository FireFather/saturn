#include "board.hpp"
#include <ostream>
#include "../zobrist.hpp"
#include "../movgen/attack.hpp"
#include "../primitives/utility.hpp"
#include "../core/eval.hpp"

/*
 * FILE: board.cpp
 * All the piece management stuff goes here, as well 
 * as some methods not worth creating a separate file
 * */

constexpr uint64_t PCKEY_INDEX[COLOR_NB][PIECE_TYPE_NB] = {
    { 0, 1ull << 0, 1ull << 4, 1ull << 8, 1ull << 12, 1ull << 16, 0 },
    { 0, 1ull << 20, 1ull << 24, 1ull << 28,  1ull << 32, 1ull << 36, 0 },
};

template<Piece p, Piece ...pcs>
constexpr uint64_t pckey_v = pckey_v<p> | pckey_v<pcs...>;

template<Piece p>
constexpr uint64_t pckey_v<p> = PCKEY_INDEX[static_cast<int>(color_of(p))][type_of(p)];

Board Board::start_pos() {
    Board board{};
    const bool b = board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    assert(b);
    board.validate();

    return board;
}

void Board::update_pin_info() {
    //could be cheaper, because we look up sliders 3(!) times
    const Color us = side_to_move_, them = ~us;
    const Square our_king = king_square(us),
                 their_king = king_square(them);

    blockers_for_king_[us] = slider_blockers<false>(pieces(them),
            our_king, pinners_[them]);
    blockers_for_king_[them] = slider_blockers<false>(pieces(us),
            their_king, pinners_[us]);

    checkers_ = attackers_to(them, our_king, combined_);
}

uint64_t Board::mat_key() const { return mat_key_; }

bool Board::is_material_draw() const {
    switch (mat_key_) {
    case 0:
    case pckey_v<W_KNIGHT>:
    case pckey_v<W_KNIGHT> * 2:
    case pckey_v<B_KNIGHT>:
    case pckey_v<B_KNIGHT> * 2:
    case pckey_v<W_KNIGHT, B_KNIGHT>:
    case pckey_v<W_BISHOP>:
    case pckey_v<B_BISHOP>:
    case pckey_v<W_BISHOP, B_KNIGHT>:
    case pckey_v<B_BISHOP, W_KNIGHT>:
        return true;
    default: ;
    }
    return false;
}

bool Board::has_nonpawns(const Color c) const {
    return pieces(c) & ~pieces(c, PAWN, KING);
}

Bitboard Board::attackers_to(const Color c, const Square s, const Bitboard blockers) const {
    return (pawn_attacks_bb(~c, s) & pieces(c, PAWN))
        | (attacks_bb<KNIGHT>(s) & pieces(c, KNIGHT))
        | (attacks_bb<BISHOP>(s, blockers) & pieces(c, BISHOP, QUEEN))
        | (attacks_bb<ROOK>(s, blockers) & pieces(c, ROOK, QUEEN))
        | (attacks_bb<KING>(s) & pieces(c, KING));
}

Bitboard Board::attackers_to(const Square s, const Bitboard blockers) const {
    return (pawn_attacks_bb(BLACK, s) & pieces(WHITE, PAWN))
        | (pawn_attacks_bb(WHITE, s) & pieces(BLACK, PAWN))
        | (attacks_bb<KNIGHT>(s) & pieces(KNIGHT))
        | (attacks_bb<BISHOP>(s, blockers) & pieces(BISHOP, QUEEN))
        | (attacks_bb<ROOK>(s, blockers) & pieces(ROOK, QUEEN))
        | (attacks_bb<KING>(s) & pieces(KING));
}

template Bitboard Board::slider_blockers<true>(Bitboard sliders, 
        Square s, Bitboard &pinners, Bitboard *checkers) const;
template Bitboard Board::slider_blockers<false>(Bitboard sliders, 
        Square s, Bitboard &pinners, Bitboard *checkers) const;

template<bool Checkers>
Bitboard Board::slider_blockers(const Bitboard sliders, const Square s,
                                Bitboard &pinners, Bitboard *checkers) const
{
    assert((combined_ & sliders) == sliders);
    Bitboard blockers = 0;
    pinners = 0;

    Bitboard snipers = ((attacks_bb<BISHOP>(s) & pieces(BISHOP, QUEEN))
        | (attacks_bb<ROOK>(s) & pieces(ROOK, QUEEN))) & sliders;

    while (snipers) {
	    const Square sniper_sq = pop_lsb(snipers);
	    if (const Bitboard b = between_bb(s, sniper_sq) & combined_; popcnt(b) == 1) {
            blockers |= b;
            pinners |= square_bb(sniper_sq);
        } else if (Checkers && !b) {
            *checkers |= square_bb(sniper_sq);
        }
    }

    return blockers;
}

void Board::put_piece(const Piece p, const Square s) {
    assert(is_ok(p) && is_ok(s));
    const Bitboard sbb = square_bb(s);
    assert(!(combined_ & sbb));

    const PieceType pt = type_of(p);
    const Color c = color_of(p);

    combined_ |= sbb;
    color_combined_[c] |= sbb;
    pieces_[pt] |= sbb;
    pieces_on_[s] = p;
    /* material_[c] += mg_value[pt]; */
    mat_key_ += PCKEY_INDEX[c][pt];

    key_ ^= ZOBRIST.psq[p][s];
}

void Board::remove_piece(const Square s) {
    assert(is_ok(s));
    const Bitboard sbb = square_bb(s);
    assert(combined_ & sbb);
    const Piece p = pieces_on_[s];
    const PieceType pt = type_of(p);
    const Color c = color_of(p);

    combined_ ^= sbb;
    color_combined_[c] ^= sbb;
    pieces_[pt] ^= sbb;
    pieces_on_[s] = NO_PIECE;
    /* material_[c] -= mg_value[pt]; */
    mat_key_ -= PCKEY_INDEX[c][pt];

    key_ ^= ZOBRIST.psq[p][s];
}

Bitboard Board::pieces() const { return combined_; }
Bitboard Board::pieces(const Color c) const { return color_combined_[c]; }

Bitboard Board::pieces(const PieceType pt) const { return pieces_[pt]; }
Bitboard Board::pieces(const PieceType pt1, const PieceType pt2) const
{ return pieces_[pt1] | pieces_[pt2]; }

Bitboard Board::pieces(const Color c, const PieceType pt) const
{ return color_combined_[c] & pieces_[pt]; }

Bitboard Board::pieces(const Color c, const PieceType pt1, const PieceType pt2) const
{ return color_combined_[c] & (pieces_[pt1] | pieces_[pt2]); }

Piece Board::piece_on(const Square s) const { return pieces_on_[s]; }

Bitboard Board::checkers() const { return checkers_; }
Bitboard Board::blockers_for_king(const Color c) const { return blockers_for_king_[c]; }
Bitboard Board::pinners(const Color c) const { return pinners_[c]; }

Square Board::king_square(const Color c) const { return lsb(pieces(c, KING)); }

Color Board::side_to_move() const { return side_to_move_; }
Square Board::en_passant() const { return en_passant_; }
CastlingRights Board::castling() const { return castling_; }

uint64_t Board::key() const { return key_; }
uint8_t Board::half_moves() const { return half_moves_; }
uint8_t Board::plies_from_null() const { return plies_from_null_; }

namespace {
    constexpr char PIECE_CHAR[PIECE_NB] = {
        ' ', 'P', 'N', 'B', 'R', 'Q', 'K', '?',
        '?', 'p', 'n', 'b', 'r', 'q', 'k'
    };
}

std::ostream& operator<<(std::ostream& os, const Board &b) {
    os << "+---+---+---+---+---+---+---+---+\n";
    for (int r = RANK_8; r >= RANK_1; --r) {
        os << "| ";
        for (int f = FILE_A; f <= FILE_H; ++f) {
	        const Piece p = b.piece_on(make_square(static_cast<File>(f), static_cast<Rank>(r)));
            os << PIECE_CHAR[p] << " | ";
        }
        os << static_cast<char>('1' + r) << '\n';
        os << "+---+---+---+---+---+---+---+---+\n";
    }

    os << "  a   b   c   d   e   f   g   h\n";
    os << "En passant: ";
    if (b.en_passant() == SQ_NONE) os << '-';
    else os << b.en_passant();

    os << "\nSide to move: " << b.side_to_move();
    os << "\nCastling rights: " << b.castling();
    os << "\nStatic evaluation: " << evaluate(b) << "\n";

    const auto flags = os.flags();
    os << "Key: " << std::hex << b.key() << "\n";
    os << "Material draw: " << std::boolalpha << b.is_material_draw() << "\n";
    os.flags(flags);

    os << "Checkers: ";
    Bitboard bb = b.checkers();
    while (bb) {
	    const Square s = pop_lsb(bb);
        os << s << ' ';
    }
    os << '\n';

    return os;
}

