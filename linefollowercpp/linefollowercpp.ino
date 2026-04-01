#define LEFT_PIN  14
#define MID_PIN   13
#define RIGHT_PIN 15
#define BUTTON_PIN 20  
#define M1A 8
#define M1B 9
#define M2A 10
#define M2B 11
#define LINE_VAL LOW  


float Kp = 60;  
float Kd = 40;  

float base_speed = 120;  
float turbo_speed = 200; 
float max_speed = 255;   


float previous_error = 0;
float last_valid_error = 0;
bool started = false;


void set_motors(float left_pwm, float right_pwm) {
  left_pwm = constrain(left_pwm, -max_speed, max_speed);
  right_pwm = constrain(right_pwm, -max_speed, max_speed);

  
  if (left_pwm >= 0) {
    analogWrite(M1A, left_pwm);
    analogWrite(M1B, 0);
  } else {
    analogWrite(M1A, 0);
    analogWrite(M1B, -left_pwm);
  }

  
  if (right_pwm >= 0) {
    analogWrite(M2A, right_pwm);
    analogWrite(M2B, 0);
  } else {
    analogWrite(M2A, 0);
    analogWrite(M2B, -right_pwm);
  }
}


float get_raw_error() {
  bool l = (digitalRead(LEFT_PIN) == LINE_VAL);
  bool m = (digitalRead(MID_PIN) == LINE_VAL);
  bool r = (digitalRead(RIGHT_PIN) == LINE_VAL);


  if (l && m && r) return 999; 

  if (!l && m && !r) return 0;     
  if (!l && m && r)  return 0.5;   
  if (l && m && !r)  return -0.5;  
  if (!l && !m && r) return 1.0;   
  if (l && !m && !r) return -1.0;  

  
  if (!l && !m && !r) {
    return (last_valid_error > 0) ? 1.5 : -1.5;
  }

  return 0; 
}


void setup() {
  pinMode(LEFT_PIN, INPUT);
  pinMode(MID_PIN, INPUT);
  pinMode(RIGHT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  pinMode(M1A, OUTPUT); pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT); pinMode(M2B, OUTPUT);
  

  set_motors(0, 0);
}


void loop() {
 
  if (!started) {
    if (digitalRead(BUTTON_PIN) == LOW) { 
      started = true;
      delay(1000); 
    } else {
      set_motors(0, 0);
      return; 
    }
  }

  
  float error = get_raw_error();
  
  if (error == 999) { 
    set_motors(0, 0);
    while(true) { delay(100); } 
  }

  
  if (error != 0 && abs(error) != 1.5) {
      last_valid_error = error;
  }

  
  if (error == 0) {
    previous_error = 0; 
    set_motors(turbo_speed, turbo_speed); 
    delay(1);
    return; 

  
  float derivative = error - previous_error;
  float correction = (Kp * error) + (Kd * derivative);
  previous_error = error; 

  float left_speed = base_speed + correction;
  float right_speed = base_speed - correction;

  set_motors(left_speed, right_speed);

  delay(1); 
}