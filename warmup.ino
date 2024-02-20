#include <LiquidCrystal.h>
#include <EEPROM.h>

#define x1          A5    //Temp 
#define x3          1     //Water level   
#define element     2     //x8    
#define waterIn     3     //x10    
#define bypass      11    //x11   
#define cool        12    //x12  
#define pump        13    //x14  
#define d           150   //Refresh Delays (milliseconds)
#define timeout     30000 //Menu timeout (milliseconds)
#define maxCorr     40    //Maximum Temperature Correction bound (40 => -20 to +20) (centigrade degree)
#define maxDelay    200   //Maximum Delay (seconds)
#define maxHis      10    //Maximum Hysteresis (centigrade degree)
#define maxSetPoint 50    //Maximum Set Point (centigrade degree)
#define pumpDelay   3000  //Pump Delay (milliseconds)
#define eeprom      false //Enable EEprom -> true // Disable EEprom -> false

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
unsigned long now, now2, startPumpTime = 0, stopPumpTime = 0;
bool waterLevel;
float  temp = 25;

struct Settings {
  unsigned int cDelay = 2, hDelay = 2, cHis = 2, hHis = 2, setPoint = 28, corr = 20;
};
Settings appSettings;
class EEprom {
  public:
    static void Save()
    {
      if (eeprom)
        EEPROM.put(0, appSettings);
    }
    static void Restore()
    {
      if (eeprom)
        EEPROM.get(0, appSettings);
    }
    static void Clear()
    {
      if (eeprom)
        for (int i = 0; i < EEPROM.length(); i++)
          EEPROM.write(i, 0);
    }
};

enum keys { up , down, right, left, ok, none};
keys ReadKeys() {
  int a = analogRead(A0);
  if (a < 50) { /* <= Incread/decrease value 50 */
    while (analogRead(A0) < 850) {}
    now = millis();
    return right; //ButtonRight;
  } else if (a < 200) { /* <= Incread/decrease value 200 */
    now = millis();
    return up; //ButtonUp;
  } else if (a < 400) { /* <= Incread/decrease value 350 */
    now = millis();
    return down; //ButtonDown;
  } else if (a < 600) { /* <= Incread/decrease value 500 */
    while (analogRead(A0) < 850) {}
    now = millis();
    return left; //ButtonLeft;
  } else if (a < 850) { /* <= Incread/decrease value 750 */
    while (analogRead(A0) < 850) {}
    now = millis();
    return ok; //ButtonSelect;
  } else {
    return none; //ButtonNone;
  }
}

int UpDown(keys key, int in, int _min, int _max) {
  if (key != up && key != down)
    return in;
  if (key == up)
    in++;
  if (key == down)
    in--;
  if (in > _max)
    in = _min;
  if (in < _min)
    in = _max;
  EEprom::Save();
  return in;
}

void subMenu(int m) {
  bool m2 = false, m3 = true;
  delay(d);
  while (true) {
    now2 = millis();
    keys key = ReadKeys();
    if (key == left || now2 > timeout + now)
      break;
    switch (m) {
      case 0:
        if (key == ok)
          m2 = !m2;
        if (m2) {
          if (m3)
            appSettings.cDelay = UpDown(key, appSettings.cDelay, 0, maxDelay);
          else
            appSettings.cHis = UpDown(key, appSettings.cHis, 0, maxHis);
        } else {
          if (key == up || (key == down))
            m3 = !m3;
        }
        lcd.clear();
        if (m3) {
          lcd.setCursor(0, 0);
          lcd.print("Cool Delay:");
          lcd.setCursor(0, 1);
          lcd.print(appSettings.cDelay);
          lcd.print("s");
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Cool Hister.:");
          lcd.setCursor(0, 1);
          lcd.print(appSettings.cHis);
          lcd.print((char)223);
          lcd.print("C");
        }
        lcd.setCursor(14, m2);
        lcd.print("<");
        break;
      case 1:
        if (key == ok)
          m2 = !m2;
        if (m2) {
          if (m3)
            appSettings.hDelay = UpDown(key, appSettings.hDelay, 0, maxDelay);
          else
            appSettings.hHis = UpDown(key, appSettings.hHis, 0, maxHis);
        } else {
          if (key == up || (key == down))
            m3 = !m3;
        }
        lcd.clear();
        if (m3) {
          lcd.setCursor(0, 0);
          lcd.print("Heat Delay:");
          lcd.setCursor(0, 1);
          lcd.print(appSettings.hDelay);
          lcd.print("s");
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Heat Hister.:");
          lcd.setCursor(0, 1);
          lcd.print(appSettings.hHis);
          lcd.print((char)223);
          lcd.print("C");
        }
        lcd.setCursor(14, m2);
        lcd.print("<");
        break;
      case 2:
        appSettings.corr = UpDown(key, appSettings.corr, 0, maxCorr);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp Correction:");
        lcd.setCursor(0, 1);
        lcd.print((appSettings.corr > maxCorr / 2) ? "+" : "");
        lcd.print((int)(appSettings.corr) - maxCorr / 2);
        lcd.print((char)223);
        lcd.print("C");
        break;
    }
    delay(d);
  }
}


void menu() {
  int m = 0;
  delay(d);
  while (true) {
    now2 = millis();
    keys key = ReadKeys();
    if (key == ok)
      subMenu(m);
    if (key == left || now2 > timeout + now)
      break;
    m = UpDown(key, m, 0, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cool/Heat/Corr.");
    lcd.setCursor(m * 5 + 2, 1);
    lcd.print("^");
    delay(d);
  }
}

void checkPump() {
  now = millis();
  if (waterLevel == HIGH) { //water level ok
    if (startPumpTime == 0) {
      startPumpTime = now + pumpDelay;
      stopPumpTime = 0;
    }
  } else {
    if (stopPumpTime == 0) { //water level not ok
      stopPumpTime = now + pumpDelay;
      startPumpTime = 0;
    }
  }
  if (startPumpTime > 0 && now > startPumpTime) {
    digitalWrite(pump, HIGH);
    startPumpTime = 0;
  }
  if (stopPumpTime > 0 && now > stopPumpTime) {
    digitalWrite(pump, LOW);
    stopPumpTime = 0;
  }
}

void setup() {
  lcd.begin(16, 2);
  pinMode(A0, INPUT);
  pinMode(x1, INPUT);
  pinMode(x3, INPUT);
  pinMode(element, OUTPUT);
  pinMode(waterIn, OUTPUT);
  pinMode(bypass, OUTPUT);
  pinMode(cool, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  lcd.clear();
  lcd.print("Soheil Optic co");
  delay(1000);
  EEprom::Restore();
}

int cNow, hNow;
void Cool() {
  if (temp > appSettings.setPoint + appSettings.cHis) {
    if (cNow == 0)
      cNow = millis();
    else if (millis() > cNow + (appSettings.cDelay * 1000)) {
      digitalWrite(cool, true);
      lcd.clear();
      lcd.print("Cooling");
      delay(500);
    } else
      digitalWrite(cool, false);
  } else {
    cNow = 0;
    digitalWrite(cool, false);
  }
}

void Heat() {
  if (temp < appSettings.setPoint - appSettings.hHis) {
    if (hNow == 0)
      hNow = millis();
    else if (millis() > hNow + (appSettings.hDelay * 1000)) {
      digitalWrite(element, true);
      lcd.clear();
      lcd.print("Heating");
      lcd.print(appSettings.hDelay);
      delay(500);
    } else
      digitalWrite(element, false);
  } else {
    hNow = 0;
    digitalWrite(element, false);
  }
}

float avg() {
  return temp;
}
//cDelay = 0, hDelay = 0, cHis = 0, hHis = 0, setPoint = 27, corr = 20;
void loop() {
  float  VRT = (5.00 / 1023.00) * analogRead(x1);
  temp = (1.00 / ((log((VRT / ((5.00 - VRT) / 10000.00)) / 10000.00) / 3977.00) + (1.00 / 298.15))) - 273.15;
  temp = avg();
  temp += appSettings.corr - 20;
  waterLevel = !digitalRead(x3);
  digitalWrite(waterIn, digitalRead(waterLevel));
  checkPump();
  keys key = ReadKeys();
  if (key == ok)
    menu();
  appSettings.setPoint = UpDown(key, appSettings.setPoint, 0, maxSetPoint);

  Cool();
  Heat();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp.: ");
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Set:   ");
  lcd.print(appSettings.setPoint);
  lcd.print((char)223);
  lcd.print("C");

  //digitalWrite(LED_BUILTIN,((now/2000)%2==0)?HIGH:LOW);

  delay(d);
}
