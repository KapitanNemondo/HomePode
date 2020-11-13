#include <GyverTimers.h>

#include <DFRobotDFPlayerMini.h>

#include <SoftwareSerial.h>
// библиотека для эмуляции Serial порта
#include <SoftwareSerial.h>
// создаём объект mySoftwareSerial и передаём номера управляющих пинов RX и TX
// RX - цифровой вывод 10, необходимо соединить с выводом TX дисплея
// TX - цифровой вывод 11, необходимо соединить с выводом RX дисплея
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "Adafruit_NeoPixel.h"


#define PIN 8           // пин DI
#define NUM_LEDS 30     // число диодов
#define PERIOD_1 500    // Период между поворотом сервы
unsigned long timer_1;  // Таймер сервы
#define PERIOD_2 60000   // Период между переключением светодиодов
unsigned long timer_2;  // Таймер адресной светодиодной ленты

int Bri = 175;          // Стартовая Яркость
boolean Light_flag = 1; // Флаг работы светодиодной ленты
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX для плеера DFPlayer Mini
DFRobotDFPlayerMini myDFPlayer;


unsigned long currentPause = millis();  
bool fl_pause = 0;
int buttonNext = 2; // кнопка следующий трек
int buttonPause = 7; // кнопка пауза/ пуск
int buttonPrevious = 4; // кнопка предыдущий трек
int buttonVolumeUp = 5; // кнопка увеличение громкости
int buttonVolumeDown = 6; // кнопка уменьшение громкости
boolean isPlaying = false; // статус воспроизведения/пауза
uint32_t myTimer1;

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

// byte button_1 = A0;  // кнопка на 3 цифровом
// byte button_2 = A1; // потенциометр на 0 аналоговом
// byte button_3 = A2; // движковый потенциометр на 1 аналоговом пине

byte transmit_data[2];  // массив, хранящий передаваемые данные
byte latest_data[2];    // массив, хранящий последние переданные данные
boolean flag;           // флажок отправки данных

boolean Servo_l(boolean serv_flag) { // Функция работы сервы, если флаг сервы равен 1

  
  if (serv_flag == 1) {
  
    for (int i = 0; i < 256; i++) {
  
      transmit_data[0] = i;
  
      for (int i = 0; i < 1; i++) { // в цикле от 0 до числа каналов
        if (transmit_data[i] != latest_data[i]) { // если есть изменения в transmit_data
          flag = 1; // поднять флаг отправки по радио
          latest_data[i] = transmit_data[i]; // запомнить последнее изменение
        }
      }
  
      if (flag == 1) {
        radio.powerUp(); // включить передатчик
        radio.write(&transmit_data, sizeof(transmit_data)); // отправить по радио
        flag = 0; //опустить флаг
        radio.powerDown(); // выключить передатчик
      }
  
      if (millis() - timer_1 >= PERIOD_1) {    // условие таймера
        timer_1 = millis();
      }
      
    }

    serv_flag = 0;

    return serv_flag;
  }
}

void setup() {

  pinMode(3, OUTPUT);                           // третий ПИН низкочастотная модуляция 100 кГц
  Timer2.setFrequency(100000 * 2);                 // настроить частоту таймера в Гц
  Timer2.outputEnable(CHANNEL_B, TOGGLE_PIN);   // в момент срабатывания таймера пин будет переключаться
  
pinMode(buttonPause, INPUT_PULLUP);
pinMode(buttonNext, INPUT_PULLUP);
pinMode(buttonPrevious, INPUT_PULLUP);
pinMode(buttonVolumeUp, INPUT_PULLUP);
pinMode(buttonVolumeDown, INPUT_PULLUP);
mySoftwareSerial.begin(9600);
Serial.begin(9600);

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик

strip.begin();
  strip.setBrightness(Bri);    // яркость, от 0 до 255
  strip.clear();                          // очистить
  strip.show(); 

delay(1000);
Serial.println();
Serial.println("DFPlayer Mini Demo");
Serial.println("Initializing DFPlayer...");
if (!myDFPlayer.begin(mySoftwareSerial)) {
Serial.println("Unable to begin:");
Serial.println("1.Please recheck the connection!");
Serial.println("2.Please insert the SD card!");
while (true);
}
Serial.println(F("DFPlayer Mini online."));
myDFPlayer.setTimeOut(300);
//----Set volume----
myDFPlayer.volume(20); //Set volume value (0~30).
//----Set different EQ----
myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
myDFPlayer.play(1); //Воспроизведение первого mp3
isPlaying = true; // воспроизводим
Serial.println("Playing..");
//----Читать информацию----
Serial.println(myDFPlayer.readState()); //читать состояние mp3
Serial.println(myDFPlayer.readVolume()); //Текущая громкость
Serial.println(myDFPlayer.readEQ()); // читаем настройку эквалайзера
Serial.println(myDFPlayer.readFileCounts()); // читать все файлы на SD-карте
Serial.println(myDFPlayer.readCurrentFileNumber()); // текущий номер файла воспроизведения
}
void loop() {
Servo_l(1);
if (Light_flag == 1) {
    
      strip.setBrightness(Bri);

      // заливаем белым
      for (int i = 0; i < NUM_LEDS; i++ ) {   // всю ленту
        strip.setPixelColor(i, 0xffffff);     // залить белым
        strip.show();                         // отправить на ленту
      }

      for (int i = 10; i != 0 ; i--) {

        
        
        if (millis() - timer_2 >= PERIOD_2) {    // условие таймера
            timer_2 = millis();
            Bri = Bri - 15;
            strip.setBrightness(Bri);
        }

      }
      Light_flag = 0;
    }

if (fl_pause == 0 && millis() - currentPause >= 600000)  //ДЛЯ ПРОБЫ 10min. отключ
      {
          digitalWrite(3, LOW);
          currentPause = millis();
          fl_pause = 1;
      }
if (isPlaying){
  if (millis() - myTimer1 >= 60000) {   // ищем разницу (1 мин)
    myTimer1 = millis();                // сброс таймера
    myDFPlayer.volumeDown();            //Volume Down
    myDFPlayer.volumeDown();
  }
  
}
if (myDFPlayer.readVolume()==0){
  isPlaying = false;
}

if (digitalRead(buttonPause) == LOW) {
if (isPlaying) { // если было воспроизведение трека
myDFPlayer.pause(); // пауза
isPlaying = false; // пауза
Serial.println("Paused..");
} else { // иначе
isPlaying = true; // воспроизводим
myDFPlayer.start(); //запускаем mp3 с паузы
}
delay(500);
}
if (digitalRead(buttonNext) == LOW) {
if (isPlaying) {
myDFPlayer.next(); //Next Song
Serial.println("Next Song..");
}
delay(500);
}
if (digitalRead(buttonPrevious) == LOW) {
if (isPlaying) {
myDFPlayer.previous(); //Previous Song
Serial.println("Previous Song..");
}
delay(500);
}
if (digitalRead(buttonVolumeUp) == LOW) {
if (isPlaying) {
myDFPlayer.volumeUp(); //Volume Up
Serial.println("Volume Up..");
}
delay(500);
}
if (digitalRead(buttonVolumeDown) == LOW) {
if (isPlaying) {
myDFPlayer.volumeDown(); //Volume Down
Serial.println("Volume Down..");
}
delay(500);
}
}
