#define DEBUG 1

#define SEALEVELPRESSURE_HPA (1013.25) // Задаем высоту
#define postingInterval 330000         // интервал между отправками данных в миллисекундах (5 минут)

unsigned long lastConnectionTime = 0; // время последней передачи данных

#include <ESP8266WiFi.h>
#include <Adafruit_BMP280.h> // Подключаем библиотеку Adafruit_BME280
#include <Adafruit_Sensor.h> // Подключаем библиотеку Adafruit_Sensor
#include <AHT10.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Ukrainer";
char pass[] = "MaxiSoft83.";

String Hostname; // имя железки - выглядит как ESPAABBCCDDEEFF т.е. ESP+mac адрес.

double bmp280_temperature;
double bmp280_pressure;
double aht10_humidity;
double aht10_temperature;

Adafruit_BMP280 bmp; // Установка связи по интерфейсу I2C
AHT10 myAHT10(AHT10_ADDRESS_0X38);

void setup()
{
  // Debug console
  Serial.begin(115200);

  Hostname = "ESP" + WiFi.macAddress();
  Hostname.replace(":", "");
  WiFi.hostname(Hostname);
  // wifimanstart();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.print("Narodmon ID: ");
  Serial.println(Hostname);

  if (!bmp.begin(0x76))
  {                                                                        // Проверка инициализации датчика
    Serial.println("Could not find a valid BMP280 sensor, check wiring!"); // Печать, об ошибки инициализации.
    while (1)
      ; // Зацикливаем
  }

  while (myAHT10.begin() != true)
  {
    Serial.println(F("AHT10 error")); //(F()) saves string to flash & keeps dynamic memory free
    delay(5000);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  delay(5000);
}

bool SendToNarodmon()
{ // Собственно формирование пакета и отправка.
  WiFiClient client;
  // DeviceAddress tempDeviceAddress;
  String buf;
  buf = "#" + Hostname + "\r\n"; // заголовок
  buf = buf + "#TEMPERATURBMP280#" + String(bmp280_temperature) + "\r\n";
  buf = buf + "#HUMIDITYAHT10#" + String(aht10_humidity) + "\r\n";
  buf = buf + "#PRESSUREBMP280#" + String(bmp280_pressure) + "\r\n";
  buf = buf + "#TEMPERATUREAHT10#" + String(aht10_temperature) + "\r\n";
  buf = buf + "##\r\n"; // закрываем пакет
  if (!client.connect("narodmon.ru", 8283))
  { // попытка подключения
    Serial.println("connection failed");
    return false; // не удалось;
  }
  else
  {
    client.print(buf); // и отправляем данные
    if (DEBUG)
      Serial.print(buf);
    while (client.available())
    {
      String line = client.readStringUntil('\r'); // если что-то в ответ будет - все в Serial
      Serial.print(line);
    }
  }
  return true; // ушло
}

void ReadSensors()
{
  bmp280_temperature = bmp.readTemperature();
  bmp280_pressure = bmp.readPressure() / 133.3F;
  aht10_humidity = myAHT10.readHumidity();
  aht10_temperature = myAHT10.readTemperature();

  Serial.println(bmp280_temperature);
  Serial.println(bmp280_pressure);
  Serial.println(aht10_humidity);
  Serial.println(aht10_temperature);
}

void loop()
{
  if (millis() - lastConnectionTime > postingInterval)
  { // ждем 5 минут и отправляем
    if (WiFi.status() == WL_CONNECTED)
    { // ну конечно если подключены
      ReadSensors();
      if (SendToNarodmon())
      {
        lastConnectionTime = millis();
      }
      else
      {
        lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      }
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      Serial.println("Not connected to WiFi");
    }
  }
}
