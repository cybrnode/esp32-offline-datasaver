#include "FS.h"
#include "SPIFFS.h"
#include "Event.h"
#include <WiFi.h>
#include <HTTPClient.h>
#define FORMAT_SPIFFS_IF_FAILED true

const char* ssid = "CYBR_node";
const char* password = "cybr.node1";

Event sensorEvent;

String takeSample(){
  return String("192......,") + (rand() % 100);
}

bool uploadData(String data){

  bool isUploaded = false;
  HTTPClient http;
  http.begin("https://postman-echo.com/post");
  
  int httpResponseCode = http.POST(data);
  if (httpResponseCode == 200) {
    isUploaded = true;
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return isUploaded;
}
int listDir(fs::FS &fs){
  int fileCount = 0;
  File root = fs.open("/");
  
  if(!root){
      Serial.println("- failed to open directory");
      return -1;
  }
  
  if(!root.isDirectory()){
      Serial.println(" - not a directory");
      return -1;
  }

  File file = root.openNextFile();
  while(file) {
    fileCount++;
    Serial.print("  FILE: ");
    Serial.print(file.name());
    Serial.print("\tSIZE: ");
    Serial.println((int) file.size());
    file = root.openNextFile();
  }

  return fileCount;
}


const char * getNextBatchFile(){
  File root = SPIFFS.open("/");
  
  if(!root){
      Serial.println("- failed to open directory");
      return "";
  }
  if(!root.isDirectory()){
      Serial.println(" - not a directory");
      return "";
  }

  File file = root.openNextFile();
  return file.name();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

String readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return "";
    }

    Serial.println("- read from file:");
    String data;
    while(file.available()){
        data = data + file.read();
    }

    return data;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }
}

void setup(){
  
  Serial.begin(115200);
  
  sensorEvent.begin(3 * 1000);
  
  WiFi.begin(ssid, password);
  Serial.println("WiFi intialized");
  
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed, Going to formate file system");
    return;
  } else {
    Serial.println("File system mounted successfully");
  }
  SPIFFS.format();
}

int currentBatch = 0;
const int BATCH_SIZE = 4;
String sample[BATCH_SIZE];
int batchNumber = 11;



void loop(){
  
  if (sensorEvent.trigger()) {
    // Don't allow shutdown
    sample[currentBatch] = takeSample();
    
    Serial.print("Sample taken [");
    Serial.print(currentBatch);
    Serial.print("] ");
    Serial.println(sample[currentBatch]);
    
    currentBatch++;
    if (currentBatch == BATCH_SIZE){
      
      // allow shutdown
      Serial.println("BATCH Complete --");
      
      currentBatch = 0;
      String data = "";  
      
      for (int n = 0; n < BATCH_SIZE; n++)
        data = sample[n] + "\n" + data;

      Serial.println("WRITING] DATA batch on file");
      Serial.println(data);
      String strFilename = String("/") + String(batchNumber++) + ".txt";
      
      const char * fileName = strFilename.c_str();

      char * dataStr = new char[data.length()];
      data.toCharArray(dataStr, data.length());
      
      writeFile(SPIFFS, fileName, dataStr);
      Serial.println("WRITE Complete");
      delete dataStr;
      // Save data on file
    }
  }

  // If there is a file available to upload
  int numberOfFiles = listDir(SPIFFS);
  if(numberOfFiles > 0){

    Serial.printf("-- %d files are available to open\n", numberOfFiles);

    delay(20);
    Serial.println(WiFi.status());
    if  (WiFi.status() == WL_CONNECTED) { // There is data in system
      Serial.print("Connection established with IP :");
      Serial.println(WiFi.localIP());
      
      Serial.println("SELECTING next batch");

      const char *  batchFileNamePtr = getNextBatchFile();
      char batchFileName[32];
      strcpy((char *) batchFileNamePtr, batchFileName);
      Serial.printf("BATCH name: %s\n", batchFileNamePtr);
      Serial.printf("BATCH name: %s\n", batchFileName);
      delay(30);
      
      String data = readFile(SPIFFS, batchFileName);
      delay(30);

      Serial.printf("after readFile: %s\n", batchFileName);
      Serial.println(data);

      Serial.println("Uploading files");
      bool batchUploadStatus = uploadData(data);
      Serial.print("BatchUploadStatus ->  ");
      Serial.println(batchUploadStatus);
      delay(30);

      Serial.println("after uploadData");
      Serial.println(batchFileName);

      if (batchUploadStatus){
        deleteFile(SPIFFS, batchFileName);
      }
    }
  }
  
  delay(50);
}
