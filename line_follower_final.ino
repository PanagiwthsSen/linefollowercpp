// -------- PINS SENSORS (ΑΝΑΛΟΓΙΚΑ), BUTTON & BUZZER --------
#define LEFT_PIN  26
#define MID_PIN   27
#define RIGHT_PIN 28
#define BUTTON_PIN 20
#define BUZZER_PIN 22 

// -------- PINS MOTORS (Maker Pi RP2040) --------
#define M1A 8
#define M1B 9
#define M2A 10
#define M2B 11

// -------- 🔧 ΤΑ ΚΑΛΙΜΠΡΑΡΙΣΜΕΝΑ ΝΟΥΜΕΡΑ ΣΟΥ --------
int min_L = 537; int max_L = 920;
int min_M = 364; int max_M = 904;
int min_R = 437; int max_R = 890;

// -------- 🏁 ΑΠΟΛΥΤΟ ΑΓΩΝΙΣΤΙΚΟ TUNING (SMOOTH & FAST) --------
float base_Kp = 70;        // Ήρεμο τιμόνι στην ευθεία - Μηδενίζει το ζιγκ-ζαγκ
float aggressive_Kp = 110;  // Μαλακώσαμε το τιμόνι στη στροφή για να μην χάνει τον έλεγχο
float Kd = 190;            // Σωστό αμορτισέρ για να ισιώνει αμέσως το ρομπότ

float turbo_speed = 255;         // Μέγιστη ταχύτητα στην ευθεία
float emergency_turn_speed = 130; // 🚀 ΑΝΕΒΑΣΑΜΕ ΤΟ ΦΡΕΝΟ: Πάει πολύ πιο γρήγορα μέσα στη στροφή (από 70 -> 130)
float max_speed = 255;
float min_turn_speed = -50;      // Πιο γλυκιά όπισθεν για να μην διπλώνει το ρομπότ

// -------- STATE VARIABLES --------
float previous_error = 0;
float last_valid_error = 0;
bool started = false;
int stop_counter = 0;

// -------- MOTOR CONTROL --------
void set_motors(float left_pwm, float right_pwm) {
  left_pwm = constrain(left_pwm, -max_speed, max_speed);
  right_pwm = constrain(right_pwm, -max_speed, max_speed);

  if (left_pwm >= 0) {
    analogWrite(M1A, left_pwm); analogWrite(M1B, 0);
  } else {
    analogWrite(M1A, 0); analogWrite(M1B, -left_pwm);
  }

  if (right_pwm >= 0) {
    analogWrite(M2A, right_pwm); analogWrite(M2B, 0);
  } else {
    analogWrite(M2A, 0); analogWrite(M2B, -right_pwm);
  }
}

// -------- 🧠 ΑΝΑΛΟΓΙΚΟΣ ΥΠΟΛΟΓΙΣΜΟΣ --------
float get_analog_error() {
  int l_raw = analogRead(LEFT_PIN);
  int m_raw = analogRead(MID_PIN);
  int r_raw = analogRead(RIGHT_PIN);

  int l = constrain(map(l_raw, min_L, max_L, 0, 1000), 0, 1000);
  int m = constrain(map(m_raw, min_M, max_M, 0, 1000), 0, 1000);
  int r = constrain(map(r_raw, min_R, max_R, 0, 1000), 0, 1000);

  // 1. ΕΞΥΠΝΟΣ ΤΕΡΜΑΤΙΣΜΟΣ
  if (l > 820 && m > 820 && r > 820) {
    stop_counter++;
    if (stop_counter > 25) { 
      return 999;
    }
    return (last_valid_error > 0) ? 1.0 : -1.0; 
  } else {
    stop_counter = 0;
  }

  // 2. ΜΝΗΜΗ ΦΑΝΤΑΣΜΑ 
  int sum = l + m + r;
  if (sum < 300) {
    return (last_valid_error > 0) ? 1.2 : -1.2; 
  }

  // 3. WEIGHTED AVERAGE
  float position = (float)(m * 1000L + r * 2000L) / sum;
  float error = (position - 1000.0) / 1000.0;

  return error;
}

// -------- SETUP --------
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(M1A, OUTPUT); pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT); pinMode(M2B, OUTPUT);
  set_motors(0, 0);
}

// -------- LOOP --------
void loop() {
  // 🟢 ΕΚΚΙΝΗΣΗ
  if (!started) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      tone(BUZZER_PIN, 1200, 150); 
      delay(400);         
      started = true;
    } else {
      set_motors(0, 0);
      return;
    }
  }

  // 🛑 ΔΙΑΒΑΣΜΑ ΚΑΙ ΤΕΡΜΑΤΙΣΜΟΣ
  float error = get_analog_error();

  if (error == 999) {
    set_motors(0, 0);
    tone(BUZZER_PIN, 900, 400); 
    while(true) { delay(100); }
  }

  if (error != 0 && abs(error) != 1.2 && abs(error) != 1.0) {
    last_valid_error = error;
  }

  // 🚀 ΑΓΩΝΙΣΤΙΚΟΣ ΕΛΕΓΧΟΣ ΤΑΧΥΤΗΤΑΣ & ΤΙΜΟΝΙΟΥ
  float current_speed = turbo_speed;
  float abs_err = abs(error);
  float active_Kp = base_Kp;
  
  if (abs_err > 0.30) {
    // 🚨 ΣΤΡΟΦΗ: Μικρότερο Kp για να μην κοσκινίζει, αλλά πολύ μεγαλύτερη ταχύτητα (130)
    active_Kp = aggressive_Kp;
    current_speed = emergency_turn_speed; 
  } else {
    // 🏎️ ΕΥΘΕΙΑ: Κλειδωμένο 255 γκάζι και απόλυτη σταθερότητα
    active_Kp = base_Kp;
    current_speed = turbo_speed; 
  }

  // ⚙️ PID COMPUTATION
  float derivative = error - previous_error;
  float correction = (active_Kp * error) + (Kd * derivative);
  previous_error = error;

  float left_speed = current_speed + correction;
  float right_speed = current_speed - correction;

  // 🛡️ ΠΡΟΣΤΑΣΙΑ ΤΡΟΧΩΝ
  if (left_speed < min_turn_speed) left_speed = min_turn_speed;
  if (right_speed < min_turn_speed) right_speed = min_turn_speed;

  left_speed = constrain(left_speed, min_turn_speed, max_speed);
  right_speed = constrain(right_speed, min_turn_speed, max_speed);

  set_motors(left_speed, right_speed);
}