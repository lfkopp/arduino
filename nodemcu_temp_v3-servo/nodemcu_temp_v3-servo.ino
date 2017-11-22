
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include <UniversalTelegramBot.h>
#include <Servo.h>
//#include <Esp.h> //#include <Arduino.h> //#include <WiFiClientSecure.h>


#define LEDBLUE 16 //D0 BLUE        - Hora de alimentar
#define LEDGREEN 5 //D1 GREEN       - 
#define LEDRED 4 //D2 RED           - 
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
const char* ssid = "zanzando";
const char* password = "clienteviadofdp";
//const char* ssid = "07dezembro";
//const char* password = "jujulipe";
//const char* ssid = "lfkopp";
//const char* password = "dezembro";


// THINGSPEAK
WiFiClient  client_thingspeak;
String      apiKey = "Y8VNVETDNZQ4EBRA";
const char* server_thingspeak = "api.thingspeak.com";
const long thingspeak_time = 5 * 60 * 1000; // 5 minutos


// TELEGRAM
WiFiClientSecure client_telegram;
String id, text;
IPAddress server(149, 154, 167, 200); // IP de api.telegram.org
UniversalTelegramBot bot(BOTtoken, client_telegram);
const long telegram_time = 1000;

// STEPPER or SERVO
Servo myservo;
const long alimenta_time = 8 * 60 * 60 * 1000; // 8 HORA


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(10);
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDBLUE, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED, LOW);
  digitalWrite(LEDBLUE, LOW);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  WiFi.mode(WIFI_STA);
  delay(100);
  connectwifi();
  delay(100);
  timer.setInterval(alimenta_time, alimenta);
  timer.setInterval(thingspeak_time, SendData);
  timer.setInterval(telegram_time,  readTel);
  delay(100);
  bot.sendMessage("126554909", "iniciando...", "");
  envia_temp();
  testa_led();
  myservo.attach(SERVO);
  myservo.write(30);
  espera(1);
  myservo.write(0);
  bot.getUpdates(bot.last_message_received + 1);
}

void loop() {
  timer.run();
}


void SendData() {
  DS18B20.requestTemperatures();
  delay(100);
  temp = DS18B20.getTempCByIndex(0);
  delay(100);
  if (temp > 50) {
    envia_temp();
  } else {
    if (client_thingspeak.connect(server_thingspeak, 80)) {
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
      if (temp < 50) {
        client_thingspeak.print(postStr);
      }
      delay(500);
      Serial.print("Temperature: ");
      Serial.println(temp);
    }
  }
  yield();
}

void connectwifi() {
  client_thingspeak.stop();
  client_telegram.stop();
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 20) {
      Serial.println("sem conexao");
      espera(5);
    }
  }
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
  espera(1);
}


void readTel() {
  int newmsg = bot.getUpdates(bot.last_message_received + 1);
  for (int i = 0; i < newmsg; i++)//Caso haja X mensagens novas, fara este loop X Vezes.
  {
    id = bot.messages[i].chat_id;//Armazenara o ID do Usuario à Váriavel.
    text = bot.messages[i].text;//Armazenara o TEXTO do Usuario à Váriavel.
    text.toUpperCase();//Converte a STRING_TEXT inteiramente em Maiuscúla.
    if (text.indexOf("TEMP") > -1) {
      envia_temp();
    } else if (text.indexOf("BLUE") > -1) {
      digitalWrite(LEDBLUE, HIGH);
      bot.sendMessage(id, "BLUE ON", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    } else if (text.indexOf("GREEN") > -1) {
      digitalWrite(LEDGREEN, HIGH);
      bot.sendMessage(id, "GREEN ON", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    } else if (text.indexOf("RED") > -1) {
      digitalWrite(LEDRED, HIGH);
      bot.sendMessage(id, "RED ON", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    } else if (text.indexOf("OFF") > -1) {
      digitalWrite(LEDGREEN, LOW);
      digitalWrite(LEDRED, LOW);
      digitalWrite(LEDBLUE, LOW);
      bot.sendMessage(id, "LED OFF", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    } else if (text.indexOf("GETID") > -1) {
      bot.sendSimpleMessage(id, id, "");//Envia uma mensagem com seu ID.
    } else if (text.indexOf("START") > -1) {
      bot.sendSimpleMessage(id, "bem vindo ao canal https://thingspeak.com/channels/35939", "");//Envia mensagem de boas vindas.
    } else if (text.indexOf("TIME") > -1) {
      bot.sendSimpleMessage(id, String(millis() / 1000 / 60) + " minutos", ""); //Envia mensagem de boas vindas.
    } else if (text.indexOf("RESET") > -1) {
      ESP.reset();
    } else if (text.indexOf("ALIMENTA") > -1) {
      alimenta();
    } else if (text.indexOf("NOBOMB") > -1) {
      digitalWrite(RELAY1, HIGH);
      bot.sendSimpleMessage(id, "bomba desligada", "");
    } else if (text.indexOf("BOMB") > -1) {
      digitalWrite(RELAY1, LOW);
      bot.sendSimpleMessage(id, "bomba ligada", "");
    } else {
      yield();
    }
  }
  yield();
}



void alimenta() {
  /// Apaga leds e bomba
  /// Ascende led azul e espera 10 segundos
  /// gira motor servo
  /// manda mensagem avisando que alimentou e a temperatura da água
  /// depois de 5 min, desliga led azul e liga a bomba


  digitalWrite(RELAY1, HIGH);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED, LOW);
  digitalWrite(LEDBLUE, HIGH);
  myservo.write(0);
  espera(60);
  myservo.write(180);
  espera(3);
  myservo.write(0);
  bot.sendSimpleMessage(id, "peixinhos alimentados", "");
  espera(200);
  digitalWrite(LEDGREEN, HIGH);
  digitalWrite(LEDRED, LOW);
  digitalWrite(LEDBLUE, LOW);
  digitalWrite(RELAY1,  LOW);
}

void envia_temp() {
  DS18B20.requestTemperatures();
  delay(100);
  temp = DS18B20.getTempCByIndex(0);
  bot.sendSimpleMessage(id, "temperatura:" + String(temp), "");
}


void espera(int seg) {
  for (int i = 0; i < seg; i++) {
    for (int j = 0; j < 10; j++) {
      delay(100);
      yield();
    }
    readTel();
  }
}

void testa_led() {
  digitalWrite(LEDGREEN, HIGH);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  LOW);
  espera(1);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   HIGH);
  digitalWrite(LEDBLUE,  LOW);
  espera(1);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  HIGH);
  espera(1);
  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED,   LOW);
  digitalWrite(LEDBLUE,  LOW);
}




