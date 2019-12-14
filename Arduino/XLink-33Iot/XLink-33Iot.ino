//Includes (for 33 IoT Network Functionalities)
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

#define SLAVE 1
#define MASTER 0
#define PACKET_STR_SIZE 20

bool useSerial = true;
int serialBaud = 9600;

String private_key = "123456789a";
String NETWORK_SSID = "JihoLABS 33 IoT";

int keyIndex = 0;
int network_status = WL_IDLE_STATUS;

WiFiServer server(23);

boolean alreadyConnected = false;

void printWiFiStatus(){
  IPAddress ip = WiFi.localIP();
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print(" | IP: ");
  Serial.println(ip);
}

short int autoConnect(bool role, int secs){
  int beginTime = millis();
  if(useSerial){
    Serial.begin(serialBaud);
    while(!Serial){
      if(millis() - beginTime > secs){
        return -1976; //TimeOut error
      }
    }
    Serial.print("[SYSTEM --verbose] OK: Serial Connection Successful! (Time Taken: ");
    Serial.print(millis()-beginTime);
    Serial.println("ms)");
  }
  if(WiFi.status() == WL_NO_MODULE){
    if(useSerial){
      Serial.println("[SYSTEM --verbose] FATAL ERROR: No WiFi Module Found! (-1)"); 
    }
    return -1;
  }
  String fv = WiFi.firmwareVersion();
  if(useSerial){
          Serial.println("[SYSTEM --verbose] Current WiFi Version: " + fv + " | Latest WiFi Version: " + WIFI_FIRMWARE_LATEST_VERSION);
  }
  if(fv < WIFI_FIRMWARE_LATEST_VERSION){
    if(useSerial){
      Serial.println("[SYSTEM --verbose] WARNING: FIRMWARE OUT OF DATE! Please update...");
    }
  }
  switch(role){
    case 0:
    {
      //Master IP is by default 192.168.4.1
      if(useSerial){
        Serial.println("[SYSTEM --verbose] Initializing AP: " + NETWORK_SSID + " With WPA2 Passphrase: " + private_key);
      }
      network_status = WiFi.beginAP(NETWORK_SSID.c_str(), private_key.c_str());
      if(network_status != WL_AP_LISTENING){
        if(useSerial){
          Serial.println("[SYSTEM --verbose] FATAL ERROR: AP Initiation FAILED! (-2)");
          return -2;
        }
      }
      if(useSerial){
        Serial.println("[SYSTEM --verbose] AP successfully initiated...");  
      }
      digitalWrite(LED_BUILTIN, HIGH); //To show AP start...
      
      server.begin();
      if(useSerial){
        Serial.println("[SYSTEM --verbose] Initiating server...");
        printWiFiStatus();
        Serial.println("[SYSTEM --verbose] Waiting MAX " + String(secs) + " secs for SLAVE... ");
      }

      beginTime = millis();
      while(network_status != WL_AP_CONNECTED){
        network_status = WiFi.status();
        if(millis() - beginTime > secs * 1000){
          if(useSerial){
            Serial.println("[SYSTEM --verbose] FATAL ERROR: SLAVE has failed to connect... (-3)");
          }
          return -3;
        }
      }
      
      if(useSerial){
        Serial.println("[SYSTEM --verbose] Client Connection Established!");
        Serial.println("[SYSTEM --verbose] Waiting " + String(secs) + " secs for client to say \"HI\"");  
      }

      String incoming = "";
      WiFiClient client = server.available();
      
      beginTime = millis();
      while(true){
        network_status = WiFi.status();
        if(network_status != WL_AP_CONNECTED){
          if(useSerial){
            Serial.println("[SYSTEM --verbose] FATAL ERROR: SLAVE disconnected without saying hi :( (-4)");  
          }
          return -4;
        }
        if(millis() - beginTime > secs*1000){
          if(useSerial){
            Serial.println("[SYSTEM --verbose] FATAL ERROR: SLAVE Response TIMEOUT (-5)");
          }
          return -5;
        }
        if(client){
          char c = '0';
          while(client.connected() && c != '\n'){
            if(client.available()){
              c = client.read();
              incoming += String(c);
            }
            if(millis() - beginTime > secs*1000){
              if(useSerial){
              Serial.println("[SYSTEM --verbose] FATAL ERROR: SLAVE Response TIMEOUT (-5)");
            }
              return -5;
            }
          }
          if(useSerial){
            Serial.println("[SYSTEM --verbose] Client Response: " + incoming);
          }
          if(incoming != String("HI\n")){
            if(useSerial){
              Serial.println("[SYSTEM --verbose] FATAL ERROR: SLAVE sent a MALFORMED RESPONSE (-6)");
            }
          }
          client.println("HI\n");
          break;
        }
      }
      if(useSerial){
        Serial.println("[SYSTEM --verbose] Successfully established connection!");
      }
      break;
    }
    case 1:
    {
       break;  
    }

  }
  return 1;
}

void setup(){
  Serial.begin(9600);
  Serial.println("hello, world!");
  autoConnect(MASTER, 20);
}

void loop(){
  Serial.println(String(autoConnect(MASTER, 20)));
  delay(2000);
}
