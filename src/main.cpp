#include <iostream>

#include "engine.cpp"

#define STARTING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

using namespace std;

int count_moves(Board* board, int depth) {
    if (depth == 0) return 1;

    int acc = 0;
    int count;
    Move* moves = get_all_moves(board, &count);
    for (int i = 0; i < count; i++) {
        Board* new_board = copy_board(board);
        apply_move(new_board, &moves[i]);
        acc += count_moves(new_board, depth - 1);
    }
    return acc;
}

void print_moves(Board* board) {
    int count;
    Move* moves = get_all_moves(board, &count);
    for (int i = 0; i < count; i++) {
        cout << loc_to_string(moves[i].start) << /*" -> " <<*/ loc_to_string(moves[i].end) << endl;
    }
}

int main() {
    gen_global_caches();

    Board b = from_fen("r1bqkbnr/pppp1ppp/8/1B6/2PpP3/8/PP1P1PPP/RNBQK2R b KQkq c3 0 5");
    cout << (int)b.fullmove_clock << endl;
    Move m {
        .start = 27,
        .end = 18,
    };
    apply_move(&b, &m);
    cout << to_fen(&b) << endl;
    // int move_count = count_moves(&b, 1);
    // print_moves(&b);

    // cout << "Move count: " << move_count << endl;

    return 0;
}