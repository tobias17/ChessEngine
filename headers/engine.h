#ifndef __ENGINE_H__
#define __ENGINE_H__

// 0b-wbdhppp
// w = is on white
// b = is on black
// d = can move on diag
// h = can move on horz/vert
// ppp = piece id
namespace Pieces {
    const uint8_t PAWN   = 0b0000001;
    const uint8_t ROOK   = 0b0001010;
    const uint8_t KNIGHT = 0b0000011;
    const uint8_t BISHOP = 0b0010100;
    const uint8_t QUEEN  = 0b0011101;
    const uint8_t KING   = 0b0000110;
    const uint8_t BLACK  = 0b0100000;
    const uint8_t WHITE  = 0b1000000;

    const uint8_t DIAG_MASK = 0b0010000;
    const uint8_t HORZ_MASK = 0b0001000;

    const uint8_t TYPE_MASK  = 0b0011111;
    const uint8_t COLOR_MASK = 0b1100000;
};

// 0b----KQkq
namespace Castling {
    const uint8_t BLACK_QUEENSIDE = 0b0001;
    const uint8_t BLACK_KINGSIDE  = 0b0010;
    const uint8_t WHITE_QUEENSIDE = 0b0100;
    const uint8_t WHITE_KINGSIDE  = 0b1000;
};

namespace StartLocs {
    const int WHITE_KING = 4;
    const int WHITE_QUEENSIDE_ROOK = 0;
    const int WHITE_KINGSIDE_ROOK = 7;

    const int BLACK_KING = 60;
    const int BLACK_QUEENSIDE_ROOK = 56;
    const int BLACK_KINGSIDE_ROOK = 63;
}

struct Board {
    uint8_t pieces[64];
    bool white_to_move;
    uint8_t castling;
    uint8_t en_passant;      // index on board "behind" the pawn, -1 means none
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
};

struct PieceLocCache {
    uint8_t* pieces;
    uint8_t* white;
    uint8_t* black;
    uint8_t piece_count;
    uint8_t white_count;
    uint8_t black_count;
};

struct Move {
    uint8_t start;
    uint8_t end;
    uint8_t en_passant = 0xff; // index on board "behind" the pawn, "-1" means none
    uint8_t promotion  = 0x00; // piece type of promotion, 0 means none
};

#endif // __ENGINE_H__