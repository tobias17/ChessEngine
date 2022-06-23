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
        Board new_board = copy_board(board);
        apply_move(&new_board, &moves[i]);
        acc += count_moves(&new_board, depth - 1);
    }
    return acc;
}

void print_moves(Board* board) {
    int count;
    Move* moves = get_all_moves(board, &count);
    for (int i = 0; i < count; i++) {
        cout << loc_to_string(moves[i].start) << loc_to_string(moves[i].end) << endl;
    }
}

int main() {
    create_global_caches();

    Board b = from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    SearchData sd = SearchData {
        .time_ms = 10000,
    };
    int count = 0;
    Move best_move = get_best_move(&b, &sd, &count);
    // int visit_count;
    // alpha_beta(&b, -LARGE_VALUE, LARGE_VALUE, 1, &visit_count);

    return 0;
}