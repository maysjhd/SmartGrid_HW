#include <WiFi.h>
#include <PubSubClient.h>

const char* topic_subscribe = "esp32/output";
const char* ssid = "Fablab";
const char* pass = "@fablab2020!";
const char* id_mqtt = "disp01";   
const char* broker_mqtt = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  conectaWiFi();
  client.setServer(broker_mqtt, 1883);
  client.setCallback(callback);
 
}

void loop() {
  // put your main code here, to run repeatedly:
  mantemConexoes();
  client.loop();
}

void conectaWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
     return;
  }
        
  Serial.print("Conectando-se na rede: ");
  Serial.print(ssid);
  Serial.println("  Aguarde!");

  WiFi.begin(ssid, pass); // Conecta na rede WI-FI  
  while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso, na rede: ");
  Serial.print(ssid);  
  Serial.print("  IP obtido: ");
  Serial.println(WiFi.localIP()); 
}

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
    
    conectaWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
