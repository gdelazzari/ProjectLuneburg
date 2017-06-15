#include "Steppers.h"
#include "Board.h"
#include "Common.h"

String serialbuf = "";

void setup() {
  Serial.begin(115200);

  Board.begin(7, 9, 8, 10, 0, 0, 0, 13, 5, 6);

  Board.setMovingMicrosteps(4);
  Board.setMovingSpeed(300);

  Board.attachLogger(Serial);

  Serial.println("Going home...");

  Board.goHome();

  Serial.println("Ready for commands");
  Serial.println("[LOG] READY");

  Board.printMatrix(Serial);
}

void loop() {
  manageSerial();

  Board.handle();
}

void manageSerial(void) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n') {
      serialbuf += c;
    } else {
      parseCommand(serialbuf);
      serialbuf = "";
    }
  }
}

void parseCommand(String command) {
  /*
    Supported commands:

    M: move piece <name> to <x>, <y> differentiating multiple candidates by <i>
    R: raw move from <sx>, <sy> to <dx>, <dy>
    X: get matrix content

    Examples (in order):
    "MP220" (move 'pedone' to 2, 2 with i=0)
    "R2234" (move from 2, 2 to 3, 4)
    "X" (read matrix)

    Commands ARE case sensitive: 'm' is not the same as 'M'
  */

  if (command.length() < 1)
    return;

  if (command[0] == 'M') {
    if (command.length() != 5)
      return;
    char piece = command[1];
    int dx = command[2] - '0';
    int dy = command[3] - '0';
    Indication i = (Indication) ((int) (command[4] - '0'));
    if (Board.queueMovePiece(piece, dx, dy, i)) {
      Serial.println("[LOG] OK");
    } else {
      Serial.println("[LOG] ERR_WRONG_MOVE");
    }
  } else if (command[0] == 'R') {
    if (command.length() != 5)
      return;
    int sx = command[1] - '0';
    int sy = command[2] - '0';
    int dx = command[3] - '0';
    int dy = command[4] - '0';
    Board.queueMovePiece(sx, sy, dx, dy, true);
  } else if (command[0] == 'X') {
    Board.logMatrix();
  }

}
