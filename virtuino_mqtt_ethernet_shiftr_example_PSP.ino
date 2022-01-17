
#include <Ethernet.h>
#include <Dns.h>
#include <EthernetUdp.h>
#include <Dhcp.h>
#include <EthernetClient.h>
#include <EthernetServer.h>

#define MAXMILLIS 4294967295

//Датчик DHT22
#include "DHT.h"
#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);


//---------------------------------------------------------------
// Project: Virtuino MQTT ethernet example
// Created by Ilias Lamprou at Jan/16/2018
// MQTT Broker: shiftr.io. https://shiftr.io/try.
// MQTT library by Joël Gähwiler: https://github.com/256dpi/arduino-mqtt
//---------------------------------------------------------------

// PSP

#include <SFE_BMP180.h>
#include <Wire.h>

// создаем экземпляр класса SFE_BMP180 и называем его «pressure»:
SFE_BMP180 pressure;

// это высота над уровнем моря в штаб-квартире SparkFun, в Боулдере;
// указана в метрах:
//#define ALTITUDE 1655.0
#define ALTITUDE 138.0 //Высота над уровнем моря в метрах - Жуковский


//---------- board settings ------------------
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//------ MQTT broker settings and topics
const char* broker = "broker.shiftr.io";
char mqttUserName[] = "a6a777bb";
char mqttPass[] = "2a21a0b2e9172136";

//------ PIN settings and topics
#define MQTT_CONNECTION_LED_PIN 4  // MQTT connection indicator           
//#define OUT1_PIN 5                 // Led or relay
//#define OUT2_PIN 6                 // Led or relay

#define OUT1_PIN 4                 // Led or relay
#define OUT2_PIN 5                 // Led or relay


#define IN1_PIN 7                  // Button

const char* topic_pub_sensor1 = "sensor1"; // You can to replace the labels: sensor1, sensor2, input_1, etc. with yours
const char* topic_pub_sensor2 = "sensor2";

const char* topic_pub_DHT22_temp = "DHT22_temp"; 
//const char* topic_pub_DHT22_hum = "DHT22_hum";

const char* topic_pub_in1 = "input_1";

const char* topic_sub_out1 = "output_1";
const char* topic_sub_out2 = "output_2";

const char* topic_sub_variable1 = "variable_1";

const unsigned long mqttPostingInterval = 10L * 1000L; // Post sensor data every 5 seconds.

//---------Variables and Libraries --------------------
#include <MQTTClient.h>
EthernetClient net;
MQTTClient client;

unsigned long lastUploadedTime = 0;
byte in1_lastState = 2;
byte in2_lastState = 2;

//========================================= connect
//=========================================
void connect() {
  digitalWrite(MQTT_CONNECTION_LED_PIN, LOW);  // Turn off the MQTT connection LED
  Serial.print("\nConnecting...");
  //--- create a random client id
  char clientID[] = "VIRTUINO_0000000000"; // For random generation of client ID.
  for (int i = 9; i < 19 ; i++) clientID[i] =  char(48 + random(10));

  while (!client.connect("Arduino", mqttUserName, mqttPass)) {
    Serial.print(".");
    digitalWrite(MQTT_CONNECTION_LED_PIN, !digitalRead(MQTT_CONNECTION_LED_PIN));
    delay(1000);
  }
  Serial.println("\nconnected!");
  digitalWrite(MQTT_CONNECTION_LED_PIN, HIGH);

  client.subscribe(topic_sub_out1);
  client.subscribe(topic_sub_out2);
  client.subscribe(topic_sub_variable1);

  // client.unsubscribe(topic_sub_out2);
}


//========================================= messageReceived
//=========================================
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == topic_sub_out1) {
    int v = payload.toInt();
    if (v == 1) digitalWrite(OUT1_PIN, HIGH);
    else digitalWrite(OUT1_PIN, LOW);
  }
  if (topic == topic_sub_out2) {
    int v = payload.toInt();
    if (v == 1) digitalWrite(OUT2_PIN, HIGH);
    else digitalWrite(OUT2_PIN, LOW);
  }

  if (topic == topic_sub_variable1) {
    int v = payload.toInt();
    Serial.println("Variable 1 = " + String(v));
    // do something with the variable here
  }

}

//========================================= setup
//=========================================
//=========================================
void setup() {
  Serial.begin(9600);
  Serial.println("Setup");
  
  dht.begin();
  
  pinMode(MQTT_CONNECTION_LED_PIN, OUTPUT);
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(IN1_PIN, INPUT);

  Serial.begin(9600);
  Ethernet.begin(mac);
  client.begin(broker, net);
  client.onMessage(messageReceived);
  connect();


  // инициализируем датчик (важно извлечь калибровочные данные,
  // хранящиеся в устройстве):
  if (pressure.begin())
    //Serial.println("Инициализация BMP180 прошла успешно");
    Serial.println("Init BMP180 success");
  // "Инициализация BMP180 прошла успешно"
  else
  {
    // упс, что-то пошло не так!
    // как правило, так происходит из-за проблем с подключением
    // (о том, как подключить датчик правильно, читайте выше):
    Serial.println("BMP180 init fail\n\n");
    // "Инициализация BMP180 не удалась"
    while (1); // вечная пауза
  }


}





//Извлечение температуры из датчика BMP180
double getTemp() {
  char status;
  double T;


  // если вы хотите измерить высоту над уровнем моря,
  // то вам нужно знать информацию о давлении; расчет высоты
  // над уровнем моря показан в конце скетча

  // чтобы рассчитать давление, сначала нужно измерить температуру

  // запускаем измерение температуры; если функция
  // будет выполнена успешно, она вернет количество
  // миллисекунд, потребовавшихся на измерение;
  // а если неуспешно, то вернет «0»:
  status = pressure.startTemperature();
  if (status != 0)
  {
    // ждем, когда завершится измерение:
    delay(status);

    // извлекаем данные о температуре; обратите внимание,
    // что измеренные данные хранятся в переменной «T»;
    // если функция будет выполнена успешно, она вернет «1»,
    // а если нет, то «0»

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // печатаем измеренную температуру:
      Serial.print("Temperature BMP180(°C): ");  //  "температура: "
      Serial.println(T, 2);
      //delay(3000); // Вроде тормозит всю программу
      return T;

    }
    else Serial.println("error retrieving temperature measurement\n");
    // "ошибка при извлечении данных о температуре"
  }
  else Serial.println("error starting temperature measurement\n");
  // "ошибка при запуске измерения температуры"
  //delay(30000);  // 30-секундная пауза
}

// Извлечение давления из датчика BMP180
double getPressure() {
  char status;
  double T, P, p0;

  status = pressure.startTemperature();
  if (status != 0) {
    // ожидание замера температуры
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0) {
      status = pressure.startPressure(3);
      if (status != 0) {
        // ожидание замера давления
        delay(status);
        status = pressure.getPressure(P, T);
        if (status != 0) {
          Serial.print("Absolute pressure: ");  //  "Давление в миллибарах: "
          Serial.print(P, 2);
          Serial.print(" millibar, ");
         double absolutePmmrtst = P*0.75;
//          Serial.print("Абсолютное давление(мм.рт.ст): ");  //  "Давление в мм.рт.ст: "
//          Serial.print("Absolute Pressure(mm.rt.st): ");  //  "Давление в мм.рт.ст: "
          Serial.print(absolutePmmrtst, 2);          
          Serial.println(" mm rt stolba");
           
          // датчик возвращает атм. давление, которое изменяется в зависимости от высоты датчика.
          // Если мы хотим как в прогнозе погоды, то нужно сделать сомнительные вычисления
          // Параметры: P = давленик с датчика в миллибарах, ALTITUDE = высота над уровнем моря в метрах.
          // Результат: p0 = давление, откорректированное по уровню моря

          p0 = pressure.sealevel(P,ALTITUDE);
          Serial.print("Pressure from sea latitude (from prognoz pogoda): ");
          Serial.print(p0,2);
          Serial.print(" millibar, ");
          double PmmrtstAltitude=p0*0.75;
          Serial.print(PmmrtstAltitude,2);
          Serial.println(" mm");

          //Возвращаем абсолютное давление в мм.рт.ст.
          return (absolutePmmrtst);
          
        }
      }
    }
  }

}

// Извлечение температуры из датчика DHT22
double getTempDHT22() {
  float value = dht.readTemperature();
    if (isnan(value)) {
    Serial.println(F("Failed DHT sensor!"));
    return 0;
    }
    Serial.print("Temperature DHT-22(°C): ");
    Serial.println(value);
    return value;    
}

// Извлечение влажности из датчика DHT22
double getHumidityDHT22() {
  float value = dht.readHumidity();
    if (isnan(value)) {
    Serial.println(F("Failed DHT sensor!"));
    return 0;
    }
    Serial.print("Humidity DHT-22(%): ");
    Serial.println(value);
    return value;    
}


//========================================= loop
//=========================================
//=========================================
void loop() {
  client.loop();
  if (!client.connected())connect();

  //---- MQTT upload
  if (millis() - lastUploadedTime > mqttPostingInterval) {
    //int sensor1_value =random(100);         // replace the random value with your sensor value
    //int sensor1_value =myMultiplyFunction(14,5);


    double sensor1_value = getTemp();
    client.publish(topic_pub_sensor1, String(sensor1_value), true, 1);

    //int sensor2_value = analogRead(A0);

    double sensor2_value = getPressure();
    client.publish(topic_pub_sensor2, String(sensor2_value), true, 1); // upload the analog A0 value


    double sensorDHT22temp_value=getTempDHT22();
    //client.publish(topic_pub_DHT22_temp, String(sensorDHT22temp_value), true, 1);


    double sensorDHT22humidity_value=getHumidityDHT22();

    lastUploadedTime = millis();
  }


  //---- check if button is pushed
  byte input1_state = digitalRead(IN1_PIN);
  if (input1_state != in1_lastState) {
    client.publish(topic_pub_in1, String(input1_state), true, 1);
    delay(100);
    in1_lastState = input1_state;
    // }
  }


}
