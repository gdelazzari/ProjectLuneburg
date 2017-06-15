#ifndef _STEPPERS_H
#define _STEPPERS_H


class SteppersManager {
  public:
    void begin(int dirX, int stepX, int dirY, int stepY, int ms1, int ms2, int ms3);

    void setSpeed(int speed); /* In steps-per-second */
    void setMicrosteps(int microsteps);

    int getSpeed(void);
    int getMicrosteps(void);

    void moveX(int steps);
    void moveY(int steps);

    bool isMovingX(void);
    bool isMovingY(void);
    bool isMoving(void);

    int getLeftX(void);
    int getLeftY(void);

    void stopX(bool force=false);
    void stopY(bool force=false);
    void stopAll(bool force=false);

  private:
    int dirX, stepX, dirY, stepY, ms1, ms2, ms3;
    int speed;
    int microsteps;

    long calculateTimerPeriod(void);

    volatile long leftX;
    volatile long leftY;

  friend void SteppersManager_onStep();
};

void SteppersManager_onStep(void);

extern SteppersManager Steppers;


#endif
