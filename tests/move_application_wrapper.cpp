#include <iostream>

#include "../src/engine.cpp"

#define STARTING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int main(int argc, char** argv) {
    create_global_caches();

    Board b = from_fen(STARTING_FEN);
    for (int i = 1; i < argc; i++) {
        string value = argv[i];
        Move m {
            .start = (uint8_t) ((value[0]-'a') + (value[1]-'1')*8),
            .end   = (uint8_t) ((value[2]-'a') + (value[3]-'1')*8),
        };
        if (value.length() > 4) {
            if (value[4] == 'q') m.promotion = Pieces::QUEEN;
            if (value[4] == 'r') m.promotion = Pieces::ROOK;
            if (value[4] == 'b') m.promotion = Pieces::BISHOP;
            if (value[4] == 'n') m.promotion = Pieces::KNIGHT;
        }
        if ((b.pieces[m.start] & Pieces::TYPE_MASK) == Pieces::PAWN && abs((int)m.end - (int)m.start) == 16) {
            m.en_passant = (uint8_t)((int)m.start + ((int)m.end - (int)m.start) / 2);
        }

        apply_move(&b, &m);
    }

    cout << to_fen(&b) << endl;
    return 0;
}