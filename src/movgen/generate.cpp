#include "generate.hpp"
#include "../primitives/common.hpp"
#include "../primitives/bitboard.hpp"
#include "../board/board.hpp"
#include "attack.hpp"

namespace {

/*---------------------Pawn moves--------------------*/

ExtMove* make_proms(const Square from, const Square to, ExtMove *moves) {
    *moves++ = make<PROMOTION>(from, to, KNIGHT);
    *moves++ = make<PROMOTION>(from, to, BISHOP);
    *moves++ = make<PROMOTION>(from, to, ROOK);
    *moves++ = make<PROMOTION>(from, to, QUEEN);
    return moves;
}

bool legal_ep_move(const Board &b, const Square from, const Square to) {
	const Square cap_sq = make_square(file_of(to), rank_of(from));
	const Bitboard combined = b.pieces()
        ^ square_bb(cap_sq)
        ^ square_bb(from) 
        ^ square_bb(to);

	const Color us = b.side_to_move(), them = ~us;
	const Square ksq = b.king_square(us);
    
    Bitboard bb = 0;
    bb |= attacks_bb<BISHOP>(ksq, combined) 
        & b.pieces(them, BISHOP, QUEEN);
    bb |= attacks_bb<ROOK>(ksq, combined)
        & b.pieces(them, ROOK, QUEEN);

    return !bb;
}

template<GenType T, bool IN_CHECK>
ExtMove* pawn_legals(const Board &b, ExtMove *moves) {
	const Color us = b.side_to_move(), them = ~us;
    const Square ksq = b.king_square(us);
    Bitboard our_pawns = b.pieces(us, PAWN);
    if (!(T & TACTICAL))
        our_pawns &= ~relative_rank_bb(us, RANK_7);

	const Bitboard my_r3 = relative_rank_bb(us, RANK_3),
	               my_r8 = relative_rank_bb(us, RANK_8);

    Bitboard check_mask = ~static_cast<Bitboard>(0);
    if (IN_CHECK) {
        check_mask = between_bb(ksq, lsb(b.checkers())) 
            | b.checkers();
    }

    const Bitboard pinned = b.blockers_for_king(us);
    Bitboard pawns = our_pawns & ~pinned;

    //May be 2 loops: first for < 7th rank, second for 7th rank?
    while (pawns) {
	    const Square from = pop_lsb(pawns);

        Bitboard dsts = pawn_pushes_bb(us, from) & ~b.pieces();
        if (!(T & NON_TACTICAL))
            dsts &= my_r8;
        if (T & NON_TACTICAL) {
            Bitboard bb = (my_r3 & b.pieces()) & ~square_bb(from);
            bb = (bb << 8) | (bb >> 8);
            dsts &= ~bb;
        }

        if (T & TACTICAL)
            dsts |= pawn_attacks_bb(us, from) & b.pieces(them);

        if (IN_CHECK)
            dsts &= check_mask;

        Bitboard bb = dsts & my_r8;
        while ((T & TACTICAL) && bb)
            moves = make_proms(from, pop_lsb(bb), moves);

        bb = dsts & ~my_r8;
        while (bb)
            *moves++ = make_move(from, pop_lsb(bb));
    }

    if (!IN_CHECK) {
        pawns = our_pawns & pinned;
        while (pawns) {
	        const Square from = pop_lsb(pawns);

            Bitboard dsts = 0;
            if (T & NON_TACTICAL) {
                dsts |= pawn_pushes_bb(us, from) & ~b.pieces();
                Bitboard bb = (my_r3 & b.pieces()) & ~square_bb(from);
                bb = (bb << 8) | (bb >> 8);
                dsts &= ~bb;
            }

            if (T & TACTICAL)
                dsts |= pawn_attacks_bb(us, from) & b.pieces(them);

            dsts &= line_bb(ksq, from);
            
            Bitboard bb = dsts & my_r8;
            while ((T & TACTICAL) && bb)
                moves = make_proms(from, pop_lsb(bb), moves);

            bb = dsts & ~my_r8;
            while (bb)
                *moves++ = make_move(from, pop_lsb(bb));
        }
    }

    //branchy boiii
	if (const Square ep = b.en_passant(); (T & TACTICAL) && ep != SQ_NONE) {
	    const Square to = ep;
	    const Bitboard rbb = relative_rank_bb(us, RANK_5),
	                   fbb = adjacent_files_bb(file_of(to));
        Bitboard bb = our_pawns & rbb & fbb;
        while (bb) {
	        if (const Square from = pop_lsb(bb); legal_ep_move(b, from, to))
                *moves++ = make<EN_PASSANT>(from, to);
        }
    }

    return moves;
}

/*----------------End of pawn moves------------------*/

/*-------------------Knight moves--------------------*/

template<GenType T, bool IN_CHECK>
ExtMove* knight_legals(const Board &b, ExtMove *moves) {
	const Color us = b.side_to_move(), them = ~us;

    const Bitboard our_knights = b.pieces(us, KNIGHT);
    const Bitboard pinned = b.blockers_for_king(us);
    const Square ksq = b.king_square(us);

    Bitboard mask = 0;
    if (T & TACTICAL)
        mask |= b.pieces(them);
    if (T & NON_TACTICAL)
        mask |= ~b.pieces();
    if (IN_CHECK)
        mask &= between_bb(ksq, lsb(b.checkers()))
            | b.checkers();

    Bitboard bb = our_knights & ~pinned;
    while (bb) {
	    const Square from = pop_lsb(bb);
        Bitboard dsts = attacks_bb<KNIGHT>(from) & mask;
        while (dsts)
            *moves++ = make_move(from, pop_lsb(dsts));
    }

    return moves;
}

/*----------------End of knight moves----------------*/

/*-------------------Slider moves--------------------*/

template<GenType T, PieceType P, bool IN_CHECK>
ExtMove* slider_legals(const Board &b, ExtMove *moves) {
    static_assert(P == BISHOP || P == ROOK || P == QUEEN);

    const Color us = b.side_to_move(), them = ~us;

    const Bitboard our_sliders = b.pieces(us, P);
    const Square ksq = b.king_square(us);
    const Bitboard pinned = b.blockers_for_king(us);

    Bitboard mask = 0;
    if (T & TACTICAL)
        mask |= b.pieces(them);
    if (T & NON_TACTICAL)
        mask |= ~b.pieces();
    if (IN_CHECK)
        mask &= between_bb(ksq, lsb(b.checkers()))
            | b.checkers();

    Bitboard bb = our_sliders & ~pinned;
    while (bb) {
        Square from = pop_lsb(bb);
        Bitboard dsts = attacks_bb<P>(from, b.pieces()) & mask;
        while (dsts)
            *moves++ = make_move(from, pop_lsb(dsts));
    }

    if (!IN_CHECK) {
        bb = our_sliders & pinned;
        while (bb) {
            Square from = pop_lsb(bb);
            Bitboard dsts = attacks_bb<P>(from, b.pieces())
                & line_bb(from, ksq) & mask;
            while (dsts)
                *moves++ = make_move(from, pop_lsb(dsts));
        }
    }

    return moves;
}

/*--------------- End of slider moves----------------*/

/*--------------------King moves--------------------*/

bool legal_king_move(const Board &b, const Square to) {
	const Color us = b.side_to_move(), them = ~us;

    const Bitboard kbb = b.pieces(us, KING);
    const Bitboard combined = b.pieces() ^ kbb;

    return !b.attackers_to(them, to, combined);
}

template<GenType T, bool IN_CHECK>
ExtMove* king_legals(const Board &b, ExtMove *moves) {
	const Color us = b.side_to_move(), them = ~us;
    const Square from = b.king_square(us);

    Bitboard mask = 0;
    if (T & TACTICAL)
        mask |= b.pieces(them);
    if (T & NON_TACTICAL)
        mask |= ~b.pieces();

    Bitboard bb = attacks_bb<KING>(from) & mask;
    while (bb) {
	    if (const Square to = pop_lsb(bb); legal_king_move(b, to))
            *moves++ = make_move(from, to);
    }

    if (IN_CHECK)
        return moves;
    if (!(T & NON_TACTICAL))
        return moves;

    const CastlingRights cr = b.castling();
    const Bitboard kingside_mask = KINGSIDE_MASK[us];
    const Bitboard queenside_mask = QUEENSIDE_MASK[us];

    if (cr & kingside_rights(us) && !(b.pieces() & kingside_mask)) {
	    const auto right = static_cast<Square>(from + 2);
        if (const auto middle = static_cast<Square>(from + 1); legal_king_move(b, middle) && legal_king_move(b, right))
            *moves++ = make<CASTLING>(from, right);
    }
    
    if (cr & queenside_rights(us) && !(b.pieces() & queenside_mask)) {
	    const auto left = static_cast<Square>(from - 2);
        if (const auto middle = static_cast<Square>(from - 1); legal_king_move(b, middle) && legal_king_move(b, left))
            *moves++ = make<CASTLING>(from, left);
    }

    return moves;
}

/*-----------------End of king moves----------------*/

} //namespace

template ExtMove* generate<TACTICAL>(const Board&, ExtMove*);
template ExtMove* generate<NON_TACTICAL>(const Board&, ExtMove*);
template ExtMove* generate<LEGAL>(const Board&, ExtMove*);

template<GenType T>
ExtMove* generate(const Board &b, ExtMove *moves) {
    if (!b.checkers()) {
        moves = pawn_legals<T, false>(b, moves);
        moves = knight_legals<T, false>(b, moves);
        moves = slider_legals<T, BISHOP, false>(b, moves);
        moves = slider_legals<T, ROOK, false>(b, moves);
        moves = slider_legals<T, QUEEN, false>(b, moves);
        moves = king_legals<T, false>(b, moves);
    } else if (popcnt(b.checkers()) == 1) {
        moves = pawn_legals<T, true>(b, moves);
        moves = knight_legals<T, true>(b, moves);
        moves = slider_legals<T, BISHOP, true>(b, moves);
        moves = slider_legals<T, ROOK, true>(b, moves);
        moves = slider_legals<T, QUEEN, true>(b, moves);
        moves = king_legals<T, true>(b, moves);
    } else {
        moves = king_legals<T, true>(b, moves);
    }

    return moves;
}

