import chess
import chess.pgn
import subprocess

def get_fen_cpp(moves):
    cmd = ['../bin/move_application_wrapper']
    for move in moves:
        cmd.append(str(move))
    proc = subprocess.run(cmd, capture_output=True)
    return proc.stdout.decode().strip()

def remove_en_passant(fen):
    values = fen.split(' ')
    return ' '.join(values[0:2] + values[4:5])

def compare_match(moves):
    board = chess.Board()
    all_moves = []
    for move in moves:
        all_moves.append(str(move))
        board.push(move)
        fen_py = board.fen()
        fen_cpp = get_fen_cpp(all_moves)
        if remove_en_passant(fen_py) != remove_en_passant(fen_cpp):
            print(f'fen mismatch at move {len(all_moves)}\npy:  {fen_py}\ncpp: {fen_cpp}')
            board.pop()
            print(f'\nprevious board: {board.fen()}')
            print(f'move made: {str(move)}')
            print(f'all moves: {" ".join(all_moves)}')
            return False
    return True

if __name__ == "__main__":
    pgn = open("ficsgamesdb_search_241098.pgn")
    for i in range(100000):
        game = chess.pgn.read_game(pgn)
        if not compare_match(game.mainline_moves()):
            break
        print(f"Completed match {i+1}")