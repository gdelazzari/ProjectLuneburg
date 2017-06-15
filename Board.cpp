#include <Arduino.h>
#include "Board.h"
#include "Steppers.h"
#include "Common.h"

/*
 * Let's instance a chessboard, so we have a 'Board' object available everywhere
 * to let us interact with the board
 */
ChessBoard Board;

/* The begin routine, which has to be called in the setup() */
void ChessBoard::begin(int dirX, int stepX, int dirY, int stepY, int ms1, int ms2, int ms3, int magnet, int ls_x, int ls_y)
{
  Steppers.begin(dirX, stepX, dirY, stepY, ms1, ms2, ms3);

  this->magnet = magnet;
  this->ls_x = ls_x;
  this->ls_y = ls_y;

  pinMode(this->magnet, OUTPUT);
  digitalWrite(this->magnet, LOW);

  pinMode(this->ls_x, INPUT_PULLUP);
  pinMode(this->ls_y, INPUT_PULLUP);

  this->currentX = 0;
  this->currentY = 0;

  this->ls_interrupts = true;

  this->resetMatrix();
}

void ChessBoard::resetMatrix(void) {
  const char firstRow[] = "TCARGACT";
  for (int x = 0; x < 8; x++) {
    MAT(this->matrix, x, 0) = firstRow[7 - x];
    MAT(this->matrix, x, 1) = 'P';
    for (int y = 2; y <= 5; y++) {
      MAT(this->matrix, x, y) = ' ';
    }
    MAT(this->matrix, x, 6) = 'p';
    MAT(this->matrix, x, 7) = TOLOWER(firstRow[x]);
  }
  for (int x = -2; x < 0; x++) {
    for (int y = 0; y < 8; y++) {
      MAT(this->matrix, x, y) = ' ';
    }
  }
  for (int x = 8; x < 10; x++) {
    for (int y = 0; y < 8; y++) {
      MAT(this->matrix, x, y) = ' ';
    }
  }
}

void ChessBoard::attachLogger(Print& logger) {
  this->logger = &logger;
}

void ChessBoard::queueMovePiece(int sx, int sy, int dx, int dy, bool eliminate) {
  if (this->logger != NULL) {
    this->logger->print("[LOG] MP ");
    this->logger->print(sx);
    this->logger->print(" ");
    this->logger->print(sy);
    this->logger->print(" ");
    this->logger->print(dx);
    this->logger->print(" ");
    this->logger->println(dy);
  }
  if (eliminate == true) {
    char target = MAT(this->matrix, dx, dy);
    if (target != ' ') {
      int gx, gy;
      if (this->getGraveyard(ISUPPER(target), &gx, &gy)) {
        this->pieceMoveQueue.push({dx, dy, gx, gy});
      }
    }
  }
  this->pieceMoveQueue.push({sx, sy, dx, dy});
}

bool ChessBoard::queueMovePiece(char p, int dx, int dy, Indication i) {
  char search = TOLOWER(p);
  char target = MAT(this->matrix, dx, dy);

  /* Figure out if we are eliminating something */
  bool elimination = (target != ' ');
  if (ISUPPER(p) && !ISLOWER(target)) elimination = false;
  if (ISLOWER(p) && !ISUPPER(target)) elimination = false;

  if ((target != ' ') && (elimination == false)) {
    return false;
  }

  int found = 0;
  int sx, sy;
  if (search == 'p') {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (MAT(this->matrix, x, y) == p) {
          if ( (!elimination && ISUPPER(p) && (x == dx) && ((dy - y) <= 2) && ((dy - y) > 0)) ||
               (!elimination && ISLOWER(p) && (x == dx) && ((y - dy) <= 2) && ((y - dy) > 0)) ||
               (elimination && ISUPPER(p) && (ABS(x - dx) == 1) && ((dy - y) == 1)) ||
               (elimination && ISLOWER(p) && (ABS(x - dx) == 1) && ((y - dy) == 1)) )
          {
            /* Check obstacles */
            if (ABS(y - dy) == 2) {
              int searchY = (y + dy) / 2;
              char target = MAT(this->matrix, x, searchY);;
              if (target != ' ') {
                continue;
              }
            }
            found++;
            sx = x;
            sy = y;
          }
        }
      }
    }
  }
  if (search == 't' || search == 'g') {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (MAT(this->matrix, x, y) == p) {
          if (((x != dx) && (y == dy)) || ((x == dx) && (y != dy))) {
            /* Check obstacles */
            if (x == dx) {
              if (ABS(y - dy) > 1) {
                // Moving along y axis
                int walk = (y < dy) ? 1 : -1;
                for (int i = (y + walk); i != dy; i += walk) {
                  if (MAT(this->matrix, x, (y + i)) != ' ') {
                    continue;
                  }
                }
              }
            } else if (y == dy) {
              if (ABS(x - dx) > 1) {
                // Moving along x axis
                int walk = (x < dx) ? 1 : -1;
                for (int i = (x + walk); i != dx; i += walk) {
                  if (MAT(this->matrix, (x + i), y) != ' ') {
                    continue;
                  }
                }
              }
            }
            found++;
            sx = x;
            sy = y;
          }
        }
      }
    }
  }
  if (search == 'a' || search == 'g') {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (MAT(this->matrix, x, y) == p) {
          if (ABS(x - dx) == ABS(y - dy)) {
            /* Checking obstacles in between */
            int diff = ABS(dx - x);
            int walkX = ((dx - x) > 0) ? 1 : -1;
            int walkY = ((dy - y) > 0) ? 1 : -1;
            int cy = y + walkY;
            bool obstacle = false;
            for (int cx = x + walkX; cx != dx; cx += walkX) {
              if (MAT(this->matrix, cx, cy) != ' ') {
                obstacle = true;
                break;
              }
              cy += walkY;
            }
            if (!obstacle) {
              found++;
              sx = x;
              sy = y;
            } else {
              continue;
            }
          }
        }
      }
    }
  }
  if (search == 'r') {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (MAT(this->matrix, x, y) == p) {
          if ((ABS(x - dx) == 1) || (ABS(y - dy) == 1)) {
            found++;
            sx = x;
            sy = y;
          }
        }
      }
    }
  }
  if (search == 'c') {
    Serial.println("Cavallo");
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (MAT(this->matrix, x, y) == p) {
          Serial.println("Trovato");
          int diffX = ABS(x - dx);
          int diffY = ABS(y - dy);
          Serial.println(diffX);
          Serial.println(diffY);
          if (((diffX == 1) && (diffY == 2)) || ((diffX == 2) && (diffY == 1))) {
            Serial.println("Cavallo valido");
            found++;
            sx = x;
            sy = y;
          }
        }
      }
    }
  }

  if (found == 0 || found > 1) {
    return false;
  } else {
    if (elimination) {
      int gx, gy;
      if (this->getGraveyard(ISUPPER(target), &gx, &gy)) {
        queueMovePiece(dx, dy, gx, gy);
      }
    }
    queueMovePiece(sx, sy, dx, dy);

    return true;
  }
}

bool ChessBoard::getGraveyard(int player, int* x, int* y) {
  int startX = -2 + (player * 10);
  int fx, fy;
  bool found = false;
  for (int sy = 0; sy < 8; sy++) {
    for (int sx = startX; sx < (startX + 2); sx++) {
      if (MAT(this->matrix, sx, sy) == ' ') {
        found = true;
        fx = sx;
        fy = sy;
        break;
      }
    }
  }

  if (found) {
    *x = fx;
    *y = fy;
    return true;
  } else {
    return false;
  }
}

void ChessBoard::setMovingSpeed(int speed) {
  Steppers.setSpeed(speed);
}

void ChessBoard::setMovingMicrosteps(int microsteps) {
  Steppers.setMicrosteps(microsteps);
}

void ChessBoard::queueMovement(float x, float y, bool magnet) {
  int stepsX = (int) (x * 200.f);
  int stepsY = (int) (y * 200.f);
  this->movementQueue.push({true, stepsX, stepsY, magnet});
}

void ChessBoard::queueGOTO(float x, float y, bool magnet) {
  int stepsX = (int) (x * 200.f);
  int stepsY = (int) (y * 200.f);
  this->movementQueue.push({false, stepsX, stepsY, magnet});
}

void ChessBoard::queueHome(bool magnet) {
  this->queueMovement(-100, -100);
}

void ChessBoard::goHome(void) {
  Steppers.stopAll();

  if (digitalRead(this->ls_x) != LOW)
    Steppers.moveX(-2000);
  if (digitalRead(this->ls_y) != LOW)
    Steppers.moveY(-2000);
  while (Steppers.isMoving()) {
    if (digitalRead(this->ls_y) == LOW) {
      Steppers.stopY();
    }
    if (digitalRead(this->ls_x) == LOW) {
      Steppers.stopX();
    }
  }
  Steppers.stopAll();

  int oldSpeed = Steppers.getSpeed();
  Steppers.setSpeed(100);
  Steppers.moveX(25);
  Steppers.moveY(25);
  while (Steppers.isMoving());
  Steppers.moveX(-50);
  Steppers.moveY(-50);
  while (Steppers.isMoving()) {
    if (digitalRead(this->ls_y) == LOW) {
      Steppers.stopY();
    }
    if (digitalRead(this->ls_x) == LOW) {
      Steppers.stopX();
    }
  }
  Steppers.stopAll();
  Steppers.setSpeed(oldSpeed);

  Steppers.moveX(430);
  while (Steppers.isMoving());

  this->currentX = 0;
  this->currentY = 0;
}

void ChessBoard::printQueueStats(Print& printer) {
  printer.print("[QUEUE] avgUsage=");
  printer.print(this->movementQueue.getAverageUsage());
  printer.print(" maxUsage=");
  printer.println(this->movementQueue.getMaximumUsage());
}

void ChessBoard::printMatrix(Print& printer) {
  printer.println("Current matrix");
  printer.println("--------------------------");
  for (int y = 7; y >= 0; y--) {
    for (int x = 11; x >= 0; x--) {
      printer.write(this->matrix[y][x]);
      printer.write(' ');
    }
    printer.println();
  }
  printer.println();
}

void ChessBoard::logMatrix(void) {
  Serial.print("[LOG] ");
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      Serial.write(MAT(this->matrix, x, y));
    }
  }
  Serial.println();
}

void ChessBoard::movePiece(const piecemove move) {
  int sx = move.sx;
  int sy = move.sy;
  int dx = move.dx;
  int dy = move.dy;

  if (MAT(this->matrix, sx, sy) == ' ') {
    if (this->logger != NULL)
      this->logger->println("[LOG] ERR_LOCATION_EMPTY");
    return;
  }
  if (MAT(this->matrix, dx, dy) != ' ') {
    if (this->logger != NULL)
      this->logger->println("[LOG] ERR_DESTINATION_OCCUPIED");
    return;
  }

  /* Go under the piece to move */
  this->queueGOTO(sx, sy, false);

  int diffX = dx - sx;
  int diffY = dy - sy;
  int walkX = (diffX > 0) ? 1 : -1; if (diffX == 0) walkX = 0;
  int walkY = (diffY > 0) ? 1 : -1; if (diffY == 0) walkY = 0;

  bool doDiagonal = false;

  if (ABS(diffX) == ABS(diffY)) {
    int absDiff = ABS(diffX);
    doDiagonal = true;
    for (int i = 1; i <= absDiff; i++) {
      int cx = sx + (i * walkX);
      int cy = sy + (i * walkY);
      if (MAT(this->matrix, cx, cy) != ' ') doDiagonal = false;
    }
  }

  if (doDiagonal) {
    this->queueMovement(diffX, diffY, true);
    this->queueMovement(0, 0, false);
  } else {
    bool dodgeX = false;
    bool dodgeY = false;

    for (int y = sy + walkY; y != (dy + walkY); y += walkY) {
      if (MAT(this->matrix, sx, y) != ' ') dodgeY = true;
    }
    if (diffY != 0) {
      for (int x = sx; x != dx; x += walkX) {
        if (MAT(this->matrix, x, dy) != ' ') dodgeX = true;
      }
    }

    float dodgeXval = 0;
    float dodgeYval = 0;
    if (dodgeX) dodgeXval = 0.5;
    if (dodgeY) dodgeYval = 0.5;
    this->queueMovement(dodgeYval, dodgeXval, true);
    this->queueMovement(0, diffY, true); // move in Y
    if (diffX > 0 && dodgeY) {
      this->queueMovement(diffX - 1, 0, true); // move in X
      this->queueMovement(dodgeYval, -dodgeXval, true);
    } else {
      this->queueMovement(diffX, 0, true); // move in X
      this->queueMovement(-dodgeYval, -dodgeXval, true);
    }
    this->queueMovement(0, 0, false);
  }

  /* Auto-homing */
  // TODO: do only once in a while (maybe keep a counter or something)
  if (dx <= 2 && dy <= 2) {
    this->queueHome();
    this->queueGOTO(0, 0);
  }

  // update internal board matrix
  MAT(this->matrix, dx, dy) = MAT(this->matrix, sx, sy);
  MAT(this->matrix, sx, sy) = ' ';
}

void ChessBoard::handle(void) {
  if (!Steppers.isMoving() && !this->movementQueue.empty()) {
    movement m = this->movementQueue.pop();

    if (m.magnet == true) {
      Steppers.setSpeed(100);
      digitalWrite(this->magnet, HIGH);
    } else {
      Steppers.setSpeed(300);
      digitalWrite(this->magnet, LOW);
    }

    int moveX = m.x;
    int moveY = m.y;
    if (m.relative == false) {
      moveX -= this->currentX;
      moveY -= this->currentY;
    }

    Serial.print("Pop: ");
    Serial.print(m.x);
    Serial.print(", ");
    Serial.print(m.y);
    Serial.print(", ");
    Serial.print(moveX);
    Serial.print(", ");
    Serial.print(moveY);
    Serial.print(" (current x=");
    Serial.print(this->currentX);
    Serial.print(", y=");
    Serial.print(this->currentY);
    Serial.println(")");

    Steppers.stopAll();
    Steppers.moveX(moveX);
    Steppers.moveY(moveY);

    this->currentX += moveX;
    this->currentY += moveY;

    if (this->movementQueue.empty()) {
      this->printMatrix(Serial);
      this->printQueueStats(Serial);
    }

    if (this->movementQueue.empty() && this->pieceMoveQueue.empty()) {
      this->logger->println("[LOG] QE");
    }

  } else if (!Steppers.isMoving() && this->movementQueue.empty() && !this->pieceMoveQueue.empty()) {
    piecemove m = this->pieceMoveQueue.pop();
    this->movePiece(m);
  }

  if (this->ls_interrupts == true) {
    if (digitalRead(this->ls_x) == LOW && Steppers.getLeftX() < 0) {
      Steppers.stopX();
      this->currentX = -430;
      Serial.println("x stop");
    }

    if (digitalRead(this->ls_y) == LOW && Steppers.getLeftY() < 0) {
      Steppers.stopY();
      this->currentY = 0;
      Serial.println("y stop");
    }
  }
}
