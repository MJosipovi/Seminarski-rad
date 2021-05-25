//https://techtutorialsx.com/2018/03/17/esp32-arduino-getting-weather-data-from-api/

#include <WiFi.h> //wifi veza
#include <ESP32Ping.h> //brze povezivanje na wifi
#include <HTTPClient.h> //prognoza
#include <Arduino_JSON.h> //za izvlacenje podataka iz odgovora openweathr-a
#include <time.h> //dohvacanje trenutnog vremena za NUCELO
#include <SPI.h> //Master (ESP32) prema NUCLEO-u

//WiFi podaci
const char* ssid = "********";
const char* pass = "********";

//openweathermap.org API call
const String poveznica = "https://api.openweathermap.org/data/2.5/onecall?lat=45.8144&lon=15.978&exclude=current,minutely,hourly&appid=*******************&units=metric";

//podaci za Network Time Protocol (NTP)
const char* ntpServer = "2.europe.pool.ntp.org"; //EU server
const long gmtOffset_sec = 3600; //GMT+1 zona
const int daylightOffset_sec = 3600; //ljetno/zimsko racunanje vremena

//SPI
static const int SPI_brzina = 1125000; // 1.125 MHz, NUCLEO ne moze 1MHz
SPIClass * vspi = NULL; //SPI
int SPI_array[7]; //array u koji se spremaju svi podaci koji ce se poslati preko SPI-a

//Gumb za refresh podataka
const int buttonPin = 14;
int buttonState = 0;


////// Kod vezan za spajanje na wifi \\\\\\

bool wifiStatus=false;
void wifiClear(){
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    wifiStatus = WiFi.status() == WL_CONNECTED;
    delay(100);
}

void wifiDisconnect(){
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    wifiStatus = false;
}

bool ping_res=false;
void wifiConnect(){
    WiFi.setSleep(false);
    IPAddress ip(1, 1, 1, 1); // The remote ip to ping

    wifiStatus=false;
    int loop_counter=0;
    
    do{
        Serial.println("Wifi connecting...");
        WiFi.begin(ssid, pass);
        delay(100);
        
        ping_res = Ping.ping(ip);
        Serial.println(ping_res);
        
        loop_counter++;
          
        if(loop_counter>=5){
            wifiDisconnect();
            wifiClear();
        }

        if(loop_counter>=10){
            ESP.restart();
        }
    }while(ping_res == false);
   Serial.println("Wifi connection established");
}
    
void check_connection(){
  IPAddress ip(1, 1, 1, 1); // The remote ip to ping
  ping_res = Ping.ping(ip);
}

////// kraj koda vezanog uz spajanje na wifi, SPI slanje podataka u nastavku \\\\\\

//funkcija za slanje podataka putem SPI-a
//svi podaci koji se trebaju poslati su spremljeni u SPI_array
void SPI_send(void){
  char data_out[16]; //8 bitova po jednom prijenosu, u ovu varijablu spremamo razdvojene podatke
  int place=0; //varijabla za pracenje mjesta upisa podataka u array

  //prva tri podatka (temperature) su velicine 1 bajta
  for(int i=0; i<3; i++){ 
    data_out[place] = SPI_array[i]; //direktno prebacivanje
    place++; //pracenje pozicije
  }

  //druga tri podatka su velicine 4 bajta
  for(int i=3; i<6; i++){ //tri podatka
    for(int t=0; t<4; t++){ //rastavljanje 4 bajta u dijelove po 1 bajt
      data_out[place]=SPI_array[i]>>(8*t) & 0xFF;
      place++;
    }
  }

  vspi->beginTransaction(SPISettings(SPI_brzina, MSBFIRST, SPI_MODE0)); //MS bit ide prvi, MOD 0
  digitalWrite(5, LOW); //spustanje "Slave select" linije kako bi se NUCLEO znao pripremiti
  delay(15);

  //jedan po jedan podatak (velicine 1 bajt) se vadi iz matrice te šalje prema NUCLEO-u
  for(int i=0; i<15; i++){ 
    vspi->transfer(data_out[i]);
    delay(15);
  }
  
  delay(10);
  digitalWrite(5, HIGH); //dizanje "Slave select" linije kako bi NUCLEO znao da je prijenos zavrsio 
  vspi->endTransaction();   
}
////// kraj koda vezanog uz slanje podataka putem SPI protokola, dohvacanje vremena(EPOCH) u nastavku \\\\\\

//Dohvacanje trenutacnog EPOCH vremena
void GETLocalTime(){
  time_t EPOCH;
  struct tm timeinfo;
  
  if (!getLocalTime(&timeinfo)) { 
    Serial.println("Greska u dohvacanju vremena");
  }
  time(&EPOCH); //sinkonizacija s ESP32 RTC-om
  EPOCH = EPOCH + gmtOffset_sec + daylightOffset_sec; //dodavanje GMT+1 i ljetno/zimsko racunanje vremena

  SPI_array[5] = EPOCH; //spremanje u matricu za slanje
}

////// kraj koda vezanog uz dohvacanje alarma s online baze, dohvacanje prognoze u nastavku \\\\\\

int prolaz=0;
void GETAlarm(void){
  if ((WiFi.status() == WL_CONNECTED)) { //Provjera statusa internetske veze
    HTTPClient http;
 
    http.begin("https://esp32testserver.000webhostapp.com/seminarski/index.php"); //link na bazu
    int httpCode = http.GET(); //dohvacanje podataka
 
    if (httpCode > 0) { //ako su podaci dohvaceni
        String payload = http.getString(); //spremanje dohvacenih podataka
        Serial.println(httpCode);
        Serial.println(payload);

        int alarm_EPOCH = payload.toInt(); //konverzija String --> int (EPOCH je dohvacen)
        alarm_EPOCH = (alarm_EPOCH + 3600); //GMT+1, 3600 sekundi = 1 sat

        SPI_array[4] = alarm_EPOCH; //spremanje u matricu za slanje
      }
 
    else {
      Serial.println("Error on HTTP request");
      prolaz = prolaz + 1;

      if(prolaz<3){
        GETAlarm(); 
      }else{
        prolaz=0;
        wifiDisconnect();
        wifiClear();
        wifiConnect();
        GETAlarm();
      }
    }
 
    http.end(); //Deinicijalizacija
  }else{
      wifiDisconnect();
      wifiClear();
      wifiConnect();
      GETAlarm();
  }
}

////// kraj koda vezanog uz dohvacanje vremena(EPOCH), dohvacanje prognoze u nastavku \\\\\\

//univerzalna funkcija za dohvacanje podataka s interneta
String httpGETRequest(const char* serverName){
    HTTPClient http;
    
    // Your IP address with path or Domain name with URL path 
    http.begin(serverName);
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    
    String payload = "{}"; 
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      
      Serial.print("Reconnecting to WiFI... ");
      wifiConnect();
    }
    // Free resources
    http.end();
    Serial.println(payload);
    return payload;
}

//funkcija za dohvacanje podataka o prognozi
void GETWeatherinfo(){
    check_connection();
    if(ping_res == true) { //Check the current connection status
      String jsonBuffer; //varijabla za RAW JSON odogovor na API call prema openweather-u
      jsonBuffer = httpGETRequest(poveznica.c_str()); //dohvacanje podataka
      
      JSONVar myObject = JSON.parse(jsonBuffer); //rastavljanje JSON-a

      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      //primjer za parsiranje iz {}
      //Serial.println(myObject["main"]["temp"]);
      
      //primjer za parsiranje iz arraya []
      //JSONVar main_weather=myObject["weather"][0]["main"];
      
      //TEMPERATURE
      long int temp_morn=myObject["daily"][0]["temp"]["morn"]; //float npr. "3.52" (izrazeno u °C, bez navodnika)
      SPI_array[0] = temp_morn;
      printf("%d\n\r", temp_morn);

      long int temp_day=myObject["daily"][0]["temp"]["day"]; //int npr. "3 (izrazeno u °C, bez navodnika)
      SPI_array[1] = temp_day;
      printf("%d\n\r", temp_day);
      
      long int temp_eve=myObject["daily"][0]["temp"]["eve"]; //float npr. "3.52" (izrazeno u °C, bez navodnika)
      SPI_array[2] = temp_eve;
      printf("%d\n\r", temp_eve);

      //VREMENSKI UVJETI
      long int weather_id=myObject["daily"][0]["weather"][0]["id"]; //npr. "Clouds" (bez navodnika)
      SPI_array[3] = weather_id;
      printf("%d\n\r", weather_id);
      
      
    }else{
          Serial.println("WiFi Disconnected, reconnecting...");
          wifiConnect();
          GETWeatherinfo();
     }

}

////// kraj koda vezanog uz dohvacanje prognoze, opca inicijalizacija u nastavku \\\\\\

void update_ALL(void){
    GETWeatherinfo(); //funkcija za dohvacanje prognoze i slanje iste putem SPI-a
    GETAlarm(); //funkcija za dohvacanje posatavljenog alarma i slanje istog putem SPI-a
    GETLocalTime(); //funckija za dohvacanje trenutnog vremena i slanje istog putem SPI-a
    SPI_send();
}

void setup(){
    //pocetak serijske komunikacije
    Serial.begin(115200);

    //wifi start
    wifiConnect();
    
    //inicijalizacija postavki za dohvacanje vremena (sati)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    //SPI inicijalizacija
    vspi = new SPIClass(VSPI);
    vspi->begin(); //SCLK = 18, MISO = 19, MOSI = 23, SS = 5 (standaradni pinovi)
    pinMode(5, OUTPUT); //VSPI SS

    attachInterrupt(14, update_ALL, FALLING);
}

void loop(){
  GETWeatherinfo(); //funkcija za dohvacanje prognoze
  GETAlarm(); //funkcija za dohvacanje posatavljenog alarma
  GETLocalTime(); //funckija za dohvacanje trenutnog vremena
  SPI_send();
  delay(3600000);
  //}
}
