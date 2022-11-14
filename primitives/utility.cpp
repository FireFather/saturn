#include "utility.hpp"
#include <ostream>
#include "../parse_helpers.hpp"
#include "../board/board.hpp"

Square square_from_str(const std::string_view sv) {
    if (sv.size() < 2)
        return SQ_NONE;
    const int
	    rank = sv[1] - '1';
    if (const int file = to_lower(sv[0]) - 'a'; file >= 0 && file < 8 && rank >= 0 && rank < 8)
        return static_cast<Square>(file + rank * 8);
    return SQ_NONE;
}

Color color_from_str(std::string_view sv) {
    if (sv.size() < 5)
        return COLOR_NONE;
    sv = sv.substr(0, 5);
    if (istr_equal(sv, "white"))
        return WHITE;
    if (istr_equal(sv, "black"))
	    return BLACK;

    return COLOR_NONE;
}

Piece piece_from_str(const std::string_view sv) {
    if (sv.empty())
        return NO_PIECE;
    const Color c = is_upper(sv[0]) ? WHITE : BLACK;
    switch (to_lower(sv[0])) {
    case 'p':
        return make_piece(c, PAWN);
    case 'n':
        return make_piece(c, KNIGHT);
    case 'b':
        return make_piece(c, BISHOP);
    case 'r':
        return make_piece(c, ROOK);
    case 'q':
        return make_piece(c, QUEEN);
    case 'k':
        return make_piece(c, KING);
    default:
        return NO_PIECE;
    }
}

PieceType ptype_from_str(const std::string_view sv) {
	const Piece p = piece_from_str(sv);
    return p != NO_PIECE ? type_of(p) : NO_PIECE_TYPE;
}

CastlingRights castling_from_str(std::string_view sv) {
    auto has = [sv](const char ch) { 
        return sv.find(ch) != std::string_view::npos; 
    };

    CastlingRights cr = NO_CASTLING;
    if (has('K'))
        cr = static_cast<CastlingRights>(cr | WHITE_KINGSIDE);
    if (has('Q'))
        cr = static_cast<CastlingRights>(cr | WHITE_QUEENSIDE);
    if (has('k'))
        cr = static_cast<CastlingRights>(cr | BLACK_KINGSIDE);
    if (has('q'))
        cr = static_cast<CastlingRights>(cr | BLACK_QUEENSIDE);

    return cr;
}

Move move_from_str(const std::string_view sv) {
    if (sv.size() < 4)
        return MOVE_NONE;
    const Square from = square_from_str(sv),
                 to = square_from_str(sv.substr(2));

    PieceType prom = NO_PIECE_TYPE;
    switch (sv.size() >= 5 ? to_lower(sv[4]) : 0) {
    case 'n': prom = KNIGHT; break;
    case 'b': prom = BISHOP; break;
    case 'r': prom = ROOK; break;
    case 'q': prom = QUEEN; break;
    default: break;
    }

    if (from == SQ_NONE || to == SQ_NONE || from == to)
        return MOVE_NONE;

    return prom == NO_PIECE_TYPE ? make_move(from, to)
        : make<PROMOTION>(from, to, prom);
}

Move move_from_str(const Board &b, const std::string_view sv) {
    Move m = move_from_str(sv);
    if (m == MOVE_NONE)
        return m;

    const Square from = from_sq(m), to = to_sq(m);
    const File ft = file_of(to);
    if (const File ff = file_of(from); b.king_square(b.side_to_move()) == from 
        && ff == FILE_E && (ft == FILE_C || ft == FILE_G))
    {
        m = make<CASTLING>(from, to);
    }

    if (b.en_passant() == to 
            && type_of(b.piece_on(from)) == PAWN)
        m = make<EN_PASSANT>(from, to);
        
    return b.is_valid_move(m) ? m : MOVE_NONE;
}

std::ostream& operator<<(std::ostream& os, const Square s) {
    if (!is_ok(s)) {
        os << "SQ_NONE";
        return os;
    }

    const char file_ch = file_of(s) + 'a',
               rank_ch = rank_of(s) + '1';

    os << file_ch << rank_ch;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Color c) {
    switch (c) {
    case WHITE: os << "white"; break;
    case BLACK: os << "black"; break;
    default: os << "COLOR_NONE"; break;
    }

    return os;
}

static const char* PIECE_NAMES[] = {
    "pawn", "knight", "bishop", "rook", "queen", "king"
};

std::ostream& operator<<(std::ostream& os, const PieceType pt) {
    if (pt == NO_PIECE_TYPE) {
        os << "NO_PIECE_TYPE";
        return os;
    }

    os << PIECE_NAMES[pt];
    return os;
}

std::ostream& operator<<(std::ostream& os, const Piece p) {
    if (p == NO_PIECE) {
        os << "NO_PIECE";
        return os;
    }

    os << (color_of(p) == WHITE ? "w" : "b") << type_of(p);
    return os;
}

std::ostream& operator<<(std::ostream& os, const CastlingRights cr) {
    if (cr == NO_CASTLING) {
        os << "NO_CASTLING";
        return os;
    }

    if (cr & WHITE_KINGSIDE)
        os << 'K';
    if (cr & WHITE_QUEENSIDE)
        os << 'Q';
    if (cr & BLACK_KINGSIDE)
        os << 'k';
    if (cr & BLACK_QUEENSIDE)
        os << 'q';

    return os;
}

std::ostream& operator<<(std::ostream& os, const Move m) {
    if (m == MOVE_NONE) {
        os << "MOVE_NONE";
        return os;
    }

    os << from_sq(m) << to_sq(m);
    if (type_of(m) == PROMOTION) {
        constexpr char proms[] = { 'n', 'b', 'r', 'q' };
        os << proms[prom_type(m) - KNIGHT];
    }

    return os;
}

std::ostream& operator<<(std::ostream &os, BBPretty bbp) {
	const Bitboard bb = bbp.bb;
    os << "+---+---+---+---+---+---+---+---+\n";
    for (int r = RANK_8; r >= RANK_1; --r) {
        os << "| ";
        for (int f = FILE_A; f <= FILE_H; ++f) {
	        const bool occupied = bb & square_bb(make_square(static_cast<File>(f), static_cast<Rank>(r)));
            os << (occupied ? "X | " : "  | ");
        }
        os << static_cast<char>('1' + r) << '\n';
        os << "+---+---+---+---+---+---+---+---+\n";
    }

    os << "  a   b   c   d   e   f   g   h\n";

    return os;
}

std::ostream& operator<<(std::ostream& os, const Score s) {
	if (const int x = s.value; abs(x) > VALUE_MATE - 100) {
        int moves = (VALUE_MATE - abs(x) + 1) 
            * (x > 0 ? 1 : -1);
        moves /= 2;
        os << "mate " << moves;
    } else {
        os << "cp " << x;
    }

    return os;
}

