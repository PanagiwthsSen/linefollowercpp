// -------- PINS SENSORS (ΑΝΑΛΟΓΙΚΑ) & BUTTON --------
#define LEFT_PIN  26
#define MID_PIN   27
#define RIGHT_PIN 28
#define BUTTON_PIN 20  // Ενσωματωμένο κουμπί στη Maker Pi

// -------- PINS MOTORS (Maker Pi RP2040) --------
#define M1A 8
#define M1B 9
#define M2A 10
#define M2B 11

// -------- 🔧 ΚΑΛΙΜΠΡΑΡΙΣΜΑ ΑΝΑΛΟΓΙΚΩΝ ΤΙΜΩΝ --------
int min_L = 760; int max_L = 940;
int min_M = 670; int max_M = 940;
int min_R = 740; int max_R = 940;

// -------- 🛡️ FLUID ΑΓΩΝΙΣΤΙΚΟ TUNING --------
// -------- 🛡️ FLUID ΑΓΩΝΙΣΤΙΚΟ TUNING --------
float Kp = 100;   // Το ρίχνουμε λίγο για να μην υπερ-αντιδρά στα μικρά λαθάκια
float Kd = 180;   // Το ανεβάζουμε ΠΟΛΥ. Αυτό θα "πνίξει" εντελώς το μικρό ζιγκ-ζαγκ!

float base_speed = 160;  // Το ανεβάζουμε! Έτσι η πτώση από το 255 δεν θα είναι βίαιη.
float turbo_speed = 255; 
float max_speed = 255;   
float min_turn_speed = 0;

// -------- STATE VARIABLES --------
float previous_error = 0;
float last_valid_error = 0;
bool started = false;
int stop_counter = 0;  // ⏱️ ΤΟ ΝΕΟ ΜΑΣ ΟΠΛΟ ΓΙΑ ΤΙΣ ΑΠΟΤΟΜΕΣ ΣΤΡΟΦΕΣ

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

// -------- 🧠 ΕΠΑΓΓΕΛΜΑΤΙΚΟΣ ΑΝΑΛΟΓΙΚΟΣ ΥΠΟΛΟΓΙΣΜΟΣ (ΚΕΝΤΡΟ ΒΑΡΟΥΣ) --------
float get_analog_error() {
  int l_raw = analogRead(LEFT_PIN);
  int m_raw = analogRead(MID_PIN);
  int r_raw = analogRead(RIGHT_PIN);

  // Μετατροπή των τιμών σε κλίμακα 0 (Άσπρο) έως 1000 (Μαύρο)
  int l = constrain(map(l_raw, min_L, max_L, 0, 1000), 0, 1000);
  int m = constrain(map(m_raw, min_M, max_M, 0, 1000), 0, 1000);
  int r = constrain(map(r_raw, min_R, max_R, 0, 1000), 0, 1000);

  // 1. ΕΞΥΠΝΟΣ ΕΛΕΓΧΟΣ ΤΕΡΜΑΤΙΣΜΟΥ (Αντιμετώπιση του Ψευδούς Τερματισμού)
  if (l > 750 && m > 750 && r > 750)  {
    stop_counter++;
    if (stop_counter > 25) { 
      // Πρέπει να δει μαύρο για 25 ΣΥΝΕΧΟΜΕΝΑ loops για να σταματήσει!
      return 999; 
    }
    // Αν είναι απλά μια φαρδιά στροφή (π.χ. counter = 5), του λέμε να 
    // συνεχίσει να στρίβει δυνατά προς τα εκεί που έστριβε ήδη!
    return (last_valid_error > 0) ? 1.2 : -1.2; 
  } else {
    // Μόλις δει έστω και λίγο άσπρο, μηδενίζει αμέσως τον μετρητή τερματισμού.
    stop_counter = 0; 
  }

  // 2. ΜΝΗΜΗ ΦΑΝΤΑΣΜΑ: Αν βγει τελείως εκτός γραμμής (όλοι άσπρο), 
  // μην τα παρατάς! Θυμήσου από πού βγήκες και στρίψε με μανία (1.5)!
  int sum = l + m + r;
  if (sum < 300) {
    return (last_valid_error > 0) ? 1.5 : -1.5;
  }

  // 3. WEIGHTED AVERAGE (Το Μυστικό των Πρωταθλητών)
  float position = (float)(m * 1000L + r * 2000L) / sum;
  float error = (position - 1000.0) / 1000.0;

  return error;
}

// -------- SETUP --------
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(M1A, OUTPUT); pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT); pinMode(M2B, OUTPUT);
  set_motors(0, 0);
}

// -------- LOOP --------
void loop() {
  // 🟢 ΕΚΚΙΝΗΣΗ
  if (!started) {
    if (digitalRead(BUTTON_PIN) == LOW) { 
      started = true;
      delay(1000); 
    } else {
      set_motors(0, 0);
      return; 
    }
  }

  // 🛑 ΔΙΑΒΑΣΜΑ ΚΑΙ ΤΕΡΜΑΤΙΣΜΟΣ
  float error = get_analog_error();
  
  if (error == 999) { 
    set_motors(0, 0); // Φρενάρει αμέσως (Είδε το ΠΡΑΓΜΑΤΙΚΟ μαύρο τερματισμού)
    while(true) { delay(100); } // Μένει σταματημένο
  }

  // Αποθήκευση της τελευταίας κατεύθυνσης για τη μνήμη
  if (error != 0 && abs(error) != 1.5 && abs(error) != 1.2) {
      last_valid_error = error;
  }

  // 🚀 ΕΞΥΠΝΟΣ ΕΛΕΓΧΟΣ ΤΑΧΥΤΗΤΑΣ
// 🚀 ΕΞΥΠΝΟΣ ΕΛΕΓΧΟΣ ΤΑΧΥΤΗΤΑΣ (Χωρίς να σβήνουμε το PID!)
  float current_speed = base_speed;

  if (abs(error) < 0.2) {
    // ΕΥΘΕΙΑ: Ανεβάζουμε την ταχύτητα στο Turbo, αλλά ΔΕΝ βάζουμε "return;".
    // Αφήνουμε το PID να τρέξει από κάτω για να κάνει "γλυκές" μικροδιορθώσεις.
    current_speed = turbo_speed; 
  } 
  else if (abs(error) > 0.7) {
    // ΚΛΕΙΣΤΗ ΣΤΡΟΦΗ: Φρενάρουμε για να μην πεταχτεί έξω
    current_speed = 110; 
  }

  // ⚙️ ΑΝΑΛΟΓΙΚΟ PID (Τώρα τρέχει ΣΥΝΕΧΕΙΑ, δεν χάνει ποτέ τη μνήμη του!)
  float derivative = error - previous_error;
  float correction = (Kp * error) + (Kd * derivative);
  previous_error = error; 

  float left_speed = current_speed + correction;
  float right_speed = current_speed - correction;

  // 🛡️ ΑΣΦΑΛΗΣ ΣΤΡΟΦΗ
  if (left_speed < min_turn_speed) left_speed = min_turn_speed;
  if (right_speed < min_turn_speed) right_speed = min_turn_speed;

  left_speed = constrain(left_speed, min_turn_speed, max_speed);
  right_speed = constrain(right_speed, min_turn_speed, max_speed);

  set_motors(left_speed, right_speed);
}