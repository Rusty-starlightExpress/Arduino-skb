/** モーターのスイッチプログラム **/
void setup() {
pinMode(11, OUTPUT);
}
void loop() {
digitalWrite(11,HIGH); //11番ピンの出力をHIGH = 5Vにする
delay(500); //1000ミリ秒 = 1秒待つ
digitalWrite(11,LOW); //11番ピンの出力をLOW = 0Vにする
delay(1000); //1000ミリ秒 = 1秒待つ
}
