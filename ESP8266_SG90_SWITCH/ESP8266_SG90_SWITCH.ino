/*
   switch.ino

    Created on: 2020-05-15
        Author: Mixiaoxiao (Wang Bin)

   HAP section 8.38 Switch
   An accessory contains a switch.

   This example shows how to:
   1. define a switch accessory and its characteristics (in my_accessory.c).
   2. get the switch-event sent from iOS Home APP.
   3. report the switch value to HomeKit.

   You should:
   1. read and use the Example01_TemperatureSensor with detailed comments
      to know the basic concept and usage of this library before other examples。
   2. erase the full flash or call homekit_storage_reset() in setup()
      to remove the previous HomeKit pairing storage and
      enable the pairing with the new accessory of this new HomeKit example.
*/
#include <EEPROM.h>
#include<wifi_connect_tool.h>
#include <Arduino.h>
#include <arduino_homekit_server.h>
#include <Servo.h>
#include <SimpleDHT.h>
#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);
#define PIN_DHT11 0
#define PIN_Servo 3
#define PIN_LED 2


SimpleDHT11 dht11(PIN_DHT11);
Servo myServo;  // 定义Servo对象来控制
int pos = 0;    // 角度存储变量


void setup() {
  Serial.begin(115200);
  //  wifi_connect(); // in wifi_info.h
  EEPROM.begin(350);
  wifi_load();
  pinMode(PIN_Servo, OUTPUT); //设置舵机输出
  pinMode(PIN_LED, OUTPUT); //设置板载LED灯
  myServo.attach(PIN_Servo);

  //homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
  my_homekit_setup();
}

void loop() {
  wifi_pant();//查看wifi情况
  if (WiFi.status() == WL_CONNECTED) {
    my_homekit_loop();
    delay(10);
  }
  //  Serial.print((int)temperature); Serial.print(" *C, ");
  //  Serial.print((int)humidity); Serial.println(" H");
  //  delay(1500);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_switch_on;
extern "C" homekit_characteristic_t cha_temperature;
extern "C" homekit_characteristic_t cha_humidity;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;


//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
  bool on = value.bool_value;
  cha_switch_on.value.bool_value = on;  //sync the value
  LOG_D("Switch: %s", on ? "ON" : "OFF");
  if (on) {
    for (pos = 90; pos < 180; pos += 5) { // 90°到180°
      // in steps of 1 degree
      myServo.write(pos);              // 舵机角度写入
      delay(15);                       // 等待转动到指定角度
    }
    for (pos = 180; pos > 90; pos -= 5) { // 从180°到90°
      myServo.write(pos);              // 舵机角度写入
      delay(15);                       // 等待转动到指定角度
    }
    digitalWrite(PIN_LED, LOW);
  } else {
    for (pos = 90; pos > 0; pos -= 5) { // 90°到0°
      // in steps of 1 degree
      myServo.write(pos);              // 舵机角度写入
      delay(15);                       // 等待转动到指定角度
    }
    for (pos = 0; pos < 90; pos += 5) { // 从0°到90°
      myServo.write(pos);              // 舵机角度写入
      delay(15);                       // 等待转动到指定角度
    }
    digitalWrite(PIN_LED, HIGH);
  }
}

void my_homekit_setup() {

  //  digitalWrite(PIN_SWITCH, HIGH);

  //Add the .setter function to get the switch-event sent from iOS Home APP.
  //The .setter should be added before arduino_homekit_setup.
  //HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
  //Maybe this is a legacy design issue in the original esp-homekit library,
  //and I have no reason to modify this "feature".
  cha_switch_on.setter = cha_switch_on_setter;
  arduino_homekit_setup(&config);

  //report the switch value to HomeKit if it is changed (e.g. by a physical button)
  //bool switch_is_on = true/false;
  //cha_switch_on.value.bool_value = switch_is_on;
  //homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_report_millis) {
    // report sensor values every 10 seconds
    next_report_millis = t + 10 * 1000;
    my_homekit_report();
  }
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
          ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
  }

}

void my_homekit_report() {
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
    return;
  }
  // FIXME, read your real sensors here.
  float t = float(temperature) ;
  float h = float(humidity);

  cha_temperature.value.float_value = t;
  homekit_characteristic_notify(&cha_temperature, cha_temperature.value);

  cha_humidity.value.float_value = h;
  homekit_characteristic_notify(&cha_humidity, cha_humidity.value);
}
