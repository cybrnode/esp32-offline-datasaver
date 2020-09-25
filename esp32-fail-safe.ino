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

  bool res = false;
  HTTPClient http;
  http.begin("https://postman-echo.com/post");
  
  int httpResponseCode = http.POST(data);
  if (httpResponseCode == 200) {
    res = true;
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return res;
}
int listDir(fs::FS &fs){
  int fileCount = 0;
  Serial.print("Listing directory:");
  
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
    Serial.println(file.size());
    file = root.openNextFile();
  }

  return fileCount;
}


String getNextBatchFile(){
  Serial.print("Listing directory:");
  
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
  if (file) return String(file.name());
  else return "";
  
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
  
 
    /*
    writeFile(SPIFFS, "/hello.txt", "Hello ");
    appendFile(SPIFFS, "/hello.txt", "World!\r\n");
    readFile(SPIFFS, "/hello.txt");
    renameFile(SPIFFS, "/hello.txt", "/foo.txt");
    readFile(SPIFFS, "/foo.txt");
    deleteFile(SPIFFS, "/foo.txt");
    testFileIO(SPIFFS, "/test.txt");
    deleteFile(SPIFFS, "/test.txt");
    Serial.println( "Test complete" );
    */
}

int currentBatch = 0;
const int BATCH_SIZE = 4;
String sample[BATCH_SIZE];
int batchNumber = 0;



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
      const char * fileName = (String("/") + batchNumber++).c_str();
      const char * dataStr = data.c_str();
      writeFile(SPIFFS, fileName, dataStr);
      Serial.println("WRITE Complete");
      delete fileName;
      delete dataStr;
      // Save data on file
    }
  }

  if (WiFi.status() == WL_CONNECTED) { // Replace with your connection check

    Serial.print("Connection established with IP :");
    Serial.println(WiFi.localIP());
    
    if (listDir(SPIFFS) > 0){ // There is data in system
      Serial.println("SELECTING next batch");
      const char * batchFileName = getNextBatchFile().c_str();
      Serial.print("BATCH name: ");
      delay(30);
      
      Serial.println(batchFileName);
      String data = readFile(SPIFFS, batchFileName);
      delay(30);

      Serial.println("Uploading files");
      bool batchUploadStatus = uploadData(data);
      Serial.print("BatchUploadStatus -> ");
      Serial.println(batchUploadStatus);
      delay(30);

      if (batchUploadStatus){
        deleteFile(SPIFFS, batchFileName);
      }
    }
  }
  
  delay(20);
}
