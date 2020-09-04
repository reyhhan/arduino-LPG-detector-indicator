#include <SoftwareSerial.h> //Library to communicate serial data using digital pins
#include <HX711.h> //Loadcell amplifier library
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //Library enabling I2C communication in LCDs

#define DOUT1  5       //Data pin for Load Cell #1
#define CLK1  2        //Clock pin for Load Cell #1

#define DOUT2  7       // Data pin for Load Cell #2
#define CLK2  6        //Clock pin for Load Cell #2

#define DOUT3  3       //Data pin for Load Cell #3
#define CLK3  4        //Clock pin for Load Cell #3

#define buzzer 8  
#define powerLED 13   //flashes red
#define gasLED 9      //flashes green
#define loadLED 12    //flashes blue
#define sensor A0     //pin for MQ2 gas sensor
#define load_Res 5    //load resistance as per the MQ2 sensor datasheet
#define air_factor 9.83  // Rs/Ro ratio for air as per the MQ2 datasheet

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); //A5 to SCL, A4 to SDA
SoftwareSerial mySerial(10,11); //RX:10 TX:11
//Creation of three Loadcell objects from HX711 class
HX711 scale1; 
HX711 scale2;
HX711 scale3;

float LPGCurve[3] ={2.3,0.21,-0.47};    // (x, y, slope) x,y coordinate of one point and the slope between two points
                                        //based on the LPG graph for MQ2 gas sensor 
float Ro=0;        //sensor resistance at 1000ppm of H2 clean air
int sms_count=0;   //to store sms count for gas leakage
int sms_count2=0;  //to store sms count for minimum gas level 
int Gas_Leak_Status=0;
int buzz_time=0;
//adjust scale to user defined calibration factors
float calibration_factor1 = 105.0; //load cell 1
float calibration_factor2 = 107.0; //load cell 2
float calibration_factor3 = 105.0; //load cell 3

float weight1=0.0;
float weight2=0.0;
float weight3=0.0;
float total=0.0;
String level; //variable to store string value of gas capacity

void setup() {
  lcd.begin(16,2); //initiation of the LCD screen
  lcd.backlight();
  Serial.begin(9600); //to get readings in the serial monitor
  pinMode(buzzer, OUTPUT);    
  pinMode(powerLED, OUTPUT);
  pinMode(loadLED, OUTPUT);
  pinMode(gasLED, OUTPUT);
  
  digitalWrite(powerLED, HIGH); //power button remains "ON" until device is switched off
  digitalWrite(gasLED, LOW); 
  digitalWrite(loadLED, LOW);
  tone(buzzer, 3000, 200);  //device initiation short tone
  
  lcd.print("Empty the Scale."); //Prompt user to empty the scale for calibration, done once after powering the device
  delay(2000);
  lcd.print("Calibrating....."); 
  loadcell_calibration(); //Performs loadcell calibration
  Ro = SensorCalibration(); //Performs Gas sensor calibration
  Serial.println(Ro);  
  lcd.clear();
  lcd.print("Calibration done.");
  lcd.setCursor(0,0); 
  lcd.print("Ready To Go!"); 
  delay(1000);
  lcd.setCursor(0,1); 
  lcd.print("Place object..."); //Prompt user to Place the cylinder
  delay(3000);
  lcd.clear();
}

void loop() {
  
  digitalWrite(powerLED, HIGH);
  GetWeight();     //to calculate the total weight 
  SetGas();        //to initiate the gas sensor
  CheckGas();      //to check the status of a gas leak 
  CheckShutDown(); //to check if the leak persists
  
}

  void GetWeight() {
    Serial.println("Reading: ");
    float w1=get_weight1(); 
    float w2=get_weight2();
    float w3=get_weight3();
    total=(w1+w2+w3)*0.001; //summation of all three weights to obtain the total in kilograms
    
    Serial.print("Total");
    Serial.print(total);
    Serial.print("grams");
    Serial.println();
    
    if(total < 2.00)               // condition to check if gas level has reached minimum
    {
      digitalWrite(loadLED, LOW);  //turn off LED indicator "FULL"
      lcd.setCursor(0,0);       
      lcd.print("Out of Gas!....");//prints warning message on the LCD
      level=String(total);         //converts float to string
      while(buzz_time<10){
         tone(buzzer, 3000, 600);   //trigger the buzzer when gas level is low
         buzz_time++;
         delay(1000);
      }
      while(sms_count2<2){         //to send upto three SMS's      
        SendTextMessage2(level);   //SMS Function to send Commands to GSM module
      }
    }
  
    if(total > 2.00){             // condition to check if level of gas level is above minimum
      sms_count2=0;
      buzz_time=0;
      digitalWrite(loadLED,HIGH); //turn on LED indicator "FULL"
      noTone(buzzer);     
      lcd.setCursor(0,0);
      lcd.print("Level :");       //display the gas level in kilograms
      lcd.print(total);
      lcd.print("kg");  
    }
  }
  
  void loadcell_calibration(){ //function to calibrate the load cells
    
    //initiate the scales for readings
    scale1.begin(DOUT1, CLK1); 
    scale2.begin(DOUT2, CLK2); 
    scale3.begin(DOUT3, CLK3);
    lcd.setCursor(0,1); 
    lcd.print("Calibrating....."); //print current status 
    delay(1000);
    scale1.set_scale(); 
    scale1.tare(); //Reset the scale 1 to 0.0
    scale2.set_scale();
    scale2.tare(); //Reset the scale 2 to 0.0
    scale3.set_scale();
    scale3.tare(); //Reset the scale 3 to 0.0
    
    //Get a baseline reading for each load cell amplifier
    long zero_factor1 = scale1.read_average(); 
    long zero_factor2 = scale2.read_average(); 
    long zero_factor3 = scale3.read_average(); 

  }
   //get the weight from load cell 1
  float get_weight1(){
    scale1.set_scale(calibration_factor1);//adjust scale readings to the defined calibration factor
    weight1 = scale1.get_units(10);
    if (weight1 < 0){ 
      weight1 = 0.0; //set weight to zero if negative readings are shown
    }
    Serial.print(weight1);
    Serial.print("grams"); 
    Serial.print(" calibration_factor1: ");
    Serial.print(calibration_factor1);
    Serial.println();
    return weight1; //return value of weight1
  }

   //get the weight from load cell 2
  float get_weight2(){
    scale2.set_scale(calibration_factor2); //Adjust to this calibration factor
    weight2 = scale2.get_units(10); // print the average of 10 readings from the ADC
    if (weight2 < 0){
      weight2 = 0.0;
    }
    Serial.print(weight2);
    Serial.print("grams"); 
    Serial.print(" calibration_factor1: ");
    Serial.print(calibration_factor2);
    Serial.println();
    return weight2;
  }

   //get the weight from load cell 3
  float get_weight3(){
    scale3.set_scale(calibration_factor3); //Adjust to this calibration factor
    weight3 = scale3.get_units(10);
    if (weight3 < 0 ){
      weight3 = 0.0;
    }
    Serial.print(weight3);
    Serial.print("grams"); 
    Serial.print(" calibration_factor3: ");
    Serial.print(calibration_factor3);
    Serial.println();
    return weight3;
  }

  float SensorCalibration()  //to calibrate the MQ2 sensor
  {  
    int i;  
    float val=0;      
    val=resistance(50,500); //for calibration we take 50 samples with a delay of 500             
    val = val/air_factor;    //calibration takes into consideration the external environmental conditions as per Datasheet
    return val;   
  }
  
   void SetGas(){
     lcd.setCursor(0,1);   
     lcd.print("LPG:");   
  }
  
  void CheckGas()
  { 
     float Rs=resistance(5,50);  //5 samples with a delay of 50
     Rs/=Ro;    //obtaining the Rs/Ro ratio to read the PPM value
     int result=pow(10,(((log(Rs)-LPGCurve[1])/LPGCurve[2]) + LPGCurve[0]));  //function to derive the PPM level based on the corresponding Rs/Ro ratio from the LPG graph in the datasheet
      //As it is a logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic value.
     lcd.print(result);  
     lcd.print( " ppm        ");
     Serial.println(result);
     delay(1000);
     if(result>200)   //LPG threshold is set to minimum 200PPM as in the datasheet
     {           
       Serial.println("SMS");
       SetAlert(); // Function to send SMS Alert when gas leak is detected
     }  
  }
 
  
  float resistance(int samples, int interval)  
  {  
     int i;  
     float Rs=0;   //sensor resistance at variuos concentration of gas
     for (i=0;i<samples;i++)   
     {  
        int adc_value=analogRead(sensor); //obtain raw analog value from the sensor 
        Rs+=((float)load_Res*(1023-adc_value)/adc_value);  //calculating the sensor resistance given the voltage and load resistence
        delay(interval);  
     }  
     Rs/=samples;  //get average reading
     return Rs;    //return current average sensor resistance 
  }
  
   void SetAlert()         
  { 
    lcd.clear();
    lcd.setCursor(0,0); 
    lcd.print("Level :");         
    lcd.print(total);
    lcd.print("kg"); 
    lcd.setCursor(0,1);
    lcd.print("GasLeak Detected"); //to display alert message on LCD
    digitalWrite(gasLED, HIGH);  
    tone(buzzer, 4000, 300); 
    while(sms_count<2) //Number of SMS Alerts to be sent
      {  
      SendTextMessage1(); // Function to send AT Commands to GSM module
      }
    Gas_Leak_Status=1; 
  }
  
  void CheckShutDown(){               // fucntion checks if the gas leak has oocured
    if(Gas_Leak_Status==1){           //if true continue to calculate the PPM level in the air 
    float Rs=resistance(5,50);  
    Rs/=Ro;  
    int result=pow(10,(((log(Rs)-LPGCurve[1])/LPGCurve[2]) + LPGCurve[0])); //function to derive the PPM level based on the corresponding Rs/Ro ratio from the LPG graph in the datasheet
      if(result<200){                                           //if gas leak does not persist, turn off "LEAK" led and buzzer
         digitalWrite(gasLED, LOW);
         noTone(buzzer); 
         delay(500);  
         sms_count=0;                                        //set sms count to back to zero
         Gas_Leak_Status=0;
         lcd.clear();
       }   
    }
  } 
  void SendTextMessage1()    // function to send sms when a gas leak is detected
  {
    mySerial.begin(9600);    //enable serial communication between GSM and arduino
    delay(300);
    mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
    delay(500);
    mySerial.println("AT+CMGF=1");    //To send SMS in Text Mode
    delay(500);
    mySerial.println("AT+CMGS=\"+94721712933\"\r"); // phone number can be changed as desired
    delay(500);
    mySerial.println("Emergency-> LPG Gas Leaking!!!");//the content of the message
    delay(200);
    mySerial.println((char)26);//the stopping character
    delay(500);
    mySerial.println("AT+CMGS=\"+94722240502\"\r"); // phone number can be changed as desired
    delay(500);
    mySerial.println("Emergency-> LPG Gas Leaking!!!");//the content of the message
    delay(200);
    mySerial.println((char)26);//the message stopping character
    delay(200);
    sms_count++;
  } 

  void SendTextMessage2(String level) //send SMS when the gas level reaches minimum; passing gas level reading as a parameter in the function
  {
    mySerial.begin(9600);
    delay(300);
    mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
    delay(500);
    mySerial.println("AT+CMGF=1");    //To send SMS in Text Mode
    delay(500);
    mySerial.println("AT+CMGS=\"+94721712933\"\r"); // phone number can be changed as desired
    delay(500);
    mySerial.println("Warning-> LPG Gas level reached minimum!!! Remaining level is " + level + "kg");//the content of the message
    delay(200);
    mySerial.println((char)26);//the stopping character
    delay(200);
    sms_count2++; //to limit the number of sms's sent
  } 
