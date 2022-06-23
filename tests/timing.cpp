#include <iostream>
#include <chrono>

#include "../src/engine.cpp"

#define STARTING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int main() {
    create_global_caches();

    auto begin = std::chrono::high_resolution_clock::now();

    Move moves[20] = {
        Move{ .start = 12, .end = 28 },
        Move{ .start = 50, .end = 34 },
        Move{ .start = 10, .end = 18 },
        Move{ .start = 51, .end = 35 },
        Move{ .start = 28, .end = 35 },
        Move{ .start = 59, .end = 35 },
        Move{ .start = 11, .end = 27 },
        Move{ .start = 62, .end = 45 },
        Move{ .start = 6,  .end = 21 },
        Move{ .start = 58, .end = 30 },
        Move{ .start = 5,  .end = 12 },
        Move{ .start = 52, .end = 44 },
        Move{ .start = 15, .end = 23 },
        Move{ .start = 30, .end = 39 },
        Move{ .start = 4,  .end = 6  },
        Move{ .start = 57, .end = 42 },
        Move{ .start = 2,  .end = 20 },
        Move{ .start = 34, .end = 28 },
        Move{ .start = 18, .end = 27 },
        Move{ .start = 61, .end = 25 },
    };

    int visit_count = 0;
    Board board = from_fen(STARTING_FEN);
    for (int i = 0; i < 20; i++) {
        apply_move(&board, &moves[i]);
        get_best_move(&board, nullptr, &visit_count);
    }

    auto end = std::chrono::high_resolution_clock::now();
    cout << "Visited " << visit_count << " board positions in " << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << " ms" << endl;

    return 0;
}