#include <Sleep_n0m1.h>

#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>

#include <dht.h>

#define ESP_RX   4
#define ESP_TX   3
#define ESP_RST  5
#define TARGET_HOST "192.168.0.201"
#define TARGET_PORT "8001"
#define SID "fifeton"
#define PWD ""
#define SLEEP_TIME 300000


SoftwareSerial BT1(ESP_RX, ESP_TX); // RX | TX
boolean wifi_initialized = false;

LiquidCrystal_I2C lcd(0x27,16,2);

dht DHT;
SFE_BMP180 pressure;

#define DHT11PIN 2
#define ALTITUDE 65.0


struct meteopi_data{
  double humidity=0;
  float temp=0;
  float dewPoint=0;
  float dewFast=0;
  double pressure;
};

meteopi_data data;
Sleep sleep;

boolean wifi_init(boolean force = false) {
  if (wifi_initialized && !force) return true;

Serial.println("Reseteando...");

  wifi_initialized = false;
  //digitalWrite(ESP_RST,LOW);
  //delay(1000);
  //digitalWrite(ESP_RST,HIGH);
  
  
  BT1.println("AT+RST");
  Serial.println(BT1.readStringUntil('\n'));
  if(!BT1.find("[System Ready, Vendor:www.ai-thinker.com]\r\n")){
    Serial.println("No reseteado");
    return false;
  }

  BT1.setTimeout(15000);
  Serial.println("Conectando...");
  BT1.print("AT+CWJAP=\"");
  BT1.print(SID);
  BT1.print("\",\"");
  BT1.print(PWD);
  BT1.println("\"");
  BT1.find("\r\n");

  if (!BT1.find("OK")) {
    Serial.println("ERROR");
    return false;
  }



  BT1.println("AT+CIFSR");
  BT1.find("\r\n");
  Serial.print("IP: ");
  Serial.println(BT1.readStringUntil('\r'));
  BT1.find("\n");

  Serial.println("Listo!");

  wifi_initialized = true;
  return true;
}


void wifi_sendData(String data) {
  Serial.println("SendData...");
  lcd.clear();
  lcd.print("Enviando datos...");
  
  BT1.println("AT+CIPMODE=1");
  BT1.find("\r\nOK");

  BT1.print("AT+CIPSTART=\"TCP\",\"");
  BT1.print(TARGET_HOST);
  BT1.print("\",");
  BT1.println(TARGET_PORT);
  
  if (BT1.find("\r\nOK\r\nLinked")) {

    BT1.println("AT+CIPSEND");
    if (BT1.find("\r\n>")) {

      BT1.print("GET /meteopi/update?");
      BT1.print(data);
      BT1.println(" HTTP/1.1");
      BT1.print("Host: ");
      BT1.println(TARGET_HOST);
      BT1.println();
      delay(1000);
      BT1.println("+++");
      Serial.println(BT1.readStringUntil('\n'));

      BT1.println("AT+CIPCLOSE");
      if (BT1.find("Unlink")) {
        Serial.println("Connection closed");
      }
      lcd.clear();
      lcd.print("Datos enviados");
      
    }
  }else{
    lcd.clear();
    lcd.print("Error. Not linked.");
    
  }

  delay(2000);
}


void readPressure(){
  char status;
  double T,P,p0,a;
  
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      data.temp=T;
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.print(" deg C, ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          data.pressure=P;
          
          Serial.print(P,2);
          Serial.print(" mb, ");
          Serial.print(P*0.0295333727,2);
          Serial.println(" inHg");

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: a = altitude in m.

          a = pressure.altitude(P,p0);
          Serial.print("computed altitude: ");
          Serial.print(a,0);
          Serial.print(" meters, ");
          Serial.print(a*3.28084,0);
          Serial.println(" feet");

          
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

  
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T);
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

   lcd.init();
   
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("Iniciando...");
   delay(500);
   lcd.clear();
  if (pressure.begin())
    lcd.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    lcd.clear();
    lcd.println("BMP180 init fail");
    while(1); // Pause forever.
  }

  lcd.clear();
  
  lcd.println("Iniciando wifi...");
  pinMode(ESP_RST,OUTPUT);
  BT1.begin(9600);
  while(!wifi_init(true)){
    lcd.clear();
    lcd.println("Error iniciando wifi");
    delay(5000);
    lcd.clear();
    lcd.println("Reiniciando wifi...");
  }

  lcd.println("OK!");
  delay(1000);
}




void loop() {
  // put your main code here, to run repeatedly:

  Serial.print("empezamos");
  int chk = DHT.read11(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.println("OK");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }


  
  data.humidity=(float)DHT.humidity;
  data.temp=(float)DHT.temperature;
  data.dewPoint=dewPoint(data.temp, data.humidity);
  data.dewFast=dewPoint(data.temp, data.humidity);

  readPressure();
  
  Serial.print("Temp: ");
  Serial.println((float)data.temp);
  Serial.print("Hum: ");
  Serial.println((float)data.humidity);
  Serial.print("Presión: ");
  Serial.println(data.pressure);

   
   String data_str;
   data_str.concat("temp=");
   data_str.concat((float)data.temp);
   data_str.concat("&hum=");
   data_str.concat((float)data.humidity);
   data_str.concat("&pres=");
   data_str.concat(data.pressure);

   wifi_sendData(data_str);
   //free(&data_str);
   data_str.remove(0);

lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print(data.temp);
   lcd.print("C ");
   lcd.print((int)data.humidity);
   lcd.print("%");
   lcd.setCursor(0,1);

   lcd.print((int)data.pressure);
   lcd.print("mb");



   //delay(10*1000);
  //delay(5*60*1000);
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME); //sleep for: sleepTime

}
