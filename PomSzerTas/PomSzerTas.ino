#include <Wire.h> // standardowa biblioteka Arduino
#include <Ethernet2.h> // biblioteka umozliwiajaca polaczenie sieciowe 
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <LiquidCrystal_I2C.h> // biblioteka umozliwiajaca kozystanie z wyswietlacza LCD
#include "advancedFunctions.h" // biblioteka zawierajaca watchdog-a


String lcdLine[2];
const String clearLcdLine = "                    ";
String LLval = "";
String LOval = "";
String HIval = "";
String HHval = "";
String IdTasma= "";
String SzerNom= "";
int dts=0;

byte mac_addr[] = { 0xB2, 0xE5, 0x96, 0xD2, 0x41, 0x7D };//BF-B1-94-D2-41-8D 

IPAddress server_addr(10,16,48,3);  // IP of the MySQL *server* here
char user[] = "r3d";                // MySQL user login username
char password[] = "r3d";           // MySQL user login password
IPAddress ip(10, 0, 21, 32);

EthernetClient client;
MySQL_Connection conn((Client *)&client);

//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Ustawienie adresu ukladu na 0x27
String inputString = "";          // a string to hold incoming data
boolean stringComplete = false;   // whether the string is complete
String inputString1 = "";         // a string to hold incoming data
boolean stringComplete1 = false;  // whether the string is complete
String inputString2 = "";         // a string to hold incoming data
boolean stringComplete2 = false;  // whether the string is complete
unsigned long previousMillis=0;
unsigned long previousMillis2=0; 
unsigned long currentMillis; 
String wartosc="";
String znacznik="";
String dts1="null";
String dts2="null";
String dts3="null";
String dts4="null";
String dts5="null";
bool wlaczalarm=false;
//const long interval = 5000;   

bool PrevEnkoder;
bool prevTrybZczyt;
bool prev5 = true;
int StepCount=0;
int MaxStep=2;
int alarm = 43;//(43)
int Enkoder=35;
int TrybZczyt=37;
int AlarmOff=41;
int AlarmLamp=45;
int Key=39;
int StripSensorPozA=47;
int StripSensorPozB=49;
  LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

// Begin reboot code
int num_fails;                  // variable for number of failure attempts
#define MAX_FAILED_CONNECTS 5   // maximum number of failed connects to MySQL

void softwareReset() {
  myPrint("......RESTART");
  wdt.disable();
  wdt.enable(500); // aktywujemy watchdog-a na 500ms
  delay(2000);
  myPrint("......2000.....");
  for(;;); //this is a deadlock
}
// End reboot code


  
void setup() {
  wdt.enable(30000); // aktywujemy watchdog-a na 30s
  pinMode(Enkoder, INPUT_PULLUP);
  pinMode(TrybZczyt, INPUT_PULLUP);
  pinMode(AlarmOff, INPUT_PULLUP);
  pinMode(alarm, OUTPUT);
  pinMode(AlarmLamp, OUTPUT);
  pinMode(Key, INPUT_PULLUP);
  pinMode(StripSensorPozA, INPUT_PULLUP);
  pinMode(StripSensorPozB, INPUT_PULLUP);
  // put your setup code here, to run once:
  Serial.begin(9600); // otwarcie portu dal komputera 9600 bps
  //while (!Serial); // wait for serial port to connect (czekamy na komputer)
  inputString.reserve(200);
  inputString1.reserve(200);
  inputString2.reserve(200);
  //wartosc.reserve(200);
  //znacznik.reserve(200);
  Serial1.begin(9600); // otwarcie portu dla kontroler pomiarowy keyence z predkoscia 9600 bps
  Serial2.begin(9600); // otwarcie portu dla czytnik kodow z predkoscia 9600 bps
  //


  
  lcd.init();                      // initialize the lcd 
 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    POMIAR TASMY    ");
  myPrint("setup...");


  Ethernet.begin(mac_addr);
  delay(1000);
  wdt.restart();
  myPrint("Connecting...");

  int conStat = 0;
  num_fails = 0;                 // reset failures
  while (conStat!=1) {
    conStat = conn.connect(server_addr, 3306, user, password);
    switch (conStat) {
      case 1:
        myPrint("Connection success.");
        num_fails = 0;                 // reset failures
        break;
      case -1:
        myPrint("Connection timed out.");
        digitalWrite (alarm, HIGH);
        delay(5000);
        break;
      case -2:
        myPrint("Error invalid server.");
        digitalWrite (alarm, HIGH);
        delay(5000);
        break;
      case -3:
        myPrint("Error truncated.");
        digitalWrite (alarm, HIGH);
        delay(5000);
        break;
      case -4:
        myPrint("Error invalid response.");
        digitalWrite (alarm, HIGH);
        delay(5000);
        break;
      default:
        myPrint("Error: connection.");
        digitalWrite (alarm, HIGH);
        delay(5000);
        break;
    }
    if (!conn.connected()){
      num_fails++;
      if (num_fails == MAX_FAILED_CONNECTS) {
        myPrint("I'm Rebooting...156");
        delay(2000);
        // Here we tell the Arduino to reboot by redirecting the instruction
        // pointer to the "top" or position 0. This is a soft reset and may
        // not solve all hardware-related lockups.
        softwareReset();
      }  
    }
  }
  
  
  // print your local IP address:
  printIPAddress();
  
//*pobranie szerokosci nominalnej ostatnio zarejestrowanej przez baze*//
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute("SELECT szer_nom FROM liniarur.tasma order by idtasma DESC LIMIT 1;");
  myPrint("SELECT...last tasma;");
  delay (1000);
  column_names *columns = cur_mem->get_columns();
  row_values *row = NULL;
  row = cur_mem->get_next_row();
  SzerNom =row->values[0];
  do {
    row = cur_mem->get_next_row();
  } while (row != NULL);

//*Pobranie z bazy ID programu na podstawie szerokosci nominalnej*//
  String zap1="SELECT idProg FROM liniarur.program Where szerokosc=";
  zap1+=SzerNom;
  zap1+=" order by idProg DESC LIMIT 1;";
  char buf0[200];
  for (int i=0;i<zap1.length();i++){
    Serial.print(zap1[i]);
  }
  Serial.println("");
  zap1.toCharArray(buf0,200);
  cur_mem->execute(buf0);
  delay (1000);
  cur_mem->get_columns();
  row = cur_mem->get_next_row();
  IdTasma =row->values[0];
  do {
    row = cur_mem->get_next_row();
  } while (row != NULL);
  delete cur_mem;
  PrevEnkoder = digitalRead(Enkoder);
  prevTrybZczyt=digitalRead(TrybZczyt);
  //
  wdt.enable(5000); //ustawiamy watchdog-a na 5s
}



void loop() {
  //static byte counter = 0;
  // sprawdzamy stan połączenia 
  if (conn.connected()) {
    num_fails = 0;                 // reset failures    
  } else {
    num_fails++;
    if (num_fails == MAX_FAILED_CONNECTS) {
      myPrint("I'm Rebooting...219");
      delay(2000);
      // Here we tell the Arduino to reboot by redirecting the instruction
      // pointer to the "top" or position 0. This is a soft reset and may
      // not solve all hardware-related lockups.
      softwareReset();
    }
  }

  
  if (wlaczalarm==false && digitalRead(AlarmOff)==LOW){
    digitalWrite(alarm, LOW);  
  }
  if (wlaczalarm==true && digitalRead(AlarmOff)==HIGH){
    wlaczalarm=false;
    delay(100);
    digitalWrite(alarm, HIGH);  
  }
 /* if (digitalRead(StripSensorPozA)==HIGH && digitalRead(StripSensorPozB)==HIGH && digitalRead(TrybZczyt)==HIGH) {
    digitalWrite(alarm, HIGH); 
    myPrint("Blad cz. ind");
    //komunikat
  }*/
///////Sprawdzenie sygnału Enkodera////////
  if (PrevEnkoder != digitalRead(Enkoder)){
    PrevEnkoder= digitalRead(Enkoder);
    if (millis()-previousMillis>100){
      previousMillis=millis();
      if (digitalRead(TrybZczyt)==LOW) 
      {
        //komunikat
        myPrint("Zmien tryb");
        wlaczalarm=true;
        //digitalWrite(alarm , HIGH);
      }
      else{
        StepCount++;
      }
    }
  }
  //jezeli zliczono odpowiednia ilosc zmian sygnalu enkodera wyslij do keyence zadanie pomiaru//
  if (StepCount>=MaxStep){
      StepCount=0;
      Serial1.write("M1,1\r");
  }
  //sprawdzanie trybu, jezeli zmiana z trybu wymiany na pomiar, przeslanie kodow tasmy//
  if (digitalRead(TrybZczyt)==1 && prevTrybZczyt==0){
    if (millis()-previousMillis2>100) {
      String Ktasma = "INSERT INTO liniarur.tasma (szer_nom,kod1,kod2,kod3,kod4,kod5) VALUES( ";
      Ktasma+=SzerNom;
      Ktasma+=", ";
      if (dts1!="null") {Ktasma+="'";}
      Ktasma+=dts1;
      if (dts1!="null") {Ktasma+="'";}
      Ktasma+=", ";
      if (dts2!="null") {Ktasma+="'";}
      Ktasma+=dts2;
      if (dts2!="null") {Ktasma+="'";}
      Ktasma+=", ";
      if (dts3!="null") {Ktasma+="'";}
      Ktasma+=dts3;
      if (dts3!="null") {Ktasma+="'";}
      Ktasma+=", ";
      if (dts4!="null") {Ktasma+="'";}
      Ktasma+=dts4;
      if (dts4!="null") {Ktasma+="'";}
      Ktasma+=", ";
      if (dts5!="null") {Ktasma+="'";}
      Ktasma+=dts5;
      if (dts5!="null") {Ktasma+="'";}
      Ktasma+=");";
      for (int i=0;i<Ktasma.length();i++){
            Serial.write(Ktasma[i]); 
      }
      Serial.println("");
      if (dts1=="null" and dts2=="null"){
        //digitalWrite(alarm , HIGH);
        wlaczalarm=true;
        myPrint("Nic nie zczytano");
      }
      char bufKtasma[200];
      Ktasma.toCharArray(bufKtasma,200);
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      cur_mem->execute(bufKtasma);
      delay (10);
      delete cur_mem;
      dts1="null";
      dts2="null";
      dts3="null";
      dts4="null";
      dts5="null";
    }
    prevTrybZczyt=1;
    previousMillis2=millis();
  }
  if (digitalRead(TrybZczyt)==0 && prevTrybZczyt==1){
    prevTrybZczyt=0;
  }
  //
  wdt.restart();   // reset  watchdog-a
}

void myPrint(String txt) {
  if (Serial) {
    //jezeli jest terminal to wyślij do niego komu
    Serial.println(txt);  
  }
  lcdLine[0] = lcdLine[1];
  lcdLine[1] = txt;
  lcd.setCursor(0,2);
  lcd.print(clearLcdLine);  
  lcd.setCursor(0,2);
  lcd.print(lcdLine[0]);
  lcd.setCursor(0,3);
  lcd.print(clearLcdLine);  
  lcd.setCursor(0,3);
  lcd.print(lcdLine[1]);
}

//event odebranie i obsluga komunikatow z komputera//
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    inputString = Serial.readStringUntil('\r');

     // stringComplete = true;
      for (int i=0;i<inputString.length();i++){
          Serial.write(inputString[i]); 
     }
     // clear the string:
     inputString = "";
  }
}

//event odebranie i obsluga komunikatow z keyence//
void serialEvent1() {
  while (Serial1.available()) {
    // get the new byte:
    inputString1 = Serial1.readStringUntil('\r');
    Serial.write("keyence"); 

        //lcd.clear();
        //lcd.setCursor(0,0);
        if (inputString1.length()==15){
          wartosc = getValue(inputString1, ',', 1);
          znacznik = getValue(inputString1, ',', 2);

          if ((znacznik=="GO")or(znacznik=="HI")or(znacznik=="LO")or(znacznik=="LL")or(znacznik=="HH"))
          {
            Serial.write("\n");
            for (int i=0;i<wartosc.length();i++){
              Serial.write(wartosc[i]); 
            }
            Serial.write("\n");
            for (int i=0;i<znacznik.length();i++){
              Serial.write(znacznik[i]); 
            }
            Serial.write("\n");

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            char buf1[200];
            for (int i=0;i<(200);i++){
              buf1[i]=0; 
            }
            String tosend="INSERT INTO liniarur.pom_szer (mesVal,result,idtasma) VALUES (";
            tosend+=wartosc;
            tosend+=", ";
            tosend+='"';

            tosend+=znacznik;
            tosend+='"';
            tosend+=", ";
            tosend+=IdTasma;
            tosend+=");";

            for (int i=0;i<tosend.length();i++){
              Serial.write(tosend[i]); 
            }
            tosend.toCharArray(buf1,200);
            MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
            Serial.print("se1");
            cur_mem->execute(buf1);
            Serial.print("se2");
            delay (10);
            delete cur_mem;
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          }
          else
          {
            myPrint(inputString1);
            //lcd.print("err1");
            //lcd.setCursor(0,1);
            //lcd.print(inputString1);
          }
          
         }
         else
         {
          myPrint(inputString1);
          //lcd.print("err2");
          //lcd.setCursor(0,1);
          //lcd.print(inputString1);
         }
         inputString1 = "";
         stringComplete1 = false;
  }
}

//event odebranie i obsluga danych z czytnika kodow//
void serialEvent2() {
  myPrint("Czytnik kodow...");
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();
    // add it to the inputString:
    inputString2 += inChar;
    // if the incoming character is a newline, set a flag

  }
  if (inputString2[inputString2.length()-1]==('\r')) {
    
    ///////////////////////stringComplete2 = true;
    //if (stringComplete2) {
     //lcd.clear();
     //lcd.setCursor(0,0);
     for (int i=0;i<inputString2.length();i++){
          Serial.write(inputString2[i]); 
     }
//     Serial.write("\n");
     ///////////////////////////////////////////////////////
     if (inputString2[0]=='k'&&inputString2[1]=='o'&&inputString2[2]=='d'){
        char buf1[200];
        for (int i=0;i<(200);i++){
          buf1[i]=0; 
        }
        String tosend="SELECT szerokosc, HH, HI, LO, LL, idProg FROM liniarur.program Where kod = '";
        String zczyt="";
        for (int i=0;i<(inputString2.length()-1);i++){
          zczyt+=inputString2[i]; 
        }
        tosend+=zczyt;
        tosend+="';";
        tosend.toCharArray(buf1,200);
//        Serial.write(buf1);
//        Serial.write("\n");
        MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
        cur_mem->execute(buf1);
        delay (1000);
        column_names *columns = cur_mem->get_columns();
        
//        Serial.write("trap\n");
        row_values *row = NULL;
        row = cur_mem->get_next_row();
       //xxx = row->values[0];
        SzerNom=(row->values[0]);
        HHval =ConvToKey(row->values[1]);
        HIval =ConvToKey(row->values[2]);
        LOval =ConvToKey(row->values[3]);
        LLval =ConvToKey(row->values[4]);
        IdTasma=(row->values[5]);
        do {
            row = cur_mem->get_next_row();
          } while (row != NULL);
        delete cur_mem;
                    
       // if (LLval!=""&&LOval!=""&&HIval!=""&&HHval!=""){
            String pomVal="";
            LLval ="SD,LL,1," + LLval + "\r";
            LOval ="SD,LO,1," + LOval + "\r";
            HIval ="SD,HI,1," + HIval + "\r";
            HHval ="SD,HH,1," + HHval + "\r";
//            Serial.println(LLval);
//            Serial.println(LOval);
//            Serial.println(HIval);
//            Serial.println(HHval);
          for(int p=0;p<4;p++){
            for (int q=0;q<LLval.length();q++){
              Serial1.write(LLval[q]);
            }
            delay (100);
            for (int r=0;r<LOval.length();r++){
              Serial1.write(LOval[r]);
            }
            delay (100);
            for (int s=0;s<HIval.length();s++){
              Serial1.write(HIval[s]);
            }
            delay (100);
            for (int t=0;t<HHval.length();t++){
              Serial1.write(HHval[t]);
            }
            delay (100);
          }
     }
     int liczdts=0;
     if (inputString2[liczdts+2]=='d'&&inputString2[liczdts+3]=='t'&&inputString2[liczdts+4]=='s'){
      liczdts=2;
     }
     if (inputString2[liczdts]=='d'&&inputString2[liczdts+1]=='t'&&inputString2[liczdts+2]=='s'){
      dts=inputString2[liczdts+3];
      dts-=48;
      Serial.print("%");
      Serial.println(dts);
     }
     else{
      if (dts==1){
        dts1=inputString2;
      }
      if (dts==2){
        dts2=inputString2;
      }
      if (dts==3){
        dts3=inputString2;
      }
      if (dts==4){
        dts4=inputString2;
      }
      if (dts==5){
        dts5=inputString2;
      }
      dts=0;
     }

     // clear the string:
     inputString2 = "";
     stringComplete2 = false;


  }
  //Serial.print("se2end");
}

//rozdzielanie wględem separatora//
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String ConvToKey (String data)
{
  int przec=9;
  String Lout="";
  if (data.length()<1) {
    return "";
  }
  else if (data[1]!='-'){
    for (int i=0;i<data.length();i++){
      if (data[i]=='.'||data[i]==',') {
        przec=i;
      }
    }
    if (przec<3) {
      Lout="0"+Lout;
    }
    if (przec<2) {
      Lout="0"+Lout;
    }
    Lout="+"+Lout;
    for (int i=0;i<data.length()&&Lout.length()<8;i++){
      if (data[i]=='.'||data[i]==','){

      }
      else {
        Lout+=data[i];
      }
    }
    while (Lout.length()<8){
      Lout+="0";
    }
    return Lout;
  }
  return "";  
}

void printIPAddress() // wypisywanie adresu IP
{
  String tempIPAddress; // stworzenie nowego stringa tempIPAddress
  tempIPAddress="My IP: "; // przypisanie wartosci poczatkowej do stringa tempIPAddress
  for (byte thisByte = 0; thisByte < 4; thisByte++) { // petla zczytujaca kolejne byte adresu IP
    // print the value of each byte of the IP address:
    tempIPAddress+=String(Ethernet.localIP()[thisByte], DEC); // dodawanie kolejnych byteów kodu do strina tempIPAddress
    if (thisByte < 3){ // warunek wstawienia separatora "." miedzy nastepnymi byteami adresu
      tempIPAddress+="."; // dodatnie do stringa tempIPAddress separatora
    }
    
  }
  myPrint(tempIPAddress); // wypisanie addresu IP
}
