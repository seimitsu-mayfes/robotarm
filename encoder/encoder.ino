const int A_pin = 3; // 割り込みピン
const int B_pin = 4;
volatile int count = 0;
volatile bool stateChanged = false;
const unsigned long debounceTime = 1; // 1ミリ秒
volatile unsigned long lastInterruptTime = 0;

void pulse_counter() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceTime) {
    if(digitalRead(A_pin) ^ digitalRead(B_pin)) {
      count++;
    } else {
      count--;
    }
    stateChanged = true;
    lastInterruptTime = interruptTime;
  }
}

void setup() {
  pinMode(A_pin, INPUT_PULLUP);
  pinMode(B_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin), pulse_counter, CHANGE);
  attachInterrupt(digitalPinToInterrupt(B_pin), pulse_counter, CHANGE);
  Serial.begin(9600);
}

void loop() {
  Serial.print("Count: ");
  Serial.print(count);
  Serial.print(", A: ");
  Serial.print(digitalRead(A_pin));
  Serial.print(", B: ");
  Serial.print(digitalRead(B_pin)); 
  Serial.println("--------------------");
  delay(100); // 100ミリ秒待機
}
