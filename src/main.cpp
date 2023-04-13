#define DEBUG 0

#define SEALEVELPRESSURE_HPA (1013.25) // Задаем высоту

#include <ESP8266WiFi.h>
#include <Adafruit_BME280.h> // Подключаем библиотеку Adafruit_BME280
#include <Adafruit_Sensor.h> // Подключаем библиотеку Adafruit_Sensor
#include <ESP8266HTTPClient.h>

void ReadSensors();
void transmit();

HTTPClient http;
WiFiClient wifiClient;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Ukrainer";
char pass[] = "MaxiSoft83.";

String Hostname; // имя железки - выглядит как ESPAABBCCDDEEFF т.е. ESP+mac адрес.

unsigned long my_time = millis();

double bme280_temperature;
double bme280_pressure;
double bme280_humidity;

Adafruit_BME280 bme; // Установка связи по интерфейсу I2C

void setup()
{
#if DEBUG == 1
  // Debug console
  Serial.begin(115200);
  Serial.println("Street");
#endif
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#if DEBUG == 1
    Serial.print(".");
#endif
  }
#if DEBUG == 1
  Serial.println("Connected");
#endif

#if DEBUG == 1
  Hostname = "ESP" + WiFi.macAddress();
  Hostname.replace(":", "");
  WiFi.hostname(Hostname);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.print("Narodmon ID: ");
  Serial.println(Hostname);
#endif

  if (!bme.begin(0x76))
  {
#if DEBUG == 1                                                             // Проверка инициализации датчика
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); // Печать, об ошибки инициализации.
#endif
    while (1)
      ; // Зацикливаем
  }

  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);
}

void loop()
{
  if ((my_time + 10000) < millis())
  {
    ReadSensors();
    transmit();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    my_time = millis();
  }
}

void ReadSensors()
{
  bme280_temperature = bme.readTemperature();
  bme280_pressure = bme.readPressure() / 133.3F;
  bme280_humidity = bme.readHumidity();
#if DEBUG == 1
  Serial.println(bme280_temperature);
  Serial.println(bme280_pressure);
  Serial.println(bme280_humidity);
#endif
}

// Обьявление функции передачи данных
void transmit()
{
  String my_message;
  my_message += "http://192.168.0.240:80/sensors?temp=";
  my_message += bme280_temperature;
  my_message += "&press=";
  my_message += bme280_pressure;
  my_message += "&hum=";
  my_message += bme280_humidity;
  http.begin(wifiClient, my_message);
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.GET();
#if DEBUG == 1
  String payload = http.getString();
  Serial.print("Ответ сервера:");
  Serial.println(httpCode);
  Serial.println(payload);
#endif
  http.end();
  delay(10000);
}