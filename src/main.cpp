#define DEBUG 1

#define SEALEVELPRESSURE_HPA (1013.25) // Задаем высоту
#define postingInterval 330000         // интервал между отправками данных в миллисекундах (5 минут)

unsigned long lastConnectionTime = 0; // время последней передачи данных

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

const char *host = "http://192.168.0.240"; // Адрес нашего веб сервера
const int httpsPort = 80;

String Link; // Адрес порта для HTTPS= 443 или HTTP = 80

String Hostname; // имя железки - выглядит как ESPAABBCCDDEEFF т.е. ESP+mac адрес.

unsigned long my_time = millis();

double bme280_temperature;
double bme280_pressure;
double bme280_humidity;

Adafruit_BME280 bme; // Установка связи по интерфейсу I2C

void setup()
{
  // Debug console
  Serial.begin(115200);
  Serial.println("Street");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");

  Hostname = "ESP" + WiFi.macAddress();
  Hostname.replace(":", "");
  WiFi.hostname(Hostname);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.print("Narodmon ID: ");
  Serial.println(Hostname);

  if (!bme.begin(0x76))
  {                                                                        // Проверка инициализации датчика
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); // Печать, об ошибки инициализации.
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

  if (millis() - lastConnectionTime > postingInterval)
  { // ждем 5 минут и отправляем

    if (WiFi.status() == WL_CONNECTED)
    { // ну конечно если подключены
      if (1)
      {
        lastConnectionTime = millis();
        Serial.println("Отправил на народмонитор");
      }
      else
      {
        lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      }
    }
    else
    {
      lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      Serial.println(WiFi.status());
    }
  }
}

void ReadSensors()
{
  bme280_temperature = bme.readTemperature();
  bme280_pressure = bme.readPressure() / 133.3F;
  bme280_humidity = bme.readHumidity();

  Serial.println(bme280_temperature);
  Serial.println(bme280_pressure);
  Serial.println(bme280_humidity);
}

// Обьявление функции передачи данных
void transmit()
{
  http.begin(wifiClient, "http://192.168.0.240");
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("/sensors?temp=25&pres=700&hum=50");
  String payload = http.getString();
  Serial.println(httpCode);
  Serial.println(payload);
  http.end();
  delay(10000);
}