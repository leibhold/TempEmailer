#include <SdFat.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <dht.h>
#include <time.h>
#include <TimeLib.h>
#include "sdios.h"
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define DHT11_PIN 5
#define w 720
#define  h 100
#define  imgSize 72000;
IPAddress ip(192, 168, 1, 231);
dht DHT;
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
byte packetBuffer[48]; //buffer to hold incoming and outgoing packets
EthernetClient client(80);
EthernetUDP Udp;
time_t LastMonth;
time_t YesterdayStamp;
long Cycledelay = 10000;
long RepeatMessage = 600000;
long lastReadingTime = 0;
int8_t tempreading = 0;
int8_t humidreading = 0;
boolean HaventSendMessage = true;
boolean ItsANewDay = false;
boolean BeenHotBefore = false;
boolean IsHot=false;
boolean BeenAnHour=false;
int8_t Daycheckold;  
char Thefilename[17]="11-11-1111.txt";    // default incase NTP doesnt work
long LastHotTime = millis() * 100;
int8_t tempthresh=27;
char * buf = (char *) malloc (3);
char DateTimeArray[11];
SdFat SD;
byte picturepoints [144];

void setup() {
  Ethernet.begin(mac, ip);
  Serial.begin(115200);
  Daycheckold=day();
  Udp.begin(8888);
  delay(1000);
  Serial.println(F("Started "));
  lastReadingTime=millis() ;
  setSyncProvider(getNtpTime);
  setSyncInterval(600000); 
  Daycheckold=day()-1;


  

}




void loopa() {

}



void loop() {
  Serial.print("Loop");

   // check for a reading no more than once a 10 seconds.
   delay(1000);
   if (millis() - lastReadingTime > Cycledelay) {
       if (timeStatus() == 0)
        {
        setSyncProvider(getNtpTime);
        }
      time_t Todays=now();
      time_t LastMonths = now()- 2592000UL ;
      time_t Yesterdays =now()-  86400UL ;
      Serial.println(" - Doing a reading");
      // do a reading since its 60 odd seconds since we did it
      int chk = DHT.read11(DHT11_PIN);
      tempreading = DHT.temperature ;
      humidreading = DHT.humidity;
      lastReadingTime=millis() ;
      if (tempreading < 0.00)
      {
        // Try another read
          int chk = DHT.read11(DHT11_PIN);
          tempreading = DHT.temperature ;
          humidreading = DHT.humidity;
          if (tempreading < 0.00)
          {
             tempreading = 0 ;
             humidreading = 0;
          }
        }

      tempSD();
      // Check the temp and set flags if hot and hot before
      if (tempreading > tempthresh)
         {
          Serial.println("Over temp");
         IsHot=true;
          // have we seen hot before ?

          if (BeenHotBefore == false ) 
             {
            // no - this is the first hot we have seen - set hotbefore asnd start the timer
            LastHotTime=millis() + RepeatMessage;
            BeenHotBefore=true;
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
                 }
                else
                {
                    BeenAnHour=false;
                }
             }
         }
         else
         {
          IsHot=false;
          BeenAnHour=false;
          BeenHotBefore=false;
         }

          // here we check for a new day
          if (Daycheckold == day())
            {
            // Nothing to see here - move along
            ItsANewDay=false;
            }
            else
            {
            // its a new day so reset all the flags
            HaventSendMessage=true; 
            ItsANewDay=true;
            BeenHotBefore=false;
            Daycheckold=day();
            Serial.println(F(" a new day -") );
            }

         if (HaventSendMessage)
           {
          
             if (ItsANewDay == true)
                {
                 strcpy(Thefilename,Returntime(Yesterdays,false));
                }
                else
                {
                strcpy(Thefilename,Returntime(Todays,false));
                }
                strcat(Thefilename,".txt");
                Serial.print(Thefilename);
                Serial.println(F(" Send email message"));
                SendEmailMessage(Thefilename);
                
                HaventSendMessage=false;
             } // end of Have we sent a message loop

  } // end of the timeing loop
  
}


char * padzeros(int digits){
   byte r;
   r = sprintf(buf, "%02d", digits);
   return buf;
}

char * Returntime( time_t herenow, boolean datetype)
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
     strcat(DateTimeArray, "-");
     buf1 = padzeros(month(herenow));
     strcat(DateTimeArray, buf1);
     strcat(DateTimeArray, "-");
     buf1 = padzeros(year(herenow));
     strcat(DateTimeArray, buf1);

     }
  return (DateTimeArray);
}






byte SendEmailMessage(char MailFilename) {
   if (SD.begin(4)){
   }
  byte thisByte = 0;
  byte respCode;
  
  SdFile thefile;
  thefile.open(MailFilename, FILE_WRITE);

  if(client.connect("leibhold-com-au.mail.protection.outlook.com",25) == 1) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return 0;
  }
 
  if(!eRcv()) return 0;
  client.println(F("helo 101.187.245.21"));
  if(!eRcv()) return 0;
  
  client.println(F("MAIL From: tech.support@leibhold.com.au"));
  if(!eRcv()) return 0;
  
  client.println(F("RCPT To: ed.leibrick@leibhold.com.au"));
  if(!eRcv()) return 0;
  
 client.println(F("DATA"));
   if(!eRcv()) return 0;

   client.println(F("To: ed.leibrick@leibhold.com.au"));
   client.println(F("From: tech.support@leibhold.com.au"));
   client.print(F("Subject: ALERT Temperature Warning  "));
   client.print(F("MIME-Version: 1.0\r\n"));
   client.print(F("Content-Type: multipart/mixed; "));
   client.println(F("boundary=XXXXboundarytext\r\n"));
   client.print(F("--XXXXboundarytext\r\n\r\n"));
  
  if (thefile.available()!=0) {


   client.println(F("Reported over temperature at client site - log file included"));
   client.println(F("--XXXXboundarytext"));
   client.println(F("Content-Type: application/octet-stream;"));
   client.print(F("Content-Disposition: attachment;"));
   client.println(F("filename=report.txt;"));
   client.print(F("Content-Transfer-Encoding: base64;\r\n\r\n"));
  // encode();
 unsigned char in[3],out[4]; int i,len,blocksout=0;
 while (thefile.available()!=0) {
   len=0; for (i=0;i<3;i++) { in[i]=(unsigned char) thefile.read(); if (thefile.available()!=0) len++; else in[i]=0; }
   if (len) { encodeblock(in,out,len); for(i=0;i<4;i++) client.write(out[i]); blocksout++; }
   if (blocksout>=19||thefile.available()==0) { if (blocksout) client.print("\r\n");  blocksout=0; }
 }


   
   client.println(F("--XXXXboundarytext--"));

    client.println(F("And  here we are "));
  }
  else
  {
    client.println(F("Reported over temperature at client site - "));
    client.print(MailFilename);
    client.println(F(" - log file not found "));
    client.println(F("--XXXXboundarytext"));
  }
  client.print(F("\r\n.\r\n\n"));
  if(!eRcv()) return 0;
  client.stop();
  Serial.println(F("disconnected"));
  thefile.close();
  return 1;
}




byte eRcv()
{
  byte respCode;
  byte thisByte;
  int8_t loopCount = 0;
  while(!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
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
  if (SD.begin(4))
  {
    SdFile file;
    Serial.print(Thefilename);
    time_t nowfile=now();
    file.open(Thefilename, FILE_WRITE);
    file.print (Returntime(nowfile,true));
    file.print (",");
    file.print (tempreading);
    file.print ( ",");
    file.println (humidreading);
    file.close();
    
  }
}


time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= 48) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, 48);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + 34200;
       setSyncInterval(60000); 
    }
  }
  Serial.println(F("No NTP Response :-("));
   setSyncInterval(6); 
  return 0; // return 0 if unable to get the time
}



void BMPcreate(char filename){
Serial.print(F("File open\n"));

SD.begin(4);
  
  SdFile filer;
  if (filer.open(filename)) {
    char val[16];
    int i = 0;
     while (filer.available()) {
      filer.fgets(val, 16);
       picturepoints[i]=(10*(val[9] - '0') + val[10] - '0');

      i++;
      }
     }
    filer.close();

  if (SD.exists("test.bmp")) 
     {
      Serial.print(F("Delteing file\n"));
         SD.remove("test.bmp");
      }

  
  Serial.print(F("File rem\n"));
  int rowSize = 4 * ((3*w + 3)/4);      // how many bytes in the row (used to create padding)
  int fileSize = 54 + h*rowSize;   

  unsigned char bmpPad[rowSize - 3*w];
  for (int i=0; i<sizeof(bmpPad); i++) {         // fill with 0s
    bmpPad[i] = 0;
  }
  // create file headers (also taken from StackOverflow example)
  unsigned char bmpFileHeader[14] = {            // file header (always starts with BM!)
    'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0   };
  unsigned char bmpInfoHeader[40] = {            // info about the file (size, etc)
    40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0   };

  bmpFileHeader[ 2] = (unsigned char)(fileSize      );
  bmpFileHeader[ 3] = (unsigned char)(fileSize >>  8);
  bmpFileHeader[ 4] = (unsigned char)(fileSize >> 16);
  bmpFileHeader[ 5] = (unsigned char)(fileSize >> 24);

  bmpInfoHeader[ 4] = (unsigned char)(       w      );
  bmpInfoHeader[ 5] = (unsigned char)(       w >>  8);
  bmpInfoHeader[ 6] = (unsigned char)(       w >> 16);
  bmpInfoHeader[ 7] = (unsigned char)(       w >> 24);
  bmpInfoHeader[ 8] = (unsigned char)(       h      );
  bmpInfoHeader[ 9] = (unsigned char)(       h >>  8);
  bmpInfoHeader[10] = (unsigned char)(       h >> 16);
  bmpInfoHeader[11] = (unsigned char)(       h >> 24);

  // write the file headers (thanks forum!)
  
  
  SdFile file;
  file.open("test.bmp", FILE_WRITE);
  Serial.print(F("Openning file\n"));
  file.write(bmpFileHeader, sizeof(bmpFileHeader));    // write file header
  file.write(bmpInfoHeader, sizeof(bmpInfoHeader));    // " info header

Serial.print(F("HEaders writtenn"));

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x++) {
      int colorValy = 255; 
      int colorValg = 255; 
      int colorValr= 255;
      int fred=x / 10 ;
     if (picturepoints[fred] == y)
     {
          if ( picturepoints[fred]<=tempthresh ){
          
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
     if ( (x < 2 ) || (x > w-2 ) || (y < 2 ) || (y > h-2 ) )
     {
       colorValy = 1;
       colorValg = 1;
       colorValr= 1;
     }
     int u = x;
          if ( (u  % 30) == 0 )
     {
       colorValy = 1;
       colorValg = 1;
       colorValr= 1;
     }
     
      file.write(colorValy);
      file.write(colorValg);
      file.write(colorValr);
    }
    file.write(bmpPad, (4-(w*3)%4)%4);
  }

  Serial.println(F("File closed"));
  file.close(); 

}



void sendNTPpacket(const char * address) {
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, 48);
  Udp.endPacket();
}

void encodeblock(unsigned char in[3],unsigned char out[4],int len) {
 out[0]=cb64[in[0]>>2]; out[1]=cb64[((in[0]&0x03)<<4)|((in[1]&0xF0)>>4)];
 out[2]=(unsigned char) (len>1 ? cb64[((in[1]&0x0F)<<2)|((in[2]&0xC0)>>6)] : '=');
 out[3]=(unsigned char) (len>2 ? cb64[in[2]&0x3F] : '=');
}

/*
void encode() {
 unsigned char in[3],out[4]; int i,len,blocksout=0;
 while (thefile.available()!=0) {
   len=0; for (i=0;i<3;i++) { in[i]=(unsigned char) thefile.read(); if (thefile.available()!=0) len++; else in[i]=0; }
   if (len) { encodeblock(in,out,len); for(i=0;i<4;i++) client.write(out[i]); blocksout++; }
   if (blocksout>=19||thefile.available()==0) { if (blocksout) client.print("\r\n");  blocksout=0; }
 }
 
}
*/
