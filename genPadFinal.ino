#include "Keyboard.h"
/*
  "THE THING" firmware code
  (C)2024 Baylon Rogers
  NOTE: Because both the aux board and the controllers have buttons, I'm going to refer to controller buttons as "inputs" to avoid confusion
*/
const bool DEBUG=false;            //enables serial output
const bool IGNORE_AUX=false;       //ignores aux board readings. used for debug
const bool IGNORE_PRESSKEYS=false; //state to set pressKeys when ignoring aux board
const int IGNORE_POS=2;           //pos to be used when ignoring aux board

int bindings[2][11]={{218,217,216,215,122,120,99,176,97,115,100},{111,108,107,59,103,104,106,34,116,32,117}}; //stores the keybindings for keyboard inputs. Array 1 is for player 1, array 2 is for player 2
char names[11][6]={{'U','p',NULL,NULL,NULL},{'D','o','w','n',NULL},{'L','e','f','t',NULL},{'R','i','g','h','t',NULL}, //lookup table for the readable names of buttons for serial output when debugging
{'A',NULL,NULL,NULL,NULL,NULL},{'B',NULL,NULL,NULL,NULL,NULL},{'C',NULL,NULL,NULL,NULL,NULL},{'S','t','a','r','t',NULL},
{'X',NULL,NULL,NULL,NULL,NULL},{'Y',NULL,NULL,NULL,NULL,NULL},{'Z',NULL,NULL,NULL,NULL,NULL}};
int playerPins[2][7]={{2,3,4,5,6,7,8},{9,10,11,12,13,18,19}}; //list of pins to read when determining inputs. Array 1 for player 1, array 2 for player 2. Last item of each array is the select line for that controller
int readPins[4][6]={{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}}; //stores the read pins from each line. 4 items to accont for Genesis "multi-sweep" approach
int buttons[2][11]={{0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0}}; //stores what inputs are pressed, later read when determining which keys to press

bool pressKeys=false;             //enables keystrokes
int isGenSetup=2;            //stores which console the pins are set up for. True for Genesis, false for MasterSystem Used in checkForGenesis() to determine if the console switch was flipped.
bool useGenesis=false;            //used to determine which console to read pins for. True for Genesis, false for MasterSystem
int pos=0;                        //stores the position of the aux board switch
int swtch=0;                      //stores the analog value returned by the aux board
int btn=0;                        //stores the analog value of the button

void readAuxBoard(){
  float threashold=256; //aux board uses an analog read mixed with resistors to determine switch positon, this is the cuttoff point.

  
  
  //TODO: Optimize this shit-ass code. Probably can do some shenanigans with the rounding function
  //but can't be fucked to do it rn.
  if(IGNORE_AUX&&DEBUG){
    pos=IGNORE_POS;
    pressKeys=IGNORE_PRESSKEYS;
    return;
  }
  btn = analogRead(A3);
  swtch = analogRead(A2);
  Serial.println(swtch);
  /*This code is really dumb, but that's mostly because I built the aux board in a stupid way
    The resistor values I picked try to aim for 256 and 768, as 256 is a quarter of 1024, the max value
    I chose quarters because I had to find four positions. This made sense when designing and almost all of the way through building
    However, because I'm using ground as a value, I should have picked thirds instead
  */
  if(swtch<=24)
  {
    pos=1; //hard case for
  }
  else if(swtch>=1000)
  {
  pos=4;
  }
  else
  {
    pos=2+(swtch>=threashold); //The boolean "(swtch>=threashold)" when converted to an int is either 0 or 1. 
                               //Because we know that the switch isn't in either full off or full on, we can
                               //shift the position up by 2 and use that boolean as an int to get either 2 or 3
  }
  if(btn>500)
  {
    pressKeys=true;
  }
  else
  {
    pressKeys=false;
    Keyboard.releaseAll(); //release all the keys being pressed in case the user was pressing an input
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(A3, INPUT); //aux board button pin
  pinMode(A2, INPUT); //aux board switch pin
  while(!Serial)
  {
    delay(1); //wait for the serial port to start up
  }
  if(DEBUG){
    delay(1500); //pause for just a bit to REALLY make sure that the Serial port is ready (it often reports true when )
    if(IGNORE_AUX)
    {
      Serial.println("IGNORING AUX BOARD");
      Serial.print("\tSETTING SWITCH POS TO ");
      Serial.println(IGNORE_POS);
      Serial.print("\tSETTING KEY BUTTON TO ");
      if(IGNORE_PRESSKEYS)
      {
        Serial.println("ON");
      }
      else
      {
        Serial.println("OFF");
      }
      
    }
  }
  checkForGenesis();
  
}
void checkForGenesis()
{
  readAuxBoard(); //sets the pos state
  useGenesis=(pos>=3); //checks if the switch is pushed to the upper half
  Serial.println(useGenesis);
  if(useGenesis) //should we be reading genesis inputs?
  {
    if(isGenSetup<1) //is the board not set up to read genesis?
    {
      if(DEBUG){
        Serial.println("SWITCHING TO GENESIS");
        Serial.println(swtch);
        delay(2000); //delay to make debugging easier
      }
      Serial.println("damn");
      setupGenesis(); //actually set it up
      isGenSetup=true; //make sure we remember its set up so we don't have to set it up again
    }
  }
  else
  {
    if(isGenSetup>=1) //is the board set up to read genesis
    {
      if(DEBUG){
        Serial.println("SWITCHING TO SMS");
        Serial.println(swtch);
        delay(2000); //delay to make debugging easier
      }
      Serial.println("huh??");
      setupSMS(); //actually set it up
      isGenSetup=false; //make sure we remember its set up so we don't have to set it up again
    }
  }

}
void loop() {
  checkForGenesis(); //check for any changes to aux board
  readButtons(); //read controller inputs
  sendButtons(); //send appropriate keystrokes
  if(DEBUG){
    delay(250); //delay to make debugging easier
  }
  else{
    delay(8);
  }
}
void setupSMS()
{
  //Keyboard.releaseAll(); //releases all keys
  
  for(int i=0;i<2;i++) //for each array in playerPins
  {
    for(int j=0;j<6;j++)
    {
      pinMode(playerPins[i][j], INPUT_PULLUP); //set the pin to use the pullup resistors
      if(DEBUG)
      {
        Serial.println(playerPins[i][j]); //shows which pins were initialized
      }
    }
  }
}
void setupGenesis()
{
  //Keyboard.releaseAll();
  for(int i=0;i<2;i++)
  {
    for(int j=0;j<6;j++)
    {
      pinMode(playerPins[i][j], INPUT); //disables pullup resistors (if they were ever enabled)
      if(DEBUG)
      {
        Serial.print("INPUT ");
        Serial.println(playerPins[i][j]); //shows which pins were initialized
      }
    }
    if(DEBUG)
    {
      Serial.print("OUTPUT ");
      Serial.println(playerPins[i][6]);
    }
    pinMode(playerPins[i][6], OUTPUT);  //sets the last pin to output for the control pin
  }
}

void readButtons(){
  if(useGenesis)
  {
    readGen3();
  }
  else
  {
    readSMS();
  }
  return;
}
void readSMS(){
  //iterate through inputs and assign readPins to player pins
  //skip readPins[1], as that's for the second sweep of player 1's genesis inputs
  for(int i=0;i<4;i+=2)
  {
    for(int j=0;j<6;j++)
    {
      readPins[i][j]=!digitalRead(playerPins[i][j]);
    }
  }
  //iterate again and send button mappings to the buttons array.
  for(int i=0;i<4;i+=2)
  {
    for(int j=0;j<6;j++)
    {
      buttons[i][j]=readPins[i][j];
    }
  }
}
void readGen3()
{
  digitalWrite(8, LOW); //set control low
  delay(5); //wait for the IC inside a 6 button controler to respond 
  for(int i=0;i<2;i++)
  {
    for(int j=0;j<6;j++)
    {
      readPins[i][j]=!digitalRead(playerPins[i][j]);
    }
    digitalWrite(8, HIGH);
    delay(5);
  }
  buttons[0][0]=readPins[0][0];
  buttons[0][1]=readPins[0][1];
  buttons[0][2]=readPins[1][2];
  buttons[0][3]=readPins[1][3];
  buttons[0][4]=readPins[0][4];
  buttons[0][5]=readPins[1][4];
  buttons[0][6]=readPins[1][5];
  buttons[0][7]=readPins[0][5];
  return;
}

void sendButtons()
{
  if(DEBUG){
    Serial.print("Pressed Buttons:");
  }
  for(int i=0;i<2;i++)
  {
    if(DEBUG){
    Serial.print(" P");
    Serial.print(i+1);
    Serial.print(" (");
    }
    for(int j=0;j<11;j++)
    {
      if(buttons[i][j]==1)
      {
        if(DEBUG){
          Serial.print(names[j]);
          Serial.print(", ");
        }
        if(pressKeys)
        {
          Keyboard.press(bindings[i][j]);
        }
      }
      else if(pressKeys)
      {
        Keyboard.release(bindings[i][j]);
      }
    }
    if(DEBUG)
    {
      Serial.print(")");
    }
  }
  if(DEBUG)
  {
    Serial.print("\n");
  }
}
