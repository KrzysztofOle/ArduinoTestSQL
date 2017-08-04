#include <Wire.h>                // biblioteka umozliwiajaca polaczenie z urzedzeniami I2C / TWI
#include <Ethernet2.h>          // biblioteka umozliwiajaca polaczenie sieciowe 
#include <MySQL_Connection.h>    // 
#include <MySQL_Cursor.h>        // 
#include <LiquidCrystal_I2C.h> // biblioteka umozliwiajaca kozystanie z wyswietlacza LCD
//#include "advancedFunctions.h"   // biblioteka zawierajaca watchdog-a


String lcdLine[2];                                  // bufor linii wyswietlacza LCD
const String clearLcdLine = "                    "; // "czysta linia"
String LLval = "";
String LOval = "";
String HIval = "";
String HHval = "";
String IdTasma = "";
String SzerNom = "";
int dts = 0;

//byte mac_addr[] = { 0xBF, 0xB1, 0x94, 0xD2, 0x41, 0x8D };  //BF-B1-94-D2-41-8D
//byte mac_addr[] = { 0xAC, 0x0A, 0x0D, 0x2E, 0x5F, 0xE2 };  //
//byte mac_addr[] = { 0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A };  //
//byte mac_addr[] = { 0xE6, 0x5F, 0x28, 0xD9, 0x59, 0x0D };  // losowy mac adres
//byte mac_addr[] = { 0x67, 0xF0, 0xEB, 0x16, 0x0F, 0x70 };  //
byte mac_addr[] = { 0xC0, 0x4A, 0x00, 0xE6, 0x04, 0x25 };  //

IPAddress server_addr(10, 16, 48, 3); // IP bazy danych MySQL
char user[] = "r3d";                   // login do bazy danych MySQL
char password[] = "r3d";               // haslo do bazy danych MySQL
IPAddress ip(10, 0, 21, 32);

EthernetClient client;
MySQL_Connection conn((Client *)&client);

//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // ustawienie adresu ukladu na 0x27
String inputString = "";          // string do przechowywania danych komputera
boolean stringComplete = false;   // flaga potwierdzajaca poprawność przekazania stringa
String inputString1 = "";         // string do przechowywania danych keyence
boolean stringComplete1 = false;  // flaga potwierdzajaca poprawność przekazania stringa
String inputString2 = "";         // string do przechowywania danych czytnika kodow
boolean stringComplete2 = false;  // flaga potwierdzajaca poprawność przekazania stringa
unsigned long previousMillis = 0; // zmienna przechowujaca poprzedni czas wykorzystywany przy pomiarach szerokosci [ms]
unsigned long previousMillis2 = 0;// zmienna przechowujaca poprzedni czas wykorzystywany przy pobieraniu kodu szpuli [ms]
unsigned long currentMillis;      // zmienna przechowujaca obecny czas [ms]
String wartosc = "";              // zmienna przechowujaca zmierzona wartosc szerokosci
String znacznik = "";             // zmienna przechowujaca znacznik przypisany zmierzonej wartosci szerokosci
String dts1 = "null";
String dts2 = "null";
String dts3 = "null";
String dts4 = "null";
String dts5 = "null";
bool wlaczalarm = false;
//const long interval = 5000;
bool PrevEnkoder;
bool prevTrybZczyt;
bool prev5 = true;
int StepCount = 0;
int MaxStep = 2;
int alarm = 43;//(43)
int Enkoder = 35;
int TrybZczyt = 37;
int AlarmOff = 41;
int AlarmLamp = 45;
int Key = 39;
int StripSensorPozA = 47;
int StripSensorPozB = 49;
LiquidCrystal_I2C lcd(0x27, 20, 4); // ustawienie adresu ukladu na 0x27 dla 20 charow i 4 linii

//-----------------Kod rebootu-----------------------------------------------------------------------------------------------
int num_fails;                  // liczba nieudanych prob polaczenia sie z baza MySQL
#define MAX_FAILED_CONNECTS 5   // maksymalna liczba nieudanych prob polaczenia sie z baza MySQL

void softwareReset()            //definicja funkcji softReset
{
  myPrint("......RESTART");     // wyswietlenie komunikatu za pomoca funkcji myPrint
  //wdt.disable();                // dezaktywacaja watchdog-a
  //wdt.enable(500);              // aktywacja watchdog-a na 500ms
  delay(2000);                  // wywolanie opoznienia na 2s
  myPrint("......2000.....");   // wyswietlenie komunikatu za pomoca funkcji myPrint
  for (;;);                     //deadlock
}
//---------------------------------------------------------------------------------------------------------------------------



void setup()
{
  //wdt.enable(30000);                              // aktywujemy watchdog-a na 30s
  pinMode(Enkoder, INPUT_PULLUP);                 // wejście - sygnal z plc lini ktory zmienia stan co 50mm tasmy
  pinMode(TrybZczyt, INPUT_PULLUP);               // wejście - tryb pracy układu
  pinMode(AlarmOff, INPUT_PULLUP);                // wejście - przycisk na panelu do wyciszania alrmu dziekowego
  pinMode(alarm, OUTPUT);                         // wyjscie -
  pinMode(AlarmLamp, OUTPUT);                     // wyjscie -
  pinMode(Key, INPUT_PULLUP);                     // wejście -
  pinMode(StripSensorPozA, INPUT_PULLUP);         // wejście -
  pinMode(StripSensorPozB, INPUT_PULLUP);         // wejście -

  Serial.begin(9600);                            // otwarcie portu dla komputera 9600 bps
  Serial1.begin(9600);                           // otwarcie portu dla kontroler pomiarowy keyence z predkoscia 9600 bps
  Serial2.begin(9600);                           // otwarcie portu dla czytnik kodow z predkoscia 9600 bps
  //while (!Serial);                              // czekanie na poloczenie z komputerem przez port
  inputString.reserve(200);                       // odwrocenie stringa inputString
  inputString1.reserve(200);                      // odwrocenie stringa inputString1
  inputString2.reserve(200);                      // odwrocenie stringa inputString2
  //wartosc.reserve(200);                         // odwrocenie stringa wartosc
  //znacznik.reserve(200);                        // odwrocenie stringa znacznik


  lcd.init();                       // inicjalizacja ekranu lcd
  lcd.backlight();                  // wlaczenie podswietlenia
  lcd.clear();                      // czyszczenie ekranu i ustawienie kursoraw w lewym gornym rogu
  lcd.setCursor(0, 0);              // ustawienie kursora w lewym gornym rogu (?do usunieciea?)
  lcd.print("    POMIAR TASMY    ");// bezposrednie wyswietlenie komunikatu
  myPrint("");    
  myPrint("setup...");              // wyswietlenie komunikatu za pomoca funkcji myPrint


  Ethernet.begin(mac_addr,ip);    // z adresem na sztywno szybciej sie łączy
  //Ethernet.begin(mac_addr);      // IT zrobiło rezerwacje na adres IP wg adresu MAC
  delay(5000);                    // wywowalanie opozniennia na 1s
  //wdt.restart();                // restart watchdog-a
  macPrint();
  myPrint("Connecting...");       // wyswietlenie komunikatu za pomoca funkcji myPrint

  int conStat = 0;               // zmienna przechowujaca "status polaczenia"
  num_fails = 0;                 // reset liczby nieudanych prob polaczenia sie z baza MySQL
  while (conStat != 1)
  {
    conStat = conn.connect(server_addr, 3306, user, password); // proba polaczenie z baza MySQL
    if (conn.connected())  // sprawdzenie stanu połączenia 
    {
      myPrint("Connection success.........");  
    } else {
      myPrint("Not connected .........");
    }
    printIPAddress(); // wywoalnie funkcji printIPAddress
    switch (conStat)
    {
      case 1:
        myPrint("Connection success.");   // wyswietlenie komunikatu za pomoca funkcji myPrint
        num_fails = 0;                    // reset liczby nieudanych prob polaczenia sie z baza MySQL
        break;
      case -1:
        myPrint("Connection timed out."); // wyswietlenie komunikatu bledu za pomoca funkcji myPrint
        digitalWrite (alarm, HIGH);       // włączenie alarmu
        delay(5000);                      // wywolanie opoznienia na 5s
        break;
      case -2:
        myPrint("Error invalid server.");  // wyswietlenie komunikatu bledu za pomoca funkcji myPrint
        digitalWrite (alarm, HIGH);        // włączenie alarmu
        delay(5000);                       // wywolanie opoznienia na 5s
        break;
      case -3:
        myPrint("Error truncated.");       // wyswietlenie komunikatu bledu za pomoca funkcji myPrint
        digitalWrite (alarm, HIGH);        // włączenie alarmu
        delay(5000);                       // wywolanie opoznienia na 5s
        break;
      case -4:
        myPrint("Error invalid response.");// wyswietlenie komunikatu bledu za pomoca funkcji myPrint
        digitalWrite (alarm, HIGH);        // włączenie alarmu
        delay(5000);                       // wywolanie opoznienia na 5s
        break;
      default:
        String temp;
        temp = "Error: connection.";
        temp = temp + conStat;
        myPrint(temp);     // wyswietlenie komunikatu bledu za pomoca funkcji myPrint
        myPrint("..........."); 
        digitalWrite (alarm, HIGH);        // włączenie alarmu
        delay(5000);                       // wywolanie opoznienia na 5s
        break;
    }
    if (!conn.connected())  // sprawdzenie stanu połączenia
    {
      num_fails++; // zwiekszenie liczb nieudanych prob polaczenia sie
      if (num_fails == MAX_FAILED_CONNECTS) // sprawdzenie czy przekroczono dopuszczalna liczbe prob
      {
        myPrint("I'm Rebooting...156"); // wyswietlenie komunikatu za pomoca funkcji myPrint
        delay(2000);     // wywolanie opoznienia na 2s
        softwareReset(); // wywoalnie funkcji softwareReset (!rozwiazanie sofwareowe nie rozwiaze problemow hardwareowych!)
      }
    }
  }

  printIPAddress(); // wywoalnie funkcji printIPAddress

//-------------------Pobranie szerokosci nominalnej ostatnio zarejestrowanej przez baze--------------------------------------
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute("SELECT szer_nom FROM liniarur.tasma order by idtasma DESC LIMIT 1;");
  myPrint("SELECT...last tasma;");
  delay (1000);
  column_names *columns = cur_mem->get_columns();
  row_values *row = NULL;
  row = cur_mem->get_next_row();
  SzerNom = row->values[0];
  do {
    row = cur_mem->get_next_row();
  } while (row != NULL);

//-------------------Pobranie z bazy ID programu na podstawie szerokosci nominalnej------------------------------------------
  String zap1 = "SELECT idProg FROM liniarur.program Where szerokosc=";
  zap1 += SzerNom;
  zap1 += " order by idProg DESC LIMIT 1;";
  char buf0[200];
  for (int i = 0; i < zap1.length(); i++)
  {
    Serial.print(zap1[i]);
  }
  Serial.println("");
  zap1.toCharArray(buf0, 200);
  cur_mem->execute(buf0);
  delay (1000);
  cur_mem->get_columns();
  row = cur_mem->get_next_row();
  IdTasma = row->values[0];
  do {
    row = cur_mem->get_next_row();
  } while (row != NULL);
  delete cur_mem;
  PrevEnkoder = digitalRead(Enkoder);
  prevTrybZczyt = digitalRead(TrybZczyt);
  //wdt.enable(5000); //ustawiamy watchdog-a na 5s
}



void loop()
{
  //static byte counter = 0;
  if (conn.connected())      // sprawdzenie stanu połączenia
  {
    num_fails = 0;           // reset liczby nieudanych prob polaczenia sie
  }
  else
  {
    num_fails++;             // zwiekszenie liczb nieudanych prob polaczenia sie
    if (num_fails == MAX_FAILED_CONNECTS) // sprawdzenie czy przekroczono dopuszczalna liczbe prob
    {
      myPrint("I'm Rebooting...219"); // wysietlenie komunikatu za pomoca funkcji myPrint
      delay(2000); // wywolanie opoznienia na 2 s
      softwareReset(); //wywoalnie funkcji softwareReset (!rozwiazanie sofwareowe nie rozwiaze problemow hardwareowych!)
    }
  }


  if (wlaczalarm == false && digitalRead(AlarmOff) == LOW)
  {
    digitalWrite(alarm, LOW);
  }
  if (wlaczalarm == true && digitalRead(AlarmOff) == HIGH)
  {
    wlaczalarm = false;
    delay(100); // wywolanie opoznienia na 100ms
    digitalWrite(alarm, HIGH);
  }
  /* if (digitalRead(StripSensorPozA)==HIGH && digitalRead(StripSensorPozB)==HIGH && digitalRead(TrybZczyt)==HIGH)
     {
       digitalWrite(alarm, HIGH);
       myPrint("Blad cz. ind"); // komunikat 
     }*/
  if (PrevEnkoder != digitalRead(Enkoder)) // sprawdzenie sygnalu Enkodera
  {
    PrevEnkoder = digitalRead(Enkoder);
    if (millis() - previousMillis > 100)
    {
      previousMillis = millis();
      if (digitalRead(TrybZczyt) == LOW) // sprawdzenie trybu pracy
      {
        myPrint("Zmien tryb"); // wyswietlenie komunikatu za pomoca funkcji myPrint
        wlaczalarm = true;
        //digitalWrite(alarm , HIGH);
      }
      else {
        StepCount++; // zwiekszenie liczby sygnalow
      }
    }
  }
  if (StepCount >= MaxStep) // sprawdzenie czy osiagnieto oczekiwana liczbe sygnalow
  {
    StepCount = 0; // reset liczby sygnalow
    Serial1.write("M1,1\r"); // wyslanie do Keyence zadania pomiarow
  }
  if (digitalRead(TrybZczyt) == 1 && prevTrybZczyt == 0) // sprawdzenie czy doszlo do zmiany z trybu wymiany na tryb pomiaru 
  {
    if (millis() - previousMillis2 > 100) // sprawdzenie czy uplynelo 100ms
    {
      String Ktasma = "INSERT INTO liniarur.tasma (szer_nom,kod1,kod2,kod3,kod4,kod5) VALUES( ";
      Ktasma += SzerNom;
      Ktasma += ", ";
      if (dts1 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += dts1;
      if (dts1 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += ", ";
      if (dts2 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += dts2;
      if (dts2 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += ", ";
      if (dts3 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += dts3;
      if (dts3 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += ", ";
      if (dts4 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += dts4;
      if (dts4 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += ", ";
      if (dts5 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += dts5;
      if (dts5 != "null")
      {
        Ktasma += "'";
      }
      Ktasma += ");";
      for (int i = 0; i < Ktasma.length(); i++)
      {
        Serial.write(Ktasma[i]); // przeslanie kodu tasmy
      }
      Serial.println("");
      if (dts1 == "null" and dts2 == "null") // sprawdzenie obecnosci dwoch pierwszych czesci kodu
      {
        //digitalWrite(alarm , HIGH);
        wlaczalarm = true;
        myPrint("Nic nie zczytano"); // wyswietlenie komunikatu za pomoca funkcji myPrint
      }
      char bufKtasma[200];
      Ktasma.toCharArray(bufKtasma, 200);
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      cur_mem->execute(bufKtasma);
      delay (10); // wywolanie opoznienia na 10ms
      delete cur_mem; // czyszczenie pamieci podrecznej
      dts1 = "null";  // czyszczenie fragmentu kodu
      dts2 = "null";  // czyszczenie fragmentu kodu
      dts3 = "null";  // czyszczenie fragmentu kodu
      dts4 = "null";  // czyszczenie fragmentu kodu
      dts5 = "null";  // czyszczenie fragmentu kodu
    }
    prevTrybZczyt = 1;
    previousMillis2 = millis();
  }
  if (digitalRead(TrybZczyt) == 0 && prevTrybZczyt == 1) // sprawdzenie czy doszlo do zmiany z trybu pomiaru na tryb zmiany
  {
    prevTrybZczyt = 0;
  }
  //wdt.restart();           // reset  watchdog-a
}

void myPrint(String txt)   // definicja funkcji myPrint
{
  if (Serial)             // jezeli jest terminal to wyślij do niego komunikat
  {  
    Serial.println(txt);  // przesłanie tekstu do portu szeregowego
  }
  lcdLine[0] = lcdLine[1]; // przeniesienie teksu z bufora dolnej linii do bufora gornej linii
  lcdLine[1] = txt;        // wprowadzenie tekstu do bufora dolnej linii
  lcd.setCursor(0, 2);     // ustawienie kursora na poczatku gornej linii
  lcd.print(clearLcdLine); // wprowadzenie na wyswietlacz czystej linii
  lcd.setCursor(0, 2);     // ustawienie kursora na poczatku gornej linii
  lcd.print(lcdLine[0]);   // wprowadzenie na wyswietlacz bufora gorenj linii
  lcd.setCursor(0, 3);     // ustawienie kursora na poczatku dolnej linii
  lcd.print(clearLcdLine); // wprowadzenie na wyswietlacz czystej linii
  lcd.setCursor(0, 3);     // ustawienie kursora na poczatku dolnej linii
  lcd.print(lcdLine[1]);   // wprowadzenie na wyswietlacz bufora dolnej linii
}

void macPrint() 
{
  Serial.print("mac:");
  Serial.print(mac_addr[0],HEX); 
  Serial.print("-");
  Serial.print(mac_addr[1],HEX); 
  Serial.print("-");
  Serial.print(mac_addr[2],HEX); 
  Serial.print("-");
  Serial.print(mac_addr[3],HEX);
  Serial.print("-");
  Serial.print(mac_addr[4],HEX);   
  Serial.print("-");
  Serial.println(mac_addr[5],HEX);   
}


void serialEvent() // odebranie i obsluga komunikatow z komputera
{
  while (Serial.available())
  {
    Serial.println("   coś odebralismy:");
    inputString = Serial.readStringUntil('\r'); // zczytywanie danych do stringa do momentu pojawienia sie znaku konca linii
    Serial.println(inputString);
    inputString = ""; // czyszczenie stringa przechowujacego dane
  }
}

void serialEvent1() // odebranie i obsluga komunikatow z Keyence
{
  while (Serial1.available())
  {
    inputString1 = Serial1.readStringUntil('\r'); // zczytywanie danych do stringa do momentu pojawienia sie znaku konca linii
    Serial.write("keyence"); // przeslanie komunikatu "keyence" do komputera

    //lcd.clear();
    //lcd.setCursor(0,0);
    if (inputString1.length() == 15) // sprawdzenie czy pobrano pelen komunikat
    {
      wartosc = getValue(inputString1, ',', 1); // odczytanie wartosci z komunikatu od keyence za pomoca funkcji getValue
      znacznik = getValue(inputString1, ',', 2);// odczytanie znacznika z komunikatu od keyence za pomoca funkcji getValue

      if ((znacznik == "GO") or (znacznik == "HI") or (znacznik == "LO") or (znacznik == "LL") or (znacznik == "HH")) // sprwdzenie czy wartosci znacznika sa poprawne
      {
        Serial.write("\n");
        for (int i = 0; i < wartosc.length(); i++)
        {
          Serial.write(wartosc[i]);   // przeslanie odczytanych wartosci
        }
        Serial.write("\n");
        for (int i = 0; i < znacznik.length(); i++)
        {
          Serial.write(znacznik[i]);  // przeslanie odczytanych znacznikow
        }
        Serial.write("\n");
        
        char buf1[200];
        for (int i = 0; i < (200); i++)
        {
          buf1[i] = 0;
        }
        String tosend = "INSERT INTO liniarur.pom_szer (mesVal,result,idtasma) VALUES (";
        tosend += wartosc;
        tosend += ", ";
        tosend += '"';
        tosend += znacznik;
        tosend += '"';
        tosend += ", ";
        tosend += IdTasma;
        tosend += ");";

        for (int i = 0; i < tosend.length(); i++)
        {
          Serial.write(tosend[i]);
        }
        tosend.toCharArray(buf1, 200);
        MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
        Serial.print("se1");
        cur_mem->execute(buf1);
        Serial.print("se2");
        delay (10);     // wywolanie opoznienia na 10ms
        delete cur_mem; // czyszczenie pamieci podrecznej
      }
      else
      {
        myPrint(inputString1); // wyswietlenie niepoprawnie odebranego komunikatu keyence za pomoca funkcji myPrint
        //lcd.print("err1");
        //lcd.setCursor(0,1);
        //lcd.print(inputString1);
      }

    }
    else
    {
      myPrint(inputString1);  // wyswietlenie niepoprawnie odebranego komunikatu keyence za pomoca funkcji myPrint
      //lcd.print("err2");
      //lcd.setCursor(0,1);
      //lcd.print(inputString1);
    }
    inputString1 = "";        // czyszczenie stringa
    stringComplete1 = false;  // oznaczenie nie powodzenia odbioru danych
  }
}

void serialEvent2() // odebranie i obsluga danych z czytnika kodow
{
  myPrint("Czytnik kodow..."); // wyswietlenie komunikatu za pomoca funkcji myPrint
  while (Serial2.available())
  {
    char inChar = (char)Serial2.read(); // zczytanie kolejnego byte'u
    inputString2 += inChar; // dodanie pobranego byte'u do stringa
  }
  if (inputString2[inputString2.length() - 1] == ('\r')) // sprawdzenie czy ostatni znak stringa to symbol konca linii
  {
    //stringComplete2 = true;
    //if (stringComplete2) {
    //lcd.clear();
    //lcd.setCursor(0,0);
    for (int i = 0; i < inputString2.length(); i++)
    {
      Serial.write(inputString2[i]); // przesyłanie odebranego kodu
    }
    //Serial.write("\n");
    if (inputString2[0] == 'k' && inputString2[1] == 'o' && inputString2[2] == 'd')
    {
      char buf1[200];
      for (int i = 0; i < (200); i++)
      {
        buf1[i] = 0;
      }
      String tosend = "SELECT szerokosc, HH, HI, LO, LL, idProg FROM liniarur.program Where kod = '";
      String zczyt = "";
      for (int i = 0; i < (inputString2.length() - 1); i++)
      {
        zczyt += inputString2[i];
      }
      tosend += zczyt;
      tosend += "';";
      tosend.toCharArray(buf1, 200);
      //Serial.write(buf1);
      //Serial.write("\n");
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      cur_mem->execute(buf1);
      delay (1000); // wywolanie opoznienia na 1 s
      column_names *columns = cur_mem->get_columns();

      //Serial.write("trap\n");
      row_values *row = NULL;
      row = cur_mem->get_next_row();
      //xxx = row->values[0];
      SzerNom = (row->values[0]);
      HHval = ConvToKey(row->values[1]);
      HIval = ConvToKey(row->values[2]);
      LOval = ConvToKey(row->values[3]);
      LLval = ConvToKey(row->values[4]);
      IdTasma = (row->values[5]);
      do {
        row = cur_mem->get_next_row();
      } while (row != NULL);
      delete cur_mem;

      // if (LLval!=""&&LOval!=""&&HIval!=""&&HHval!=""){
      String pomVal = "";
      LLval = "SD,LL,1," + LLval + "\r";
      LOval = "SD,LO,1," + LOval + "\r";
      HIval = "SD,HI,1," + HIval + "\r";
      HHval = "SD,HH,1," + HHval + "\r";
      //Serial.println(LLval);
      //Serial.println(LOval);
      //Serial.println(HIval);
      //Serial.println(HHval);
      for (int p = 0; p < 4; p++)
      {
        for (int q = 0; q < LLval.length(); q++)
        {
          Serial1.write(LLval[q]);
        }
        delay (100);  // wywolanie opoznienia na 100ms
        for (int r = 0; r < LOval.length(); r++)
        {
          Serial1.write(LOval[r]);
        }
        delay (100);  // wywolanie opoznienia na 100ms
        for (int s = 0; s < HIval.length(); s++)
        {
          Serial1.write(HIval[s]);
        }
        delay (100);  // wywolanie opoznienia na 100ms
        for (int t = 0; t < HHval.length(); t++)
        {
          Serial1.write(HHval[t]);
        }
        delay (100);  // wywolanie opoznienia na 100ms
      }
    }
    int liczdts = 0;
    if (inputString2[liczdts + 2] == 'd' && inputString2[liczdts + 3] == 't' && inputString2[liczdts + 4] == 's')
    {
      liczdts = 2;
    }
    if (inputString2[liczdts] == 'd' && inputString2[liczdts + 1] == 't' && inputString2[liczdts + 2] == 's')
    {
      dts = inputString2[liczdts + 3];
      dts -= 48;
      Serial.print("%");
      Serial.println(dts);
    }
    else
    {
      if (dts == 1)
      {
        dts1 = inputString2;
      }
      if (dts == 2)
      {
        dts2 = inputString2;
      }
      if (dts == 3)
      {
        dts3 = inputString2;
      }
      if (dts == 4)
      {
        dts4 = inputString2;
      }
      if (dts == 5) {
        dts5 = inputString2;
      }
      dts = 0;
    }
    inputString2 = "";        // czyszczenie stringa
    stringComplete2 = false;
  }
  //Serial.print("se2end");
}

String getValue(String data, char separator, int index) // definicja funkcji getValue która rozdziela dane względem separatora
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String ConvToKey (String data) // definicja funkcji ConvToKey
{
  int przec = 9;
  String Lout = "";
  if (data.length() < 1)
  {
    return "";
  }
  else if (data[1] != '-')
  {
    for (int i = 0; i < data.length(); i++)
    {
      if (data[i] == '.' || data[i] == ',')
      {
        przec = i;
      }
    }
    if (przec < 3)
    {
      Lout = "0" + Lout;
    }
    if (przec < 2)
    {
      Lout = "0" + Lout;
    }
    Lout = "+" + Lout;
    for (int i = 0; i < data.length() && Lout.length() < 8; i++)
    {
      if (data[i] == '.' || data[i] == ',')
      {

      }
      else
      {
        Lout += data[i];
      }
    }
    while (Lout.length() < 8)
    {
      Lout += "0";
    }
    return Lout;
  }
  return "";
}

void printIPAddress()                                            // definicja funkcji printIPAddress
{
  String tempIPAddress;                                          // stworzenie nowego stringa tempIPAddress
  tempIPAddress = "My IP: ";                                     // przypisanie wartosci poczatkowej do stringa tempIPAddress
  for (byte thisByte = 0; thisByte < 4; thisByte++)              // petla zczytujaca kolejne byte adresu IP
  {
    tempIPAddress += String(Ethernet.localIP()[thisByte], DEC); // dodawanie kolejnych byteów kodu do stringa tempIPAddress
    if (thisByte < 3)                                            // warunek wstawienia separatora "." miedzy nastepnymi byteami adresu
    {
      tempIPAddress += ".";                                      // dodatnie do stringa tempIPAddress separatora
    }

  }
  myPrint(tempIPAddress);                                        // wyswietlenie IP za pomoca funkcji myPrint
}
