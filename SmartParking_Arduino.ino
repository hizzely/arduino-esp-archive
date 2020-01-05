#define RGB_PIN_TOTAL 3
#define PARKING_LANE_TOTAL 2

/*
 * --------------------------------------------------
 * PIN MAPPING
 */

// Buzzer 1
const int buzzerPin1 = 2;

// RGB LED 1 {R, G, B}
const int rgbPin1[RGB_PIN_TOTAL] = { 3, 5, 6 };

// Ultrasonic Sensor 1 {Trigger, Echo}
const int ultrasonicPin1[] = { 7, 4 };


// RGB LED 2 {R, G, B}
const int rgbPin2[RGB_PIN_TOTAL] = { 9, 10, 11 };

// Ultrasonic Sensor 2 {Trigger, Echo}
const int ultrasonicPin2[] = { 12, 13 };


/*
 * --------------------------------------------------
 * GLOBAL VARIABLES
 * 
 */
int it = 0;

struct IntervalStopwatch {
  int currentTime = 0;
  int startTime = 0;
  int interval = 0;

  void refresh() {
    currentTime = (millis() / 1000);
  }

  void setInterval(int *seconds) {
    startTime = (millis() / 1000);
    interval = *seconds;
  }

  bool isIntervalPassed() {
    return (((currentTime - startTime) >= interval) ? true : false);
  }
} timer[PARKING_LANE_TOTAL];

struct ParkingLane {
  const int EMPTY_STATE = 0;
  const int PARKING_STATE = 1;
  const int IDLE_STATE = 2;
  const int WAIT_STATE = 3;
  const int PARKED_STATE = 4;
  const int PICKED_STATE = 5;
  const int INPARKING_TIMEOUT = 5; // secs
  const int VALID_PARKED_DISTANCE = 5; // cm
  const int VALID_DISTANCE_MAX = 15; // cm
  const float DISTANCE_ERROR_TOLERATION = 1.5; // cm
  float vehicleCurrentDistance;
  float vehicleParkedDistance;
  int currentState;

  ParkingLane() {
    stateReset();
  }

  int getCurrentState() {
    return currentState;
  }

  void setCurrentState(int *state) {
    currentState = *state;
  }

  float getVehicleCurrentDistance() {
    return vehicleCurrentDistance;
  }

  bool distanceAreEqual() {
    return ((vehicleCurrentDistance >= (vehicleParkedDistance - DISTANCE_ERROR_TOLERATION)) && (vehicleCurrentDistance <= (vehicleParkedDistance + DISTANCE_ERROR_TOLERATION))) ? true : false;
  }

  void stateReset() {
    vehicleCurrentDistance = -1;
    vehicleParkedDistance = -1;
    currentState = EMPTY_STATE;
  }

  void stateUpdate(float sensorValue) {
    vehicleCurrentDistance = sensorValue;

    if ((vehicleCurrentDistance >= VALID_DISTANCE_MAX) &&
        ((currentState == PARKING_STATE) || (currentState == IDLE_STATE) || (currentState == WAIT_STATE) || (currentState == PICKED_STATE))) {
      // Vehicle was inside the valid parking distance but not parking, and then go away.
      // Resetting state and marked as EMPTY_STATE.
      stateReset();
    }
    else if (distanceAreEqual() && ((currentState == PARKING_STATE) || (currentState == WAIT_STATE))) {
      // Vehicle is idling on parking lane, marked as IDLE_STATE
      // Waiting external timer to change the state to PARKED_STATE
      if (currentState == WAIT_STATE) {
        return;
      }

      // Check for vehicle distance validity
      if (vehicleCurrentDistance > VALID_PARKED_DISTANCE) {
        return;
      }
      
      currentState = IDLE_STATE;
    }
    else if ((vehicleCurrentDistance <= VALID_DISTANCE_MAX) && (currentState != PARKED_STATE)) {
      if ((currentState == PICKED_STATE)) {
        return;
      }
      
      // Vehicle is inside the valid parking distance, marked as PARKING_STATE.
      currentState = PARKING_STATE;
      vehicleParkedDistance = vehicleCurrentDistance;
    }
    else if ((vehicleCurrentDistance >= VALID_PARKED_DISTANCE) && (currentState == PARKED_STATE)) {
      // Vehicle was parked, but now going away (picked up) from it's current position
      currentState = PICKED_STATE;
    }
  }
} lane[PARKING_LANE_TOTAL];


/*
 * --------------------------------------------------
 * CUSTOM FUNCTIONS
 * 
 */

void setRgbColor(const int *pin, int r, int g, int b, bool anode = false) {
  if (anode) {
    r = map(r, 0, 255, 255, 0);
    g = map(g, 0, 255, 255, 0);
    b = map(b, 0, 255, 255, 0);  
  }
  
  analogWrite(pin[0], r); // Pin RED
  analogWrite(pin[1], g); // Pin GREEN
  analogWrite(pin[2], b); // Pin BLUE
}

void distanceBeeper(const float &distance, const int &buzzerPin) {
  if (distance >= 0 && distance <= 5)
    tone(buzzerPin, 1000, 150);
  else if (distance >= 6 && distance <= 10)
    tone(buzzerPin, 750, 100);
  else
    tone(buzzerPin, 500, 100);
}

float ultrasonicPulse(const int *pin) {
  digitalWrite(pin[0], LOW);
  delayMicroseconds(5);
  digitalWrite(pin[0], HIGH);
  delayMicroseconds(10);
  digitalWrite(pin[0], LOW);
  return ((pulseIn(pin[1], HIGH) / 2) / 29.1);
}

void printSerializedParkingData() {
  Serial1.print(lane[0].getCurrentState());
  Serial1.print(F(";"));
  Serial1.print(lane[0].getVehicleCurrentDistance());
  Serial1.print(F("|"));
  Serial1.print(lane[1].getCurrentState());
  Serial1.print(F(";"));
  Serial1.print(lane[1].getVehicleCurrentDistance());
  Serial1.println();
}

/*
 * --------------------------------------------------
 * MAIN ROUTINE
 * 
 */
 
void setup() { 
  // Buzzer Pin Setup
  pinMode(buzzerPin1, OUTPUT);

  // Ultrasonic Pin Setup
  pinMode(ultrasonicPin1[0], OUTPUT);
  pinMode(ultrasonicPin1[1], INPUT);
  pinMode(ultrasonicPin2[0], OUTPUT);
  pinMode(ultrasonicPin2[1], INPUT);
  
  // RGB Pin Setup
  for (it = 0; it < RGB_PIN_TOTAL; ++it) {
    pinMode(rgbPin1[it], OUTPUT);
    pinMode(rgbPin2[it], OUTPUT);
  }

  // Begin serial communication
  // Serial.begin(115200);
  Serial1.begin(115200);
}


void loop() {
  // Refresh Interval Timer
  timer[0].refresh();
  timer[1].refresh();

  // Refresh Parking Lane State
  lane[0].stateUpdate(ultrasonicPulse(ultrasonicPin1));
  lane[1].stateUpdate(ultrasonicPulse(ultrasonicPin2));

  for (it = 0; it < PARKING_LANE_TOTAL; ++it) {
    // Activate timer before changing state to parked.
    if (lane[it].getCurrentState() == lane[it].IDLE_STATE) {
      timer[it].setInterval(&lane[it].INPARKING_TIMEOUT);
      lane[it].setCurrentState(&lane[it].WAIT_STATE);
    }
    else if (lane[it].getCurrentState() == lane[it].WAIT_STATE) {
      if (timer[it].isIntervalPassed()) {
        lane[it].setCurrentState(&lane[it].PARKED_STATE);
      }
    }

    // RGB
    if (lane[it].getCurrentState() == lane[it].PARKING_STATE)
      setRgbColor((it == 0 ? rgbPin1 : rgbPin2), 255, 255, 0, true); // yellow
    else if (lane[it].getCurrentState() == lane[it].WAIT_STATE)
      setRgbColor((it == 0 ? rgbPin1 : rgbPin2), 0, 255, 255, true); // cyan
    else if (lane[it].getCurrentState() == lane[it].PARKED_STATE)
      setRgbColor((it == 0 ? rgbPin1 : rgbPin2), 0, 255, 10, true); // green
    else if (lane[it].getCurrentState() == lane[it].PICKED_STATE)
      setRgbColor((it == 0 ? rgbPin1 : rgbPin2), 255, 10, 10, true); // red
    else
      setRgbColor((it == 0 ? rgbPin1 : rgbPin2), 255, 255, 255, true); // white
  }

  // Beeper on vehicle in range and not in parked state
  if ((lane[0].getVehicleCurrentDistance() <= lane[0].VALID_DISTANCE_MAX) && 
      (lane[0].getCurrentState() != lane[0].PARKED_STATE)) {
    distanceBeeper(lane[0].getVehicleCurrentDistance(), buzzerPin1);
  }

  // Send data to Serial1
  printSerializedParkingData();

  delay(100);
}
