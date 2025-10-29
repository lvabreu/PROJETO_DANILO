// ====== Blynk Configuração ======
#define BLYNK_TEMPLATE_ID "TMPL2SmHyie2X"
#define BLYNK_TEMPLATE_NAME "Supervisório Danilo"
#define BLYNK_AUTH_TOKEN "ceN3txRs_7NoPG1HmxxD44FBVJYlZsU1"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// ====== Credenciais Wi-Fi ======
char ssid[] = "Wi-fi OPTIMUSJr";       // coloque o nome da sua rede Wi-Fi
char pass[] = "interoptimus";      // coloque a senha da sua rede Wi-Fi

// ====== Sensor de Fluxo 1 (apenas envia vazão em V0) ======
const int PINO_SENSOR1 = 19;
const int INTERRUPCAO_SENSOR1 = digitalPinToInterrupt(PINO_SENSOR1);
volatile unsigned long contador1 = 0;
float fluxo1 = 0;

// ====== Sensor de Fluxo 2 (calcula volume e envia vazão em V3) ======
const int PINO_SENSOR2 = 18;
const int INTERRUPCAO_SENSOR2 = digitalPinToInterrupt(PINO_SENSOR2);
volatile unsigned long contador2 = 0;
float fluxo2 = 0;
float volume = 0;
float volume_total = 0;

// ====== Sensor Digital ======
const int PINO_DIGITAL = 5;  // ajuste conforme seu hardware

// ====== Sensor JSN-SR04T (nível em V1) ======
const int trigPin = 13;
const int echoPin = 12;
float distancia = 0.0;

// ====== Constantes ======
const float FATOR_CALIBRACAO = 4.5;
unsigned long tempo_antes = 0;

// ====== Funções de Interrupção ======
void IRAM_ATTR contador_pulso1() { contador1++; }
void IRAM_ATTR contador_pulso2() { contador2++; }

// ====== Função de Medição de Distância ======
float medirDistancia() {
  long duracao;
  float distancia_cm;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duracao = pulseIn(echoPin, HIGH, 30000); // timeout de 30ms
  distancia_cm = duracao * 0.034 / 2;

  if (distancia_cm < 25 || distancia_cm > 450) return -1; // fora do range confiável
  return distancia_cm;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 - Vazão (2 sensores) + Nível JSN-SR04T + Sensor Digital + Blynk");

  // Sensores de fluxo
  pinMode(PINO_SENSOR1, INPUT);
  attachInterrupt(INTERRUPCAO_SENSOR1, contador_pulso1, FALLING);

  pinMode(PINO_SENSOR2, INPUT);
  attachInterrupt(INTERRUPCAO_SENSOR2, contador_pulso2, FALLING);

  // Sensor digital
  pinMode(PINO_DIGITAL, INPUT);

  // JSN-SR04T
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  if ((millis() - tempo_antes) > 1000) {
    // ====== Sensor de Fluxo 1 (V0) ======
    detachInterrupt(INTERRUPCAO_SENSOR1);
    fluxo1 = ((1000.0 / (millis() - tempo_antes)) * contador1) / FATOR_CALIBRACAO;
    Serial.print("Fluxo 1: ");
    Serial.print(fluxo1, 2);
    Serial.println(" L/min");
    Blynk.virtualWrite(V0, fluxo1); // só envia vazão
    contador1 = 0;
    attachInterrupt(INTERRUPCAO_SENSOR1, contador_pulso1, FALLING);

    // ====== Sensor de Fluxo 2 (V3 + volume em V2) ======
    detachInterrupt(INTERRUPCAO_SENSOR2);
    fluxo2 = ((1000.0 / (millis() - tempo_antes)) * contador2) / FATOR_CALIBRACAO;
    volume = fluxo2 / 60.0;       // L por segundo
    volume_total += volume;

    Serial.print("Fluxo 2: ");
    Serial.print(fluxo2, 2);
    Serial.println(" L/min");

    Serial.print("Volume acumulado (sensor 2): ");
    Serial.print(volume_total, 3);
    Serial.println(" L");
    Blynk.virtualWrite(V3, fluxo2);
    Blynk.virtualWrite(V2, volume_total);
    contador2 = 0;
    attachInterrupt(INTERRUPCAO_SENSOR2, contador_pulso2, FALLING);

    // ====== Nível (JSN-SR04T) em V1 ======
    distancia = medirDistancia();
    if (distancia > 0) {
      Serial.print("Nível (distância): ");
      Serial.print(distancia, 1);
      Serial.println(" cm");
      Blynk.virtualWrite(V1, distancia);
    } else {
      Serial.println("Distância fora do range confiável.");
    }

    // ====== Sensor Digital em V4 ======
    int estadoDigital = digitalRead(PINO_DIGITAL); // 0 ou 1
    Serial.print("Sensor Digital: ");
    Serial.println(estadoDigital);
    Blynk.virtualWrite(V4, estadoDigital);

    tempo_antes = millis();
  }
}

