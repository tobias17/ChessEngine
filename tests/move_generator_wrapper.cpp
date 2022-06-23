#include <iostream>

#include "../src/engine.cpp"

void print_moves(Board* board) {
    int count;
    Move* moves = get_all_moves(board, &count);
    for (int i = 0; i < count; i++) {
        cout << loc_to_string(moves[i].start) << /*" -> " <<*/ loc_to_string(moves[i].end);
        if (moves[i].promotion) {
            switch (moves[i].promotion) {
                case Pieces::QUEEN:
                    cout << "q";
                    break;
                case Pieces::ROOK:
                    cout << "r";
                    break;
                case Pieces::BISHOP:
                    cout << "b";
                    break;
                case Pieces::KNIGHT:
                    cout << "n";
                    break;
            }
        }
        cout << endl;
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    string fen = argv[1];

    create_global_caches();

    Board b = from_fen(fen);
    print_moves(&b);
    return 0;
}