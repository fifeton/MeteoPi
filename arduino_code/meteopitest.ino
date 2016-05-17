#include <Sleep_n0m1.h>

#include <SoftwareSerial.h>
#include <SFE_BMP180.h>

#include "DHT.h"


#define ESP_RX   4
#define ESP_TX   3
#define ESP_RST  7

//#define LOW_BATTERY 800
#define LOW_BATTERY 1

#define TARGET_HOST "192.168.0.201"
#define TARGET_PORT "8001"
#define SID "fifeton"
#define PWD ""
#define ALTITUDE 65.0

/*
#define TARGET_HOST "fifeton.com.es"
#define TARGET_PORT "8001"
#define SID "campingberceo"
#define PWD "1234567890"
#define ALTITUDE 700.0
*/
#define SLEEP_TIME 900000
//#define SLEEP_TIME 10000
#define SLEEP_TIME_NOK 60000
#define SLEEP_TIME_LB 1800000

#define MAX_ERRORS 8
#define MAX_LOOPS 20


#define DHT11PIN       2
#define TRANS_WIFI    11
#define TRANS_SENSORS 12
#define LED_ACTIVIDAD 13
#define RAIN_PIN       5
#define RESET_PIN      6
#define LOW_BAT_PIN    A3

SoftwareSerial BT1(ESP_RX, ESP_TX); // RX | TX
Sleep sleep;
SFE_BMP180 pressure;
DHT dht(DHT11PIN, DHT11);

struct meteopi_data{
  double humidity=0;
  double temp=0;
  double pressure=0;
  double pressureAbs=0;
  double alt=0;
  boolean rain=false;
};

boolean dataSentOK;
meteopi_data data;

byte maxErrors=MAX_ERRORS;
byte loops=0;

int bateria=0;
/**
 * Inicializa el esp8266
 */
boolean wifi_init(boolean force = false) {
  Serial.println("Iniciando wifi...");
	
  //Reseteamos...
  digitalWrite(ESP_RST, LOW);
  delay(1000);
  BT1.begin(9600);
  digitalWrite(ESP_RST, HIGH);
  
  delay(1000);

  

  //Lo intentamos 3 veces máximo
  for (int i = 0; i < 3; i++) {
    Serial.println(i);
	
    //Mandando reset
    BT1.println("AT+RST");
    
    if (!BT1.find("[System Ready, Vendor:www.ai-thinker.com]")) {
      Serial.println("No reseteado");
      delay(3000);
      continue;
    }
    Serial.println("Reset OK");
    
	
    BT1.setTimeout(10000);
    Serial.print("Conectando con ");
    Serial.println(SID);
    
	//Conectamos con el SID
	BT1.print("AT+CWJAP=\"");
    BT1.print(SID);
    BT1.print("\",\"");
    BT1.print(PWD);
    BT1.println("\"");

    
    if (!BT1.find("OK")) {
      Serial.println("ERROR");
	  //Si falla, lo volvemos a intentar
      continue;
    }else{
      Serial.println("Conectado");
    }

	//Leemos lo que haya para vaciar el buffer
    while(BT1.available()) Serial.print(char(BT1.read()));

	//Comprobamos la ip
    BT1.println("AT+CIFSR");
    
    Serial.print("IP: ");
    Serial.print(BT1.readStringUntil('\n'));

	//Vaciamos el buffer
    while(BT1.available()) Serial.print(char(BT1.read()));
    Serial.println();
    
    Serial.println("Listo!");

	//Bajamos el timeout a 5 secs
    BT1.setTimeout(5000);
    return true;
  }
  
  //Si llegamos aquí, es que ha fallado tres veces.
  return false;
}

/**
 * Lee los datos del BMP180
 */
void readPressure() {

  if (!pressure.begin()) {
    Serial.println("Pressure nok");
    return;
  }else{
    Serial.println("Pressure ok");
  }

  char status;
  

  status = pressure.startTemperature();
  if (status != 0)
  {
    Serial.print("Esperamos ");
    Serial.println(status,DEC);
    // Wait for the measurement to complete:
    delay(status);
    
    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(data.temp);
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {
        Serial.print("Esperamos ");
      Serial.println(status,DEC);
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(data.pressureAbs, data.temp);
        if (status != 0)
        {
          // Print out the measurement:

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          data.pressure = pressure.sealevel(data.pressureAbs, ALTITUDE); // we're at 1655 meters (Boulder, CO)


          data.alt = pressure.altitude(data.pressureAbs, data.pressure);
          
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");


}

/**
 * Envia los datos vía wifi
*/
void wifi_sendData() {
  dataSentOK=false;  

   String data_str;
   data_str.concat("temp=");
   data_str.concat((float)data.temp);
   data_str.concat("&hum=");
   data_str.concat((float)data.humidity);
   data_str.concat("&pres=");
   data_str.concat(data.pressure);
  data_str.concat("&rain=");
   data_str.concat(data.rain);
   data_str.concat("&bateria=");
   data_str.concat(bateria);
   data_str.concat("&date=");
   
   
   
  Serial.print("Enviando datos... ");
  Serial.println(data_str);
  
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
      BT1.print(data_str);
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

      while(BT1.available()) Serial.print(char(BT1.read()));
      
      Serial.println("Datos enviados");
      dataSentOK=true;        
    }
  }else{
    
    Serial.println("Error. Not linked.");
    
  }

  
}

void setup() {
  //Mandamos un 1 al pin de reset
  digitalWrite(RESET_PIN,HIGH);
  //Inicializamos el serial
  Serial.begin(9600);
  Serial.println("Init...");

}

void setup2(){
  
  //Configuramos los pines
  pinMode(RESET_PIN, OUTPUT);
  pinMode(LED_ACTIVIDAD, OUTPUT);
  pinMode(TRANS_WIFI, OUTPUT);
  pinMode(TRANS_SENSORS, OUTPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(DHT11PIN, INPUT);
  pinMode(ESP_RST, OUTPUT);
  pinMode(LOW_BAT_PIN, INPUT);

  //Desactivamos todo
  digitalWrite(TRANS_WIFI, LOW);
  digitalWrite(TRANS_SENSORS, LOW);
  digitalWrite(LED_ACTIVIDAD, LOW);
  digitalWrite(ESP_RST, LOW);
  
  Serial.println("Init fin");
}

void blink(int veces,int del=250){
  for(int i=0;i<veces;i++){
    digitalWrite(LED_ACTIVIDAD,HIGH);
    delay(del);
    digitalWrite(LED_ACTIVIDAD,LOW);
    delay(del);
  }
}

void loop() {
  setup2();

  bateria=analogRead(LOW_BAT_PIN);
  
  Serial.print("Batería: ");
  Serial.println(bateria);
  
  if (bateria<LOW_BATTERY){
    Serial.println("Batería baja");
    
    sleep.pwrDownMode(); //set sleep mode
    sleep.sleepDelay(SLEEP_TIME_LB); //sleep for: sleepTime
    return;
  }
  
  Serial.print("maxErrors=");
  Serial.println(maxErrors);
  
  //Si se ha alcanzado el máximo de errores, mandamos un cero al pin de reset para reiniciar
  if(maxErrors==0 || loops++>MAX_LOOPS){
    Serial.println("Reseteando...");
    blink(5,100);
    digitalWrite(RESET_PIN,LOW);
    delay(2000);
    Serial.println("NO RESETEADO!!!!!");
    digitalWrite(RESET_PIN,HIGH);
    return;
  }

    
  // put your main code here, to run repeatedly:
  Serial.println("Habilitando sensores");

  blink(1);

  //Habilitamos los sensores.
  digitalWrite(TRANS_SENSORS, HIGH);
  delay(5000);

  //Leemos el DHT
  Serial.println("Leyendo DHT");
  dht.begin();
  
  //Guardamos la humedad
  data.humidity = dht.readHumidity();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(data.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    data.humidity=0;
  }  
  

  blink(2);

  Serial.println("Leyendo BPM180...");
  
  readPressure();
  

  Serial.print("Llueve? ");
  data.rain=(digitalRead(RAIN_PIN)?false:true);
  Serial.println(data.rain);

  //Deshabilitamos los sensores
  digitalWrite(TRANS_SENSORS, LOW);

  blink(3);

  delay(2000);
  //Habilitamos el wifi
  digitalWrite(TRANS_WIFI, HIGH);
  
  //Inicializamos el wifi
  if(wifi_init())
    wifi_sendData(); //Enviamos los datos
  
  //deshabilitamos el chip esp8266
  digitalWrite(ESP_RST, LOW);
  
  //Deshabilitamos el wifi
  digitalWrite(TRANS_WIFI, LOW);

  blink(4,100);

  

  Serial.println("A dormir");
  sleep.pwrDownMode(); //set sleep mode
  if(dataSentOK){
	maxErrors=MAX_ERRORS;
    sleep.sleepDelay(SLEEP_TIME); //sleep for: sleepTime
	
  }else{
	maxErrors--;
    sleep.sleepDelay(SLEEP_TIME_NOK); //sleep for: sleepTime
    
  }

  Serial.println("Desperté");
}
