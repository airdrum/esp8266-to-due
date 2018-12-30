#include <ESP8266WiFi.h>       // Built-in
#include <ESP8266WiFiMulti.h>  // Built-in
#include <ESP8266WebServer.h>  // Built-in
#include <ESP8266mDNS.h>

ESP8266WiFiMulti wifiMulti; 
ESP8266WebServer server(80);
// Adjust the following values to match your needs
// -----------------------------------------------


#define   servername "fileserver"  // Set your server's logical name here e.g. if 'myserver' then address is http://myserver.local/

const char* ssid_1     = "****";
const char* password_1 = "****";

#define ServerVersion "1.0"
String webpage = "";

#define UART_DATA_SIZE 120
#define MAX_NUMBER_OF_CHECKSUM 65535
#define UART_PACKAGE_SIZE 128

struct uart_transfer_format{
  uint16_t header;
  uint16_t sequence_package_number;
  uint16_t total_package_number;
  uint8_t data[UART_DATA_SIZE];
  uint16_t checksum;
};

unsigned char uart_raw_data[UART_PACKAGE_SIZE];
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  delay(10000);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_1, password_1);
  //Serial.println("Connecting ...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  /*Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use*/
  // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
  if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    Serial.println(F("Error setting up MDNS responder!")); 
    ESP.restart(); 
  } 
  server.on("/",         HomePage);
  server.on("/upload",   File_Upload);
  server.on("/fupload",  HTTP_POST,[]()
  { server.send(200);}, handleFileUpload);
  ///////////////////////////// End of Request commands
  server.begin();
  //Serial.println("HTTP server started");
  //Serial.println("Ready to receive data");
  pinMode(5,OUTPUT);
  digitalWrite(5,HIGH);
}

byte serialOut[128];
uart_transfer_format uart_data;
bool initFile =true;
void loop(void){
  
  server.handleClient(); // Listen for client connections
  delay(10);
}

// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
  SendHTML_Header();
  webpage += F("<a href='/upload'><button>Upload</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}

int TotalSize;
int inner_loop;
int sq_count = 0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& uploadfile = server.upload();
      //Serial.printf("\ncurrentSize: %d\n",uploadfile.currentSize);
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    digitalWrite(5,LOW);
    delay(250);
    digitalWrite(5,HIGH);
    
    delay(250);
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    filename = String();    
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    byte *sendFile;
    sendFile = uploadfile.buf;
    sendFile[uploadfile.currentSize] = NULL;
    delay(20);
    sq_count++;
    
    uint16_t checksum;
    uart_data.header = 0xABCD;
    uart_data.total_package_number = MAX_NUMBER_OF_CHECKSUM;
    uart_data.sequence_package_number = sq_count;
    for(int k=0; k<UART_DATA_SIZE; k++){
      if(k<uploadfile.currentSize){
        uart_data.data[k] = sendFile[k];
        checksum += uart_data.data[k];
      }else{
        uart_data.data[k] = 0xFF;
        checksum += uart_data.data[k];
      }
    }
    checksum += uart_data.sequence_package_number;
    checksum += MAX_NUMBER_OF_CHECKSUM;//uart_data.total_package_number;
    uart_data.header = 0xABCD;
    uart_data.checksum = checksum;

    memcpy(&uart_raw_data[0], (unsigned char *)&uart_data, UART_PACKAGE_SIZE);
    delayMicroseconds(1000);
    for(int i=0; i<UART_PACKAGE_SIZE; i++){
      Serial.write(uart_raw_data[i]);//Serial.printf("0x%02X ",uart_raw_data[i]);//
    }
    delay(150);
  }
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
      
      uint16_t checksum;
      uart_data.header = 0xABCD;
      uart_data.total_package_number = sq_count;
      uart_data.sequence_package_number = sq_count;
      for(int k=0; k<UART_DATA_SIZE; k++){
          uart_data.data[k] = 0xFF;
          checksum += uart_data.data[k];
      }
      checksum += uart_data.sequence_package_number;
      checksum += sq_count;//uart_data.total_package_number;
      uart_data.header = 0xABCD;
      uart_data.checksum = checksum;

      memcpy(&uart_raw_data[0], (unsigned char *)&uart_data, UART_PACKAGE_SIZE);
      delayMicroseconds(1000);
      for(int i=0; i<UART_PACKAGE_SIZE; i++){
        Serial.write(uart_raw_data[i]);//Serial.printf("0x%02X ",uart_raw_data[i]);
      }
      delay(150);
      sq_count=0;
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); 
      webpage += uploadfile.filename+"</h2>";
      append_page_footer();
      server.send(200,"text/html",webpage);
      delay(1000);   
      
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<a href='/'>[Back]</a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

void append_page_header() {
  webpage  = F("<!DOCTYPE html><html>");
  webpage += F("<head>");
  webpage += F("<title>Vulpus Electronics ESP8266 Firmware Uploader</title>"); // NOTE: 1em = 16px
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<style>");
  webpage += F("body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}");
  webpage += F("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}");
  webpage += F("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
  webpage += F("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}");
  webpage += F("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
  webpage += F("section {font-size:0.88em;}");
  webpage += F("h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}");
  webpage += F("h2{color:orange;font-size:1.0em;}");
  webpage += F("h3{font-size:0.8em;}");
  webpage += F("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}"); 
  webpage += F("th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}"); 
  webpage += F("tr:nth-child(odd) {background-color:#eeeeee;}");
  webpage += F(".rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}");
  webpage += F(".rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}");
  webpage += F(".rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}");
  webpage += F(".column{float:left;width:50%;height:45%;}");
  webpage += F(".row:after{content:'';display:table;clear:both;}");
  webpage += F("*{box-sizing:border-box;}");
  webpage += F("footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
  webpage += F("button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}");
  webpage += F(".buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}");
  webpage += F(".buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}");
  webpage += F(".buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}");
  webpage += F(".buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}");
  webpage += F("a{font-size:75%;}");
  webpage += F("p{font-size:75%;}");
  webpage += F("</style></head><body><h1>Profelis Firmware Uploader "); webpage += String(ServerVersion) + "</h1>";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void append_page_footer(){ // Saves repeating many lines of code for HTML page footers
  webpage += F("<ul>");
  webpage += F("<li><a href='/'>Home</a></li>"); // Lower Menu bar command entries
  webpage += F("<li><a href='/upload'>Upload</a></li>"); 
  webpage += F("</ul>");
  webpage += F("</body></html>");
}
