#include <Arduino.h>
#include <EEPROM.h>
#include <MyUltrasoundSensor.h>

// pins
int BUTTON_UP = D1;   // pull-up !
int BUTTON_DOWN = D2; // pull-up !
int US_TRIGGER = D3;
int US_ECHO = D0;
int OUTPUT_UP = D5;
int OUTPUT_DOWN = D6;
int LED_UP = D7;
int LED_DOWN = D8;

// variables for button logic
enum direction {
  NONE, UP, DOWN
};
enum mode {
  MANUAL, AUTOMATIC
};
direction initialDirection = NONE;
bool buttonPressed = false;
bool buttonLongPressed = false;
bool positionStored = false;
uint32_t  buttonTimer = 0;
uint32_t  longPressTimeout = 1000;

// variables for driving desk
MyUltrasoundSensor uss(US_TRIGGER, US_ECHO);
int lastPosition;
int positionUp;
int positionDown;
int EEPROM_UP = 0;
int EEPROM_DOWN = 1;
direction drivingDirection = NONE;
mode drivingMode = MANUAL;
uint32_t  drivingTimer = 0;
uint32_t  drivingTimeout = 20000;

// stop driving desk
void stopDesk() {
  Serial.println("Stopping desk");
  // stop motor
  digitalWrite(OUTPUT_UP, LOW);
  digitalWrite(LED_UP, LOW);
  digitalWrite(OUTPUT_DOWN, LOW);
  digitalWrite(LED_DOWN, LOW);
  drivingDirection = NONE;
  drivingMode = MANUAL;
}

// start driving desk in direction
void driveDesk(direction dir) {
  // drive motor in direction (stop going the other way first)
  if (drivingDirection != NONE && drivingDirection != dir) {
    stopDesk();
  }
  if (drivingDirection == NONE) {
    drivingTimer = millis();
  }
  if (drivingDirection == dir) {
    return;
  }
  if (dir == UP) {
    Serial.println("Driving desk up");
    digitalWrite(OUTPUT_UP, HIGH);
    digitalWrite(LED_UP, HIGH);
    drivingDirection = UP;
  } else {
    Serial.println("Driving desk down");
    digitalWrite(OUTPUT_DOWN, HIGH);
    digitalWrite(LED_DOWN, HIGH);
    drivingDirection = DOWN;
  }
}

// start autodriving desk to position
void autoDriveDesk(direction dir) {
  // auto drive desk to position, stop motor if already going in that direction
  if (drivingDirection != NONE && drivingDirection == dir) {
    stopDesk();
    return;
  } 

  if ((dir == UP && lastPosition >= positionUp) || (dir == DOWN && lastPosition <= positionDown)) {
    Serial.println("Not driving desk. Already in position.");
    return;
  }

  Serial.print("Driving desk to position ");
  if (dir == UP) {
    Serial.println("UP");
  } else {
    Serial.println("DOWN");
  }
  drivingMode = AUTOMATIC;
  driveDesk(dir);
}

void blink(int ledPin) {
  for (int i=0; i<3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    if (i<2) {
      delay(200);
    }
  }
}

// store position to eeprom
void writeToEeprom(int position, direction dir) {
  position = (position < 0 || position > 255) ? 100 : position;
  EEPROM.begin(2);
  if (dir == UP) {
    Serial.print("Storing position UP: ");
    if (position != EEPROM.read(EEPROM_UP)) {
      Serial.print("*new* ");
      EEPROM.write(EEPROM_UP, position);
    }
    blink(LED_UP);
    positionUp = position;
  } else {
    Serial.print("Storing position DOWN: ");
    if (position != EEPROM.read(EEPROM_DOWN)) {
      Serial.print("*new* ");
      EEPROM.write(EEPROM_DOWN, position);
    }
    blink(LED_DOWN);
    positionDown = position;
  }
  EEPROM.commit();
  Serial.println(position);
}

// read up and down positions from eeprom and store in global variables
void readPositions() {
  EEPROM.begin(2);
  Serial.println("Reading positions from EEPROM");
  positionUp = EEPROM.read(EEPROM_UP);
  positionDown = EEPROM.read(EEPROM_DOWN);
  EEPROM.end();
  Serial.print("Position UP: ");
  Serial.println(positionUp);
  Serial.print("Position DOWN: ");
  Serial.println(positionDown);
}

// measure position in cm using HC-SR04
int measure() {
  // Clears the trigPin
  digitalWrite(US_TRIGGER, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(US_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIGGER, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(US_ECHO, HIGH);
  // Calculating the distance
  int distance = duration*0.034/2;
  return distance;
}

int measurePosition() {
  int distance = uss.distanceTempCompMedian(20);
  Serial.print("Current position: ");
  Serial.println(distance);
  lastPosition = distance;
  return distance;
}
 
// measure and store current position for given direction
void storePosition(direction dir) {
  if (!positionStored) {
    int position = measurePosition();
    position = (position > 255) ? 255 : (position < 0) ? 0 : position;
    writeToEeprom(position, dir);
  }
  positionStored = true;
}

// run when button press is started
void initializeButtonPress(direction dir) {  
  Serial.print("Button pressed: ");
  if (dir == UP) {
    Serial.println("UP");
  } else {
    Serial.println("DOWN");
  }
  buttonPressed = true;
  buttonTimer = millis();
  initialDirection = dir;
}

void setup() {
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(OUTPUT_UP, OUTPUT);
  pinMode(OUTPUT_DOWN, OUTPUT);
  pinMode(LED_UP, OUTPUT);
  pinMode(LED_DOWN, OUTPUT);
  Serial.begin(9600);
  readPositions();
  uss.medianNumber = 3;
  uss.measureDelay = 30;
  measurePosition();
}

void loop() {

  if (digitalRead(BUTTON_UP) == LOW) { 
    if (!buttonPressed) {
      initializeButtonPress(UP);
    } else if (!buttonLongPressed && initialDirection == DOWN) {
      storePosition(initialDirection);
    }
  }  

  if (digitalRead(BUTTON_DOWN) == LOW) { 
    if (!buttonPressed) {
      initializeButtonPress(DOWN);
    } else if (!buttonLongPressed && initialDirection == UP) {
      storePosition(initialDirection);
    }
  }  

  if (buttonPressed && ((millis() - buttonTimer) > longPressTimeout) 
      && !buttonLongPressed && !positionStored) { // long press button
    buttonLongPressed = true;
    driveDesk(initialDirection);
  }

  if (digitalRead(BUTTON_UP) == HIGH && digitalRead(BUTTON_DOWN) == HIGH) { // button release, react to short press and do long press end
    if (buttonPressed) {
      if (buttonLongPressed) {
        buttonLongPressed = false;
        stopDesk();
        measurePosition();
      } else if (!positionStored) {
        autoDriveDesk(initialDirection);
      }
      initialDirection = NONE;
      buttonPressed = false;
      positionStored = false;
    }
  }

  if (drivingDirection != NONE) {
    if ((millis() - drivingTimer) > drivingTimeout) {
      Serial.println("ERROR: Driving timeout reached!");
      stopDesk();
    } else if (drivingMode == AUTOMATIC) {
      long currentPosition = measurePosition();
      if (drivingDirection == UP && currentPosition >= positionUp) {
        Serial.println("UP Position reached!");
        stopDesk();
      } else if (drivingDirection == DOWN && currentPosition <= positionDown) {
        Serial.println("DOWN Position reached!");
        stopDesk();
      }
    }
  }

}