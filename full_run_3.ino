//＃define tukaitaitokinitukawu
#include "run_file.h"



void setup() {
  //モーターのピン出力設定
  pinMode(motor_out[LEFT][0],OUTPUT);
  pinMode(motor_out[LEFT][1],OUTPUT);
  pinMode(motor_out[RIGHT][0],OUTPUT); 
  pinMode(motor_out[RIGHT][1],OUTPUT);
  
  //フォトリフレクタ信号のアナログ入力設定
  pinMode(pin_fl,INPUT);
  pinMode(pin_l,INPUT);
  pinMode(pin_c,INPUT);
  pinMode(pin_r,INPUT);
  pinMode(pin_fr,INPUT);

  //タクトスイッチの入力設定
  pinMode(sw,INPUT);

  //サーボの制御
  l_servo.attach(6);
  r_servo.attach(5);
  up_servo.attach(4);
  l_servo.write(servo_angle[0]);
  r_servo.write(servo_angle[1]);
  up_servo.write(servo_angle[2]);

  delay(1000);
}

void loop() {//全体の流れ
  //スイッチ読み取り
  sw_vals[0]=digitalRead(sw);
  delay(50);
  sw_vals[1]=digitalRead(sw);
  delay(50);
  sw_vals[2]=digitalRead(sw);
  if (sw_vals[0]&&sw_vals[1]&&sw_vals[2]== HIGH){//三回連続でHIGH
    digitalWrite(13,HIGH);
    delay(2000);
    digitalWrite(13,LOW);
    while (true){
      //スタート
      catching(65,true);
      delay(1000);
      for (int i=0;i<30;i++){
        move_servo(2,true);
      }
      motor_step_count[0]= 0;
      motor_step_count[1]= 0;
      line_over_count=0;
      go_straight(450,true,300);
      while(line_over_count <= 0){//二本目の線を越えるまでライントレース
        line_trace_cycle();
      }
      go_straight(70,true,500);
      //ゴールに入れる
      turn(85,false,400);
      turn_at_cross();
      delay(100);
      turn_at_cross();
      delay(500);
      go_straight(170,true,500);
      delay(500);
      for (int i=0;i<30;i++){
        move_servo(2,false);
      }
      catching(65,false);
      delay(500);
      go_straight(180,false,400);//後ろに戻る
      delay(500);
      //帰る
      turn(90,true,400);//後ろを向く
      turn_at_cross();
      delay(100);
      turn_at_cross();
      delay(500);
      int over = 1;
      while (true){
        go_straight(40,true,400);
        //ボール回収エリアへ
        line_over_count=0;//line_over_countをリセット
        while(line_over_count <= over){//2回線を越えるまでライントレース
          line_trace_cycle();
        }
        //ボールをつかむプログラムはじめ
        go_straight(50,false,400);
        while (true){
          search_boals(40,300);
          if (servo_angle[0]!=70) break;
          go_straight(100,true,400);
        }
        delay(100);
        int over = color_read()+1;
        delay(500);
        turn(170,false,400);
        turn_at_cross();
        delay(100);
        turn_at_cross();
        go_straight(80,false,400);
        delay(500);
        //ボールをつかむプログラム終了
        line_over_count=0;//line_over_countをリセット
        while(line_over_count <= over){//３回線を越えるまでライントレース
          line_trace_cycle();
        }
        //ゴールに入れる
        if (over==1){
          go_straight(500,true,500);
          delay(500);
          for (int i=0;i<30;i++){
            move_servo(2,false);
          }
          catching(65,false);
          delay(500);
          go_straight(500,false,500);
          delay(500);
          turn(170,false,300);
          turn_at_cross();
          delay(100);
          turn_at_cross();
          delay(500);
          go_straight(300,true,500);
          over-=1;
        }else{
          go_straight(70,true,400);
          turn(85,true,400);
          turn_at_cross();
          delay(100);
          turn_at_cross();
          delay(500);
          go_straight(180,true,400);
          delay(500);
          for (int i=0;i<30;i++){
            move_servo(2,false);
          }
          catching(65,false);
          delay(500);
          go_straight(180,false,400);//後ろに戻る
          delay(500);
          turn(90,true,400);//後ろを向く
          turn_at_cross();
          turn_at_cross();
          delay(500);
          over-=1;
        }
      }
    }
  }
  delay(500);
}

