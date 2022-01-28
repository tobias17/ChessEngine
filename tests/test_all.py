import chess
import chess.pgn
import test_move_generation
import test_move_application

if __name__ == "__main__":
    pgn = open("ficsgamesdb_search_241098.pgn")
    for i in range(100000):
        game = chess.pgn.read_game(pgn)
        # if i < 700: continue
        if not test_move_generation.compare_match(game.mainline_moves()):
            break
        if not test_move_application.compare_match(game.mainline_moves()):
            break
        print(f"Completed match {i+1}")