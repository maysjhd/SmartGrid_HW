#ifndef FILES_H
#define FILES_H

#include "SPIFFS.h"

//Variáveis para salvar os valores do formulário HTML
String ssid;
String pass;
String ip;
String gateway;
String broker;
String id;

// Diretório para salvar os valores de entrada
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
const char* brokerPath = "/broker.txt";
const char* idPath = "/id.txt";

void initSPIFFS();
void listAllFiles();
String readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void deleteFile(const char* path);

#endif