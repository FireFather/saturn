#ifndef BOARD_HPP
#define BOARD_HPP

#include "../primitives/common.hpp"
#include "../primitives/bitboard.hpp"
#include <string_view>
#include <iosfwd>

class Board {
public:
    Board() = default;

    static Board start_pos();

    bool load_fen(std::string_view fen);

    /*
     * Check that the board contains valid information
     * and everything is synchronized
     * */
    void validate() const;

    [[nodiscard]] Board do_move(Move m) const;
    [[nodiscard]] Board do_null_move() const;

    /*
     * Used for:
     * 1. Checking moves probed from TT
     * 2. Checking moves from various move ordering structures
     * */
    [[nodiscard]] bool is_valid_move(Move m) const;

    [[nodiscard]] bool see_ge(Move m, int threshold = 0) const;

    [[nodiscard]] bool is_quiet(Move m) const;

    void update_pin_info();

    [[nodiscard]] uint64_t mat_key() const;
    [[nodiscard]] bool is_material_draw() const;
    [[nodiscard]] bool has_nonpawns(Color c) const;

    [[nodiscard]] Bitboard attackers_to(Color c, Square s, Bitboard blockers) const;
    [[nodiscard]] Bitboard attackers_to(Square s, Bitboard blockers) const;

    template<bool Checkers>
    Bitboard slider_blockers(Bitboard sliders, Square s,
            Bitboard &pinners, Bitboard *checkers = nullptr) const;

    void put_piece(Piece p, Square s);
    void remove_piece(Square s);

    [[nodiscard]] Bitboard pieces() const;
    [[nodiscard]] Bitboard pieces(Color c) const;

    [[nodiscard]] Bitboard pieces(PieceType pt) const;
    [[nodiscard]] Bitboard pieces(PieceType pt1, PieceType pt2) const;

    [[nodiscard]] Bitboard pieces(Color c, PieceType pt) const;
    [[nodiscard]] Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;

    [[nodiscard]] Piece piece_on(Square s) const;

    [[nodiscard]] Bitboard checkers() const;
    [[nodiscard]] Bitboard blockers_for_king(Color c) const;
    [[nodiscard]] Bitboard pinners(Color c) const;

    [[nodiscard]] Square king_square(Color c) const;

    [[nodiscard]] Color side_to_move() const;
    [[nodiscard]] Square en_passant() const;
    [[nodiscard]] CastlingRights castling() const;

    [[nodiscard]] uint64_t key() const;

    //returns number of moves since last capture/pawn move
    [[nodiscard]] uint8_t half_moves() const;
    [[nodiscard]] uint8_t plies_from_null() const;

private:
    Bitboard pieces_[PIECE_TYPE_NB];
    Bitboard combined_;
    Bitboard color_combined_[COLOR_NB];

    Bitboard checkers_;
    Bitboard blockers_for_king_[COLOR_NB];
    Bitboard pinners_[COLOR_NB];

    Piece pieces_on_[SQUARE_NB];
    uint64_t mat_key_;

    uint64_t key_;
    CastlingRights castling_;
    Color side_to_move_;
    Square en_passant_;
    uint8_t half_moves_;
    uint8_t plies_from_null_;
};

std::ostream& operator<<(std::ostream& os, const Board &b);

#endif
