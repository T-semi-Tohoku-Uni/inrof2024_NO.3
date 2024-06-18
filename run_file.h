#ifndef run_file_h
#define run_file_h

#include <Arduino.h>
#include <Servo.h>

#include <Wire.h>
//アドレス指定
#define S11059_ADDR 0x2A
#define CONTROL_MSB 0x00
#define CONTROL_1_LSB 0x84
#define CONTROL_2_LSB 0b00001010//0x04
#define SENSOR_REGISTER 0x03


#define RIGHT true
#define LEFT false
//モーターの設定
const int motor_out[2][2] ={{11,12},{2,3}};//1 左右　2 ステッピングモーター　相
const int ref = 300;//ライン読み取り　閾値
int limit_speed = 500; //基準速度//0<speed=<100で指定
int motor_step_count[2] = {0,0};
int motor_speed[2] = {limit_speed,limit_speed};//0<speed=<1000で指定
unsigned long microtime;//現在時間の設定
unsigned long motor_time_marker[2] = {0,0};

int approach_cycle = 0;

//ライントレースの設定　フォトリフレクタ信号入力ピン
const int pin_fl = A1;//左前
const int pin_l = A2;//左
const int pin_c = A3;//中央
const int pin_r = A6;//右
const int pin_fr = A7;//右前

int line_over_count = 0;
bool line_over_pre_count = false;

int pre_out = 0;

//タクトスイッチピン指定
const int sw =7;
int sw_vals[3];

//IRセンサ　ピン指定
int pin_IR = 14;

//サーボ　関数　定義
Servo l_servo;
Servo r_servo;
Servo up_servo;
int servo_angle[3]={70,110,100};

//プロトタイプ宣言
int wait(int s);
void line_trace_cycle();
void running(bool control,bool wise,int count);
void line_trace(bool control);
void go_straight(int distance,bool forth,int speed);
void turn(int angle,bool wise,int sp);
void search_boals(int angle,int sp);
void turn_at_cross();
int line_read();
void into_line(bool control);
void move_servo(int n,bool wise);
void catching(int angle,bool wise);
int color_read();
float getHue(float r,float g,float b);
void approach();

int wait(int s){
  int value;
  value = 6000000/s;
  return value;
}

void line_trace_cycle(){//ライントレース　1サイクル
  microtime = micros();
  //左モーターの基準時間と現在の時間の差が一定以上進んだらPWMのクロックを進めるプログラム
  if (microtime - motor_time_marker[0] < wait(motor_speed[0])){
    running(LEFT,false,motor_step_count[0]);
  }
  else{
    motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
    motor_step_count[0]++;//カウントを進める。
    if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
    //if (in) into_line(LEFT);
    line_trace(LEFT);
  }
  //左モーターと同様の右モーターのプログラム
  if (microtime - motor_time_marker[1] < wait(motor_speed[1])){
    running(RIGHT,true,motor_step_count[1]);
  }
  else{
    motor_time_marker[1] = microtime;//基準時間との差が一定以上広がったのでリセット
    motor_step_count[1]++;//カウントを進める。
    if (motor_step_count[1]>3) motor_step_count[1]=0;//カウントが4以上の時は0にリセット
    //if (in) into_line(RIGHT);
    line_trace(RIGHT);
  }
  delayMicroseconds(100);
}

//走るプログラム全般に使う
void running(bool control,bool wise,int count){//control:モーターの左右 wise:回転方向 count:pwmの現在のステップ
  int re_count;//回転方向の決定
  if (wise) re_count = count;
  else re_count=3-count;
  //１ステップ目
  if (re_count==0){
    digitalWrite(motor_out[control][0],LOW);
    digitalWrite(motor_out[control][1],LOW);
  }
  //２ステップ目
  else if (re_count==1){
    digitalWrite(motor_out[control][0],HIGH);
    digitalWrite(motor_out[control][1],LOW);
  }
  //３ステップ目
  else if (re_count==2){
    digitalWrite(motor_out[control][0],HIGH);
    digitalWrite(motor_out[control][1],HIGH);
  }
  //４ステップ目
  else if (re_count==3){
    digitalWrite(motor_out[control][0],LOW);
    digitalWrite(motor_out[control][1],HIGH);
  }
}


void line_trace(bool control){//ラインの読み取りとスピードの決定 line_trace_cycleに使う
  int val_fl = analogRead(pin_fl)/0.57;//左前
  int val_l = analogRead(pin_l)/0.62;//左
  int val_c = analogRead(pin_c)/0.76;//中央
  int val_r = analogRead(pin_r)/0.97;//右
  int val_fr = analogRead(pin_fr);//右前

  float diff = val_l - val_r;
  int out = diff;
  //float diff2 = val_fl - val_fr;
  //out+=0.5*diff2;

  if (control){//controlが右の時
    if (limit_speed - out + pre_out > 200){
      if (limit_speed - out + pre_out < 900){
        motor_speed[1] = motor_speed[1]*0.9 +0.1*(limit_speed - out + pre_out);
      }else{
        motor_speed[1] = motor_speed[1]*0.9 +90;
      }
    }else{
      motor_speed[1] = motor_speed[1]*0.9+20;
    }
  }else{//controlが左の時
    if (limit_speed + out - pre_out > 200){
      if (limit_speed + out - pre_out < 900){
        motor_speed[0] = motor_speed[0]*0.9 +0.1*(limit_speed + out - pre_out);
      }else{
        motor_speed[0] = motor_speed[0]*0.9 + 90;
      }
    }else{
      motor_speed[0] = motor_speed[0]*0.9+20;
    }
  } 
  pre_out = 0.5*out;//

  if (val_l - val_fl > 250 & val_r - val_fr > 250){
    line_over_pre_count = true;
    //digitalWrite(13,HIGH);
    //delay(500);
    //digitalWrite(13,LOW);

  }
  if (line_over_pre_count){
    if (val_fl - val_l > 250 & val_fr - val_r > 250){
      //digitalWrite(13,HIGH);
      //delay(10);
      //digitalWrite(13,LOW);
      line_over_count++;
      line_over_pre_count = false;
      //digitalWrite(13,HIGH);
      //delay(500);
      //digitalWrite(13,LOW);
    }
  }
  
}

void go_straight(int distance,bool forth,int speed){//distance 走る距離 forth 走る向き(前)　speed スピード
  delay(1);
  float cycle = distance/1.6;
  motor_step_count[0]= 0;
  motor_step_count[1]= 0;
  int i=0;
  //int p_speed=300;
  delay(1);
  while(i<cycle){ //前
    delayMicroseconds(200);
    while (true){
      microtime = micros();
      if (microtime - motor_time_marker[0] < wait(speed)){
        
        running(LEFT,!forth,motor_step_count[0]);
        running(RIGHT,forth,motor_step_count[1]);
      }
      else{
        digitalWrite(13,HIGH);
        motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
        motor_step_count[0]++;//カウントを進める。
        motor_step_count[1]++;
        if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
        if (motor_step_count[1]>3) motor_step_count[1]=0;
        i++;
        break;
      }/*
      //加速する
      if (cycle-i<(speed-300)/ace){//減速する
        p_speed-=ace;
      }else if (p_speed<speed){//加速する
        p_speed+=ace;
      }*/
      delayMicroseconds(200);
      digitalWrite(13,LOW);
    }
  }
}

void turn(int angle,bool wise,int sp){//決められた角度回転するプログラム
  delay(1);
  float cycle = (angle)/0.9;
  motor_step_count[0]= 0;
  motor_step_count[1]= 0;

  int i=0;
  while(i<cycle){ //左回転  
  
    microtime = micros();
    motor_time_marker[0] = microtime;
    while (true){
      microtime = micros();
      if (microtime - motor_time_marker[0] < wait(sp)){
        
        running(LEFT,!wise,motor_step_count[0]);
        running(RIGHT,!wise,motor_step_count[1]);
      }
      else{
        //digitalWrite(13,HIGH);
        motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
        motor_step_count[0]++;//カウントを進める。
        motor_step_count[1]++;
        if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
        if (motor_step_count[1]>3) motor_step_count[1]=0;
        i++;
        break;
      }
      delayMicroseconds(100);
      digitalWrite(13,LOW);
    }
  }
}

void search_boals(int angle,int sp){
  delay(1);
  int kenchi =14;
  float cycle = (angle)/0.9;
  motor_step_count[0]= 0;
  motor_step_count[1]= 0;
  int wise = true;
  int val_IR = 100;
  float i;
  turn (angle,wise,sp);

  delay(1);
  for (int j =0;j<1;j++){
    i=0;
    wise = !wise;
    motor_step_count[0]= 0;
    motor_step_count[1]= 0;
    if (val_IR < kenchi) break;
    while(i<cycle*2){ //左回転  
      if (val_IR < kenchi){
        turn(8,wise,sp);
        break;
      }
      microtime = micros();
      delay(10);
      motor_time_marker[0] = microtime;
      
      while (true){
        microtime = micros();
        if (microtime - motor_time_marker[0] < wait(sp)){
          running(LEFT,!wise,motor_step_count[0]);
          running(RIGHT,!wise,motor_step_count[1]);
        }
        else{
          //digitalWrite(13,HIGH);
          motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
          motor_step_count[0]++;//カウントを進める。
          motor_step_count[1]++;
          if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
          if (motor_step_count[1]>3) motor_step_count[1]=0;
          i++;
          val_IR = 0;
          val_IR += analogRead(pin_IR);
          val_IR += analogRead(pin_IR);
          val_IR += analogRead(pin_IR);
          val_IR += analogRead(pin_IR);
          val_IR += analogRead(pin_IR);
          delayMicroseconds(200);
          val_IR = 19988 * pow(val_IR / 5, -1.252);
          break;
        }
        delayMicroseconds(200);
      }
    }
  }
  if (val_IR < kenchi){
    digitalWrite(13,HIGH);
    delay(100);
    digitalWrite(13,LOW);
    approach();
    delay(1);
    catching(65,true);
    for (int i=0;i<30;i++){
      move_servo(2,true);
    }
    delay(500);
    go_straight(approach_cycle*1.6,false,400);
    delay(500);
    if (angle-i*0.9-8>=0) turn(abs(angle-i*0.9-8),wise,sp);
    else turn(abs(angle-i*0.9-8),!wise,sp);
  }else{
    turn(angle,!wise,sp);
  }
}

void turn_at_cross(){
  delay(1);
  motor_step_count[0]= 0;
  motor_step_count[1]= 0;
  motor_speed[0]=limit_speed;
  motor_speed[1]=limit_speed;
  delay(1);
  bool wise = true;
  while(true){ //左回転  
    microtime = micros();
    motor_time_marker[0] = microtime;
    if (line_read()==0){
      wise = false;
    }else if (line_read()==1){
      wise = true;
    }else break;
    delayMicroseconds(100);
    while (true){
      microtime = micros();
      if (microtime - motor_time_marker[0] < wait(200)){
        running(LEFT,wise,motor_step_count[0]);
        running(RIGHT,wise,motor_step_count[1]);
      }
      else{
        //digitalWrite(13,HIGH);
        motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
        motor_step_count[0]++;//カウントを進める。
        motor_step_count[1]++;
        if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
        if (motor_step_count[1]>3) motor_step_count[1]=0;
        //i++;
        break;
      }
      delayMicroseconds(100);
      digitalWrite(13,LOW);
    }
  }
}

int line_read(){
  int val_fl = analogRead(pin_fl);//左前
  int val_l = analogRead(pin_l);//左
  int val_c = analogRead(pin_c);//中央
  int val_r = analogRead(pin_r);//右
  int val_fr = analogRead(pin_fr);//右前

  if (val_l - val_r > 100) return 0;
  else if (val_r - val_l > 100) return 1;
  else return 2;
}

void into_line(bool control){//ラインの読み取りとスピードの決定 line_trace_cycleに使う
  int val_fl = analogRead(pin_fl);//左前
  int val_l = analogRead(pin_l);//左
  int val_c = analogRead(pin_c);//中央
  int val_r = analogRead(pin_r);//右
  int val_fr = analogRead(pin_fr);//右前


  float diff = val_fl - val_fr;
  int out = diff;

  if (control){//controlが右の時
    if (limit_speed - out + pre_out > 0){
      if (limit_speed - out + pre_out < 1000){
        motor_speed[1] = motor_speed[1]*0.9 +0.1*(limit_speed - out + pre_out);
      }else{
        motor_speed[1] = motor_speed[1]*0.9 +0.1*(1000);
      }
    }else{
      motor_speed[1] = motor_speed[1]*0.9;
    }
  }else{//controlが左の時
    if (limit_speed + out - pre_out > 0){
      if (limit_speed + out - pre_out < 1000){
        motor_speed[0] = motor_speed[0]*0.9 +0.1*(limit_speed + out - pre_out);
      }else{
        motor_speed[0] = motor_speed[0]*0.9 +0.1*(1000);
      }
    }else{
      motor_speed[0] = motor_speed[0]*0.9;
    }
  } 
  pre_out = 0.5*out;

  if (val_fl-val_c>100 || val_fr-val_c>100) line_over_count++;
}

void move_servo(int n,bool wise){
  if (n==0){
    if (wise) servo_angle[0]++;
    else servo_angle[0]--;
    l_servo.write(servo_angle[0]);
    delay(10);
  }
  if (n==1){
    if (wise) servo_angle[1]++;
    else servo_angle[1]--;
    r_servo.write(servo_angle[1]);
    delay(10);
  }
  if (n==2){
    if (wise) servo_angle[2]++;
    else servo_angle[2]--;
    up_servo.write(servo_angle[2]);
    delay(50);
  }
}

void catching(int angle,bool wise){
  if (wise){
    for (int i=0;i<angle;i++) {
     move_servo(0,true);
     move_servo(1,false);
    }
  }else{
    for (int i=0;i<angle;i++) {
      move_servo(0,false);
      move_servo(1,true);
    }
  }
  l_servo.write(servo_angle[0]);
  r_servo.write(servo_angle[1]);
  delay(500);
  l_servo.write(servo_angle[0]);
  r_servo.write(servo_angle[1]);
}

int color_read(){
  delay(100);
  //Serial.begin(9600);//シリアル通信を9600bpsで初期化
  Wire.begin();//I2Cを初期化
  Wire.beginTransmission(S11059_ADDR);//I2Cスレーブ「Arduino Uno」のデータ送信開始
  Wire.write(CONTROL_MSB);//コントロールバイトを指定
  Wire.write(CONTROL_1_LSB);//ADCリセット、スリープ解除
  Wire.endTransmission();//I2Cスレーブ「Arduino Uno」のデータ送信終了
  Wire.beginTransmission(S11059_ADDR);//I2Cスレーブ「Arduino Uno」のデータ送信開始
  Wire.write(CONTROL_MSB);//コントロールバイトを指定
  Wire.write(CONTROL_2_LSB);//ADCリセット解除、バスリリース
  Wire.endTransmission();//I2Cスレーブ「Arduino Uno」のデータ送信終了

  int high_byte, low_byte;
  uint16_t red, green, blue, IR;
  delay(2500);//2500msec待機(2.5秒待機)
  Wire.beginTransmission(S11059_ADDR);//I2Cスレーブ「Arduino Uno」のデータ送信開始
  Wire.write(SENSOR_REGISTER);//出力データバイトを指定
  Wire.endTransmission();//I2Cスレーブ「Arduino Uno」のデータ送信終了
  Wire.requestFrom(S11059_ADDR, 8);//I2Cデバイス「S11059_ADDR」に8Byteのデータ要求
  if(Wire.available()){
    /*for (int i = 0;i<8;i++){
      value[i]=Wire.read();
    }*/
    high_byte = Wire.read();//high_byteに「赤(上位バイト)」のデータ読み込み
    low_byte = Wire.read();//high_byteに「赤(下位バイト)」のデータ読み込み
    red = high_byte << 8|low_byte;//1Byte目のデータを8bit左にシフト、OR演算子で2Byte目のデータを結合して、redに代入
    high_byte = Wire.read();//high_byteに「緑(上位バイト)」のデータ読み込み
    low_byte = Wire.read();//high_byteに「緑(下位バイト)」のデータ読み込み
    green = high_byte << 8|low_byte;//1Byte目のデータを8bit左にシフト、OR演算子で2Byte目のデータを結合して、greenに代入
    high_byte = Wire.read();//high_byteに「青(上位バイト)」のデータ読み込み
    low_byte = Wire.read();//high_byteに「青(下位バイト)」のデータ読み込み
    blue = high_byte << 8|low_byte;//1Byte目のデータを8bit左にシフト、OR演算子で2Byte目のデータを結合して、blueに代入
    high_byte = Wire.read();//high_byteに「赤外(上位バイト)」のデータ読み込み
    low_byte = Wire.read();//high_byteに「赤外(下位バイト)」のデータ読み込み
    IR = high_byte << 8|low_byte;//1Byte目のデータを8bit左にシフト、OR演算子で2Byte目のデータを結合して、IRに代入
  }
  Wire.endTransmission();//I2Cスレーブ「Arduino Uno」のデータ送信終了
  Wire.end();
  float hue = getHue(red,green,blue);
  float hue_b = abs(hue - 200);
  float hue_y = abs(hue - 80);
  float hue_r1 = abs(hue - 10);
  float hue_r2 = abs(hue-360);
  if (hue_b<hue_y & hue_b<hue_r1 & hue_b<hue_r2){
    return 0;
  }else if (hue_y<hue_b & hue_y<hue_r1 & hue_y<hue_r2){
    return 1;
  }else if (hue_r1<hue_b & hue_r1<hue_y & hue_r1<hue_r2){
    return 2;
  }else if (hue_r2<hue_b & hue_r2<hue_y & hue_r2<hue_r1){
    return 2;
  }else return 1;
  delay(100);
}


float getHue(float r,float g,float b){
  float min = r,max = r,a = 0,t = g- b;
  if(min > g){min = g;}
  if(min > b){min = b;}
  if(max < g){max = g;a = 120;t = b - r;}
  if(max < b){max = b;a = 240;t = r - g;}
  float res = 60 * t / (max - min) + a;
  if(res < 0)res += 360;
  return res;
} 

void approach(){
  delay(1);
  motor_step_count[0]= 0;
  motor_step_count[1]= 0;
  int val_IR = 100;
  int pre_val_IR;
  approach_cycle = 0;
  delay(1);
  while(true){ //左回転  
    microtime = micros();
    delay(10);
    motor_time_marker[0] = microtime;  
    while (true){
      microtime = micros();
      if (microtime - motor_time_marker[0] < wait(200)){
        running(LEFT,false,motor_step_count[0]);
        running(RIGHT,true,motor_step_count[1]);
      }
      else{
        //digitalWrite(13,HIGH);
        motor_time_marker[0] = microtime;//基準時間との差が一定以上広がったのでリセット
        motor_step_count[0]++;//カウントを進める。
        motor_step_count[1]++;
        if (motor_step_count[0]>3) motor_step_count[0]=0;//カウントが4以上の時は0にリセット
        if (motor_step_count[1]>3) motor_step_count[1]=0;
        val_IR = 0;
        val_IR += analogRead(pin_IR);
        val_IR += analogRead(pin_IR);
        val_IR += analogRead(pin_IR);
        val_IR += analogRead(pin_IR);
        val_IR += analogRead(pin_IR);
        delayMicroseconds(200);
        val_IR = 19988 * pow(val_IR / 5, -1.252);
        break;
      }
      delayMicroseconds(200);
    }
    if (val_IR<7){
      break;
    }else{
      pre_val_IR = val_IR;
      approach_cycle++;
    }
  }
  delayMicroseconds(100);
  go_straight(80,true,300);
}


#endif