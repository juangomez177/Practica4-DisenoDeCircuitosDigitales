// C++ code

// Librerías requeridas
#include <Adafruit_LiquidCrystal.h>
#include <Servo.h>
#include <avr/sleep.h>

// Pantalla LCD
Adafruit_LiquidCrystal lcd(0);

// Motor Servo
Servo myservo;
int motor = 8;

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
const int maxLength = 32; // Se permitirá una longitud de mensaje de hasta 32 caracteres
char inputS[maxLength];   // Recibe el imput de la entrada Serial

char contrasena[maxLength]; // Para guardar la contraseña
bool contrasenaAct = false; // Para controlar si la contrasena está activa

char mensaje[maxLength]; // Para guardar el mesaje que se imprimirá por pantalla

char c0[maxLength];       // Calefacción Minima
char c1[maxLength];       // Calefacción Bajo
char c2[maxLength];       // Calefacción Medio
char c3[maxLength];       // Calefacción Alto
char c4[maxLength];       // Calefacción Máxima
char cMensaje[maxLength]; // Para imprimir el estado de la calefacción

char v1[maxLength];       // Ventilación Minima
char v2[maxLength];       // Ventilación Normal
char v3[maxLength];       // Ventilación Intermedia
char v4[maxLength];       // Ventilación Máxima
char vMensaje[maxLength]; // Para imprimir el estado de la ventilación

char c; // para guardatr 1 sólo caracter

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
    suspension,
    maximo,
    alto,
    medio,
    bajo,
    minimo,

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
    Serial.begin(9600); // Intenta con diferentes valores de baud rate

    // Configuración del Timer 2
    cli();
    TCCR2B = (TCCR2B & 0xF8) | 0x05; // 0x05 ajusta el prescaler a 1024
    TIMSK2 |= (1 << TOIE2);
    TCNT2 = 0;
    sei();
}

void loop()
{
    // Lee la entrada serial
    leerSerial();

    // Lee el Switch
    switchValue = digitalRead(switchConm);

    if (switchValue == 1)
    {
        // Verificar la contraseña solo si el switch está en posición 1
        if (strcmp(contrasena, "1234") == 0)
        {
            if (contrasenaAct == false)
            {
                Serial.println("CONTRASENA OK");
                lcd.clear();
                lcd.setCursor(0, 1);
                lcd.print("CONTRASENA OK   ");

                contrasenaAct = true;
            }

            estadoA = encendido;//Se enciende el Sistema para entrar en estado de operación
        }
        else
        {
            Serial.println("INGRESE CONTRASENA");
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("INGRESE CONTRASENA");

            estadoA = apagado;
        }
    }
    else
    {
        estadoA = apagado;
    }

    // Estados principales
    switch (estadoA)
    {

    case encendido:
        // Verifica si hay que mostrar un mensaje
        activarMensaje();

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
                Serial.println("FUGA DE LUZ");
                lcd.clear();
                lcd.setCursor(0, 1);
                lcd.print("FUGA DE LUZ     ");
            }
            if (temp >= 227)
            {
                Serial.println("Temp>60");
                lcd.clear();
                lcd.setCursor(0, 1);
                lcd.print("Temp>60         ");
            }
            if (co2 >= 662)
            {
                Serial.println("C02 ELEVADO");
                lcd.clear();
                lcd.setCursor(0, 1);
                lcd.print("C02 ELEVADO     ");
            }
            estadoB = suspension; // Entra el sistema en estado de suspension
            noSuspension = false;
        }
        // No hay un estado de suspensión
        else
        {
            Serial.println("HORNO ON");
            lcd.clear();
            lcd.setBacklight(1);
            lcd.print("HORNO ON        ");
            digitalWrite(luzOn, HIGH); // Enciende la luz del horno

            noSuspension = true; // Se quita el estado de suspension (En caso de haberlo)
        }

        // Comprueba si hay un estado de suspensión; si no hay suspensión, el temporizador de 30 segundos se mantiene para la calefacción y ventilación
        if (noSuspension == true & segundos<=1)
        {
            // Condiciones de temperatura para actualizar la calefacción cada 30 segundos

            // Para la lectura por monitor serial
            if (strcmp(c4, "C4") == 0)
            {
                estadoB = maximo;
            }
            else if (strcmp(c3, "C3") == 0)
            {
                estadoB = alto;
            }
            else if (strcmp(c2, "C2") == 0)
            {
                estadoB = medio;
            }
            else if (strcmp(c1, "C1") == 0)
            {
                estadoB = bajo;
            }
            else if (strcmp(c0, "C0") == 0)
            {
                estadoB = minimo;
            }

            // Para la lectura por la variable análoga
            else if (temp < 145)
            {
                estadoB = maximo;
            }
            else if (temp < 164 & temp >= 145)
            {
                estadoB = alto;
            }
            else if (temp < 184 & temp >= 164)
            {
                estadoB = medio;
            }
            else if (temp < 207 & temp >= 184)
            {
                estadoB = bajo;
            }
            else if (temp < 227 & temp >= 207)
            {
                estadoB = minimo;
            }

            // Condiciones de C02 para la ventilación, cada 30 segundos

            // Para la lectura por monitor serial
            if (strcmp(v1, "V1") == 0) // Nivel de CO2 Mínimo, compuertas cerradas
            {
                strcpy(vMensaje, "Vent MIN, C=0   ");
                myservo.write(0);
            }
            else if (strcmp(v2, "V2") == 0) // Nivel de CO2 Normal, compuertas abiertas 60°
            {
                strcpy(vMensaje, "Vent NORM, C=60 ");
                myservo.write(60);
            }
            else if (strcmp(v3, "V3") == 0)
            {
                strcpy(vMensaje, "Vent INT, C=120 "); // Nivel de CO2 Intermedio, compuertas abiertas 120°
                myservo.write(120);
            }
            else if (strcmp(v4, "V4") == 0)
            {
                strcpy(vMensaje, "Vent MAX, C=180 "); // Nivel de CO2 Máximo, compuertas abiertas 180° completamente
                myservo.write(180);
            }

            // Para la lectura por la variable análoga
            else if (co2 < 395)
            {
                strcpy(vMensaje, "Vent MIN, C=0   "); // Nivel de CO2 Mínimo, compuertas cerradas
                myservo.write(0);
            }
            else if (co2 < 484 && co2 >= 395)
            {
                strcpy(vMensaje, "Vent NORM, C=60 "); // Nivel de CO2 Normal, compuertas abiertas 60°
                myservo.write(60);
            }
            else if (co2 < 573 && co2 >= 484)
            {
                strcpy(vMensaje, "Vent INT, C=120 "); // Nivel de CO2 Intermedio, compuertas abiertas 120°
                myservo.write(120);
            }
            else if (co2 < 662 && co2 >= 573)
            {
                strcpy(vMensaje, "Vent MAX, C=180 "); // Nivel de CO2 Máximo, compuertas abiertas 180° completamente
                myservo.write(180);
            }
        } // FIN DEL IF QUE EVALUA LA CONDICIÓN DE TEMPORIZADOR DE 30 SEGUNDOS

        // Estados anidados
        switch (estadoB)
        {

        case maximo:
            strcpy(cMensaje, "Temp [<20]      Calef MAX       ");

            digitalWrite(luz1, HIGH);
            digitalWrite(luz2, HIGH);
            digitalWrite(luz3, HIGH);
            digitalWrite(luz4, HIGH);

            break;

        case alto:
            strcpy(cMensaje, "Temp [20 30]    Calef ALTA      ");

            digitalWrite(luz1, HIGH);
            digitalWrite(luz2, HIGH);
            digitalWrite(luz3, HIGH);
            digitalWrite(luz4, LOW);

            break;

        case medio:
            strcpy(cMensaje, "Temp [30 40]    Calef MEDIA     ");

            digitalWrite(luz1, HIGH);
            digitalWrite(luz2, HIGH);
            digitalWrite(luz3, LOW);
            digitalWrite(luz4, LOW);

            break;

        case bajo:
            strcpy(cMensaje, "Temp [40 50]    Calef BAJA      ");

            digitalWrite(luz1, HIGH);
            digitalWrite(luz2, LOW);
            digitalWrite(luz3, LOW);
            digitalWrite(luz4, LOW);

            break;

        case minimo:
            strcpy(cMensaje, "Temp [50 60]    Calef MIN       ");

            digitalWrite(luz1, LOW);
            digitalWrite(luz2, LOW);
            digitalWrite(luz3, LOW);
            digitalWrite(luz4, LOW);

            break;

        case suspension:
            Serial.println("HORNO SUSPENDIDO");
            lcd.clear();
            lcd.print("HORNO SUSPENDIDO");

            // Enciende la alarma, apaga las luces y pone el led del horno en intermitente
            apagarLuces();
            tone(alarma, 2500);
            digitalWrite(luzOn, LOW);
            delay(500);
            noTone(alarma);
            digitalWrite(luzOn, HIGH);

            // Reinicia las variables de conteo
            segundos = 0;
            counter = 0;

            break;
        }

        if (noSuspension == true)
        {
            imprimirEstadoCalefaccion(cMensaje); // Imprime el estado de la calefacción
            imprimirEstadoVentilacion(vMensaje); // Imprime el estado de la ventilación
        }

        Serial.println("");
        break;

    case apagado:
        Serial.println("HORNO APAGADO");
        Serial.println("");
        lcd.clear();
        lcd.print("HORNO OFF       ");

        // Apaga los mecanismos de salida
        apagarLuces();

        // Reinicia las variables de conteo
        counter = 0;
        segundos = 0;

        // Reinicia las variables del la entrada Serial
        inputS[0] = '\0';
        contrasena[0] = '\0';
        contrasenaAct = false;

        break;
    }

    // Para evitar saturación de la ejecución
    Serial.flush();
    lcd.clear();
    // delay(1000);
    // sleep_enable();
}

// Método para apagar las luces y compuertas
void apagarLuces()
{
    digitalWrite(luzOn, LOW);
    digitalWrite(luz1, LOW);
    digitalWrite(luz2, LOW);
    digitalWrite(luz3, LOW);
    digitalWrite(luz4, LOW);
    myservo.write(0);
}

// Método para imprimir mensaje que llega del monitor serial
void activarMensaje()
{
    c = inputS[0]; // Lee el caracter cero de la cadena
    if (c == '>')  // Verifica si es un mesaje
    {
        Serial.println("MENSAJE");
        Serial.println(inputS);
        lcd.clear();
        lcd.print("MENSAJE         ");
        lcd.setCursor(0, 1);
        lcd.print(inputS + 1); // Imprimir los primeros 16 caracteres del mensaje
        delay(500);
        lcd.clear();
        lcd.print(inputS + 17); // imprime el resto del mensaje, si es necesario
        delay(500);
    }
}

// Metodo para imprimir Estado de calefaccion
void imprimirEstadoCalefaccion(const char *mensaje)
{

    Serial.println(mensaje);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(mensaje);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(mensaje + 16);
}

// Metodo para imprimir Estado de ventilacion
void imprimirEstadoVentilacion(const char *mensaje)
{
    Serial.println(mensaje);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(mensaje);
}

// Medodo de impresión del estado de las variables ingresadas por el monitor serial
void imprimirMecanismo(const char *mensaje, bool tipo)
{
    if (tipo == true) // Activado
    {
        Serial.println("ACTIVADO");
        Serial.println(mensaje);
        lcd.clear();
        lcd.print("ACTIVADO");
        lcd.setCursor(0, 1);
        lcd.print(mensaje);
        lcd.print("              ");
    }
    else // Desactivado
    {
        Serial.println("DESACTIVADO");
        Serial.println(mensaje);
        lcd.clear();
        lcd.print("DESACTIVADO");
        lcd.setCursor(0, 1);
        lcd.print(mensaje);
        lcd.print("              ");
    }
    Serial.println("");
}

// Método para leer la entrada del monitor serial, ya sea para la contrasena, imprimir un mensaje o actvar/desactivar algún mecanismo del horno
void leerSerial()
{
    if (Serial.available() > 0)
    {
        int bytesRead = Serial.readBytesUntil('\n', inputS, sizeof(inputS) - 1);
        inputS[bytesRead] = '\0'; // Agrega el carácter nulo al final de la cadena

        if (bytesRead == sizeof(inputS) - 1)
        {
            Serial.println("Input muy largo");
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Input muy largo ");
        }
    }

    // Para contrasena
    if (strcmp(inputS, "1234") == 0)
    {
        strcpy(contrasena, inputS);
    }

    // Para activar algún mecanismo del horno
    else if (strcmp(inputS, "C0") == 0)
    {
        strcpy(c0, inputS);
        c1[0] = '\0';
        c2[0] = '\0';
        c3[0] = '\0';
        c4[0] = '\0';
        imprimirMecanismo(c0, true);
    }
    else if (strcmp(inputS, "C1") == 0)
    {
        strcpy(c1, inputS);
        c0[0] = '\0';
        c2[0] = '\0';
        c3[0] = '\0';
        c4[0] = '\0';
        imprimirMecanismo(c1, true);
    }
    else if (strcmp(inputS, "C2") == 0)
    {
        strcpy(c2, inputS);
        c0[0] = '\0';
        c1[0] = '\0';
        c3[0] = '\0';
        c4[0] = '\0';
        imprimirMecanismo(c2, true);
    }
    else if (strcmp(inputS, "C3") == 0)
    {
        strcpy(c3, inputS);
        c0[0] = '\0';
        c1[0] = '\0';
        c2[0] = '\0';
        c4[0] = '\0';
        imprimirMecanismo(c3, true);
    }
    else if (strcmp(inputS, "C4") == 0)
    {
        strcpy(c4, inputS);
        c0[0] = '\0';
        c1[0] = '\0';
        c2[0] = '\0';
        c3[0] = '\0';
        imprimirMecanismo(c4, true);
    }
    else if (strcmp(inputS, "V1") == 0)
    {
        strcpy(v1, inputS);
        v2[0] = '\0';
        v3[0] = '\0';
        v4[0] = '\0';
        imprimirMecanismo(v1, true);
    }
    else if (strcmp(inputS, "V2") == 0)
    {

        strcpy(v2, inputS);
        v1[0] = '\0';
        v3[0] = '\0';
        v4[0] = '\0';
        imprimirMecanismo(v2, true);
    }
    else if (strcmp(inputS, "V3") == 0)
    {
        strcpy(v3, inputS);
        v1[0] = '\0';
        v2[0] = '\0';
        v4[0] = '\0';
        imprimirMecanismo(v3, true);
    }
    else if (strcmp(inputS, "V4") == 0)
    {
        strcpy(v4, inputS);
        v1[0] = '\0';
        v2[0] = '\0';
        v3[0] = '\0';
        imprimirMecanismo(v4, true);
    }

    // Para el mensaje
    else
    {
        strcpy(mensaje, inputS);
    }

    // Desactivar algun mecanismo del horno
    if (strcmp(inputS, "DC") == 0)
    {
        c0[0] = '\0';
        c1[0] = '\0';
        c2[0] = '\0';
        c3[0] = '\0';
        c4[0] = '\0';
        imprimirMecanismo("Calef AUTO      ", false);
    }
    else if (strcmp(inputS, "DV") == 0)
    {
        // Serial.println("Desactivando DV");
        v1[0] = '\0';
        v2[0] = '\0';
        v3[0] = '\0';
        v4[0] = '\0';
        imprimirMecanismo("Ventil AUTO     ", false);
    }
    inputS[0] = '\0'; // Limpia el imput del monitor serial
}
