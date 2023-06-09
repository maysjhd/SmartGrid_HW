#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <PubSubClient.h>
#include "serial_comm.h"
#include "files.h"

//MQTT config
const char* topic_subscribe = "esp32/output";
const char* id_mqtt = "disp01";  
StaticJsonDocument<256> doc_tx; 

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

 // Funções relacionadas a MQTT
void conectaMQTT();
void mantemConexoes();
void callback(char* topic, byte* payload, unsigned int length);
void enviaValores();

// Objeto Serial_comm
Serial_comm serial;

// Cria um objeto AsyncWebServer na porta 80
AsyncWebServer server(80);

// Parâmetros para HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "setor";
const char* PARAM_INPUT_4 = "broker";
const char* PARAM_INPUT_5 = "id";

// controle
boolean modeWifi = false;

// Variáveis de tempo
unsigned long previousMillis = 0;
const long interval = 10000;  // Intervalo para esperar pela conexão WiFi
unsigned long currentMillis = 0;
unsigned long tempoPassado = 0;

//=========================================================================
// Função setup
void setup() {

  Serial.begin(115200);

  initSPIFFS();
  savedData();

  if (initWiFi()) {
    Serial.println("ESP conectado a internet...");
    client.setServer(broker.c_str(), 1883);
    client.setCallback(callback);
    modeWifi = true;
  }
  else {
    modeAP();
  }
}

//{"broker":"teste", "id":"111#$#$%"}

void loop() { 
  if(modeWifi){
    unsigned long tempoAtual = millis();
    if (tempoAtual - tempoPassado >= interval){ // envia uma mensagem a cada 10 s para o Broker
      tempoPassado = tempoAtual;
      enviaValores();
    }
    mantemConexoes();
    client.loop();
  }
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
  setor = readFile(SPIFFS, setorPath);
  broker = readFile (SPIFFS, brokerPath);
  id = readFile (SPIFFS, idPath);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(setor);
  Serial.println(broker);
  Serial.println(id);

};

//=========================================================================
// FUNÇÕES RELACIONADAS AO WIFI

// Inicializa WiFi
bool initWiFi() {
  if (ssid == "" || pass == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);

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
          setor = p->value().c_str();
          Serial.print("Setor set to: ");
          Serial.println(setor);
          // Write file to save value
          writeFile(SPIFFS, setorPath, setor.c_str());
        }
        
        // HTTP POST broker Address value
        if (p->name() == PARAM_INPUT_4) {
          broker = p->value().c_str();
          Serial.print("Broker Address set to: ");
          Serial.println(broker);
          // Write file to save value
          writeFile(SPIFFS, brokerPath, broker.c_str());
        }
        // HTTP POST ID value
        if (p->name() == PARAM_INPUT_5) {
          id = p->value().c_str();
          Serial.print("ID set to: ");
          Serial.println(id);
          // Write file to save value
          writeFile(SPIFFS, idPath, id.c_str());
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(200, "text/plain", "Done. ESP será reiniciado em 3 segundos...");
    modeWifi = true;
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
        Serial.println(broker);
        if (client.connect(id_mqtt)) {
            Serial.println("Conectado ao Broker com sucesso!");
            client.subscribe(topic_subscribe);
        } 
        else {
            Serial.println("Noo foi possivel se conectar ao broker.");
            Serial.println("Nova tentatica de conexao em 5 s");
            delay(5000);
        }
    }
}

// Funções MQTT
// Recebe para receber os dados do broker
void callback(char* topic, byte* payload, unsigned int length) 
{
  String msg;
  String valorJson;

  // Debug da mensagem recebida no tópico inscrito
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
   char c = (char)payload[i];
    msg += c;
  }

  // Cria um documento JSON
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  valorJson = doc["teste"].as<String>(); // Acessa o valor de uma chave qualquer e converte para String

  Serial.println();
  Serial.println(msg);
  Serial.println(valorJson);
  Serial.println("-----------------------");
}

void enviaValores() {
  String jsonString = "";
  JsonObject obj = doc_tx.to<JsonObject>();
  doc_tx["broker"] = broker;
  doc_tx["setor"] = setor;
  serializeJson(doc_tx, jsonString); 
  client.publish(topic_subscribe, jsonString.c_str());
  Serial.println(jsonString); // Json montado
  Serial.println("Payload enviado.");
}

void mantemConexoes() {
    if (!client.connected()) {
       conectaMQTT(); 
    }
}