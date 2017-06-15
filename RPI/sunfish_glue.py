# This module provides various methods to convert between our own data format and
# representation style for moves and matrix, to the Sunfish algorithm one

translateRules = [
    ('T', 'R'),
    ('C', 'N'),
    ('A', 'B'),
    ('G', 'Q'),
    ('R', 'K'),
    (' ', '.')
]

def luneburgToSunfishMatrix(matrix):
    output = []
    for i in range(12):
        output.append([])
        for j in range(10):
            output[i].append(' ')
        output[i][9] = '\n'
    for y in range(8):
        for x in range(8):
            waslower = matrix[x][y].islower()
            translated = matrix[x][y]
            for r in translateRules:
                if r[0].lower() == matrix[x][y].lower():
                    translated = r[1]
                    if waslower == True:
                        translated = translated.lower()
                    else:
                        translated = translated.upper()
                    break
            output[y + 2][x + 1] = translated
    return output

def sunfishToLuneburgMove(move):
    sy, sx = divmod((119 - move[0]) - 91, 10)
    sy = 7 - (-sy)
    dy, dx = divmod((119 - move[1]) - 91, 10)
    dy = 7 - (-dy)
    return (sx, sy, dx, dy)

def luneburgToSunfishMove(move):
    s = 119 - ((move[0] + (move[1] - 7) * 10) + 91)
    d = 119 - ((move[2] + (move[3] - 7) * 10) + 91)
    return (s, d)

def sunfishToLuneburgRotatedMove(move):
    move = sunfishToLuneburgMove(move)
    return (
        7 - move[0],
        7 - move[1],
        7 - move[2],
        7 - move[3]
    )

# If this module is run standalone as a script, some tests are carried out
if __name__ == "__main__":
    summary = {
        'tests': 0,
        'passed': 0
    }

    def test(name, r, e):
        print("[TEST] \"" + name + "\" ->"),
        summary['tests'] += 1
        if r == e:
            summary['passed'] += 1
            print("*PASS*"),
            print("result=" + str(r))
        else:
            print("ERROR!"),
            print(str(r) + " but expected " + str(e))

    def printSummary():
        print("")
        print("Passed " + str(summary['passed']) + " out of " + str(summary['tests']) + " tests")
        print("")

    # Some test cases
    test("s2l_move_1", sunfishToLuneburgMove((97, 76)), (1, 0, 2, 2))
    test("s2l_move_2", sunfishToLuneburgMove((92, 73)), (6, 0, 5, 2))
    test("l2s_move_1", luneburgToSunfishMove((1, 0, 2, 2)), (97, 76))
    test("l2s_move_2", luneburgToSunfishMove((6, 0, 5, 2)), (92, 73))
    test("s2l_rot_move_1", sunfishToLuneburgRotatedMove((92, 73)), (1, 7, 2, 5))
    test("s2l_rot_move_2", sunfishToLuneburgRotatedMove((84, 64)), (3, 6, 3, 4))

    printSummary();
