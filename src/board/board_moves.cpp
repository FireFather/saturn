#include "board.hpp"
#include "../zobrist.hpp"
#include "../movgen/attack.hpp"

/*
 * FILE: board_moves.cpp
 * Here goes all the stuff to do with (un)doing moves
 * */

constexpr File rook_start(const bool queenside) {
    return static_cast<File>(FILE_H ^ (7 * queenside));
}

constexpr File rook_end(const bool queenside) {
    return static_cast<File>((FILE_F ^ (7 * queenside)) + queenside);
}

Board Board::do_move(const Move m) const {
    Board result = *this;

    result.en_passant_ = SQ_NONE;
    result.checkers_ = 0;

    const Square from = from_sq(m), to = to_sq(m);
    const Color us = side_to_move_, them = ~us;

    const Bitboard from_bb = square_bb(from), to_bb = square_bb(to),
                   mbb = from_bb | to_bb;
    const Piece moved = piece_on(from);

    result.remove_piece(from);

    const Piece captured = piece_on(to);
    if (captured != NO_PIECE)
        result.remove_piece(to);
    const Piece p = type_of(m) == PROMOTION ? make_piece(
		                    us, prom_type(m)) : moved;
    result.put_piece(p, to);

    const uint8_t disable_wks = (mbb & KINGSIDE_BB[WHITE]) != 0,
                  disable_bks = (mbb & KINGSIDE_BB[BLACK]) != 0,
                  disable_wqs = (mbb & QUEENSIDE_BB[WHITE]) != 0,
                  disable_bqs = (mbb & QUEENSIDE_BB[BLACK]) != 0;

    const uint8_t cr_disabled = (disable_bqs << 3) | (disable_bks << 2)
        | (disable_wqs << 1) | disable_wks;
    result.castling_ = static_cast<CastlingRights>(castling_
	    & (ALL_CASTLING ^ cr_disabled));

    const Bitboard ksq_bb = result.pieces(them, KING);
    const Square ksq = lsb(ksq_bb);

    if (type_of(moved) == KNIGHT) {
        result.checkers_ |= attacks_bb<KNIGHT>(ksq) & to_bb;
    } else if (type_of(moved) == PAWN) {
        if (type_of(m) == EN_PASSANT) {
	        const Square cap_sq = make_square(file_of(to), rank_of(from));
            result.remove_piece(cap_sq);
            result.checkers_ |= pawn_attacks_bb(them, ksq) & to_bb;
        } else if (type_of(m) == PROMOTION) {
	        if (const PieceType prom = prom_type(m); prom == KNIGHT)
                result.checkers_ |= attacks_bb<KNIGHT>(ksq) & to_bb;
        } else if (from_bb & (RANK_2_BB | RANK_7_BB) 
                && to_bb & (RANK_4_BB | RANK_5_BB)) 
        {
            Bitboard ep_bb = from_bb & RANK_2_BB;
            ep_bb |= to_bb & RANK_5_BB;
            ep_bb <<= 8;
            result.en_passant_ = pop_lsb(ep_bb);
            result.checkers_ |= pawn_attacks_bb(them, ksq) & to_bb;
        } else {
            assert(type_of(m) == NORMAL);
            result.checkers_ |= pawn_attacks_bb(them, ksq) & to_bb;
        }
    } else if (type_of(m) == CASTLING/* && type_of(moved) == KING*/) {
	    const Rank rank = rank_of(to);
	    const bool queenside = file_of(to) == FILE_C;
	    const Square rk_from = make_square(rook_start(queenside), rank),
	                 rk_to = make_square(rook_end(queenside), rank);
        result.remove_piece(rk_from);
        result.put_piece(make_piece(us, ROOK), rk_to);
    }

    result.blockers_for_king_[us] = result.slider_blockers<false>(
            result.pieces(them), result.king_square(us),
            result.pinners_[them]);
    result.blockers_for_king_[them] = result.slider_blockers<true>(
            result.pieces(us), result.king_square(them),
            result.pinners_[us], &result.checkers_);

    result.side_to_move_ = them;

    //this may possibly overflow only in quiescience
    //and there we don't care about half_moves
    result.half_moves_++;
    result.plies_from_null_++;
    if (type_of(moved) == PAWN || captured != NO_PIECE)
        result.half_moves_ = 0;

    result.key_ ^= ZOBRIST.side
        ^ ZOBRIST.castling[castling_]
        ^ ZOBRIST.castling[result.castling_]
        ^ (ZOBRIST.enpassant[file_of(en_passant_)] 
            * (en_passant_ != SQ_NONE))
        ^ (ZOBRIST.enpassant[file_of(result.en_passant_)] 
            * (result.en_passant_ != SQ_NONE));

    return result;
}

Board Board::do_null_move() const {
    assert(!checkers_);

    Board result = *this;
    result.side_to_move_ = ~side_to_move_;
    result.en_passant_ = SQ_NONE;
    result.plies_from_null_ = 0;
    result.half_moves_++;
    result.update_pin_info();

    result.key_ ^= ZOBRIST.side
        ^ (ZOBRIST.enpassant[file_of(en_passant_)] 
                * (en_passant_ != SQ_NONE));

    return result;
}

bool Board::is_valid_move(const Move m) const {
	const Color us = side_to_move_, them = ~us;

	const Square from = from_sq(m), to = to_sq(m);
	const Bitboard from_bb = square_bb(from), to_bb = square_bb(to);

	const Piece moved = piece_on(from), captured = piece_on(to);
    
    if (from == to || moved == NO_PIECE || color_of(moved) != us)
        return false;
    if (captured != NO_PIECE && (color_of(captured) != them
            || type_of(captured) == KING))
        return false;

    /* We know that we move our piece and if any capture 
     * takes place, it was the opponent's piece that gets captured.
     * Now we only need to make sure that 
     * 1) this kind of piece can make that move
     * (i.e. the move is pseudo legal)
     * 2) the move doesn't leave the king in check
     * (i.e. the move is legal)
     * */

    //ocupied board after the moves takes place
    //If the move is a capture, then don't touch
    //the destination square (as it is already occupied)
    Bitboard occupied = combined_ ^ from_bb 
        ^ (to_bb * (captured == NO_PIECE));
    Bitboard enemies = pieces(them) 
        ^ (to_bb * (captured != NO_PIECE));
    Bitboard dsts = 0;
    Square ksq = king_square(us);

	if (const PieceType pt = type_of(moved); pt == PAWN) {
	    const Bitboard my_r3 = relative_rank_bb(us, RANK_3),
	                   my_r8 = relative_rank_bb(us, RANK_8);
        switch (type_of(m)) {
        case NORMAL:
        case PROMOTION:
        {
            dsts = pawn_pushes_bb(us, from) & ~pieces();
            Bitboard bb = (my_r3 & pieces()) & ~square_bb(from);
            bb = (bb << 8) | (bb >> 8);
            dsts &= ~bb; //handle long pushes
            dsts |= pawn_attacks_bb(us, from) & pieces(them);

            if (type_of(m) == PROMOTION)
                dsts &= my_r8;
            break;
        }
        case EN_PASSANT:
        {
            if (en_passant() != SQ_NONE)
                dsts = square_bb(en_passant())
                    & pawn_attacks_bb(us, from);
            const Bitboard cap_bb = square_bb(make_square(
                        file_of(to), rank_of(from)));
            occupied ^= cap_bb;
            enemies ^= cap_bb;
            break;
        }
        default:
            return false;
        }
    } else if (pt == KNIGHT && type_of(m) == NORMAL) {
        dsts = attacks_bb<KNIGHT>(from) & ~pieces(us);
    } else if (pt == KING) {
        ksq = to;
        switch (type_of(m)) {
        case NORMAL:
            dsts = attacks_bb<KING>(from) & ~pieces(us);
            break;
        case CASTLING:
        {
            if (checkers_ || relative_rank(us, to) != RANK_1)
                return false;
            const Bitboard kingside = KINGSIDE_MASK[us] & pieces(),
                           queenside = QUEENSIDE_MASK[us] & pieces();

            Square rk_from, rk_to;
            const Rank rank = rank_of(to);
            const File file = file_of(to);
            const bool
	            can_queenside = queenside_rights(us) & castling_;
            if (const bool can_kingside = kingside_rights(us) & castling_; file == FILE_G && can_kingside &&!kingside) {
                rk_from = make_square(FILE_H, rank);
                rk_to = make_square(FILE_F, rank);
                if (attackers_to(them, sq_shift<EAST>(from),
                        combined_)) return false;
            } else if (file == FILE_C && can_queenside && !queenside) {
                rk_from = make_square(FILE_A, rank);
                rk_to = make_square(FILE_D, rank);
                if (attackers_to(them, sq_shift<WEST>(from), 
                        combined_)) return false;
            } else {
                return false;
            }

            occupied ^= square_bb(rk_from) ^ square_bb(rk_to);
            dsts |= to_bb;
            break;
        }
        default:
            return false;
        }
    } else if (type_of(m) == NORMAL) { //sliders
        dsts = attacks_bb(pt, from, combined_) & ~pieces(us);
        if (type_of(m) != NORMAL)
            return false;
    }

    //Check if the move is pseudo legal
    if (!(dsts & to_bb))
        return false;

    //Finally let's see if the move leaves our king in check
    return !(attackers_to(them, ksq, occupied) & enemies);
}

bool Board::is_quiet(const Move m) const {
	const MoveType mt = type_of(m);
    return (mt == NORMAL && !(square_bb(to_sq(m)) 
        & pieces())) || mt == CASTLING;
}

