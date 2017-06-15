# -*- coding: utf8 -*-

import db
import time
import voice
import serial
import sunfish
import sunfish_glue

# Initialize the serial port
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.25)  # open serial port

# Load/initialize the voice replacements database, which is used to
# fix some misunderstandings of Google Voice
replacements = db.loadDB('replacements')
if replacements is None:
    replacements = []
    db.saveDB('replacements', replacements)

# This method allows to wait for the Arduino firmware to report the ready state
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

# This method sends a move to the Arduino, in the form
#  <piece>, <dx>, <dy>
# and returns the real *raw* move which has been made on the board, which will
# be in the form
#  <sx>, <sy>, <dx>, <dy>
# Both for the input and the returned move, a list/tuple is used
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
            # Check if the move is not valid
            if any(n < 0 for n in boardMove):
                # If it is not, the move probably is an elimination
                # move, so not taken into consideration
                continue
            return boardMove
        elif tokens[1] == 'ERR_WRONG_MOVE':
            return None
        elif tokens[1] == 'ERR_DESTINATION_OCCUPIED':
            return None
        else:
            return None
    return boardMove

# This method is similar to the one above, but instead sends a
# raw move in the form:
#  <sx>, <sy>, <dx>, <dy>
# TODO: catch and return errors
def sendRawMove(move):
    string = "R" + str(move[0]) + str(move[1]) + str(move[2]) + str(move[3])
    print("[SERIAL] send: \"" + string + "\"")
    ser.flushInput() # Clear the input buffer
    ser.write(string + '\n')

# This method waits for the Arduino to report its move queue empty, meaning
# that all the requested moves have been carried out
# Returns False if a timeout occurred, otherwise returns True if the Arduino
# completed the move within the specified time
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

# This method reads the board pieces matrix from the Arduino and returns it,
# already converted/parsed/etc...
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

# Quick method to print on screen the matrix read with the method above
def printMatrix(m):
    for y in range(8):
        for x in range(8):
            print(m[x][y]),
        print("")

# This method uses the voice replacements database (loaded at the beginning)
# to fix a string caught by Google Voice Recognition (through the custom voice.py
# library)
def fixMove(moveStr):
    result = moveStr.lower()
    for r in replacements:
        if r[0].lower() in result:
            print("[VOICE] replacing \"" + r[0] + "\" with \"" + r[1] + "\"")
            result = result.replace(r[0], r[1])
    return result

# This method parses a string move caught by the voice library (and eventually
# fixed) into a move tuple in the form
#  <piece>, <dx>, <dy>
# The input string must be something like "pedone in B4". Syntax is really strict,
# no flexibility is given.
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

# This method asks the user on the console for a new replacement to add into
# the database
def acquireReplacement():
    print("[DEBUG] acquiring new replacement")
    print("[DEBUG] leave empty to ignore")
    r = raw_input("[DEBUG] replacement in form \"<old>,<new>\": ")
    if r != "":
        tokens = r.split(",")
        replacements.append((tokens[0], tokens[1]))
        db.saveDB('replacements', replacements)

# This method vocally asks the specified player (black/white) for its move,
# and then returns the parsed move already in the form
#  <piece>, <dx>, <dy>
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

# This method is called every time the main loop needs to know what the move
# of one player will be. This method wraps up both the vocal part and the Sunfish
# autoplay algorithm, meaning that depending on <player_type> ('p' or 'c') this
# function will either call the method above to ask the move vocally, or query
# the algorithm for a computer generated move. This translates to a layer of
# abstraction for the main loop that makes it possible not worry about whether a
# player is a human or a computer.
# <player> is either 'black' or 'white'
# <player_type> is either 'p' (human) or 'c' (computer)
# <pos> is a sunfish.Position object representing the game state
# <searcher> is a sunfish.Searcher object associated with <pos>
# Returns:
#  <move>, <pos>
# where <move> is the move either in the form
#  <piece>, <dx>, <dy>
# OR
#  <sx>, <sy>, <dx>, <dy>
# based on the player_type
# while the returned <pos> is the updated position after the move
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

# This method is used simply to reduce the code in the main loop
# It simply gets the move for the specified player, sends it to the Arduino
# and eventually updates the sunfish.Position <pos> with the Arduino parsed move.
# All of this is done while checking for errors.
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
            if pos is not None:
                smove = sunfish_glue.luneburgToSunfishMove(res)
                pos = pos.move(smove).rotate()
                sunfish.print_pos(pos)
        elif len(move) == 4:
            sendRawMove(move)
        waitMoveEnd()
        print("[DEBUG] move finished")
        if pos is not None: sunfish.print_pos(pos)
        return pos

# Main game method, containing the game loop
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

# Main program
if __name__ == "__main__":
    voice.init() # Init voice engine (speech recognition and TTS)

    waitReady() # Wait for the firmware to report ready state

    # Ask for the playing mode
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

    # Enter the game in the specified move
    game(mode)

    # Close the serial at the end
    ser.close()
