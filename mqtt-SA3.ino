#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <LiquidCrystal_I2C.h>
#include <ThingSpeak.h>


#define I2C_ADDR 0X27
#define LCD_COLUMNS 16
#define LCD_LINES  2

// Informações da rede Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Informações do servidor MQTT
const char* mqttServer = "broker.hivemq.com";
const int port = 1883;

// Tópicos MQTT
const char* temperatureTopic = "casa/temperatura";
const char* humidityTopic = "casa/umidade";
const char* actuator1Topic = "casa/acionador1";
const char* actuator2Topic = "casa/acionador2";

// Pinos dos atuadores
const int actuator1Pin = 13;
const int actuator2Pin = 14;

// Pino do sensor DHT
const int DHT_PIN = 15;

unsigned long myChannelNumber = 2;
const char* server = "api.thingspeak.com";
const char* myWriteApiKey = "31NFP9UI9EMX0PAJ";

int tempC;
int umidadeT;

DHTesp dhtSensor;

WiFiClient espClient;
PubSubClient client(espClient);

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

void setup() {
  Serial.begin(115200);
  connectToWifi();
  connectToMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  delay(10);

  lcd.init();
  lcd.backlight();

  pinMode(actuator1Pin, OUTPUT);
  pinMode(actuator2Pin, OUTPUT);

  ThingSpeak.begin(espClient);
}

void loop() {
  client.loop();
  publishTemperatureAndHumidity();
  checkThresholds();
  delay(5000);
}

void connectToWifi() {
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao Wi-Fi...");
  }
  
  Serial.println("Conectado ao Wi-Fi");
}

void connectToMqtt() {
  client.setServer(mqttServer, port);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Conectando ao broker MQTT...");
    
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao broker MQTT");
      client.subscribe(actuator1Topic);
      client.subscribe(actuator2Topic);
    } else {
      Serial.print("Falha na conexão ao broker MQTT. Estado: ");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Conteúdo da mensagem: ");
  Serial.println(message);
  
  // Verificar ação do acionador 1
  if (strcmp(topic, actuator1Topic) == 0) {
    if (message.equals("ON")) {
      digitalWrite(actuator1Pin, HIGH);
      Serial.println("Acionador 1 ligado");
    } else if (message.equals("OFF")) {
      digitalWrite(actuator1Pin, LOW);
      Serial.println("Acionador 1 desligado");
    }
  }
  
  // Verificar ação do acionador 2
  if (strcmp(topic, actuator2Topic) == 0) {
    if (message.equals("ON")) {
      digitalWrite(actuator2Pin, HIGH);
            Serial.println("Acionador 2 ligado");
    } else if (message.equals("OFF")) {
      digitalWrite(actuator2Pin, LOW);
      Serial.println("Acionador 2 desligado");
    }
  }
}

void publishTemperatureAndHumidity() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Publicar temperatura
  String temperaturePayload = String(data.temperature, 1) + "°C";
  client.publish(temperatureTopic, temperaturePayload.c_str());

  // Publicar umidade
  String humidityPayload = String(data.humidity, 1) + "%";
  client.publish(humidityTopic, humidityPayload.c_str());
}

void checkThresholds() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Verificar temperatura
  if (data.temperature > 35) {
    digitalWrite(actuator1Pin, HIGH);
    client.publish(actuator1Topic, "ON");
    Serial.println("Acionador 1 ativado (temperatura alta)");
  } else {
    digitalWrite(actuator1Pin, LOW);
    client.publish(actuator1Topic, "OFF");
    Serial.println("Acionador 1 desativado (temperatura normal)");
  }

  // Verificar umidade
  if (data.humidity > 70) {
    digitalWrite(actuator2Pin, HIGH);
    client.publish(actuator2Topic, "ON");
    Serial.println("Acionador 2 ativado (umidade alta)");
  } else {
    digitalWrite(actuator2Pin, LOW);
    client.publish(actuator2Topic, "OFF");
    Serial.println("Acionador 2 desativado (umidade normal)");
  }

  lcd.setCursor(0, 0);
  lcd.print(" Temp: " + String(data.temperature, 1) + "\xDF" + "C");
  lcd.setCursor(0, 1);
  lcd.print(" Umidade: " + String(data.humidity, 1) + "%");

  ThingSpeak.setField(1, data.temperature);
  ThingSpeak.setField(2, data.humidity);

  int x = ThingSpeak.writeFields(myChannelNumber,myWriteApiKey);
}


