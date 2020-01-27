//MyUltrasoundSensor.h

#ifndef MyUltrasoundSensor_h
#define MyUltrasoundSensor_h

#include "Arduino.h"

class MyUltrasoundSensor
{
  public:
    MyUltrasoundSensor(byte trigPin, byte echoPin);
    float distance(void);
    float distanceTempComp(float temp);
    float distanceTempCompMedian(float temp);
    bool detect(float temp, int minDistance, int maxDistance);
    float soundVelosity(float temp);
    byte medianNumber;
    int measureDelay;
    long maxEchoImpulsTime;
    int maxOverflowVal;

  private:
    unsigned long runTime(void);
    void dummyRunTime();
    byte _trigPin;
    byte _echoPin;
    float _temp;
    float vSchall;
    byte overflowDelay;
 };

#endif
