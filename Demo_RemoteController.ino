#include <TimerOne.h>       //定时器库接口函数头文件（标准）
#include <String.h>         //C语言标准库中一个常用的头文件，在使用到字符数组时需要使用
#include <PS2X_lib.h>       //PS2手柄库接口函数头文件（标准）
//手柄相关引脚分配宏定义
#define PS2_DAT        9    
#define PS2_CMD        8
#define PS2_SEL        7
#define PS2_CLK        6

#define BOX_voltage_pin A0        //控制盒电压采集引脚分配宏定义
#define pressures   false         //手柄按键按压压力采集标志宏定义
#define rumble      false         //手柄启动声音标志宏定义
PS2X ps2x;                        //创建PS2手柄类实例
int error = 0;                    //保存手柄配置返回值
byte type = 0;                    //保存手柄
byte vibrate = 0;                 //保存手柄震动级数
volatile bool E_Key_Flag = false; //代表推进器解锁状态，true代表解锁、false代表锁定
//setup()初始换设置函数
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial2.flush();

  //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);  //调用手柄配置函数
  delay(1000);
  while (error != 0) {
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
    delay(1000);
    Serial.println("config_gamepad again");
  }
  if (error == 0) {
    Serial.println("Found Controller, configured successful ");
    Serial.print("pressures = ");
    if (pressures)
      Serial.println("true ");
    else
      Serial.println("false");
    Serial.print("rumble = ");
    if (rumble)
      Serial.println("true)");
    else
      Serial.println("false");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Note: Go to www.billporter.info for updates and to report bugs.");
  }
  else if (error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");

  else if (error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

  else if (error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");

  type = ps2x.readType();  //调用手柄类型获取函数
  switch (type) {
    case 0:
      Serial.print("Unknown Controller type found ");
      break;
    case 1:
      Serial.print("DualShock Controller found ");
      break;
    case 2:
      Serial.print("GuitarHero Controller found ");
      break;
    case 3:
      Serial.print("Wireless Sony DualShock Controller found ");
      break;
  }
  //定时器Timer1设置，50ms，回调函数callback()
  Timer1.initialize(50000);
  Timer1.attachInterrupt(callback);
}
//控制命令包发送函数
void Send_data(int cmd_type, int cmd_data1, int cmd_data2, int cmd_data3) {
  uint8_t cmd[12] = {0xFE, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  cmd[2] = cmd_type;          //保存控制命令包类型
  if (0 == cmd[2]) {          //控制命令包类型等于0时，发送心跳包以及控制盒电压
    cmd[3] = (cmd_data1 & 0xFF00) >> 8;         //保存控制盒电压高8位
    cmd[4] = cmd_data1 & 0xFF;                  //保存控制盒电压低8位
  } else if (1 == cmd[2]) {   //控制命令包类型等于1时，发送按键指令包以及按键键值
    cmd[3] = cmd_data1;                         //保存按键键值
  } else if (2 == cmd[2]) {   //控制命令包类型等于2时，发送摇杆操作包以及摇杆各轴轴向值
    cmd[3] = (cmd_data1 & 0xFF00) >> 8;         //保存手柄摇杆右侧X轴值高8位
    cmd[4] = cmd_data1 & 0xFF;                  //保存手柄摇杆右侧X轴值低8位
    cmd[5] = (cmd_data2 & 0xFF00) >> 8;         //保存手柄摇杆左侧Y轴值高8位
    cmd[6] = cmd_data2 & 0xFF;                  //保存手柄摇杆左侧Y轴值低8位
    cmd[7] = (cmd_data3 & 0xFF00) >> 8;         //保存手柄摇杆右侧Y轴值高8位
    cmd[8] = cmd_data3 & 0xFF;                  //保存手柄摇杆右侧Y轴值低8位
  }

  for (int i = 0; i < 11; i++) {                //计算出控制命令包校验和
    cmd[11] = cmd[11] ^ cmd[i];
  }

  Serial2.write(cmd, 12);                      //调用通讯串口发送函数，发送控制命令包
}

volatile bool intFlag = false;                  //用于手柄采集的标志
volatile bool intFlag_500ms = false;            //用于500ms间隔定时的标志
int timer_count = 0;                            //用于500ms间隔定时的计数
//回调函数callback()
void callback() {
  //Serial.println("callback");
  intFlag = true;
  timer_count++;                                //用于500ms间隔定时的计数加一
  if (10 == timer_count) {                      //用于500ms间隔定时的计数等于5时，用于500ms间隔定时的计数置0、用于500ms间隔定时的标志置true
    timer_count = 0;
    intFlag_500ms = true;
  }
}
//loop()主循环函数
void loop() {
  if (true == intFlag) {
    intFlag = false;
    if (error == 1)                 //如果找不到手柄，则跳过循环
      return;

    if (type == 2) {                //Guitar Hero Controller
      ps2x.read_gamepad();          //读取手柄
    } else {
      ps2x.read_gamepad(false, vibrate);

      int LY = ps2x.Analog(PSS_LY);     //读取手柄摇杆左侧Y轴值
      int RX = ps2x.Analog(PSS_RX);     //读取手柄摇杆右侧X轴值
      int RY = ps2x.Analog(PSS_RY);     //读取手柄摇杆右侧Y轴值

      if (ps2x.ButtonPressed(PSB_PAD_UP)) {             //读取1号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("Up held this hard: ");
        Send_data(1, 1, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_PAD_RIGHT)) {          //读取2号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("Right held this hard: ");
        Send_data(1, 2, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_PAD_LEFT)) {          //读取4号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("LEFT held this hard: ");
        Send_data(1, 4, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_PAD_DOWN)) {          //读取3号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("DOWN held this hard: ");
        Send_data(1, 3, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_SELECT)) {            //读取5号按键，按键摁下，推进器解锁状态赋值 false，并调用控制命令包发送函数，发送按键值
        //Serial.println("SELECT pressed");
        E_Key_Flag = false;
        Send_data(1, 5, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_START)) {             //读取6号按键，按键摁下，推进器解锁状态赋值 true，并调用控制命令包发送函数，发送按键值
        //Serial.println("Start is being held");
        E_Key_Flag = true;
        Send_data(1, 6, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_CROSS)) {              //读取10号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("X just changed");
        Send_data(1, 10, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_CIRCLE)) {             //读取7号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("Circle just pressed");
        Send_data(1, 7, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_TRIANGLE)) {           //读取8号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("Triangle just pressed");
        Send_data(1, 8, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_SQUARE)) {             //读取9号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("Square just pressed");
        Send_data(1, 9, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_L1)) {                 //读取11号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("L1 pressed");
        Send_data(1, 11, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_L3)) {                 //读取13号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("L3 pressed");
        Send_data(1, 13, 0, 0);
      }
      if (ps2x.ButtonPressed(PSB_R1)) {                 //读取12号按键，按键摁下，并调用控制命令包发送函数，发送按键值
        //Serial.println("R1 pressed");
        Send_data(1, 12, 0, 0);
      }

      if (true == E_Key_Flag) {                         //推进器解锁状态为 true 时，调用控制命令包发送函数，发送摇杆值
        //    Serial.println("Send control data");
        Send_data(2, map(255 - RX, 0, 255, 0, 1023), map(LY, 0, 255, 0, 1023), map(255 - RY, 0, 255, 0, 1023));
      }
    }
  }

  if (true == intFlag_500ms) {                          //判断 intFlag_500ms 为 true 时，intFlag_500ms 置 false，并调用控制命令包发送函数，发送心跳包和控制盒电压
    intFlag_500ms = false;

    Send_data(0, analogRead( BOX_voltage_pin ), 0, 0);
  }
}
