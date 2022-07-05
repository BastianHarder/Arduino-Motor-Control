#include <Arduino.h>

//function prototypes
void isr_readEncoder();
void control_motor_automatically();
void control_motor_manually(int user_input);
void set_motor_signals(int direction, int speed);

/*
 *Encoder Pinning:
Red   motor power (connects to one motor terminal)
Black   motor power (connects to the other motor terminal)
Green   encoder GND
Blue  encoder Vcc (3.5 – 20 V)
Yellow  encoder A output
White   encoder B output
*/

//define Pins
#define ENCA 2
#define ENCB 3
#define PWM_MOTOR_FWD 9
#define PWM_MOTOR_RVS 10
//define useful acronyms
#define FWD 1
#define RVS -1
#define MAX_MOTOR_SPEED 200		//for automatic operation mode; do not exceed 255!
#define MIN_MOTOR_SPEED 20		//for operation mode: between 0 and 255

//global variables
int operation_mode = 0;			//0=select operation mode, 1=automatic operation, 2=manual operation
int target_pos = 0;
int current_pos = 0;

void setup() {
  //start serial communication
  Serial.begin(9600);
  //set direction of neccessary pins
  pinMode(ENCA,INPUT);
  pinMode(ENCB,INPUT);
  pinMode(PWM_MOTOR_FWD,OUTPUT);
  pinMode(PWM_MOTOR_RVS,OUTPUT);
  //configure interrupt that tracks the current position
  attachInterrupt(digitalPinToInterrupt(ENCA),isr_readEncoder,RISING);
  //TBD initialise HW Timer to be used to activate serial communication only every 333ms
}

void loop() {
  int serial_buffer = 0;
  //select operation mode
  switch(operation_mode){
	case 0: //look if operational mode has been specified
		Serial.println("Please select operation mode: (1= automatic, 2=manual)");
		while(Serial.available() == 0){
		delay(500);
		}
		if (Serial.available() > 0){
			serial_buffer = Serial.parseInt();
			//get rid of end line symbol
			Serial.read();
			if (serial_buffer == 1){
				operation_mode = 1;
				// report selection
				Serial.println("Automatic operation mode selected");
			}
			else{
				operation_mode = 2;
				// report selection
				Serial.println("Manual operation mode selected");
				Serial.println("Please select motor speed: (between -255 and 255)");
			}
		}
		delay(100);
		break;
    // Operation Mode 1 (automatic mode)
	case 1:   
		//calculate and set the control signal for the motor
		control_motor_automatically();
		//look if new target position has been received
		if (Serial.available() > 0){
			target_pos = Serial.parseInt();
			//get rid of end line symbol
			Serial.read();
		}
		//report current position
		Serial.print("Current Position: ");
		Serial.println(current_pos);
		delay(1000);
		break;
    // Operation Mode 2 (manual mode)
	case 2:
		if (Serial.available() > 0){
			serial_buffer = Serial.parseInt();
			//get rid of end line symbol
			Serial.read();
			control_motor_manually(serial_buffer);
			Serial.println("Please select motor speed: (between -255 and 255)");
		}
		//report current position
		Serial.print("Current Position: ");
		Serial.println(current_pos);
		delay(1000);
		break;
	}
}

void isr_readEncoder(){
  //interrupt service routine which is executed when a rising edge on ENCA is detected
  int b = digitalRead(ENCB);
  if(b>0){
	//if ENCB is high, the motor spins forward
    current_pos++;
  }
  else{
	//if ENCB is low, the motor spins backwards
    current_pos--;
  }
}

void control_motor_automatically()
{
  //define and initialize function variables
  int pos_error = 0;
  int abs_pos_error = 0;
  int motor_direction = FWD;
  int motor_speed = 0;
  //calculate mismatch between target position and current position
  pos_error = target_pos - current_pos;
  //define desired motor direction according to the position error
  if (pos_error >= 0){
	motor_direction = FWD;
  }
  else{
    motor_direction = RVS;
  }
  abs_pos_error = abs(pos_error);
  //define desired motor speed according to the absolute value of the position error
  if (abs_pos_error > MAX_MOTOR_SPEED){
	motor_speed = MAX_MOTOR_SPEED;
  }
  //below a motor speed of 20, the motor not expected to move, therefore ensure a minimum value of 20
  else if ( (abs_pos_error > 0) && (abs_pos_error < MIN_MOTOR_SPEED) ){
	motor_speed = MIN_MOTOR_SPEED;
  }
  else{
	motor_speed = abs_pos_error;
  }
  //send the calculated control signals to the motor
  set_motor_signals(motor_direction, motor_speed);
  
  //Debug prints
  Serial.print("Motor Direction: ");
  Serial.println(motor_direction, DEC);
  Serial.print("Motor Speed: ");
  Serial.println(motor_speed, DEC); 
}

void control_motor_manually(int user_input)
{
	int motor_direction = FWD;
	int motor_speed = 0;
    //define desired motor direction
    if (user_input >= 0){
	    motor_direction = FWD;
    }
    else{
	    motor_direction = RVS;
    }
    motor_speed = abs(user_input);
    //send the calculated control signals to the motor
    set_motor_signals(motor_direction, motor_speed);
    
    //Debug prints
    Serial.print("Motor Direction: ");
    Serial.println(motor_direction, DEC);
    Serial.print("Motor Speed: ");
    Serial.println(motor_speed, DEC);
}

void set_motor_signals(int direction, int speed)
{
  if (direction == FWD){
	analogWrite(PWM_MOTOR_RVS,0);
	analogWrite(PWM_MOTOR_FWD,speed);
  }
  else{
	analogWrite(PWM_MOTOR_FWD,0);
	analogWrite(PWM_MOTOR_RVS,speed);
  }
}