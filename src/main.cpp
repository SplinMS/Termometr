/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPLaaezrtv-"
#define BLYNK_TEMPLATE_NAME "Termometr" //"Quickstart Device"
#define BLYNK_AUTH_TOKEN "GFWi6czUwLJU_f5PPTRnkKS9mD5TZE6f"
#define DEBUG 1

#define SEALEVELPRESSURE_HPA (1013.25) // Задаем высоту
#define postingInterval 330000         // интервал между отправками данных в миллисекундах (5 минут)

unsigned long lastConnectionTime = 0; // время последней передачи данных

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_BMP280.h> // Подключаем библиотеку Adafruit_BME280
#include <Adafruit_Sensor.h> // Подключаем библиотеку Adafruit_Sensor
#include <AHT10.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Ukrainer";
char pass[] = "MaxiSoft83.";

String Hostname; // имя железки - выглядит как ESPAABBCCDDEEFF т.е. ESP+mac адрес.

unsigned long my_time = millis();

double bme280_temperature;
double bme280_pressure;
double aht10_humidity;
double aht10_temperature;

BlynkTimer timer;

Adafruit_BMP280 bme; // Установка связи по интерфейсу I2C
AHT10 myAHT10(AHT10_ADDRESS_0X38);

// This function is called every time the Virtual Pin 0 state changes
/*BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V0, value);
}*/

// This function is called every time the device is connected to the Blynk.Cloud
/*BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  //Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  //Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  //Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}*/

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V0, millis() / 1000);
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // You can also specify server:
  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);
  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, IPAddress(192,168,1,100), 8080);

  Hostname = "ESP" + WiFi.macAddress();
  Hostname.replace(":", "");
  WiFi.hostname(Hostname);
  // wifimanstart();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.print("Narodmon ID: ");
  Serial.println(Hostname);

  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);

  if (!bme.begin(0x76))
  {                                                                        // Проверка инициализации датчика
    Serial.println("Could not find a valid BME280 sensor, check wiring!"); // Печать, об ошибки инициализации.
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
  buf = buf + "#TEMPERATURBMP280#" + String(bme280_temperature) + "\r\n";
  buf = buf + "#HUMIDITYAHT10#" + String(aht10_humidity) + "\r\n";
  buf = buf + "#PRESSUREBMP280#" + String(bme280_pressure) + "\r\n";
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

void loop()
{
  Blynk.run();
  timer.run();

  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
  if ((my_time + 10000) < millis())
  {
    bme280_temperature = bme.readTemperature();
    bme280_pressure = bme.readPressure() / 133.3F;
    aht10_humidity = myAHT10.readHumidity();
    aht10_temperature = myAHT10.readTemperature();

    Serial.println(bme280_temperature);
    Serial.println(bme280_pressure);
    Serial.println(aht10_humidity);
    Serial.println(aht10_temperature);

    Blynk.virtualWrite(V1, bme280_temperature);
    Blynk.virtualWrite(V2, bme280_pressure);
    Blynk.virtualWrite(V3, aht10_humidity);
    Blynk.virtualWrite(V4, aht10_temperature);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    // delay(120000);
    my_time = millis();
  }

  if (millis() - lastConnectionTime > postingInterval)
  { // ждем 5 минут и отправляем
    if (WiFi.status() == WL_CONNECTED)
    { // ну конечно если подключены
      if (SendToNarodmon())
      {
        lastConnectionTime = millis();
      }
      else
      {
        lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      }
    }
    else
    {
      lastConnectionTime = millis() - postingInterval + 15000; // следующая попытка через 15 сек
      Serial.println("Not connected to WiFi");
    }
  }
}
