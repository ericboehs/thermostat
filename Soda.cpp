#include "application.h"
#include "Soda.h"

//===============================Constructor
Soda::Soda(){
	numeral[0] = 252; // B 11111100;  // 0
	numeral[1] = 96;  // B 01100000;  // 1
	numeral[2] = 218; // B 11011010; // 2
	numeral[3] = 242; // B 11110010; // 3
	numeral[4] = 102; // B 01100110; // 4
	numeral[5] = 182; // B 10110110; // 5
	numeral[6] = 190; // B 10111110; // 6
	numeral[7] = 224; // B 11100000; // 7
	numeral[8] = 254; // B 11111110; // 8
	numeral[9] = 230; // B 11100110; // 9
}

//===============================Destructor
Soda::~Soda() {

}

//===============================Pin Setup
void Soda::pins(int a, int b, int c, int d, int e, int f, int g, int dp) {

	segmentPins[0] = dp;
	segmentPins[1] = g;
	segmentPins[2] = f;
	segmentPins[3] = e;
	segmentPins[4] = d;
	segmentPins[5] = c;
	segmentPins[6] = b;
	segmentPins[7] = a;

    for(int i=0; i < 8; i++) {

        pinMode(segmentPins[i], OUTPUT);			//Set pins to output mode
        digitalWrite(segmentPins[i], HIGH);			//Initialize everything to be off (HIGH)
    }

}

//===============================Clear
void Soda::clear(){
    for(int i=0; i < 8; i++) {

        digitalWrite(segmentPins[i], HIGH);			//Sets everything to be off (HIGH)
    }
}

//===============================Write a Number
void Soda::write(int number) {
	boolean isBitSet;

	if(number >= 0 && number < 10){ 		//Check to make sure the input isn't out of the display range
		_number = number;

		for(int segment=1; segment < 8; segment++) {

			isBitSet = bitRead(numeral[number], segment);   //Check each bit value to be set
			isBitSet = ! isBitSet;				//Flip it (a low value lights up the LED)
			digitalWrite(segmentPins[segment], isBitSet);	//Write the value to that pin

		}

	}
	else if (number == '.') {				//Check for the decimal point
        digitalWrite(segmentPins[0], LOW);			//Turn the decimal point on
	}
	else if (number == ' ') {				//Check for decimal point clear
		digitalWrite(segmentPins[0], HIGH);		//Turn decimal point off
	}

}

//===============================Add one to the existing number
void Soda::increment(){
	if (_number == 9){		//Check to see if digit needs to loop back to zero
		_number=-1;
	}
	write(_number+1);
}

//===============================Subtract one from the existing number
void Soda::decrement(){
	if (_number == 0){		//Check to see if digit needs to loop back to nine
		_number = 10;
	}
	write(_number-1);
}
