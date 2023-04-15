#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>
#include "serial_comm.h"
#include "files.h"

//MQTT config
const char* topic_subscribe = "esp32/output";
const char* id_mqtt = "disp01";   
const char* broker_mqtt = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

//IP padrão para configuração da rede Wifi e outras coisas
// 192.168.4.1


//=========================================================================
// DEFINIÇÕES
 // Funções relacionadas ao WIFI
bool initWiFi();
void modeAP();
void savedData();
void conectaMQTT();
void mantemConexoes();
void callback(char* topic, byte* payload, unsigned int length);

// Objeto Serial_comm
Serial_comm serial;

// Cria um objeto AsyncWebServer na porta 80
AsyncWebServer server(80);

// Parâmetros para HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
const char* PARAM_INPUT_5 = "broker";
const char* PARAM_INPUT_6 = "id";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Configuração do endereço Gateway IP
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Variáveis de tempo
unsigned long previousMillis = 0;
const long interval = 10000;  // Intervalo para esperar pela conexão WiFi
unsigned long currentMillis = 0;

//=========================================================================
// Função setup
void setup() {

  Serial.begin(115200);

  initSPIFFS();
  savedData();

  if (initWiFi()) {
    Serial.println("ESP conectado a internet...");
  }
  else {
    modeAP();
  }
  // client.setServer(broker_mqtt, 1883);
  // client.setCallback(callback);
}

//{"broker":"teste", "id":"111#$#$%"}

void loop() { 
// mantemConexoes();
// client.loop();
//  savedData(); 
//  serial.sendJson(broker, id);
//  listAllFiles();
}


//=========================================================================
// FUNÇÕES RELACIONADAS A SPIFFS

// Inicializa SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Lê arquivos do SPIFFS
String readFile(fs::FS &fs, const char * path) {
//  Serial.printf("Reading file: %s\r\n", path)

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Grava arquivos no SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message) {
//  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

void listAllFiles(){
  File root = SPIFFS.open("/"); 
  File file = root.openNextFile();
  while(file){
      Serial.print("FILE: ");
      Serial.println(file.name());
      file = root.openNextFile();
  }
}

void deleteFile(const char* path){
  SPIFFS.remove(path);
}

void savedData() {
  // Recuperar os valores salvos em SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  broker = readFile (SPIFFS, brokerPath);
  id = readFile (SPIFFS, idPath);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(broker);
  Serial.println(id);
  Serial.println(ip);
  Serial.println(gateway);
};
//=========================================================================

//=========================================================================
// FUNÇÕES RELACIONADAS AO WIFI

// Inicializa WiFi
bool initWiFi() {
  if (ssid == "" || ip == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)) {
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Conectando ao WiFi...");

  currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void modeAP() {
  // Connect to Wi-Fi network with SSID and password
  Serial.println("Configuracao AP (Access Point)");
  // NULL sets an open Access Point
  WiFi.softAP("ESP-WIFI", NULL);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/wifimanager.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          Serial.print("SSID set to: ");
          Serial.println(ssid);
          // Write file to save value
          writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          Serial.print("Password set to: ");
          Serial.println(pass);
          // Write file to save value
          writeFile(SPIFFS, passPath, pass.c_str());
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          ip = p->value().c_str();
          Serial.print("IP Address set to: ");
          Serial.println(ip);
          // Write file to save value
          writeFile(SPIFFS, ipPath, ip.c_str());
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          gateway = p->value().c_str();
          Serial.print("Gateway set to: ");
          Serial.println(gateway);
          // Write file to save value
          writeFile(SPIFFS, gatewayPath, gateway.c_str());
        }
        // HTTP POST broker Address value
        if (p->name() == PARAM_INPUT_5) {
          broker = p->value().c_str();
          Serial.print("Broker Address set to: ");
          Serial.println(broker);
          // Write file to save value
          writeFile(SPIFFS, brokerPath, broker.c_str());
        }
        // HTTP POST ID value
        if (p->name() == PARAM_INPUT_6) {
          id = p->value().c_str();
          Serial.print("ID set to: ");
          Serial.println(id);
          // Write file to save value
          writeFile(SPIFFS, idPath, id.c_str());
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
    delay(3000);
    ESP.restart();
  });
  server.begin();
}
//=========================================================================

// Funções MQTT
void conectaMQTT() { 
    while (!client.connected()) {
        Serial.print("Conectando ao Broker MQTT: ");
        Serial.println(broker_mqtt);
        if (client.connect(id_mqtt)) {
            Serial.println("Conectado ao Broker com sucesso!");
            client.subscribe(topic_subscribe);
        } 
        else {
            Serial.println("Noo foi possivel se conectar ao broker.");
            Serial.println("Nova tentatica de conexao em 10s");
            delay(10000);
        }
    }
}

// Funções MQTT
void callback(char* topic, byte* payload, unsigned int length) 
{

  String msg;
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
   char c = (char)payload[i];
    msg += c;
  }

  Serial.println();
  Serial.println(msg);
  Serial.println("-----------------------");
}


void mantemConexoes() {
    if (!client.connected()) {
       conectaMQTT(); 
    }
  
}