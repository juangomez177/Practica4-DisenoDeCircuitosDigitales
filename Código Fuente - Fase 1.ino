// C++ code

#include <Adafruit_LiquidCrystal.h>
#include <Servo.h>
#include <avr/sleep.h>

// Pantalla LCD
Adafruit_LiquidCrystal lcd(0);

// Motor Servo
Servo myservo;
int motor = 8;
int angle = 0;

// Entradas/Salidas Digitales
int luzOn = 2;
int luz1 = 3;
int luz2 = 4;
int luz3 = 5;
int luz4 = 6;
int alarma = 7;
int switchConm = 9;
int switchValue = 0;

// Entradas/Salidas Análogas
int sensorT = A0;
int temp = 0;

int sensorCo2 = A1;
int co2 = 0;

int sensorLuz = A2;
int luz = 0;

// Input del monitor en serie
char inputS;

// Variables para el temporizador
int segundos = 0;
int counter = 0;

// Variables adicionales
bool noSuspension = false; // Verifica si no hay un estado de suspension antes de hacer cualquier lectura

// Estados del sistema
// Principales
enum fsm_estadosA
{
  encendido,
  apagado,

};
enum fsm_estadosA estadoA;

// Anidados, que se activan cuando está el sistema encendido
enum fsm_estadosB
{
  suspendido,
  maximo,
  intermedio,
  moderado,
  minimoA,
  minimoB,

};
enum fsm_estadosB estadoB;

// Subrutina de interrupción del Timer 2
ISR(TIMER2_OVF_vect)
{
  counter++;
  // Serial.println("Ejecutando SUBRUTINA");

  if (counter == 255) // Cada vez que el contado llegue a 255 es igual a 1 segundo en tiempo de ejecución
  {
    segundos++;
    counter = 0;
  }

  if (segundos == 30) // Reinicia los segundos para las condiciones de temperatura
  {
    segundos = 0;
    counter = 0;
    // Serial.println("SEGUNDOS ES 30");
    // Serial.println("");
  }
}

void setup()
{
  // Entradas/Salidas Digitales-análogas
  pinMode(switchConm, INPUT);
  pinMode(sensorT, INPUT);
  pinMode(sensorCo2, INPUT);
  pinMode(sensorLuz, INPUT);

  pinMode(luzOn, OUTPUT);
  pinMode(luz1, OUTPUT);
  pinMode(luz2, OUTPUT);
  pinMode(luz3, OUTPUT);
  pinMode(luz4, OUTPUT);
  pinMode(alarma, OUTPUT);

  // Config del LCD
  lcd.begin(16, 2);
  lcd.setBacklight(1);

  // Config del Servo motor
  myservo.attach(motor);

  // Config del Serial
  Serial.begin(9600);

  // Configuración del Timer 2
  cli();                           // Detiene todas las interrupciones
  TCCR2B = (TCCR2B & 0xF8) | 0x05; // 0x05 ajusta el prescaler a 1024
  TIMSK2 |= (1 << TOIE2);
  TCNT2 = 0;
  sei();
}

void loop()
{
  Serial.println("SEGUNDOS TRANSCURRIDOS");
  Serial.println(segundos);
  Serial.println("");

  // Lee el Switch
  switchValue = digitalRead(switchConm);
  
  if (switchValue == 1 )
  {
   estadoA = encendido;
  }
  else
  {
    estadoA = apagado;
  }

  // Estados principales
  switch (estadoA)
  {

  case encendido:

    // Lectura de los sensores
    temp = analogRead(sensorT);
    // Serial.println(temp);
    luz = analogRead(sensorLuz);
    // Serial.println(luz);
    co2 = analogRead(sensorCo2);
    // Serial.println(co2);

    // Condiciones de Suspensión, pueden ocurrir en cualquier momento
    // Ocurren cuando se detecta cierto umbral de luz en el horno
    // O cuando la temperatura es mayor a 60°
    // O cuando las concentraciones de C02 son elevadas y el abierto de compuestas no suple los niveles
    if (luz < 946 | temp >= 227 | co2 >= 662)
    {
      // Para  Impresiones
      if (luz < 946)
      {
        Serial.println("SUSPENSION - FUGA DE LUZ");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("FUGA DE LUZ    ");
      }
      if (temp >= 227)
      {
        Serial.println("SUSPENSION - Temperatura>60");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Temp>60        ");
      }
      if (co2 >= 662)
      {
        Serial.println("SUSPENSION - C02 ELEVADO");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("C02 ELEVADO    ");
      }
      estadoB = suspendido; // Entra el sistema en estado de suspension
      noSuspension = false;
    }
    // No hay un estado de suspensión
    else
    {
      Serial.println("HORNO HERMETICO ENCENDIDO");
      lcd.clear();
      lcd.setBacklight(1);
      lcd.print("HORNO ON       ");
      digitalWrite(luzOn, HIGH);
      Serial.println("NO HAY ESTADO DE SUSPENSION");

      noSuspension = true; // Se quita el estado de suspension (En caso de haberlo)
    }

    // Comprueba que si hay un estado de suspensión,  no hay que oparar sobre las compuertas y el temporizador de 30 segundos se mantiene
    if (noSuspension == true & segundos == 0)
    {
      // Condiciones de temperatura para actualizar la calefacción cada 30 segundos
      if (temp < 145)
      {
        estadoB = maximo;
      }
      if (temp < 164 & temp >= 145)
      {
        estadoB = intermedio;
      }
      if (temp < 184 & temp >= 164)
      {
        estadoB = moderado;
      }
      if (temp < 207 & temp >= 184)
      {
        estadoB = minimoA;
      }
      if (temp < 227 & temp >= 207)
      {
        estadoB = minimoB;
      }
    }

    if (noSuspension == true)
    {
      // Condiciones de C02 para la ventilación, pueden ocurrir en cualquier momento
      if (co2 < 395)
      { // Nivel de CO2 Mínimo, compuertas cerradas
        Serial.println("Ventilacion MINIMA - Compuestas Cerradas");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Vent MIN, C=0  ");

        myservo.write(0);
      }
      if (co2 < 484 & co2 >= 395)
      { // Nivel de CO2 Normal, compuertas abiertas 60°
        Serial.println("Ventilacion NORMAL - Compuestas a 60");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Vent NORM, C=60");

        myservo.write(60);
      }
      if (co2 < 573 & co2 >= 484)
      { // Nivel de CO2 Intermedio, compuertas abiertas 120°
        Serial.println("Ventilacion INTERMEDIA - Compuestas a 120");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Vent INT, C=120");

        myservo.write(120);
      }
      if (co2 < 662 & co2 >= 573)
      { // Nivel de CO2 Máximo, compuertas abiertas 180° completamente

        Serial.println("Ventilacion MAXIMA - Compuestas a 180");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Vent MAX, C=180");

        myservo.write(180);
      }
    }

    // Estados anidados
    switch (estadoB)
    {

    case maximo:
      Serial.println("HORNO ENCENDIDO - Temp [<20] - Calefaccion MAXIMO");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Temp >20       ");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Calef MAX      ");

      digitalWrite(luz1, HIGH);
      digitalWrite(luz2, HIGH);
      digitalWrite(luz3, HIGH);
      digitalWrite(luz4, HIGH);

      break;

    case intermedio:
      Serial.println("HORNO ENCENDIDO - Temp [20 30] - Calefaccion INTERMEDIO");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Temp [20 30]    ");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Calef INTERMED");

      digitalWrite(luz1, HIGH);
      digitalWrite(luz2, HIGH);
      digitalWrite(luz3, HIGH);
      digitalWrite(luz4, LOW);

      break;

    case moderado:
      Serial.println("HORNO ENCENDIDO - Temp [30 40] - Calefaccion MODERADO");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Temp [30 40]   ");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Calef MODERADO ");

      digitalWrite(luz1, HIGH);
      digitalWrite(luz2, HIGH);
      digitalWrite(luz3, LOW);
      digitalWrite(luz4, LOW);

      break;

    case minimoA:
      Serial.println("HORNO ENCENDIDO - Temp [40 50] - Calefaccion MINIMO PARCIAL");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Temp [40 50]   ");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Calef MIN P    ");

      digitalWrite(luz1, HIGH);
      digitalWrite(luz2, LOW);
      digitalWrite(luz3, LOW);
      digitalWrite(luz4, LOW);

      break;

    case minimoB:
      Serial.println("HORNO ENCENDIDO - Temp [50 60] - Calefaccion MINIMO FINAL");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Temp [50 60]   ");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Calef MIN FINAL");

      digitalWrite(luz1, LOW);
      digitalWrite(luz2, LOW);
      digitalWrite(luz3, LOW);
      digitalWrite(luz4, LOW);

      break;

    case suspendido:
      Serial.println("HORNO SUSPENDIDO");
      Serial.println("");
      lcd.clear();
      lcd.print("HORNO SUSPENDIDO");

      digitalWrite(luz1, LOW);
      digitalWrite(luz2, LOW);
      digitalWrite(luz3, LOW);
      digitalWrite(luz4, LOW);
      myservo.write(0);

      // Enciende la alarma
      tone(alarma, 2500);
      digitalWrite(luzOn, HIGH);
      delay(500);
      noTone(alarma);
      digitalWrite(luzOn, LOW);

      // Reinicia las variables de conteo
      segundos = 0;
      counter = 0;

      break;
    }
    Serial.println("");
    break;

  case apagado:
    Serial.println("HORNO APAGADO");
    Serial.println("");
    lcd.clear();
    lcd.print("HORNO OFF      ");

    digitalWrite(luzOn, LOW);
    digitalWrite(luz1, LOW);
    digitalWrite(luz2, LOW);
    digitalWrite(luz3, LOW);
    digitalWrite(luz4, LOW);
    myservo.write(0);

    // Reinicia las variables de conteo
    counter = 0;
    segundos = 0;

    break;
  }
  // delay(1000);
  sleep_enable();
}
