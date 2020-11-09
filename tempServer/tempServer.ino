/*
 * This is a combination of a lot of hard work by others in the community 
 * If you recogise the code, let me know and I will attribute it to you.
 * I am afraid the origin code has been lost in the mists of time and copypasta
 * 
 * Note: there are some hard coded email entries required in the SendEmal section,
 * 
 * Base 64 encoder
 * 
 * 
 * Sneaky Pad zeros
 * https://forum.arduino.cc/index.php?topic=371117.msg2559670#msg2559670
 * 
 * BMP file create  - might be this source
 * https://forum.arduino.cc/index.php?topic=112733.msg849962#msg849962
 * 
 * NTP - from the example
 * 
 * 
 * 
 * 
 */

#include <SdFat.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <dht.h>
#include <TimeLib.h>
#include <Base64.h>
const byte ip[] = { 192, 168, 1, 231 };                                // change this for your network
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };            // change this for your card
// BMP header based on our file size atc
const unsigned char bmpFileHeader[14] PROGMEM =  {'B','M', '"','M',0,0, 0,0, 0,0, 54,0,0,0};
const unsigned char bmpInfoHeader[40] PROGMEM =  {40,0,0,0, 208,2,0,0, 100,0,0,0, 1,0, 24,0};

long Cycledelay =1000;                                       // 600000= 10 - minutes
const long RepeatMessage = 1800000;                                 // 1800000 = 30 minutes between nagging 
const int  tempthresh = 30;   
dht DHT;                                                            // sensor
EthernetClient client;
EthernetUDP Udp;
time_t  LastMonth;
time_t  YesterdayStamp;
byte    packetBuffer[48]; 
long    lastReadingTime = 0;
int  tempreading = 0;
int  humidreading = 0;
boolean HaventSendMessage = true;                                    // logic for not spamming
boolean ItsANewDay = false;                                          // logic for yesterdays log file email
boolean BeenHotBefore = false;                                       // logic for how many hot times
boolean IsHot=false;                                                 // logic for is it hot now
boolean BeenAnHour=false;                                            // logic for not spamming
int     Daycheckold;                                                  // holder for day check today or yesterday
time_t  Todays;
char    Todayfilename[13]="11111111.txt";                               // default incase NTP doesnt work
char    Yestderdayfilename[13]="10111111.txt";                         // default incase NTP doesnt work
char    MailFilename[13];                                              // default incase NTP doesnt work
long    LastHotTime = millis() * 100;
char *  buf = (char *) malloc (3);                                    // buffer for date characters
char    DateTimeArray[11];                     
byte    picturepoints [144]; // points for 10 minute temp checks
byte    picturepointsH [144]; // points for 10 minute humid checks
int     chk;
SdFat   SD;
SdFile  thefile;


const char SMTPserver[44] = "xxxxxxxx-com-au.mail.protection.outlook.com"; // change the size of this to match entry +1
const char FromEmail[29]="tech.support@xxxxxxxx.com.au";    // change the size of this to match entry +1
const char ToEmail[28]=  "ed.xxxxxxxx@xxxxxxxx.com.au";     // change the size of this to match entry +1
const char bmpfile[11]=  "report.bmp";                      // change the size of this to match entry +1

void setup() {
  Serial.begin(115200);
  SD.begin(4);
  Serial.println(F(" Setup Started  "));
  Ethernet.begin(mac,ip);
  Udp.begin(8888);
  lastReadingTime=millis() ;                           // last read time
  setSyncInterval(60000);                              // time sync
  setSyncProvider(Getthetime);                         // function to return epoc to timelib from NTP
    if (timeStatus() == timeNotSet)
    {
      // Try Again
      setSyncProvider(Getthetime); 
    }
  
  Daycheckold=day()-1;                                 // yesterday, every thing seems so far away
  time_t Todays1=now();                                // today
  strcpy(Todayfilename,ReturnDateTime(Todays1,false)); // file name create
  strcat(Todayfilename,".txt");                        // add extension
  chk = DHT.read11(6);                                 // read sensor from pin 6
  tempreading = DHT.temperature ;
  humidreading = DHT.humidity;
  PrintLogic();
}

void loop() {
 
  if (millis() - lastReadingTime > Cycledelay) {                // cycle delay is 10 minutes
  Serial.print(F(" Reading to "));
  Todays=now();                                                 // todays
  time_t Yesterdays =now()-  86400UL ;                          // yesterday
  strcpy(Todayfilename,ReturnDateTime(Todays,false));           // file name create
  strcat(Todayfilename,".txt");                                 // add extension
  strcpy(Yestderdayfilename,ReturnDateTime(Yesterdays,false));  // file name create
  strcat(Yestderdayfilename,".txt");                            // add extension
  Serial.println(Todayfilename);
  chk = DHT.read11(6);                                          // read sensor from pin 6
  tempreading = DHT.temperature ;
  humidreading = DHT.humidity;
  lastReadingTime=millis() ;                                     // set last read time
  if (tempreading < 0.00)                                        // sometimes -999  
    {
    // Try another read
    chk = DHT.read11(6);                                       // read sensor from pin 6
    tempreading = DHT.temperature ;
    humidreading = DHT.humidity;
      if (tempreading < 0.00)
        {
        tempreading = 0 ;
        humidreading = 0;
        }
      }
    tempSD();     // write to the log file
    // Check the temp and set flags if hot and hot before
PrintLogic();
    if (tempreading > tempthresh)
      {
      
      IsHot=true;                     // logic for hot
PrintLogic();
      
      if (BeenHotBefore == false ) 
        {
        // no - this is the first hot we have seen - 
        // set hot before logic  and set the limit for RepeatMessage time
        LastHotTime=millis() + RepeatMessage;
        BeenHotBefore=true;
 PrintLogic();

        
        }
        else
        {
        //yes we have but has it been an hour ?
        if ( LastHotTime < millis())
          {
          BeenAnHour=true;
          // Been more than an hour since we last complained so reset send a message
          HaventSendMessage=true;
          LastHotTime=millis() + RepeatMessage;  
PrintLogic();             
          }
          else
          {
          BeenAnHour=false;
PrintLogic();             
          }
        }
    }
    else
    {
    IsHot=false;
    BeenAnHour=false;
    BeenHotBefore=false;
PrintLogic();   
    }
    // here we check for a new day
    if (Daycheckold == day())
      {
      // Nothing to see here - move along
      ItsANewDay=false;
PrintLogic();   
      }
      else
      {
      // its a new day so reset all the flags
      HaventSendMessage=true; 
      ItsANewDay=true;
      BeenHotBefore=false;
      Daycheckold=day();
PrintLogic();   
      Serial.println(F("Reset as new day or startup ") );
      }

      if (HaventSendMessage)
      {
        if (ItsANewDay == true)
          {
          // send an email message for end of day
          Serial.println(F("Doing Yesterdays ") );
          strcpy(MailFilename,Yestderdayfilename);
          BMPcreate();
          strcpy(MailFilename,Yestderdayfilename);
          Serial.println(F("Starting Email ") );
          SendEmailMessage();
          ItsANewDay=false;
PrintLogic();           
            }
        }
        
        
        if (IsHot)
          {
            strcpy(MailFilename,Todayfilename);
            if (BeenHotBefore && BeenAnHour){
              Serial.println(F("Doing Todays log again") );
              SendEmailMessage();
              HaventSendMessage=false;
PrintLogic();   
              
            }
            else
            {
             Serial.println(F("Doing Todays log as its hot") );
             SendEmailMessage();
             HaventSendMessage=false;
PrintLogic();   
            }
          }
PrintLogic();
          
  } // end of the timing loop
  PrintLogic();
  //Serial.print(F( "/"));
  Cycledelay =600000;  // put the cycle backup to 10 minutes after initial startup
  delay(60000);
}


// Just a test
void PrintLogic(){

  Serial.print(F(" Hot:"));
  Serial.print(IsHot);
    Serial.print(F(" BeenAnHour:"));
  Serial.print(BeenAnHour);

    Serial.print(F(" BeenHotBefore:"));
  Serial.print(BeenHotBefore);

   Serial.print(F(" ItsANewDay:"));
  Serial.print(ItsANewDay);

   Serial.print(F(" Ram :"));
  Serial.print(freeRam ());


   Serial.print(F("\t Last Hot Time:"));
  Serial.println(LastHotTime);
 
  
  
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// just so we have nice even fields in the log file
char * padzeros(int digits){
   byte r;
   r = sprintf(buf, "%02d", digits);
   return buf;
}


// returns either date or time
char * ReturnDateTime( time_t herenow, boolean datetype)
  {  
  DateTimeArray[0] = '\0';
    if (datetype == true ){
      char * buf1 = padzeros(hour(herenow));
      strcat(DateTimeArray, buf1);
      strcat(DateTimeArray, ":");
      buf1 = padzeros(minute(herenow));
      strcat(DateTimeArray, buf1);
      strcat(DateTimeArray, ":");
      buf1 = padzeros(second(herenow));
      strcat(DateTimeArray, buf1);
      }
      else
      {
      char * buf1 = padzeros(day(herenow));
      strcat(DateTimeArray, buf1);
      buf1 = padzeros(month(herenow));
      strcat(DateTimeArray, buf1);
      buf1 = padzeros(year(herenow));
      strcat(DateTimeArray, buf1);
      }
  return (DateTimeArray);
}


// Send an email message - hard coded to save variables
  void SendEmailMessage() {


   Serial.print(MailFilename);
   Serial.println(F(" <M--- ") );

   
  byte thisByte = 0;
  byte respCode;
  if(client.connect(SMTPserver,25) == 1) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return 0;
  }
  client.println(F("helo"));
  if(!eRcv()) return 0;

  client.print(F("MAIL From: "));
  client.println(FromEmail);
  client.print(F("RCPT To: "));
  client.println(ToEmail);
  client.println(F("DATA"));
  client.print(F("To: "));
  client.println(ToEmail);
  client.print(F("From: "));
  client.println(FromEmail);
  client.print(F("Return-Path: <"));
  client.print(FromEmail);
  client.println(F(">"));
  client.print(F("Subject: Office "));
  if (IsHot) {
  client.print(F("ALERT Temperature Warning  "));
  client.print(tempreading);
  }
  else
  {
  client.print(F("Temperature Logs  "));
  client.print(MailFilename);
  }
  
   client.print(F("  \r\n"));
   client.print(F("MIME-Version: 1.0\r\n"));
   client.print(F("Content-Type: multipart/mixed; "));
   client.println(F("boundary=XXXXboundarytext\r\n"));
  
   Email_Section(MailFilename);
   Email_Section(bmpfile);
  
     client.println(F("--XXXXboundarytext--"));
     //client.println(F("Reported over temperature at client site "));
     client.print(F("\r\n.\r\n"));
     if(!eRcv()) return 0;
     client.stop();
     Serial.println(F("disconnected"));
     
     return 1;
}

void Email_Section (char ThisFilename[13])
{
   thefile.open(ThisFilename, FILE_READ);
   Serial.println(ThisFilename);
   if (thefile.available()!=0) {
   Serial.println(F(" available "));
   client.println(F("--XXXXboundarytext"));
   client.println(F("Content-Type: application/octet-stream;"));
   client.print(F("Content-Disposition: attachment;"));
   client.print(F("filename="));
   client.print(ThisFilename);
   client.println(F(";"));
   client.print(F("Content-Transfer-Encoding: base64;\r\n\r\n"));
   while (thefile.available()!=0) {
     char encoded[1];
     char Bringit[3];
     thefile.read(Bringit,3);
     base64_encode(encoded, Bringit, 3);
     client.print(encoded);
     }
   client.print(F("\r\n")); 
   thefile.close();
   client.println(F("--XXXXboundarytext"));
   }
}
byte eRcv()
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;
  while(!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F(" Timeout "));
      return 0;
    }
  }
  respCode = client.peek();
  while(client.available())
  {  
    thisByte = client.read();    
  }
  if(respCode >= '4')
  {
    return 0;  
  }
  return 1;
}
void tempSD (){
    thefile.open(Todayfilename, FILE_WRITE);
    thefile.print(ReturnDateTime(Todays,true));
    thefile.print (F(","));
    thefile.print (tempreading);
    thefile.print (F(","));
    thefile.println (humidreading);
    thefile.close();
}

void BMPcreate(){
    
    Serial.print (F("BMP  "));
    Serial.println (MailFilename);
    if (thefile.open(MailFilename)) {
      char val[16];
      int  i = 0;
      
      while (thefile.available() )  {
        thefile.fgets(val, 16);
        picturepoints[i]=(10*(val[9] - '0') + val[10] - '0');
        picturepointsH[i] = (10*(val[12] - '0') + val[13] - '0');
        i++;
        if ( i > 144 ) continue;
        }
      }
      
      thefile.close();

      if (SD.exists("report.bmp")) 
      {
        SD.remove("report.bmp");
      }


      thefile.open("report.bmp", FILE_WRITE);
      Serial.println ("report.bmp");
      for (int i=0; i<14; i++) {       
      thefile.write(pgm_read_byte_near(bmpFileHeader + i));
      }
      for (int i=0; i<40; i++) {       
      thefile.write(pgm_read_byte_near(bmpInfoHeader + i));
      }
       // Serial.println (F("BMP Headers end"));
      for (int y=0; y<100; y++) {
        for (int x=0; x<720; x++) {
          int colorValy = 255; 
          int colorValg = 255; 
          int colorValr= 255;
          int ZPoints=x / 10 ;
          if (picturepoints[ZPoints]> y)
          {
            if ( picturepoints[ZPoints]<=tempthresh ){
              colorValy = 0;
              colorValg = 230;
              colorValr= 0; 
              } 
              else
              {
              colorValy = 0;
              colorValg = 0;
              colorValr= 230;         
              }
          }
              if (picturepointsH[ZPoints] == y)
              {
              colorValy = 0;
              colorValg = 0;
              colorValr= 0; 
              }
          int  u = x & B11110;   // mod 30
          if ( (x < 2 ) || (x > 718 ) || (y < 2 ) || (y > 98 ) || (u  == 0)  )
          {
           colorValy = 1;
           colorValg = 1;
           colorValr= 1;
          }
          thefile.write(colorValy);
          thefile.write(colorValg);
          thefile.write(colorValr);
          }
        }
        thefile.close();
Serial.println (F("BMP end"));
}
// just so we have time t
time_t Getthetime(){
    sendNTPpacket("pool.ntp.org");
    delay(2000);
    if (Udp.parsePacket()) {
    Udp.read(packetBuffer, 48);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    unsigned long epoch = secsSince1900 - 2208988800UL + 34200UL;
    return epoch;
    }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;  
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123); 
  Udp.write(packetBuffer, 48);
  Udp.endPacket();
}
