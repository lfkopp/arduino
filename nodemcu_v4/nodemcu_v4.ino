#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include <UniversalTelegramBot.h>
#include <Servo.h>
#define LEDBLUE 16 //D0 BLUE        - Hora de alimentar
#define LEDGREEN 5 //D1 GREEN       - 
#define LEDRED 4 //D2 RED           - Sem conexão
#define ONE_WIRE_BUS 2 // D4        - 
#define SERVO 14 //D5               - 
#define RELAY1 12 //D6              - 
#define RELAY2 13 //D7              - 
#define BOTtoken "445110681:AAG1sEVjzZ2vRbJuTaKOuM8UlSytOAsIttc"

float temp;

SimpleTimer timer;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// WIFI
//const char* ssid = "07dezembro";
//const char* password = "jujulipe";
const char* ssid = "zanzando";
const char* password = "aquitembaratanacomida";
//const char* ssid = "lfkopp";
//const char* password = "dezembro";


// THINGSPEAK
WiFiClient  client_thingspeak;
String      apiKey = "Y8VNVETDNZQ4EBRA";
const char* server_thingspeak = "api.thingspeak.com";
const long thingspeak_time = 5 * 60 * 1000; // 5 minutos


// TELEGRAM
WiFiClientSecure client_telegram;
String id;
String text;
IPAddress server(149, 154, 167, 200); // IP de api.telegram.org
UniversalTelegramBot bot(BOTtoken, client_telegram);
const long telegram_time = 1000;


// STEPPER or SERVO
Servo myservo;
const long alimenta_time = 1 * 60 * 60 * 1000; // 1 HORA
unsigned long alimenta = alimenta_time / 2;


unsigned long bomba = 10000;
unsigned long bomba_time = 5 * 60 * 1000;
unsigned long telegram = 0;
unsigned long thingspeak = 0;
int bomba_ligada = 0;


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDBLUE, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  testa_led();
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  WiFi.mode(WIFI_STA);
  delay(100);
  connectwifi();
  delay(100);
  myservo.attach(SERVO);
  myservo.write(0);
  bot.sendMessage("126554909", "iniciando...", "");
}

void loop() {
  yield();
  if (alimenta < millis()) {
    Serial.println("alimenta");
    alimentar();
  }
  if (bomba_ligada == 0 && bomba < millis()) {
    Serial.println("bomba_ligada");
    liga_bomba();
    digitalWrite(LEDBLUE,  LOW);
  }

  if (thingspeak < millis()) {
    Serial.println("thingspeak");
    envia_thingspeak();
  }
  if (telegram < millis()) {
    Serial.println("telegram");
    readTel();
  }
}

void envia_telegram(String id, String msg) {
  bot.sendSimpleMessage(String(id), String(msg), "");
}

void envia_thingspeak() {
  if (client_thingspeak.connect(server_thingspeak, 80)) {
    temp = pega_temp();
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temp);
    postStr += "\r\n\r\n";
    client_thingspeak.print("POST /update HTTP/1.1\n");
    client_thingspeak.print("Host: api.thingspeak.com\n");
    client_thingspeak.print("Connection: close\n");
    client_thingspeak.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client_thingspeak.print("Content-Type: application/x-www-form-urlencoded\n");
    client_thingspeak.print("Content-Length: ");
    client_thingspeak.print(postStr.length());
    client_thingspeak.print("\n\n");
    client_thingspeak.print(postStr);
  }
  thingspeak = millis() + thingspeak_time;
}

float pega_temp() {
  DS18B20.requestTemperatures();
  delay(100);
  return DS18B20.getTempCByIndex(0);
}

void desliga_bomba() {
  digitalWrite(RELAY1, HIGH);
  bomba_ligada = 0;
  envia_telegram(id, "bomba desligada");
}

void liga_bomba() {
  digitalWrite(RELAY1, LOW);
  bomba_ligada = 1;
  envia_telegram(id, "bomba ligada");
}

void alimentar() {
  digitalWrite(LEDBLUE, HIGH);
  myservo.write(0);
  delay(100);
  myservo.write(180);
  delay(3000);
  myservo.write(0);
  envia_telegram(id, "peixinhos alimentados");
  alimenta = millis() + alimenta_time;
  bomba = millis() + bomba_time;
}

void testa_led() {
  digitalWrite(LEDGREEN, HIGH);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  LOW);
  delay(500);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   HIGH);
  digitalWrite(LEDBLUE,  LOW);
  delay(500);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  HIGH);
  delay(500);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  LOW);
}

void connectwifi() {
  client_thingspeak.stop();
  client_telegram.stop();
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDRED,   HIGH);
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 20) {
      Serial.println("sem conexao");
      delay(500);
    }
  }
  digitalWrite(LEDRED,   LOW);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
  delay(500);
}

void readTel() {
  telegram = millis() + telegram_time;
  int newmsg = bot.getUpdates(bot.last_message_received + 1);
  for (int i = 0; i < newmsg; i++) {
    id = bot.messages[i].chat_id;
    text = bot.messages[i].text;
    text.toUpperCase();
    if (text.indexOf("TEMP") > -1) {
      envia_telegram(id, "temp: " + String(pega_temp()));
    } else if (text.indexOf("BLUE") > -1) {
      digitalWrite(LEDBLUE, HIGH);
      envia_telegram(id, "BLUE ON");
    } else if (text.indexOf("GREEN") > -1) {
      digitalWrite(LEDGREEN, HIGH);
      envia_telegram(id, "GREEN ON");
    } else if (text.indexOf("RED") > -1) {
      digitalWrite(LEDRED, HIGH);
      envia_telegram(id, "RED ON");
    } else if (text.indexOf("OFF") > -1) {
      testa_led();
      envia_telegram(id, "LED OFF");
    } else if (text.indexOf("GETID") > -1) {
      envia_telegram(id, id);
    } else if (text.indexOf("START") > -1) {
      envia_telegram(id, "bem vindo ao canal https://thingspeak.com/channels/35939");
    } else if (text.indexOf("TIME") > -1) {
      envia_telegram( id, String(millis() / 1000 / 60) + " minutos");
    } else if (text.indexOf("RESET") > -1) {
      ESP.reset();
    } else if (text.indexOf("ALIMENTA") > -1) {
      alimentar();
    } else if (text.indexOf("NOBOMB") > -1) {
      desliga_bomba();
    } else if (text.indexOf("BOMB") > -1) {
      liga_bomba();
    } else {
      yield();
    }
    yield();
  }

}
