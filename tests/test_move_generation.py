import chess
import chess.pgn
import subprocess

def get_moves_py(board):
    moves = []
    for move in board.legal_moves:
        moves.append(str(move))
    return moves

def get_moves_cpp(board):
    fen_str = board.fen()
    proc = subprocess.run(['../bin/move_generator_wrapper', fen_str], capture_output=True)
    moves = [v for v in proc.stdout.decode().split('\n') if v]
    return moves

def compare_moves(board):
    if not board.is_valid():
        return True

    moves_py = get_moves_py(board)
    moves_cpp = get_moves_cpp(board)

    identical = True
    for move in moves_py:
        if move not in moves_cpp:
            if identical:
                print(board.fen())
                print(board)
            print(f'Move {move} not in cpp')
            identical = False
    for move in moves_cpp:
        if move not in moves_py:
            if identical:
                print(board.fen())
                print(board)
            print(f'Move {move} not in py')
            identical = False
    return identical

# def compare_trees(board, depth=2):
#     if depth == 0:
#         return True
#     if not compare_moves(board):
#         return False
#     for move in board.legal_moves:
#         board.push(move)
#         if not compare_trees(board, depth - 1):
#             return False
#         board.pop()
#     return True

# starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

# board = chess.Board(starting_fen)
# print(compare_trees(board, depth=4))

def compare_match(moves):
    board = chess.Board()
    for move in moves:
        board.push(move)
        if not compare_moves(board):
            return False
    return True

if __name__ == "__main__":
    pgn = open("ficsgamesdb_search_241098.pgn")
    for i in range(100000):
        if i < 880: continue
        game = chess.pgn.read_game(pgn)
        if not compare_match(game.mainline_moves()):
            break
        print(f"Completed match {i+1}")
