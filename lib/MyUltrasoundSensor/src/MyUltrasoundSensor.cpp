//MyUltrasoundSensor.cpp
//Code fuer Arduino und Attiny
//Author Retian
//Version 1.1

/*
  Ansteuerung eines Ultraschallmoduls HC-SR04 oder HY-SRF05 zur Entfernungsmessung.

  MyUltrasoundSensor Name(TriggerPin, EchoPin);

  byte TriggerPin - Arduino-Pin verbunden mit Ultraschallmodul-Pin Trig
  byte EchoPin    - Arduino-Pin verbunden mit Ultraschallmodul-Pin Echo

  Beispiel siehe unter:
  http://arduino-projekte.webnode.at/meine-libraries/ultraschallsensor/

  Funktionen siehe unter:
  http://arduino-projekte.webnode.at/meine-libraries/ultraschallsensor/funktionen/

*/

//*************************************************************************
//*************************************************************************

#include "Arduino.h"
#include "MyUltrasoundSensor.h" 

MyUltrasoundSensor::MyUltrasoundSensor(byte trigPin, byte echoPin)
{
  _trigPin = trigPin;
  _echoPin = echoPin;

  pinMode(_trigPin, OUTPUT);
  pinMode(_echoPin, INPUT);

  maxEchoImpulsTime = 160000L;
  maxOverflowVal = 999;
  medianNumber = 5; //Anzahl von Messungen bei Medianwertbildung
  measureDelay = 50; //Verzoegerung zwischen zwei Messungen [ms]
  overflowDelay = 100; //Verzoegerung nach Overflow [ms]
}

//*************************************************************************
//Distanz bei angenommener Raumtemperatur von 20 Grad C

float MyUltrasoundSensor::distance()
{
  unsigned long echoImpulsDauer;
  float distance;
  //vSchall = 34346.21; //Schallgeschwindigkeit [cm/s] bei 20 Grad C
  vSchall = soundVelosity(20.0689);
  echoImpulsDauer = runTime();
  //Serial.print("EchoImpulsDauer: ");
  //Serial.println(echoImpulsDauer);
  if (echoImpulsDauer < maxEchoImpulsTime)
  {
    distance = vSchall * echoImpulsDauer / (2 * 1000000L); //Wert in cm
  }
  else
  {
    dummyRunTime();
    distance = maxOverflowVal;
  }
  return distance;
}

//*************************************************************************
// Distanz bei gemessender Raumtemperatur

float MyUltrasoundSensor::distanceTempComp(float temp)
{
  _temp = temp;
  unsigned long echoImpulsDauer;
  float distance;

  //Schallgeschwindigkeit [cm/s]
  vSchall = soundVelosity(_temp);
  echoImpulsDauer = runTime();
  if (echoImpulsDauer < maxEchoImpulsTime)
    distance = vSchall * echoImpulsDauer / (2 * 1000000L); //Wert in cm
  else
  {
    dummyRunTime();
    distance = maxOverflowVal;
  }
  return distance;
}


//*************************************************************************
// Median-Distanz bei gemessender Raumtemperatur

float MyUltrasoundSensor::distanceTempCompMedian(float temp)
{
  _temp = temp;
  unsigned long echoImpulsDauer;
  float distance = 0;
  float distanceBuffer[medianNumber];
  float buf;

  //Schallgeschwindigkeit [cm/s]
  vSchall = soundVelosity(_temp);

  for (byte i = 0; i < medianNumber; i++)
  {
    echoImpulsDauer = runTime();
    if (echoImpulsDauer < maxEchoImpulsTime)
      distanceBuffer[i] = vSchall * echoImpulsDauer / (2 * 1000000L); //Wert in cm
    else
    {
      dummyRunTime();
      distanceBuffer[i] = maxOverflowVal;
    }
    //Serial.print(distanceBuffer[i]);
    //Serial.print(" ");
  }
  //Serial.println();

  //Messwerte aufsteigend der Groesse nach sortieren
  for (int y = 0; y < medianNumber; y++)
  {
    for (int i = 1; i < medianNumber; i++)
    {
      if (distanceBuffer[i] < distanceBuffer[i - 1])
      {
        buf = distanceBuffer[i];
        distanceBuffer[i] = distanceBuffer[i - 1];
        distanceBuffer[i - 1] = buf;
      }
    }
  }

  /*
    //Messwerte sortiert ausgeben
    for (int i = 0; i < medianNumber; i++)
    {
    Serial.print(distanceBuffer[i]);
    Serial.print(" ");
    }
    Serial.println();
  */

  //Median-Wert ermitteln
  if (medianNumber % 2 == 1) //Anzahl Messwerte ist ungerader Wert
  {
    //Serial.println((medianNumber/ 2) + 1);
    distance = distanceBuffer[(medianNumber / 2)];
  }
  else if (medianNumber % 2 == 0) //Anzahl Messwerte ist gerader Wert
  {
    //Serial.print((medianNumber / 2));
    //Serial.print(" ");
    //Serial.println((medianNumber / 2) + 1);
    distance = (distanceBuffer[(medianNumber / 2) - 1]
                + distanceBuffer[(medianNumber / 2)]) / 2.0;
  }
  //Serial.println();
  return (distance);
}

//*************************************************************************
// Gegenstand innerhalb eines Bereiches detektieren

bool MyUltrasoundSensor::detect(float temp, int minDistance, int maxDistance)
{
  _temp = temp;
  int _minDistance = minDistance;
  int _maxDistance = maxDistance;
  float distance;

  distance = distanceTempComp(_temp);
  if (distance >= minDistance && distance <= maxDistance) return true;
  else return false;
}

//*************************************************************************
// Schallgeschwindigkeit berechnen

float MyUltrasoundSensor::soundVelosity(float temp)
{
  //Schallgeschwindigkeit [cm/s]
  vSchall = 100 * 331.5 * sqrt(1 + temp / 273.15);
  return vSchall;
}

//*************************************************************************
// Laufzeit ermitteln (Ergebnis in uS)

unsigned long MyUltrasoundSensor::runTime()
{
  delay(max(measureDelay, 20));
  digitalWrite(_trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(_trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(_trigPin, LOW);
  return (pulseIn(_echoPin, HIGH));
}

//*************************************************************************
// Dummy-Laufzeit nach Overflow ermitteln

void MyUltrasoundSensor::dummyRunTime()
{
  delay(overflowDelay);
  digitalWrite(_trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(_trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(_trigPin, LOW);
}
