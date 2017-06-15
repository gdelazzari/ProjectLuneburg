#include <Arduino.h>
#include <TimerOne.h>
#include "Steppers.h"

/*
 * Let's instance a stepper manager, so we have a 'Steppers' object available everywhere
 * to let us control the motors
 */
SteppersManager Steppers;

/* The begin routine, which has to be called in the setup() */
void SteppersManager::begin(int dirX, int stepX, int dirY, int stepY, int ms1, int ms2, int ms3)
{
  /* Register pins */
  this->dirX = dirX;
  this->stepX = stepX;
  this->dirY = dirY;
  this->stepY = stepY;
  this->ms1 = ms1;
  this->ms2 = ms2;
  this->ms3 = ms3;

  /* Initialize own variables */
  this->speed = 100;
  this->microsteps = 1;

  this->leftX = 0;
  this->leftY = 0;

  /* Initialize and configure the timer */
  Timer1.initialize();
  Timer1.stop();
  Timer1.attachInterrupt(SteppersManager_onStep);

  /* Initialize pins */
  pinMode(this->dirX, OUTPUT);
  pinMode(this->stepX, OUTPUT);
  digitalWrite(this->dirX, LOW);
  digitalWrite(this->stepX, LOW);
  pinMode(this->dirY, OUTPUT);
  pinMode(this->stepY, OUTPUT);
  digitalWrite(this->dirY, LOW);
  digitalWrite(this->stepY, LOW);
}

void SteppersManager::setSpeed(int speed) {
  if (speed == this->speed)
    return;

  this->speed = speed;
  if (this->isMoving()) {
    Timer1.setPeriod(this->calculateTimerPeriod());
  }
}

void SteppersManager::setMicrosteps(int microsteps) { this->microsteps = microsteps; }

int SteppersManager::getSpeed() { return this->speed; }

int SteppersManager::getMicrosteps() { return this->microsteps; }

int SteppersManager::getLeftX() { return this->leftX; }

int SteppersManager::getLeftY() { return this->leftY; }

/* Interrupt Service Routine on the timer overflow, used to count steps */
void SteppersManager_onStep(void) {
  if (Steppers.leftX != 0) {
    Steppers.leftX += (Steppers.leftX > 0) ? -1 : 1;
    if (Steppers.leftX == 0) {
      Steppers.stopX(true);
    }
  }
  if (Steppers.leftY != 0) {
    Steppers.leftY += (Steppers.leftY > 0) ? -1 : 1;
    if (Steppers.leftY == 0) {
      Steppers.stopY(true);
    }
  }
}

void SteppersManager::moveX(int s) {
  if (s == 0) return;

  bool wasMoving = this->isMoving();
  bool wasMovingX = this->isMovingX();

  this->leftX += s * this->microsteps;

  // Write new direction
  digitalWrite(this->dirX, (this->leftX < 0) ? HIGH : LOW);

  if (!wasMoving) {
    Timer1.setPeriod(this->calculateTimerPeriod());
    Timer1.stop();
  }
  if (!wasMovingX) {
    Timer1.pwm(this->stepX, 512);
    // With Timer1.pwm the timer is automatically resumed (not a good library design decision) so we skip the part
    // where we start the timer only if the axis aren't moving yet
  }
  /*
  if (!wasMoving) {
    Timer1.resume();
  }*/
}

void SteppersManager::moveY(int s) {
  if (s == 0) return;

  bool wasMoving = this->isMoving();
  bool wasMovingY = this->isMovingY();

  this->leftY += s * this->microsteps;

  // Write new direction
  digitalWrite(this->dirY, (this->leftY < 0) ? HIGH : LOW);

  if (!wasMoving) {
    Timer1.setPeriod(this->calculateTimerPeriod());
    Timer1.stop();
  }
  if (!wasMovingY) {
    Timer1.pwm(this->stepY, 512);
    // With Timer1.pwm the timer is automatically resumed (not a good library design decision) so we skip the part
    // where we start the timer only if the axis aren't moving yet
  }
}

bool SteppersManager::isMovingX(void) {
  return (this->leftX != 0);
}

bool SteppersManager::isMovingY(void) {
  return (this->leftY != 0);
}

bool SteppersManager::isMoving(void) {
  return (this->leftX != 0 || this->leftY != 0);
}

long SteppersManager::calculateTimerPeriod(void) {
  long period = 1000000 / this->speed / this->microsteps;
  return period;
}

void SteppersManager::stopX(bool force) {
  if (!this->isMovingX() && !force)
    return;

  this->leftX = 0;
  // We're done with the X axis
  Timer1.disablePwm(Steppers.stepX);
  if (!Steppers.isMoving()) {
    Timer1.stop();
  }
}

void SteppersManager::stopY(bool force) {
  if (!this->isMovingY() && !force)
    return;

  this->leftY = 0;
  // We're done with the X axis
  Timer1.disablePwm(Steppers.stepY);
  if (!Steppers.isMoving()) {
    Timer1.stop();
  }
}

void SteppersManager::stopAll(bool force) {
  if (!this->isMoving() && !force)
    return;

  this->leftX = 0;
  this->leftY = 0;

  Timer1.disablePwm(Steppers.stepX);
  Timer1.disablePwm(Steppers.stepY);
  Timer1.stop();
}
