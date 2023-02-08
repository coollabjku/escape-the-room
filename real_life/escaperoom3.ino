#include <EEPROM.h>
#include <Keypad.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include "TM1637.h"
#include "Adafruit_TCS34725.h"

#include <SPI.h>
#include <SD.h>

volatile unsigned int remainingTime;// = 123;
volatile int8_t ListDisp[4] = {0};

enum states_quiz{startState, error, writeQuestion, waitConformation, writeAnswers, waitAnswer, checkAnswer, waitNextQuestion, endGame};

/*variables for the quiz*/
const int chipSelect = 53; //pin for sd card
int fileNr = 0; //number of file to load
char filename[7] = "0.csv"; //filename to load
char OutputLine[200] = {0}; //line to print on the lcd
int numberQuestions = 0;

states_quiz state_quiz;
File dataFile;
String buffer;
char *Question, *correctAnswer, *saveptr;
char *Answer[4];
int correctAnswerInt = 0, c = 0, inputAnswer = 0;

/* For 7seg display */
TM1637 tm1637(23,25); // sevensegdisp

/* For the color sensor */
int color_state = 0;
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);


/* Outputs for the LEDs*/
const int LED1 = 9;
const int LED2 = 10;
const int LED3 = 11;
const int LED4 = 12;
const int LED5 = 6;
const int LED6 = 7;
const int LED7 = 8;
const int relay_pin = 46;

/* Keypanel */
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {30, 32, 34, 36}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {22, 24, 26, 28}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

volatile int state = 0, i = 0;
char pin[21] = {0};
char key_input = 0;
char correct_pin[21] = "42069";

void setup(){
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin,HIGH);
  
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  
  pinMode(LED5, OUTPUT);
  pinMode(LED6, OUTPUT);
  pinMode(LED7, OUTPUT);


  analogWrite(2,255);
  analogWrite(3,200);
  analogWrite(4,140);
  analogWrite(5,90);

  /*Init LCD */
  lcd.init();
  lcd.backlight();
  /* Init 7seg Time Display */
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);

  //Serial.begin(9600);
  //while (!Serial)
  remainingTime = (EEPROM.read(1) << 8) | EEPROM.read(0);
  Serial.println(remainingTime);
  fileNr = EEPROM.read(2);
  sprintf(filename,"%d.csv", fileNr);
  
/* Setup menu, enter the time*/
  int loop1 = 1, setFile = 0;
  if(customKeypad.getKey() == '5')
  {
    lcd.setCursor(0,0);
    lcd.print("Setup Menu");
    lcd.setCursor(0,1);
    lcd.print("Zeit in s eingeben:");
    while(loop1)
    {
      key_input = customKeypad.getKey();
      if( key_input != '\0' && key_input != 'A' && key_input != 'B' && key_input != 'C' && key_input != 'D' )
      {
        if(key_input == '*')
        {
            /* First setting the Time*/
            if(!setFile)
            {
                remainingTime = atoi(pin);
                lcd.clear();
                lcd.setCursor(0,0);
                sprintf(pin,"Neue Zeit: %ds",remainingTime);
                lcd.print(pin);
                lcd.setCursor(0,1);
                lcd.print("gespeichert!");
                EEPROM.write(0, remainingTime & 0b11111111);
                EEPROM.write(1, remainingTime >> 8);
                Serial.println(remainingTime);
                while(customKeypad.getKey() != '*');
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Setup Menu");
                lcd.setCursor(0,1);
                lcd.print("Datei Nr. eingeben:");
                memset(pin,' ',20);
                i = 0;
                setFile= 1;
            }
            /*Now set the csv File of the quiz */
            else
            {
               lcd.clear();
               lcd.setCursor(0,0);
               lcd.print("File gespeichert:");
               lcd.setCursor(0,1);
               EEPROM.write(2,atoi(pin));
               sprintf(filename,"%d.csv", atoi(pin));
               lcd.print(filename);
               while(1);
            }
         }
        else if (key_input == '#')
        {
           pin[--i] = '\0';
           clearLine(1);
        }
        else if(i < 20)
        {
          pin[i++] = key_input;
        }
        clearLine(2);
        lcd.setCursor(0,3);
        lcd.print(pin);
      }
    }
  }

  /*Init Color Sensor */
  tcs.begin();

  /* Init SD Card */
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    lcd.print("SD Karte fehlt!");
    while (1);
  }

    Serial.println("card initialized.");
    do
    {
      //getFilename(filename); // ask the user for the game he wanzts to play
      dataFile = SD.open(filename); //try to open the game
      if(!dataFile)
      {
        Serial.println("Spiel nicht gefunden!");
        lcd.setCursor(0,0);
        lcd.print("Datei nicht gefunden");
        lcd.setCursor(0,1);
        lcd.print(filename);
      }
      delay(500);
    }
    while (!dataFile); //stop here if the game doesn't exist

      if(!checkCSV(dataFile, &numberQuestions)) //check if the csv is correct
      {
        Serial.println("Spieldatei fehlerhaft!");
        lcd.setCursor(0,0);
        lcd.print("Datei fehlerhaft");
        lcd.setCursor(0,1);
        lcd.print(filename);
        dataFile.close();
        while(1);
      }
      else
      {
        Serial.println("Spieldatei korrekt!"); //csv is correct
      }
      start_timer();
}
  
void loop(){
char line[200];
char options[4] = { 'A', 'B','C','D'};
char ch;

  switch (state)
  {
    case 0:
      lcd.clear();
      digitalWrite(LED4, HIGH); 
      digitalWrite(LED3, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED1, HIGH);
      lcd.print("Startcode eingeben:");
      state = 1;
    break;


    //now entering pin
    case 1:
      key_input = customKeypad.getKey();

      lcd.setCursor(0,1);
      lcd.print(pin);
      
      if(key_input != '\0')
      {
        clearLine(3);
        if (key_input == '#')
        {
           pin[--i] = '\0';
           clearLine(1);
        }
        else if (key_input == '*')
        {
            if( strcmp(pin, correct_pin) == 0)
            {
              clearLine(0);
              clearLine(1);
              lcd.setCursor(2,0);
              lcd.print("Passwort richtig");
              lcd.setCursor(2,2);
              lcd.print("Lasset das Spiel");
              lcd.setCursor(5,3);
              lcd.print("beginnen!");
              state = 2;
              i = 0;
            }
            else
            {
              memset(pin, '\0', 20);
              lcd.setCursor(0,3);
              lcd.print("Falsches Passwort!!");
              clearLine(1);
              i = 0;
              remainingTime -= 60;
            }
        }
        else if(i < 19)
       {
        pin[i++] = key_input;
       }
      }
      break;
    
    case 2:
      if( i <4)
      {
         digitalWrite(LED4, HIGH); 
         digitalWrite(LED3, HIGH);
         digitalWrite(LED2, HIGH);
         digitalWrite(LED1, HIGH);
         delay(100);
         digitalWrite(LED4, LOW); 
         digitalWrite(LED3, LOW);
         digitalWrite(LED2, LOW);
         digitalWrite(LED1, LOW);
         delay(100);
         i++;
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Bitte Drahtbrücken richtig");
        lcd.setCursor(0,2);
        lcd.print("verbinden");
        state = 3;
      }
    break;

    case 3:
    {
      if(drahtbruecken() == 1)
      {
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Steuerstäbe");
        lcd.setCursor(0,2);
        lcd.print("einstellen");
        state = 4;
      }
    }
    break;

    case 4:
      if(poti())
      {
        state = 5;
        lcd.clear();
      }
      break;

    case 5:
    /* starte das quiz */
        dataFile = SD.open(filename);
        dataFile.readStringUntil('\n'); //clear first line, because these are the headers in the csv
        state = startState;// now start the game
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Ein kleines Quiz.");
        lcd.setCursor(0,1);
        lcd.print("\"0\" druecken");
        lcd.setCursor(0,2);
        lcd.print("fuer vorwaerts");
        lcd.setCursor(0,3);
        lcd.print("Viel Erfolg!");
            
        state = 6;
      break;

      case 6:
      if(quiz())
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Bitte Farbkarten in");
        lcd.setCursor(0,1);
        lcd.print("richtiger Reihenfolge");
        lcd.setCursor(0,2);
        lcd.print("einfuehren!!!");
        digitalWrite(LED5, LOW); 
      digitalWrite(LED6, LOW); 
      digitalWrite(LED7, LOW);
        state = 7;
      }  
      break;
      
      case 7:
        if(farbspiel())
          state = -1;
        break;

   case -1:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Du hast gewonnen!");
      lcd.setCursor(0,1);
      lcd.print(" A Game");
      lcd.setCursor(0,2);
      lcd.print("powered by ");
      lcd.setCursor(0,3 );
      lcd.print("Cool Lab ;)");
      stop_timer();
      state = -3;
      break;

  case -2:
        lcd.clear();
        lcd.setCursor(0,2);
        lcd.print("Zeit ist vorbei!");
        lcd.setCursor(0,3);
        lcd.print("Roggenberg hat gewonnen!");
        state = -3;
  break;

  case -3:
  delay(10000);//wait 10s
  digitalWrite(relay_pin, LOW); // Turn off the arduino

    default:
    break;
    
    
  }
      delay(10);
}
/*********************************/
/*             SPIELE            */
/*********************************/

int drahtbruecken()
{
  bool wire1 = (analogRead(A0) <= 1000) && (analogRead(A0) >=900);
  bool wire2 = (analogRead(A1) <= 810) && (analogRead(A1) >= 700);
  bool wire3 = (analogRead(A2) <= 580) && (analogRead(A2) >= 500);
  bool wire4 = (analogRead(A3) <= 355) && (analogRead(A3) >= 330);

  if(wire1 || wire2 || wire3 || wire4)
  {
     digitalWrite(LED1, HIGH);
  }
  else
  {
      digitalWrite(LED4, LOW); 
      digitalWrite(LED3, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED1, LOW);
  }


  if( (wire1 && wire2) || (wire1 && wire3) || (wire1 && wire4) || (wire2 && wire3) || (wire2 && wire4) || (wire3 && wire4))
  {
  digitalWrite(LED2, HIGH);
  digitalWrite(LED1, HIGH);
  }
  else
  {
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);

  }
  
  if( (wire1 &&wire2 &&wire3) || (wire1 && wire2 && wire4) || (wire1 && wire3 && wire4) || (wire2 && wire3 && wire4))
  {
  digitalWrite(LED3, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED1, HIGH);
  }
  else
  {
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
  }


  if(wire1 && wire2 && wire3 && wire4)
  {
  digitalWrite(LED4, HIGH); 
  digitalWrite(LED3, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED1, HIGH);
  return 1;
  }
  else
  {
    digitalWrite(LED4, LOW);
  }
  return 0;
}

int poti()
{
  bool poti1 = (analogRead(A8) <= 400) && (analogRead(A8) >=345);
  bool poti2 = (analogRead(A9) <= 1010) && (analogRead(A9) >= 1000);
  bool poti3 = (analogRead(A10) <= 770) && (analogRead(A10) >= 760);

if (poti1)
  digitalWrite(LED5,HIGH);
else
  digitalWrite(LED5,LOW);
  
if (poti2)
  digitalWrite(LED6,HIGH);
else
  digitalWrite(LED6,LOW);
  
if (poti3)
  digitalWrite(LED7,HIGH);
else
  digitalWrite(LED7,LOW);

  if(poti1 &&poti2&& poti3)
  return 1;
  return 0;
}

int farbspiel()
{

  float red, green, blue;

  delay(60);  // takes 50ms to read
  tcs.getRGB(&red, &green, &blue);
  bool is_green = red > 115 && red <125 && green >75 && green <85 && blue > 35 && blue <45;
  bool is_red = red > 140 && red <165 && green >50 && green <60 && blue > 35 && blue <45;
  bool is_blue = red > 100 && red <125 && green >70 && green <85 && blue > 50 && blue <60;
  bool is_empty = red > 125 && red < 135 && green > 65 && green < 75 && blue > 40 && blue <50;
  bool is_else = !is_blue && !is_red && !is_blue && !is_empty;
  
  switch (color_state)
  {
    case 0: 
      if (is_red)
      {
        color_state = 1;
        digitalWrite(LED5, HIGH); 
      }

    break;

    case 1: 
      if(is_empty)
      {
        color_state = 2;
      }
      break;
    
    case 2:
      if(is_green)
      {
        digitalWrite(LED6, HIGH); 
        color_state = 3;
      }
      else if(!is_empty)
      {
        color_state = 0;
        digitalWrite(LED5, LOW); 
      }
      break;

    case 3: 
      if(is_empty)
        color_state = 4;
      break;

    case 4:
      if(is_blue)
      {        
        digitalWrite(LED7, HIGH); 
        color_state = 0;
        return 1;
      }
      else if(!is_empty)
      {
        color_state = 0;
        digitalWrite(LED5, LOW); 
        digitalWrite(LED6, LOW); 
      }
        
  }
  Serial.println(color_state);
  return 0;
}



/* Quiz Function

return value: 1 if quiz is complete, optherwise, return 0
*/
int quiz()
{

char line[200];
char options[4] = { 'A', 'B','C','D'};
char ch;

  /* This is a finite state machine for the game logic*/
switch(state_quiz)
{

  case startState:
    if(customKeypad.getKey() == '0')
    {
      state_quiz = writeQuestion;
    }
    
  break;
  
  /*Write the Question from the csv on the display and store the correct answer*/
  case writeQuestion:
    if(dataFile.available())
    {
        buffer = dataFile.readStringUntil('\n');
        buffer.toCharArray(line, buffer.length() + 1);
        Question = strtok_r(line, ";", &saveptr); 
        for(int i = 0; i <4; i++) Answer[i] = strtok_r(NULL, ";", &saveptr);
        correctAnswer = strtok_r(NULL, ";", &saveptr);
        correctAnswerInt = atoi(correctAnswer);
        lcd.clear();
        lcd.setCursor(0,0);
        printLongString(0,Question);
        state_quiz = waitConformation;
    }
    else
    {
      //state = endGame;
      return 1;
    }
  break;
  
  case waitConformation:
      if(customKeypad.getKey() == '0')
      {
        lcd.clear();
        state_quiz = writeAnswers;
      }
        
  break;

  case writeAnswers:
      lcd.clear();
      for(int i = 0; i <4; i++)
      {
        lcd.setCursor(0,i);
        sprintf(line, "%c: %s", options[i], Answer[i]);
        lcd.print(line);
      }
      state_quiz = waitAnswer;
  break;
  
  /* Now waiting that the user enters a correct answer */
  case waitAnswer:

  ch = customKeypad.getKey();

  switch (ch)
  {
    case 'A':
      inputAnswer = 1;
      state_quiz = checkAnswer;
      break;
    case 'B':
      inputAnswer = 2,
      state_quiz = checkAnswer;
      break;
    case 'C':
      inputAnswer = 3;
      state_quiz = checkAnswer;
      break;
    case 'D':
      inputAnswer = 4;
      state_quiz = checkAnswer;
      break;
     
  }
  break;

  /* check if the answer is correct, if yes go to the next question */
  case checkAnswer:
  lcd.clear();
      lcd.setCursor(0,0);
      if(inputAnswer == correctAnswerInt)
      {
        lcd.print("Antwort richtig");
      }
      else
      {
        //playtime -= timePerQuestion/2; // if a wrong answer is given subtract the half of the time for the questino from the time.
        remainingTime -= 10;
        lcd.print("Antwort falsch!");
        sprintf(line, "Richtige Anwort: %s", Answer[correctAnswerInt-1]);
        printLongString(1,line);
      }
      lcd.setCursor(0,3);
      //lcd.print("Bitte 0 druecken");
      state_quiz = waitNextQuestion;

      break;

  case waitNextQuestion:
    if(customKeypad.getKey() == '0')
      state_quiz = writeQuestion;
    
    break;

  default:
      Serial.println("Error");
      while(true);
      break;
}
return 0;
}


/* checks if the csv file is correct */
bool checkCSV(File dataFile, int *numberQuestions)
{
  String buffer;
  char *Question, *Answer1, *Answer2, *Answer3, *Answer4, *correctAnswer;
  char *Answer[4];
  char *saveptr;
  int correctAnswerInt, counter = 0;

  char line[200];
  dataFile.readStringUntil('\n');
  while (dataFile.available())
  {
      buffer = dataFile.readStringUntil('\n');
      buffer.toCharArray(line, buffer.length() + 1);
      Question = strtok_r(line, ";", &saveptr); 
      Answer1 = strtok_r(NULL, ";", &saveptr);
      Answer2 = strtok_r(NULL, ";", &saveptr);
      Answer3 = strtok_r(NULL, ";", &saveptr);
      Answer4 = strtok_r(NULL, ";", &saveptr);
      correctAnswer = strtok_r(NULL, ";", &saveptr);
      correctAnswerInt = atoi(correctAnswer);
      counter++;
      if( Question[0] == '\0' || Answer1[0] == '\0' || Answer2[0] == '\0' || Answer3[0] == '\0' || Answer4[0] == '\0' || correctAnswerInt == 0)
      return false;
  }
      *numberQuestions = counter;
      return true;
}



/******************************/
/*      HELPER FUNCTIONS      */
/******************************/

/* Helper function to clear one line on the lcd */
void clearLine(int line)
{
  char emptyline[21] = {0};
  memset(emptyline, ' ', 20);
  lcd.setCursor(0,line);
  lcd.print(emptyline);
  delay(10);
}


/* Printing the time on the 7seg display */
void createOutput(int input, int8_t *output)
{
int seconds = 0;
int minutes = 0;

minutes = input / 60;
seconds = input - minutes*60;

output[3] = seconds % 10;
output[2] = seconds / 10;
output[1] = minutes % 10;
output[0] = minutes / 10;
}

/* Helper function for the Quiz to make automatic line rbeaks on the display with longer strings */
void printLongString(int startline, char *input)
{
  char line1[21] = {0}, line2[21] = {0}, line3[21] = {0},line4[21] = {0};
  lcd.setCursor(0,0);
  //lcd.print(input);
  if(strlen(input) <=20)
  {
    lcd.print(input);
    return;
  }
  strncpy(line1,&input[0],20);
  lcd.setCursor(0,startline);
  lcd.print(line1);
  
  if(strlen(input) <= 40)
  {
   strncpy(line2,&input[20],strlen(input)-20);
   lcd.setCursor(0,startline+1);
   lcd.print(line2);
   return;
  }

  strncpy(line2,&input[20],20);
  lcd.setCursor(0,startline+1);
  lcd.print(line2);

  if(strlen(input) <= 60)
  {
   strncpy(line3,&input[40],strlen(input)-40);
   lcd.setCursor(0,startline+2);
   lcd.print(line3);
   return;
  }
      strncpy(line3,&input[40],20);
      lcd.setCursor(0,startline+2);
      lcd.print(line3);
  
  if(strlen(input) <= 80)
  {
    strncpy(line4,&input[60],strlen(input)-60);
   lcd.setCursor(0,startline+3);
   lcd.print(line4);
   return;
  }
  
  else
  {
      strncpy(line4,&input[60],20);
      lcd.setCursor(0,3);
      lcd.print(line4);
  }
}

/* Check if the time is finished and put it to the "lost" state */
void checktime()
{
  if (remainingTime == 0)
  state = -2;
}

/******************************/
/*       TIMER FUNCTIONS      */
/******************************/

/* Starting the Timer Interupts for the 7seg display */
void start_timer()
{
      cli();                      //stop interrupts for till we make the settings
  /*1. First we reset the control register to amke sure we start with everything disabled.*/
  TCCR1A = 0;                 // Reset entire TCCR1A to 0 
  TCCR1B = 0;                 // Reset entire TCCR1B to 0
 
  /*2. We set the prescalar to the desired value by changing the CS10 CS12 and CS12 bits. */  
  TCCR1B |= B00000100;        //Set CS12 to 1 so we get prescalar 256  
  
  /*3. We enable compare match mode on register A*/
  TIMSK1 |= B00000010;        //Set OCIE1A to 1 so we enable compare match A 
  
  /*4. Set the value of register A to 31250*/
  OCR1A = 62499;             //Finally we set compare register A to this value  
  sei();                     //Enable back the interrupts
}

//With the settings above, this IRS will trigger each 500ms.
ISR(TIMER1_COMPA_vect){
  TCNT1  = 0;                  //First, set the timer back to 0 so it resets for next interrupt
  if(remainingTime <= 0)
  {
  remainingTime = 1;
  }
  
  createOutput(--remainingTime, ListDisp);
  tm1637.display(ListDisp);
  checktime();
}


void stop_timer()
{
  cli();                      //stop interrupts for till we make the settings
  /*1. First we reset the control register to amke sure we start with everything disabled.*/
  TCCR1A = 0;                 // Reset entire TCCR1A to 0 
  TCCR1B = 0;                 // Reset entire TCCR1B to 0
 
  /*2. We set the prescalar to the desired value by changing the CS10 CS12 and CS12 bits. */  
  TCCR1B |= 0;        //Set CS12 to 1 so we get prescalar 256  
  
  /*3. We enable compare match mode on register A*/
  TIMSK1 |= 0;        //Set OCIE1A to 1 so we enable compare match A 
  
  /*4. Set the value of register A to 31250*/
  OCR1A = 0;             //Finally we set compare register A to this value  
  sei();                     //Enable back the interrupts
}
