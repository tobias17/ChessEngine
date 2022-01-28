#include <iostream>
#include <cstdint>
#include <string>
#include <assert.h>
#include <sstream>
#include <vector>
#include <chrono>

#include "engine.h"

using namespace std;

uint8_t DIST_TO_EDGE[8][64];
uint8_t DIST_IN_DIR[8][64][64]; // dir, source, dest
bool CAN_HOP_ONTO[8][64];
const int DIRECTIONS[8]     = {   7,   8,   9,  1, -7, -8, -9, -1 }; // @Restricion do not modify - must alternate horz/diag, starting with diag, v[i%8] = -v[(i+4)%8]
const int KNIGHT_OFFSETS[8] = { -17, -15, -10, -6,  6, 10, 15, 17 };
const uint8_t PROMOTIONS[4] = { Pieces::QUEEN, Pieces::ROOK, Pieces::BISHOP, Pieces::KNIGHT };
const float LARGE_VALUE = 1000000;

bool is_on_board(int x, int y, int dx, int dy) {
    return x + dx >= 0 && x + dx < 8 && y + dy >= 0 && y + dy < 8;
}

int walk_to_edge(int x, int y, int dx, int dy, int count) {
    if (!is_on_board(x, y, dx, dy)) return count;
    return walk_to_edge(x + dx, y + dy, dx, dy, count + 1);
}

// @Procedure - modifies global caches
void gen_global_caches() {
    // dist to edge
    for (int dir_i = 0; dir_i < 8; dir_i++) {
        int dx = DIRECTIONS[dir_i] % 8;
        int dy = DIRECTIONS[dir_i] / 8;
        if (dx == -7) {
            dx = 1;
            dy = -1;
        } else if (dx == 7) {
            dx = -1;
            dy = 1;
        }

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                DIST_TO_EDGE[dir_i][y*8 + x] = walk_to_edge(x, y, dx, dy, 0);
            }
        }
    }

    // dist in dir
    for (int dir_i = 0; dir_i < 8; dir_i++) {
        for (int start_loc = 0; start_loc < 64; start_loc++) {
            for (int end_loc = 0; end_loc < 64; end_loc++) {
                DIST_IN_DIR[dir_i][start_loc][end_loc] = 0;
            }

            for (int delta_i = 1; delta_i <= DIST_TO_EDGE[dir_i][start_loc]; delta_i++) {
                DIST_IN_DIR[dir_i][start_loc][start_loc + delta_i*DIRECTIONS[dir_i]] = delta_i;
            }
        }
    }

    // can hop onto
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            CAN_HOP_ONTO[0][y*8 + x] = is_on_board(x, y, -1, -2);
            CAN_HOP_ONTO[1][y*8 + x] = is_on_board(x, y,  1, -2);
            CAN_HOP_ONTO[2][y*8 + x] = is_on_board(x, y, -2, -1);
            CAN_HOP_ONTO[3][y*8 + x] = is_on_board(x, y,  2, -1);
            CAN_HOP_ONTO[4][y*8 + x] = is_on_board(x, y, -2,  1);
            CAN_HOP_ONTO[5][y*8 + x] = is_on_board(x, y,  2,  1);
            CAN_HOP_ONTO[6][y*8 + x] = is_on_board(x, y, -1,  2);
            CAN_HOP_ONTO[7][y*8 + x] = is_on_board(x, y,  1,  2);
        }
    }
}

void delete_cache(PieceLocCache cache) {
    delete cache.pieces;
    delete cache.white;
    delete cache.pieces;
}

PieceLocCache get_loc_cache(Board* board) {
    PieceLocCache cache;
    cache.piece_count = 0;
    for (int i = 0; i < 64; i++) {
        if (board->pieces[i]) {
            cache.piece_count++;
            if (board->pieces[i] & Pieces::WHITE) {
                cache.white_count++;
            } else {
                assert(board->pieces[i] & Pieces::BLACK); // double check this is black
                cache.black_count++;
            }
        }
    }
    cache.pieces = new uint8_t[cache.piece_count];
    int pieceIndex = 0;
    int whiteIndex = 0;
    int blackIndex = 0;
    for (int i = 0; i < 64; i++) {
        if (board->pieces[i]) {
            cache.pieces[pieceIndex] = i;
            pieceIndex++;
            if (board->pieces[i] & Pieces::WHITE) {
                cache.white[whiteIndex] = i;
            } else {
                assert(board->pieces[i] & Pieces::BLACK); // double check this is black
                cache.white[blackIndex] = i;
            }
        }
    }
    // make sure we got all pieces
    assert(pieceIndex == cache.piece_count);
    assert(whiteIndex == cache.white_count);
    assert(blackIndex == cache.black_count);
    return cache;
}

string loc_to_string(uint8_t loc) {
    string s= "  ";
    s[0] = (loc % 8 + 'a');
    s[1] = (loc / 8 + '1');
    return s;
}

void apply_move(Board* board, Move* move) {
    assert(move->start >= 0 && move->start < 64);
    assert(move->end   >= 0 && move->end   < 64);
    assert((move->en_passant >= 0 && move->en_passant < 64) || (move->en_passant == (uint8_t)-1));

    if (board->pieces[move->end] || (board->pieces[move->start] & Pieces::TYPE_MASK) == Pieces::PAWN) {
        board->halfmove_clock = 0;
    } else {
        board->halfmove_clock += 1;
    }

    // castling
    if (move->start == StartLocs::WHITE_KING)
        board->castling &= (~Castling::WHITE_QUEENSIDE & ~Castling::WHITE_KINGSIDE);
    if (move->start == StartLocs::BLACK_KING)
        board->castling &= (~Castling::BLACK_QUEENSIDE & ~Castling::BLACK_KINGSIDE);
    if (move->start == StartLocs::WHITE_QUEENSIDE_ROOK || move->end == StartLocs::WHITE_QUEENSIDE_ROOK)
        board->castling &= ~Castling::WHITE_QUEENSIDE;
    if (move->start == StartLocs::WHITE_KINGSIDE_ROOK || move->end == StartLocs::WHITE_KINGSIDE_ROOK)
        board->castling &= ~Castling::WHITE_KINGSIDE;
    if (move->start == StartLocs::BLACK_QUEENSIDE_ROOK || move->end == StartLocs::BLACK_QUEENSIDE_ROOK)
        board->castling &= ~Castling::BLACK_QUEENSIDE;
    if (move->start == StartLocs::BLACK_KINGSIDE_ROOK || move->end == StartLocs::BLACK_KINGSIDE_ROOK)
        board->castling &= ~Castling::BLACK_KINGSIDE;

    // update board pieces
    if ((board->pieces[move->start] & Pieces::TYPE_MASK) == Pieces::KING) {
        if (move->end - move->start == -2) {
            uint8_t rook_start = (board->white_to_move ? StartLocs::WHITE_QUEENSIDE_ROOK : StartLocs::BLACK_QUEENSIDE_ROOK);
            board->pieces[move->start - 1] = board->pieces[rook_start];
            board->pieces[rook_start] = 0x00;
        } else if (move->end - move->start == 2) {
            uint8_t rook_start = (board->white_to_move ? StartLocs::WHITE_KINGSIDE_ROOK : StartLocs::BLACK_KINGSIDE_ROOK);
            board->pieces[move->start + 1] = board->pieces[rook_start];
            board->pieces[rook_start] = 0x00;
        }
    }
    // en passant
    if (board->en_passant == move->end && (board->pieces[move->start] & Pieces::TYPE_MASK) == Pieces::PAWN) {
        board->pieces[move->end + (board->white_to_move ? -8 : 8)] = 0x00;
    }
    board->pieces[move->end] = board->pieces[move->start];
    board->pieces[move->start] = 0x00;
    board->en_passant = move->en_passant;
    if (move->promotion) {
        board->pieces[move->end] &= ~Pieces::TYPE_MASK;
        board->pieces[move->end] |= move->promotion;
    }

    board->white_to_move = !board->white_to_move;
    board->fullmove_clock += (board->white_to_move);
}

Board copy_board(Board* board) {
    Board new_board = Board {
        .white_to_move = board->white_to_move,
        .castling = board->castling,
        .en_passant = board->en_passant,
        .halfmove_clock = board->halfmove_clock,
        .fullmove_clock = board->fullmove_clock,
    };
    for (int i = 0; i < 64; i++) {
        new_board.pieces[i] = board->pieces[i];
    }
    return new_board;
}

struct PinningData {
    uint8_t pin_src;
    uint8_t pin_dir_i;
};

bool does_stop_single_check(PinningData check, uint8_t team_king_loc, uint8_t end_loc, int king_dist) {
    if (end_loc == check.pin_src) return true;
    if (check.pin_dir_i == (uint8_t) -1) return false;
    int pin_dist = DIST_IN_DIR[check.pin_dir_i][check.pin_src][end_loc];
    if (pin_dist == 0 || pin_dist > king_dist) return false;
    return true;
}

Move* get_all_moves(Board* board, int* count) {
    assert(board); assert(count);
    
    uint8_t team_color = board->white_to_move ? Pieces::WHITE : Pieces::BLACK;
    uint8_t other_color = board->white_to_move ? Pieces::BLACK : Pieces::WHITE;
    uint8_t team_king_loc;

    vector<PinningData> checks;
    uint8_t pins[64]; // hold direction of pin
    uint8_t seen[64];

    for (int i = 0; i < 64; i++) {
        pins[i] = 0;
        seen[i] = 0;
    }

    // find out what squares can be attacked by opposing pieces
    [board, other_color, /* -> */ &seen] {
        for (int start_loc = 0; start_loc < 64; start_loc++) {
            uint8_t start_piece = board->pieces[start_loc];
            uint8_t start_type = (start_piece & Pieces::TYPE_MASK);
            if ((start_piece & Pieces::COLOR_MASK) == other_color) {
                if (start_type == Pieces::KNIGHT) {
                    for (int dir_i = 0; dir_i < 8; dir_i++) {
                        if (!CAN_HOP_ONTO[dir_i][start_loc]) continue;
                        int end_loc = start_loc + KNIGHT_OFFSETS[dir_i];
                        seen[end_loc] = 1;
                    }
                } else if (start_type == Pieces::PAWN) {
                    int end_loc = start_loc + (board->white_to_move ? -8 : 8); // flipped since looking at other team
                    if (end_loc >= 0 && end_loc < 64) {
                        for (int dx = -1; dx <= 1; dx += 2) {
                            if ((end_loc % 8 + dx) < 0 || (end_loc % 8 + dx) >= 8) continue;
                            seen[end_loc + dx] = 1;
                        }
                    }
                } else if (start_type == Pieces::KING) {
                    for (int dir_i = 0; dir_i < 8; dir_i++) {
                        if (1 <= DIST_TO_EDGE[dir_i][start_loc]) {
                            seen[start_loc + DIRECTIONS[dir_i]] = 1;
                        }
                    }
                } else {
                    bool can_diag = start_piece & Pieces::DIAG_MASK;
                    bool can_horz = start_piece & Pieces::HORZ_MASK;
                    assert(can_diag || can_horz);

                    for (int d = can_diag ? 0 : 1; d < 8; d += (can_diag && can_horz ? 1 : 2)) {
                        for (int i = 1; i <= DIST_TO_EDGE[d][start_loc]; i++) {
                            uint8_t end_loc = start_loc + i * DIRECTIONS[d];
                            uint8_t end_piece = board->pieces[end_loc];
                            seen[end_loc] = 1;
                            if (end_piece) break;
                        }
                    }
                }
            }
        }
    } ();

    // cout << "Seen:" << endl;
    // for (int y = 7; y >= 0; y--) {
    //     for (int x = 0; x < 8; x++) {
    //         cout << (seen[y*8 + x] == 1 ? "X" : "-");
    //     }
    //     cout << endl;
    // }
    // cout << endl;



    // find out what pieces are being pinned and what checks are in play
    [board, team_color, other_color, /* -> */ &team_king_loc, &checks, &pins] {
        for (uint8_t start_loc = 0; start_loc < 64; start_loc++) {
            uint8_t start_piece = board->pieces[start_loc];
            if ((start_piece & team_color) && (start_piece & Pieces::TYPE_MASK) == Pieces::KING) {
                team_king_loc = start_loc;
                for (int dir_i = 0; dir_i < 8; dir_i++) {
                    // line moves
                    int pin_loc = -1;
                    for (int i = 1; i <= DIST_TO_EDGE[dir_i][start_loc]; i++) {
                        int end_loc = start_loc + i * DIRECTIONS[dir_i];
                        uint8_t end_piece = board->pieces[end_loc];
                        if (end_piece) {
                            if (end_piece & team_color) {
                                // our team
                                if (pin_loc >= 0) break;
                                pin_loc = end_loc;
                            } else {
                                // other team
                                uint8_t dir_mask = (dir_i % 2 == 0) ? Pieces::DIAG_MASK : Pieces::HORZ_MASK;
                                if (end_piece & dir_mask) {
                                    if (pin_loc >= 0) {
                                        pins[pin_loc] = (uint8_t)abs(DIRECTIONS[dir_i]);
                                    } else {
                                        checks.push_back(PinningData{
                                            .pin_src   = (uint8_t) end_loc,
                                            .pin_dir_i = (uint8_t) ((dir_i + 4) % 8), // flips dir (since we were moving from king out)
                                        });
                                    }
                                } else if ((end_piece & Pieces::TYPE_MASK) == Pieces::PAWN && i == 1 && dir_i % 2 == 0) { // (dir_i % 2 == 0) is true when dir is diag)
                                    if (pin_loc < 0 && (dir_i < 4 || !board->white_to_move) && (dir_i >= 4 || board->white_to_move)) {
                                        checks.push_back(PinningData{
                                            .pin_src   = (uint8_t) end_loc,
                                            .pin_dir_i = (uint8_t) -1, // treat pawns checks like knight checks
                                        });
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // knight moves
                    if (CAN_HOP_ONTO[dir_i][start_loc]) {
                        uint8_t end_piece = board->pieces[start_loc + KNIGHT_OFFSETS[dir_i]];
                        if ((end_piece & other_color) && ((end_piece & Pieces::TYPE_MASK) == Pieces::KNIGHT)) {
                            checks.push_back(PinningData{
                                .pin_src   = (uint8_t) (start_loc + KNIGHT_OFFSETS[dir_i]),
                                .pin_dir_i = (uint8_t) -1,
                            });
                        }
                    }
                }

                // since we found the king, break the search
                break;
            }
        }
    } ();

    // cout << "checks:" << endl;
    // for (int i = 0; i < checks.size(); i++) {
    //     cout << "src: " << (int)checks[i].pin_src << ", dir: " << (int)checks[i].pin_dir_i << endl;
    // }
    // cout << endl;

    // cout << "pins:" << endl;
    // for (int i = 0; i < 64; i++) {
    //     if (pins[i] >= 0 && pins[i] < 64) cout << "i: " << i << ", l: " << (int)pins[i] << endl;
    // }
    // cout << endl;



    vector<Move> moves;

    // find out what moves can be made
    [board, team_color, other_color, team_king_loc, checks, pins, seen, /* -> */ &moves] {
        int single_check_king_dist = 0;
        if (checks.size() == 1 && checks[0].pin_dir_i != (uint8_t) -1) {
            single_check_king_dist = DIST_IN_DIR[checks[0].pin_dir_i][checks[0].pin_src][team_king_loc];
        }

        for (uint8_t start_loc = 0; start_loc < 64; start_loc++) {
            uint8_t start_piece = board->pieces[start_loc];
            if (!(start_piece & team_color)) continue;
            uint8_t start_type  = start_piece & Pieces::TYPE_MASK;

            if (start_type == Pieces::KNIGHT && checks.size() <= 1) {
                if (pins[start_loc]) continue;
                for (int dir_i = 0; dir_i < 8; dir_i++) {
                    if (!CAN_HOP_ONTO[dir_i][start_loc]) continue;
                    int end_loc = start_loc + KNIGHT_OFFSETS[dir_i];
                    if (board->pieces[end_loc] & team_color) continue;
                    if (checks.size() > 0 && !does_stop_single_check(checks[0], team_king_loc, end_loc, single_check_king_dist)) continue;

                    moves.push_back(Move{
                        .start = start_loc,
                        .end = (uint8_t) end_loc,
                    });
                }
            } else if (start_type == Pieces::PAWN && checks.size() <= 1) {
                // single move
                int move_delta = board->white_to_move ? 8 : -8;
                for (int dx = -1; dx <= 1; dx++) {
                    if (pins[start_loc] && abs(move_delta + dx) != pins[start_loc]) continue;
                    int end_loc = start_loc + move_delta;
                    if (end_loc < 0 || end_loc > 64) continue;
                    if (end_loc % 8 + dx < 0 || end_loc % 8 + dx >= 8) continue;
                    end_loc += dx;
                    bool is_en_passant = end_loc == board->en_passant;
                    if (dx == 0) { if (board->pieces[end_loc]) continue; }
                    else { if ( !(board->pieces[end_loc] & other_color) && !is_en_passant ) continue; }
                    if (checks.size() > 0 && !does_stop_single_check(checks[0], team_king_loc, end_loc, single_check_king_dist)) {
                        if (!is_en_passant) continue;
                        if (checks[0].pin_src < 24) continue;
                        if (checks[0].pin_src >= 40) continue;
                        if (checks[0].pin_src + move_delta != board->en_passant) continue;
                    }

                    if (end_loc / 8 == 0 || end_loc / 8 == 7) {
                        for (int prom_i = 0; prom_i < 4; prom_i++) {
                            moves.push_back(Move{
                                .start = start_loc,
                                .end = (uint8_t) end_loc,
                                .promotion = PROMOTIONS[prom_i],
                            });
                        }
                    } else {
                        moves.push_back(Move{
                            .start = start_loc,
                            .end = (uint8_t) end_loc,
                        });
                    }
                }
                
                // double move
                if ( (!pins[start_loc] || abs(move_delta) == pins[start_loc]) && (board->white_to_move ? (start_loc >= 8 && start_loc < 16) : (start_loc >= 48 && start_loc < 56)) ) {
                    uint8_t end_loc = (uint8_t) (start_loc + 2*move_delta);
                    if (!board->pieces[start_loc + move_delta] && !board->pieces[end_loc]) {
                        if (checks.size() > 0 && !does_stop_single_check(checks[0], team_king_loc, end_loc, single_check_king_dist)) continue; // fine here, done generating for this square
                        moves.push_back(Move{
                            .start = start_loc,
                            .end = end_loc,
                            .en_passant = (uint8_t) (start_loc + move_delta),
                        });
                    }
                }
            } else if (start_type == Pieces::KING) {
                // normal movement
                for (int dir_i = 0; dir_i < 8; dir_i++) {
                    if (DIST_TO_EDGE[dir_i][start_loc] >= 1) {
                        uint8_t end_loc = start_loc + DIRECTIONS[dir_i];
                        uint8_t end_piece = board->pieces[end_loc];
                        if (end_piece & team_color) continue;
                        if (seen[end_loc]) continue;
                        
                        bool leaves_check = true;
                        for (int i = 0; i < checks.size(); i++) {
                            if (checks[i].pin_dir_i == (uint8_t) -1) continue;
                            if (abs(DIRECTIONS[dir_i]) == abs(DIRECTIONS[checks[i].pin_dir_i]) && end_loc != checks[i].pin_src) {
                                leaves_check = false;
                                break;
                            }
                        }
                        if (!leaves_check) continue;

                        moves.push_back(Move{
                            .start = start_loc,
                            .end = end_loc,
                        });
                    }
                }

                // castling
                if (checks.size() > 0) continue;
                for (int dir = 1; dir >= -1; dir -= 2) {
                    if (dir ==  1 && !(board->castling & (team_color == Pieces::WHITE ? Castling::WHITE_KINGSIDE  : Castling::BLACK_KINGSIDE)))  continue;
                    if (dir == -1 && !(board->castling & (team_color == Pieces::WHITE ? Castling::WHITE_QUEENSIDE : Castling::BLACK_QUEENSIDE))) continue;
                    if (board->pieces[start_loc +   dir]) continue;
                    if (board->pieces[start_loc + 2*dir]) continue;
                    if (dir == -1 && board->pieces[start_loc + 3*dir]) continue;
                    if (seen[start_loc +   dir]) continue;
                    if (seen[start_loc + 2*dir]) continue;
                    moves.push_back(Move{
                        .start = start_loc,
                        .end = (uint8_t) ((int)start_loc + (2 * dir)),
                    });
                }
            } else if (checks.size() <= 1) { // bishop, rook, queen
                bool can_diag = start_piece & Pieces::DIAG_MASK;
                bool can_horz = start_piece & Pieces::HORZ_MASK;
                assert(can_diag || can_horz);

                for (int dir_i = can_diag ? 0 : 1; dir_i < 8; dir_i += (can_diag && can_horz ? 1 : 2)) {
                    if (pins[start_loc] > 0 && abs(DIRECTIONS[dir_i]) != pins[start_loc]) continue;
                    for (int i = 1; i <= DIST_TO_EDGE[dir_i][start_loc]; i++) {
                        uint8_t end_loc = start_loc + i * DIRECTIONS[dir_i];
                        uint8_t end_piece = board->pieces[end_loc];
                        if (end_piece & team_color) break;
                        if (checks.size() > 0 && !does_stop_single_check(checks[0], team_king_loc, end_loc, single_check_king_dist)) {
                            if (end_piece) break;
                            continue;
                        };

                        moves.push_back(Move{
                            .start = start_loc,
                            .end =  end_loc,
                        });
                        if (end_piece) break;
                    }
                }
            }
        }

    } ();

    *count = moves.size();
    Move* retVal = new Move[moves.size()];
    copy(moves.begin(), moves.end(), retVal);

    return retVal;
}

string board_to_string(Board* board) {
    stringstream ss;
    ss << "Board:\n  A B C D E F G H\n";

    for (int y = 7; y >= 0; y--) {
        ss << (y + 1) << ' ';
        for (int x = 0; x < 8; x++) {
            uint8_t piece = board->pieces[y*8 + x];
            uint8_t rem = piece & Pieces::TYPE_MASK;
            if (piece == 0) {
                ss << "- ";
                continue;
            }
            uint8_t type = piece & Pieces::TYPE_MASK;
            uint8_t isWhite = piece & Pieces::WHITE;
            if      (type == Pieces::PAWN)   { ss << (isWhite ? "P " : "p "); }
            else if (type == Pieces::ROOK)   { ss << (isWhite ? "R " : "r "); }
            else if (type == Pieces::KNIGHT) { ss << (isWhite ? "N " : "n "); }
            else if (type == Pieces::BISHOP) { ss << (isWhite ? "B " : "b "); }
            else if (type == Pieces::QUEEN)  { ss << (isWhite ? "Q " : "q "); }
            else if (type == Pieces::KING)   { ss << (isWhite ? "K " : "k "); }
            else                             { ss << "? "; }
        }
        ss << '\n';
    }

    ss << '\n' << (board->white_to_move ? "w " : "b ");
    if (!board->castling) ss << '-';
    if (board->castling & Castling::WHITE_KINGSIDE)  ss << 'K';
    if (board->castling & Castling::WHITE_QUEENSIDE) ss << 'Q';
    if (board->castling & Castling::BLACK_KINGSIDE)  ss << 'k';
    if (board->castling & Castling::BLACK_QUEENSIDE) ss << 'q';
    ss << ' ';
    if (board->en_passant < 0) ss << "- ";
    else ss << (board->en_passant < 0 ? '-' : board->en_passant) << ' ';
    ss << (board->halfmove_clock ? board->halfmove_clock : 0) << ' ' << board->fullmove_clock;

    return ss.str();
}

string move_to_string(Move* move) {
    stringstream ss;
    ss << loc_to_string(move->start) << loc_to_string(move->end);
    if (move->promotion) {
        switch (move->promotion) {
            case Pieces::QUEEN:  ss << "q"; break;
            case Pieces::ROOK:   ss << "r"; break;
            case Pieces::BISHOP: ss << "b"; break;
            case Pieces::KNIGHT: ss << "n"; break;
        }
    }
    return ss.str();
}

Board from_fen(string fen) {
    Board board;

    // set to zero, might not set flags
    board.castling = 0;
    board.en_passant = 0;
    board.halfmove_clock = 0;
    board.fullmove_clock = 0;

    // variables to be used by for loop
    int i = 0;
    int state = 0;
    for (int c = 0; c < fen.length(); c++) {
        char fenchar = fen[c];
        switch (state) {
            case 0:
                // find pieces layout on the board
                switch (tolower(fenchar)) {
                    case 'p': board.pieces[(7-i/8)*8 + i%8] = Pieces::PAWN   | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case 'r': board.pieces[(7-i/8)*8 + i%8] = Pieces::ROOK   | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case 'n': board.pieces[(7-i/8)*8 + i%8] = Pieces::KNIGHT | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case 'b': board.pieces[(7-i/8)*8 + i%8] = Pieces::BISHOP | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case 'q': board.pieces[(7-i/8)*8 + i%8] = Pieces::QUEEN  | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case 'k': board.pieces[(7-i/8)*8 + i%8] = Pieces::KING   | (isupper(fenchar) ? Pieces::WHITE : Pieces::BLACK); i++; break;
                    case '/': assert(i % 8 == 0); break; // char not necessary, assert to make sure we are reading correctly
                    default:
                        if (fenchar >= '1' && fenchar <= '8') {
                            for (int j = 0; j < fenchar - '0'; j++) {
                                board.pieces[(7-i/8)*8 + i%8 + j] = 0;
                            }
                            i += fenchar - '0';
                            break;
                        }
                        assert(false); // character was not valid
                }
                if (i >= 64) {
                    state++;
                    c++;
                    assert(fen[c] == ' '); // make sure we are skipping over the space
                }
                break;
            case 1:
                // active color
                assert(fenchar == 'w' || fenchar == 'b'); // make sure we are getting the correct digits
                board.white_to_move = (fenchar == 'w');
                state++;
                c++;
                assert(fen[c] == ' '); // make sure we are skipping over the space
                break;
            case 2:
                // castling availability
                switch (fenchar) {
                    case '-':
                        state++;
                        c++;
                        assert(fen[c] == ' '); // make sure we are skipping over the space
                        break;
                    case 'k': board.castling |= Castling::BLACK_KINGSIDE;  break;
                    case 'q': board.castling |= Castling::BLACK_QUEENSIDE; break;
                    case 'K': board.castling |= Castling::WHITE_KINGSIDE;  break;
                    case 'Q': board.castling |= Castling::WHITE_QUEENSIDE; break;
                    case ' ': state++; break;
                    default: assert(false); // character was not valid
                }
                break;
            case 3:
                // en passant
                if (fenchar == '-') {
                    board.en_passant = -1;
                    state++;
                    c++;
                    assert(fen[c] == ' '); // make sure we are skipping over the space
                } else if (fenchar >= 'a' && fenchar <= 'h') {
                    board.en_passant += (fenchar - 'a');
                } else if (fenchar >= '1' && fenchar <= '8') {
                    board.en_passant += 8 * (fenchar - '1');
                } else if (fenchar == ' ') {
                    state++;
                } else {
                    assert(false); // character was not valid
                }
                break;
            case 4:
                // halfmove clock
                assert(fenchar >= '0' && fenchar <= '9'); // make sure count is integer
                if (fen[c+1] == ' ') {
                    // single digit number
                    board.halfmove_clock = fenchar - '0';
                } else if (fen[c+2] == ' ') {
                    // double digit number
                    assert(fen[c+1] >= '0' && fen[c+1] <= '9'); // make sure count is integer
                    board.halfmove_clock = 10 * (fenchar - '0') + (fen[c+1] - '0');
                    c++; // increment c 1 more, used 2 digits chars from string
                } else {
                    // triple digit number
                    assert(fen[c+1] >= '0' && fen[c+1] <= '9'); // make sure count is integer
                    assert(fen[c+2] >= '0' && fen[c+2] <= '9'); // make sure count is integer
                    board.halfmove_clock = 100 * (fenchar - '0') + 10 * (fen[c+1] - '0') + (fen[c+2] - '0');
                    c += 2; // increment c 2 more, used 3 digits chars from string
                }
                state++;
                c++;
                assert(fen[c] == ' '); // make sure we are skipping over the space
                break;
            case 5:
                // fullmove clock
                assert(fen.length() - c <= 5); // make sure we have no more than 5 digits, 65k is max supported fullmove count
                while (c < fen.length()) {
                    assert(fen[c] >= '0' && fen[c] <= '9');
                    board.fullmove_clock *= 10;
                    board.fullmove_clock += (fen[c] - '0');
                    c++;
                }
                state++;
                break;
        }
    }
    assert(state == 6); // make sure we ran through all the required fields
    return board;
}

string to_fen(Board* board) {
    stringstream ss;

    int empty_count = 0;
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            int index = y*8 + x;
            uint8_t piece = board->pieces[index];
            if (!piece) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    ss << empty_count;
                }
                empty_count = 0;

                char c = ' ';
                uint8_t type = piece & Pieces::TYPE_MASK;
                switch (type) {
                    case Pieces::ROOK:   c = 'r'; break;
                    case Pieces::KNIGHT: c = 'n'; break;
                    case Pieces::BISHOP: c = 'b'; break;
                    case Pieces::QUEEN:  c = 'q'; break;
                    case Pieces::KING:   c = 'k'; break;
                    case Pieces::PAWN:   c = 'p'; break;
                    default: assert(false);
                }
                if ((piece & Pieces::COLOR_MASK) == Pieces::WHITE) {
                    c += ('A' - 'a');
                }
                ss << c;
            }
        }
        if (empty_count > 0) {
            ss << empty_count;
        }
        empty_count = 0;
        if (y > 0) ss << "/";
    }

    ss << (board->white_to_move ? " w " : " b ");
    if (board->castling) {
        if (board->castling & Castling::WHITE_KINGSIDE)  ss << "K";
        if (board->castling & Castling::WHITE_QUEENSIDE) ss << "Q";
        if (board->castling & Castling::BLACK_KINGSIDE)  ss << "k";
        if (board->castling & Castling::BLACK_QUEENSIDE) ss << "q";
    } else {
        ss << "-";
    }
    ss << " " << (board->en_passant != (uint8_t) -1 ? loc_to_string(board->en_passant) : "-");
    ss << " " << (int)board->halfmove_clock;
    ss << " " << (int)board->fullmove_clock;

    return ss.str();
}

namespace Evals {
    float PAWN_VALUE = 1.0;
    float KNIGHT_VALUE = 3.0;
    float BISHOP_VALUE = 3.0;
    float ROOK_VALUE = 5.0;
    float QUEEN_VALUE = 9.0;
}

float quick_eval(Board* board) {
    float eval = 0.0;
    for (int start_loc = 0; start_loc < 64; start_loc++) {
        uint8_t start_piece = board->pieces[start_loc];
        if (start_piece) {
            float color_sign = (board->pieces[start_loc] & Pieces::COLOR_MASK) == Pieces::WHITE ? 1.0 : -1.0;
            switch (board->pieces[start_loc] & Pieces::TYPE_MASK) {
                case Pieces::PAWN:   eval += Evals::PAWN_VALUE   * color_sign; break;
                case Pieces::KNIGHT: eval += Evals::KNIGHT_VALUE * color_sign; break;
                case Pieces::BISHOP: eval += Evals::BISHOP_VALUE * color_sign; break;
                case Pieces::ROOK:   eval += Evals::ROOK_VALUE   * color_sign; break;
                case Pieces::QUEEN:  eval += Evals::QUEEN_VALUE  * color_sign; break;
            }
        }
    }
    return eval;
}

struct Node {
    float value;
    int position;
    Node* next_node = nullptr;
};

/*int* get_eval_order_ll(Board* new_boards, int count, float eval_sign) {
    Node* root_node = nullptr;

    for (int i = 0; i < count; i++) {
        float eval = eval_sign * quick_eval(&new_boards[i]);
        Node* new_node = new Node {
            .value = eval,
            .position = i,
        };
        if (root_node == nullptr || eval > root_node->value) {
            new_node->next_node = root_node;
            root_node = new_node;
            continue;
        }
        Node* curr_node = root_node;
        while (curr_node->next_node != nullptr) {
            if (eval >= curr_node->next_node->value) {
                new_node->next_node = curr_node->next_node;
                curr_node->next_node = new_node;
                break;
            }
            curr_node = curr_node->next_node;
        }
        if (new_node->next_node == nullptr) {
            curr_node->next_node = new_node;
        }
    }

    int* eval_order = new int[count];
    Node* curr_node = root_node;
    for (int i = 0; i < count; i++) {
        eval_order[i] = curr_node->position;
        Node* next_node = curr_node->next_node;
        delete curr_node;
        curr_node = next_node;
    }

    return eval_order;
}*/

int* get_eval_order(Board* new_boards, int count, float eval_sign) {
    int* eval_order    = new int[count];
    float* eval_values = new float[count];

    for (int i = 0; i < count; i++) {
        float eval = eval_sign * quick_eval(&new_boards[i]);

        // bubble insert
        int insert_loc;
        for (insert_loc = 0; insert_loc < i; insert_loc++) {
            if (eval > eval_values[insert_loc]) {
                for (int swap_loc = i; swap_loc > insert_loc; swap_loc--) {
                    eval_order[swap_loc]  = eval_order[swap_loc - 1];
                    eval_values[swap_loc] = eval_values[swap_loc - 1];
                }
                break;
            }
        }
        eval_order[insert_loc]  = i;
        eval_values[insert_loc] = eval;
    }

    delete[] eval_values;
    return eval_order;
}

float alpha_beta(Board* board, float alpha, float beta, int depth, int* visit_count) {
    *visit_count += 1;
    if (depth == 0) {
        return quick_eval(board);
    }

    int count;
    Move* moves = get_all_moves(board, &count);
    float value = board->white_to_move ? -LARGE_VALUE : LARGE_VALUE;

    Board* new_boards = new Board[count];
    for (int i = 0; i < count; i++) {
        new_boards[i] = copy_board(board);
        apply_move(&new_boards[i], &moves[i]);
    }

    int* eval_order = get_eval_order(new_boards, count, board->white_to_move ? 1 : -1);

    for (int i = 0; i < count; i++) {
        float new_board_eval = alpha_beta(&new_boards[eval_order[i]], alpha, beta, depth - 1, visit_count);
        if (board->white_to_move) {
            value = max(value, new_board_eval);
            if (value >= beta) break;
            alpha = max(alpha, value);
        } else {
            value = min(value, new_board_eval);
            if (value <= alpha) break;
            beta = min(beta, value);
        }
    }

    delete[] eval_order;
    return value;
}

struct SearchData {
    int time_ms;
};

Move get_best_move(Board* board, SearchData* search_data) {
    int count;
    Move* moves = get_all_moves(board, &count);
    assert(count > 0);
    int visit_count = 0;

    float best_eval = board->white_to_move ? -LARGE_VALUE : LARGE_VALUE;
    Move best_move;
    auto begin = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; i++) {
        Board new_board = copy_board(board);
        apply_move(&new_board, &moves[i]);
        float eval = alpha_beta(&new_board, -LARGE_VALUE, LARGE_VALUE, 5, &visit_count);
        if ((board->white_to_move && eval > best_eval) || (!board->white_to_move && eval < best_eval)) {
            best_move = moves[i];
            best_eval = eval;
        }
    }

    cout << move_to_string(&best_move) << endl;

    auto end = std::chrono::high_resolution_clock::now();
    cout << "Visited " << visit_count << " board positions in " << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << " ms" << endl;

    return best_move;
}