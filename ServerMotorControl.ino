#include <ArduinoJson.h>
#include <aJSON.h>
#include <WiFi.h>
#include <SPI.h>

WiFiClient client;
char ssid[] = "Erik";
char password[] = "password";
int keyIndex = 0;
int lastStatus = 0;
const int buttonPin = PUSH2;     // the number of the pushbutton pin
const int butt2 = PUSH1;
const int ledPin =  RED_LED;      // the number of the LED pin
const int LED = GREEN_LED;
int butt2state = 0;
int buttonState = 0;         // variable for reading the pushbutton status
int status = 3;

// Name address for Open Weather Map API
const char* server = "vama.herokuapp.com";

// Replace with your url for other devices: change id=2 for switch, id=3 for lock
const char* resource = "/api/view.php?id=3";

const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// The type of data that we want to extract from the page
struct clientData {
  char name[8];
  char Status[8];
};

void setup() {
   //check for wifi connection
  Serial.begin(115200);      // initialize serial communication
  pinMode(RED_LED, OUTPUT);      // set the LED pin mode
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to Network named: ");
  // print the network name (SSID);
  Serial.println(ssid); 
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while waiting to connect
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nYou're connected to the network");
  pinMode(9, OUTPUT);//phase
  pinMode(5, OUTPUT);//enable 
  pinMode(7, INPUT);//"locked" status = 0 input 
  pinMode(8, INPUT);//"unlocked" status = 1 input
  delay(1000);
}

void loop() {
  if(connect(server)) {
    if(sendRequest(server, resource) && skipResponseHeaders()) {
      clientData x; //initialize a clientData object named x
      if(readReponseContent(&x)) {
      printclientData(&x);
      
      }
    }
  }
  disconnect();
  wait();
}

// Open connection to the HTTP server
bool connect(const char* hostName) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, 80);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

// Send the HTTP GET request to the server
bool sendRequest(const char* host, const char* resource) {
  Serial.print("GET ");
  Serial.println(resource);

  client.print("GET ");
  client.print(resource);
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }
  return ok;
}

bool readReponseContent(struct clientData* clientData) {

  const size_t capacity = JSON_OBJECT_SIZE(5) + 50;
  
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(client);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in using to your struct data
  strcpy(clientData->name, root["name"]);
  strcpy(clientData->Status, root["status"]);
  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string
  return true;
}

// Print the data extracted from the JSON
void printclientData(const struct clientData* clientData) {
  Serial.print("Name = ");
  Serial.println(clientData->name);
  Serial.print("Status = ");
  Serial.println(clientData->Status);
  if (atoi(clientData->Status)==0){                     //if the server status of the appliance is off or locked
    if (atoi(clientData->Status) != lastStatus){        //if the server status does not match the status of the appliance currently
      while (digitalRead(7) == LOW){                    // loop until the system reaches a new state
        digitalWrite(9, HIGH);                          // phase set to high to turn motor clockwise
        digitalWrite(5, HIGH);                          // enable set to high to enable motor
        
      }
      digitalWrite(5,LOW);                              //enable set to low to brake the motor
    }
    lastStatus = 0;                                     //update status of appliance
  }
  else if(atoi(clientData->Status)==1){                 //if the server status is on or unlocked
    if (atoi(clientData->Status) != lastStatus){        //if the server status does not match the status of the appliance currently
    while (digitalRead(8) == LOW){                      //loop until system reaches a new state
      digitalWrite(9,LOW);                              //phase set to low to turn motor counterclockwise
      digitalWrite(5,HIGH);                             //enable set to high to enable motor
      
    }
    digitalWrite(5,LOW);                                //enable set to low to brake the motor
  }
  lastStatus =1;                                        //update status of appliance
  }
}

// Close the connection with the HTTP server
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}

// Pause for a 1 minute
void wait() {
  Serial.println("Wait 5 seconds");
  delay(5000);
}
