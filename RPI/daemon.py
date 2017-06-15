# -*- coding: utf8 -*-

import db
import time
import voice
import serial
import sunfish
import sunfish_glue

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.25)  # open serial port

replacements = db.loadDB('replacements')
if replacements is None:
    replacements = []
    db.saveDB('replacements', replacements)

'''
replacements = [
    ("invita", "in B3"),
    ("Ingianni ", "in G"),
    ("gianni ", "G"),
    ("Gianni ", "G"),
    ("bidone", "pedone"),
    ("cinque", "5")
]
'''

def waitReady(timeout=15):
    print("[DEBUG] waiting for chessboard to be ready")
    start = time.time()
    while True:
        if (time.time() - start) > timeout:
            return False
        line = ser.readline()
        if line is None:
            continue
        if not "[LOG]" in line:
            continue
        line = line.replace('\n', '').replace('\r', '')
        print("[SERIAL] parsing line: " + line)
        tokens = line.split(' ')
        if tokens is None or len(tokens) < 2:
            continue
        if tokens[1] == 'READY':
            return True

def sendMove(move, timeout=3):
    string = "M" + str(move[0]) + str(move[1]) + str(move[2]) + '0'
    print("[SERIAL] send: \"" + string + "\"")
    ser.flushInput() # Clear the input buffer
    ser.write(string + '\n')
    start = time.time()
    while True:
        if (time.time() - start) > timeout:
            return None
        line = ser.readline()
        if line is None:
            return None
        if not "[LOG]" in line:
            continue
        line = line.replace('\n', '').replace('\r', '')
        print("[SERIAL] parsing line: " + line)
        tokens = line.split(' ')
        if tokens is None or len(tokens) < 2:
            continue
        if tokens[1] == 'MP':
            if len(tokens) != 6:
                continue
            boardMove = []
            boardMove.append(int(tokens[2]))
            boardMove.append(int(tokens[3]))
            boardMove.append(int(tokens[4]))
            boardMove.append(int(tokens[5]))
            return boardMove
        elif tokens[1] == 'ERR_WRONG_MOVE':
            return None
        elif tokens[1] == 'ERR_DESTINATION_OCCUPIED':
            return None
        else:
            return None
    return boardMove

def sendRawMove(move):
    string = "R" + str(move[0]) + str(move[1]) + str(move[2]) + str(move[3])
    print("[SERIAL] send: \"" + string + "\"")
    ser.flushInput() # Clear the input buffer
    ser.write(string + '\n')

def waitMoveEnd(timeout=30):
    start = time.time()
    while True:
        if (time.time() - start) > timeout:
            return False
        line = ser.readline()
        if line is None:
            continue
        if not "[LOG]" in line:
            continue
        line = line.replace('\n', '').replace('\r', '')
        print("[SERIAL] parsing line: " + line)
        tokens = line.split(' ')
        if tokens is None or len(tokens) < 2:
            continue
        if tokens[1] == 'QE':
            return True

def getMatrix():
    ser.flushInput()
    ser.write('X\n')
    r = ser.readline()
    if r is None or r == "":
        return None
    else:
        r = r.replace("[LOG] ", '').replace('\n', '').replace('\r', '').strip()
        matrix = []
        for i in range(8):
            matrix.append([])
            for j in range(8):
                matrix[i].append(' ')
        x = 0
        y = 7
        for c in r:
            # print("c=" + str(c) + ", x=" + str(x) + ", y=" + str(y))
            matrix[x][y] = c
            y -= 1
            if y < 0:
                y = 7
                x += 1
        if x != 8 or y != 7:
            print("[SERIAL] something went wrong getting matrix")
            return None
        else:
            print("[SERIAL] got matrix")
            printMatrix(matrix)
            return matrix

def printMatrix(m):
    for y in range(8):
        for x in range(8):
            print(m[x][y]),
        print("")

def fixMove(moveStr):
    result = moveStr.lower()
    for r in replacements:
        if r[0].lower() in result:
            print("[VOICE] replacing \"" + r[0] + "\" with \"" + r[1] + "\"")
            result = result.replace(r[0], r[1])
    return result

def parseMove(moveStr, player):
    tokens = moveStr.lower().split(" ")
    if len(tokens) != 3:
        return None
    if tokens[1] != "in":
        return None
    piece = None
    if tokens[0] == "pedone":
        piece = 'P'
    elif tokens[0] == "cavallo" or tokens[0] == "cavalli":
        piece = 'C'
    elif tokens[0] == "alfiere":
        piece = 'A'
    elif tokens[0] == "torre":
        piece = 'T'
    elif tokens[0] == "re":
        piece = 'R'
    elif tokens[0] == "regina":
        piece = 'G'
    if piece is None:
        return None
    try:
        if len(tokens[2]) < 2:
            return None
        x = ord(tokens[2][0].lower()) - ord('a')
        if x > 7 or x < 0:
            return None
        y = int(tokens[2][1]) - 1
        if y > 7 or y < 0:
            return None
        x = 7 - x
    except:
        return None

    if player == 'black':
        piece = piece.lower()

    return (piece, x, y)

def acquireReplacement():
    print("[DEBUG] acquiring new replacement")
    print("[DEBUG] leave empty to ignore")
    r = raw_input("[DEBUG] replacement in form \"<old>,<new>\": ")
    if r != "":
        tokens = r.split(",")
        replacements.append((tokens[0], tokens[1]))
        db.saveDB('replacements', replacements)

def askMove(player):
    talkStr = "Turno del giocatore "
    if player == "black":
        talkStr += "nero"
    else:
        talkStr += "bianco"
    voice.talk(talkStr)
    voice.play("sounds/ready.mp3")

    while True:
        moveStr = None
        while moveStr is None:
            moveStr = voice.listen()
        print("[VOICE] understood: " + moveStr)
        fixedMoveStr = fixMove(moveStr)
        print("[VOICE] fixed to: " + fixedMoveStr)
        move = parseMove(fixedMoveStr, player)
        if move is None:
            voice.talk("Non è una mossa valida, riprova")
            acquireReplacement()
            voice.play("sounds/ready.mp3")
        else:
            print("[MOVES] parsed move:", move)
            return move

def getMove(player, player_type, pos=None, searcher=None):
    if player_type == 'p':
        return askMove(player), pos
    elif player_type == 'c':
        print("[AUTOPLAY] the computer is thinking")
        if player == 'black':
            print("[DEBUG] rotating pos because running black player")
            pos = pos.rotate()

        sunfish.print_pos(pos)
        move, score = searcher.search(pos, secs=2)

        pos = pos.move(move)
        if player == 'white':
            pos = pos.rotate()

        if player == 'black':
            lmove = sunfish_glue.sunfishToLuneburgRotatedMove(move)
        else:
            lmove = sunfish_glue.sunfishToLuneburgMove(move)
        return lmove, pos
    else:
        return None, pos

def doPlayer(player, player_type, pos=None, searcher=None):
    print("[doplayer] pos=" + str(pos) + " searcher=" + str(searcher))
    while True:
        move, npos = getMove(player, player_type, pos, searcher)
        pos = npos
        if len(move) == 3:
            res = sendMove(move)
            if res is None:
                continue
            print("[DEBUG] arduino parsed move to " + str(res))
            smove = sunfish_glue.luneburgToSunfishMove(res)
            pos = pos.move(smove).rotate()
            sunfish.print_pos(pos)
        elif len(move) == 4:
            sendRawMove(move)
        waitMoveEnd()
        print("[DEBUG] move finished")
        if pos is not None: sunfish.print_pos(pos)
        return pos

def game(mode):
    white_type = 'p'
    black_type = 'p'

    if mode == 'p_vs_c':
        black_type = 'c'
    if mode == 'c_vs_c':
        white_type = 'c'
        black_type = 'c'

    if mode != 'p_vs_p':
        pos = sunfish.Position(sunfish.initial, 0, (True,True), (True,True), 0, 0)
        searcher = sunfish.Searcher()
    else:
        pos = None
        searcher = None

    while True:
        pos = doPlayer('white', white_type, pos=pos, searcher=searcher)
        pos = doPlayer('black', black_type, pos=pos, searcher=searcher)

if __name__ == "__main__":
    voice.init()

    waitReady()

    time.sleep(1)

    getMatrix()

    voice.talk("Benvenuto, quale modalità vuoi attivare?")
    voice.play("sounds/ready.mp3")

    mode = ''
    while mode == '':
        listen = voice.listen().lower()
        print("[VOICE] understood: " + listen)
        if listen is None:
            continue
        if listen == "giocatore contro giocatore":
            mode = 'p_vs_p'
        elif listen == "giocatore contro computer" or listen == "computer contro giocatore":
            mode = 'p_vs_c'
        elif listen == "computer contro computer":
            mode = 'c_vs_c'
        else:
            voice.talk("Non è una modalità valida, riprova")
            voice.play("sounds/ready.mp3")

    game(mode)

    ser.close()
