/* 蓝牙库 */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ESP32Servo.h> // 标准的Servo库

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  // UUID 值
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

Servo servos[4];  // 创建4个舵机对象

int delayTime = 1;  // 脉冲停留
int minAngle = 0;   // 初始角度
int maxAngle = 180;

int servoIndex = 0;
int angle = 0;

bool deviceConnected = false;  // 蓝牙设备连接状态
bool oldDeviceConnected = false;  // 记录上一次设备连接状态

/* 接收到数据后的处理逻辑写在这里 */
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    uint8_t *data = pCharacteristic->getData();
    int len = pCharacteristic->getValue().empty() ? 0 : pCharacteristic->getValue().length();
    if (len >= 2) {
      servoIndex = data[0];
      angle = data[1];
     // Serial.println(servoIndex);
     // Serial.println(angle);
    }
  }
};

/* 服务器回调，用于检测连接状态 */
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
   // Serial.println("Device connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
   // Serial.println("Device disconnected");
  }
};

void setup() {
 // Serial.begin(115200);
  // 给舵机绑定引脚
  servos[0].attach(25);servos[1].attach(32);
  servos[2].attach(33);servos[3].attach(26);
  
  // 初始化位置
  servos[0].write(minAngle);servos[1].write(minAngle);
  servos[2].write(minAngle);servos[3].write(minAngle);
  
  //StartMessage();               // 启动消息
  delay(200);                   // 给芯片启动时间，然后再启动蓝牙

  SetBLEservice("MyESP32", SERVICE_UUID);  // 设置蓝牙
  
  // 创建BLE特征
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);  // 去掉PROPERTY_NOTIFY

  pCharacteristic->setCallbacks(new MyCallbacks());  // 当有数据写入时，调用回调函数
 // pCharacteristic->setValue("Hello World");          // 设置特征的初始值
  pServer->setCallbacks(new ServerCallbacks());   // 设置服务器回调
  StartBLEservice();  // 启动蓝牙
}

void loop() {
  // 检查蓝牙连接状态，断开时重启广播以重连
  if (deviceConnected) {
    servos[servoIndex].write(angle);  // 控制舵机角度
  }
  
  // 检测设备断开连接后重连
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 稍作等待，确保蓝牙断开
    pServer->startAdvertising(); // 重新启动广告
   // Serial.println("Start advertising to reconnect");
    oldDeviceConnected = deviceConnected;
  }

  // 记录当前连接状态
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

//------------------------- 蓝牙相关函数封装 -----------------------------------
/*void StartMessage() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
}*/

void SetBLEservice(std::string BLEname, std::string uuid) {
  BLEDevice::init(BLEname);   // 创建蓝牙设备，名称
  pServer = BLEDevice::createServer();   // 创建蓝牙服务器
  pService = pServer->createService(uuid);   // 创建蓝牙服务
}

void StartBLEservice() {
  pService->start();  // 启动服务
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponse(true);  // 设置扫描响应
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();  // 启动广播
}
