#define DEBUG 1

#define SEALEVELPRESSURE_HPA (1013.25) // Задаем высоту
#define postingInterval 330000         // интервал между отправками данных в миллисекундах (5 минут)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_BME280.h> // Подключаем библиотеку Adafruit_BME280
#include <Adafruit_Sensor.h> // Подключаем библиотеку Adafruit_Sensor

void ReadSensors();
bool SendToNarodmon();
void HTTP_init(void);
void handleNotFound();
void handleRoot();
void handleSensors();
void handle_Restart();

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Ukrainer";
char pass[] = "MaxiSoft83.";

String Hostname; // имя железки - выглядит как ESPAABBCCDDEEFF т.е. ESP+mac адрес.

unsigned long my_time = millis();

double bme280_temperature;
double bme280_pressure;
double bme280_humidity;

unsigned long lastConnectionTime = 0; // время последней передачи данных

Adafruit_BME280 bme; // Установка связи по интерфейсу I2C
ESP8266WebServer HTTP(80);

void setup()
{
  // Debug console
  Serial.begin(115200);

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

  Serial.println("Start 2-WebServer");
  HTTP_init();

  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);
}

void loop()
{
  HTTP.handleClient();
  delay(1);

  if ((my_time + 10000) < millis())
  {
    ReadSensors();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    my_time = millis();
  }

  if (millis() - lastConnectionTime > postingInterval)
  { // ждем 5 минут и отправляем

    if (WiFi.status() == WL_CONNECTED)
    { // ну конечно если подключены
      if (SendToNarodmon)
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

bool SendToNarodmon()
{ // Собственно формирование пакета и отправка.
  WiFiClient client;
  // DeviceAddress tempDeviceAddress;
  Serial.println("Формирование пакета");
  String buf;
  buf = "#" + Hostname + "\r\n"; // заголовок
  buf = buf + "#TEMPERATURBME280#" + String(bme280_temperature) + "\r\n";
  buf = buf + "#HUMIDITYBME280#" + String(bme280_humidity) + "\r\n";
  buf = buf + "#PRESSUREBME280#" + String(bme280_pressure) + "\r\n";
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
  bme280_temperature = bme.readTemperature();
  bme280_pressure = bme.readPressure() / 133.3F;
  bme280_humidity = bme.readHumidity();

  Serial.println(bme280_temperature);
  Serial.println(bme280_pressure);
  Serial.println(bme280_humidity);
}

void HTTP_init(void)
{
  HTTP.onNotFound(handleNotFound);     // Сообщение если нет страницы. Попробуйте ввести http://192.168.0.101/restar?device=ok&test=1&led=on
  HTTP.on("/", handleRoot);            // Главная страница http://192.168.0.101/
  HTTP.on("/sensors", handleSensors);
  HTTP.on("/restart", handle_Restart); // Перезагрузка модуля по запросу вида http://192.168.0.101/restart?device=ok
  // Запускаем HTTP сервер
  HTTP.begin();
}

// Ответ если страница не найдена
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += HTTP.uri();
  message += "\nMethod: ";
  message += (HTTP.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTP.args();
  message += "\n";
  for (uint8_t i = 0; i < HTTP.args(); i++)
  {
    message += " " + HTTP.argName(i) + ": " + HTTP.arg(i) + "\n";
  }
  HTTP.send(404, "text/plain", message);
}

// Ответ при обращении к основной странице
void handleRoot()
{
  HTTP.send(200, "text/plain", "hello from esp8266!");
  Serial.println("Обращение к корню сервера");
}

void handleSensors()
{
  String message="";
  for (uint8_t i = 0; i < HTTP.args(); i++)
  {
    message += " " + HTTP.argName(i) + ": " + HTTP.arg(i) + "\n";
  }
  HTTP.send(404, "text/plain", message);
  Serial.println("handleSensors");
}

// Перезагрузка модуля по запросу вида http://192.168.0.101/restart?device=ok
void handle_Restart()
{
  String restart = HTTP.arg("device");
  if (restart == "ok")
    ESP.restart();
  HTTP.send(200, "text/plain", "OK");
}