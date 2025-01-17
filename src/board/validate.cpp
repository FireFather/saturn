#include "board.hpp"
#include "../zobrist.hpp"

void Board::validate() const {
    //Make sure color_combned_ and combined_ are synced
    assert(!(color_combined_[WHITE] & color_combined_[BLACK]));
    assert((color_combined_[WHITE] | color_combined_[BLACK])
            == combined_);

    //color_combined_ and combined_ - synced

    //Check that pieces_ has the same number 
    //of pieces as combined_
    int total_pieces = 0;
    for (PieceType pt = PAWN; pt <= KING; ++pt)
        total_pieces += popcnt(pieces_[pt]);
    assert(total_pieces == popcnt(combined_));

    //Now make sure pieces_ and pieces_on_ are synced
    //with combned_ and color_combined_
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
	    const Bitboard sb = square_bb(s);

	    if (const Piece p = pieces_on_[s]; p != NO_PIECE) {
            assert(is_ok(p));

            assert(color_combined_[color_of(p)] & sb);
            assert(pieces_[type_of(p)] & sb);
        } else {
            assert(!(combined_ & sb));
            assert(!(pieces_[type_of(p)] & sb));
        }
    }

    //color_combined_, combined_, 
    //pieces_, pieces_on_ are synced

    //Let's see if castling_, side_to_move_,
    //end_passant_ are valid

    //Castling
    assert(is_ok(castling_));
    if (castling_ & WHITE_CASTLING)
        assert(pieces_on_[SQ_E1] == W_KING);
    if (castling_ & WHITE_KINGSIDE)
        assert(pieces_on_[SQ_H1] == W_ROOK);
    if (castling_ & WHITE_QUEENSIDE)
        assert(pieces_on_[SQ_A1] == W_ROOK);

    if (castling_ & BLACK_CASTLING)
        assert(pieces_on_[SQ_E8] == B_KING);
    if (castling_ & BLACK_KINGSIDE)
        assert(pieces_on_[SQ_H8] == B_ROOK);
    if (castling_ & BLACK_QUEENSIDE)
        assert(pieces_on_[SQ_A8] == B_ROOK);

    assert(is_ok(side_to_move_));

    //En passant
    if (en_passant_ != SQ_NONE) {
        assert(is_ok(en_passant_));
        const Square s = make_square(file_of(en_passant_), 
                                     relative_rank(~side_to_move_, RANK_4));
        const Piece p = pieces_on_[s];

        assert(p == make_piece(~side_to_move_, PAWN));
    }

    //castling_, side_to_move_, en_passant_ are valid

    //Check that there are kings on the board
    assert(popcnt(pieces(WHITE, KING)) == 1);
    assert(popcnt(pieces(BLACK, KING)) == 1);

    //Make sure checkers_, blockers_for_king_ and pinners_
    //are vaild

    const Square stm_ksq = king_square(side_to_move_);

    const Bitboard attackers = attackers_to(~side_to_move_, stm_ksq, combined_);
    assert(attackers == checkers_); 
    //checkers are ok

    const Bitboard sliders = pieces_[BISHOP] | pieces_[ROOK] 
        | pieces_[QUEEN];
    Bitboard pinners[COLOR_NB];
    Bitboard blockers[COLOR_NB];
    blockers[WHITE] = slider_blockers<false>(sliders & pieces(BLACK),
            king_square(WHITE), pinners[BLACK]);
    blockers[BLACK] = slider_blockers<false>(sliders & pieces(WHITE), 
            king_square(BLACK), pinners[WHITE]);

    assert(blockers_for_king_[WHITE] == blockers[WHITE]);
    assert(blockers_for_king_[BLACK] == blockers[BLACK]);
    //blockers are ok
    
    assert(pinners_[WHITE] == pinners[WHITE]);
    assert(pinners_[BLACK] == pinners[BLACK]);
    //pinners are ok

    //Finally, let's see if the zobrist key is correct
    uint64_t k = 0;
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
	    if (const Piece p = piece_on(s); p != NO_PIECE)
            k ^= ZOBRIST.psq[p][s];
    }

    if (en_passant_ != SQ_NONE)
        k ^= ZOBRIST.enpassant[file_of(en_passant_)];

    k ^= ZOBRIST.castling[castling_];

    if (side_to_move_ == BLACK)
        k ^= ZOBRIST.side;

    assert(key_ == k);
    //position key is correct

    //can't bother checking state info lmao
    //may be iterate through all moves backwards
    //and see if position is valid?
}

