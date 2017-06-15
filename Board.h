#ifndef _BOARD_H
#define _BOARD_H


#include "Queue.h"

enum Indication {
  NONE,
  TOP,
  BOTTOM,
  LEFT,
  RIGHT
};

class ChessBoard {

  public:
    void begin(int dirX, int stepX, int dirY, int stepY, int ms1, int ms2, int ms3, int magnet, int ls_x, int ls_y);
    void setMovingSpeed(int speed); /* In steps-per-second */
    void setMovingMicrosteps(int microsteps);

    /* Put a movement in queue. x and y are expressed in units (1u = 1 cell) */
    void queueMovement(float x, float y, bool magnet = false);

    /* Put a go-to in queue. x and y are expressed in units (1u = 1 cell) */
    void queueGOTO(float x, float y, bool magnet = false);

    /* Put a go-to home in queue */
    void queueHome(bool magnet = false);

    /* Resets the internal board matrix */
    void resetMatrix(void);

    /* Put in queue a piece move from sx, sy to dx, dy */
    void queueMovePiece(int sx, int sy, int dx, int dy, bool eliminate=false);

    /* Put in queue a piece <p> move to dx, dy, indicated by <i> in case of multiple candidates */
    bool queueMovePiece(char p, int dx, int dy, Indication i = NONE);

    /* Search for a graveyard cell to put a piece in. <player> indicates which player, 1 is white and 0 is black */
    bool getGraveyard(int player, int* x, int* y);

    /* Put the axis in home position. Blocking routine. */
    void goHome(void);

    /* Writes the status of the queue to a print object */
    void printQueueStats(Print& printer);

    /* Writes the current matrix to a print object */
    void printMatrix(Print& printer);

    /* Logs the matrix in one line for a host to read it through serial
       The matrix is logged to the attached logger */
    void logMatrix(void);

    /* Logging: attach logger */
    void attachLogger(Print& logger);

    /* Loop handle function that must be called regularly */
    void handle(void);

  private:
    /* Pin numbers */
    int magnet, ls_x, ls_y;

    /* Limit switch interrupt mode toggle */
    bool ls_interrupts;

    /* The matrix */
    char matrix[8][12];

    /* Current magnet position in steps */
    int currentX;
    int currentY;

    /* Logger */
    Print* logger = NULL;

    /* Internal structures */
    struct movement {
      bool relative;
      int x;
      int y;
      bool magnet;
    };

    struct piecemove {
      int sx;
      int sy;
      int dx;
      int dy;
    };

    /* Movement queue */
    Queue<movement> movementQueue = Queue<movement>(32);
    Queue<piecemove> pieceMoveQueue = Queue<piecemove>(8);

    /* Put in the movement queue the moves needed to move a piece */
    void movePiece(const piecemove move);
};

extern ChessBoard Board;


#endif
