#include <Arduino.h>
//#include <Bounce.h>
//#include <vector>

/* Tony Tap Tempo

   Turns on and off a light emitting diode(LED) connected to a digital
   pin, without using the delay() function.  This means that other code
   can run at the same time without being interrupted by the LED code.

http://www.sengpielaudio.com/calculator-bpmtempotime.htm
Calculation of the delay timet for a quarter note (crotchet) at the tempo b in bpm.
t = 1 / b. Therefore: 1 min / 96bpm = 60,000 ms / 96bpm = 625 ms.

The circuit:
 * LED attached from pin 13 to ground.
 * Note: on most Arduinos, there is already an LED on the board
 that's attached to pin 13, so no hardware is needed for this example.


 created 2005
 by David A. Mellis
 modified 8 Feb 2010
 by Paul Stoffregen

 This example code is in the public domain.


http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
*/

// constants won't change. Used here to set pin numbers:
// Pin 13: Arduino has an LED connected on pin 13
// Pin 11: Teensy 2.0 has the LED on pin 11
// Pin  6: Teensy++ 2.0 has the LED on pin 6
// Pin 13: Teensy 3.0 has the LED on pin 13
const int presetPin =  A0; //14;      // the number of the tap transistor pin
const int tapPin =  A1; //15;      // the number of the tap transistor pin
const int tapPin2 =  A2; //15;      // the number of the tap transistor pin
const int ledPin =  13;      // the number of the LED pin
const int ledPin2 =  9;      // the number of the LED pin
const int tempoLED = A3;    //the led on the outside of the enclosure
const int tempoButton = A5;    // set tempo button
const int presetButton = A4;    // set preset button
unsigned int tempoButtonState = 0; // true when button is pressed
unsigned int presetButtonState = 0; //true when button is pressed

byte midi_start = 0xfa;
byte midi_stop = 0xfc; 
byte midi_clock = 0xf8; //midi clock pulse
byte midi_continue = 0xfb;
int play_flag = 0; //current state of the track
byte serialData; //stores the message from midi presented over serial
unsigned int ppqn = 24; // zero indexed 24 pulses per quarter note. possibly configurable in the master clock?
unsigned int pulse = 0; //count the current midi pulses to find quarter notes every $ppqn

unsigned int tapping = 0;
unsigned int tapsSent = 0; //number of consecutive taps that have been sent
unsigned long tap_hold_ms = 50;     // if pedal reacts to tap start we can "toe up" anytime so use eighth notes just like my foot does
const unsigned int minViableQuarter = 200; //ms of fastest acceptable quarter note. prob shouldn't do this if we want slapback delays
unsigned int receivingTaps = 0; //true while tempo button is being used.
unsigned int processTaps = 0;  //flag to tell the loop to process a tap input from button or midi pulse
// Variables will change:
unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long previousTapButtonMillis = 0;        // will store last time LED was updated
unsigned long cummulativeButtonTapsMillis = 0;
unsigned int numButtonTaps = 0;

const unsigned int tapsToAverage = 3;
unsigned long tapIntervals[tapsToAverage] = {}; //create an array to store the last x tap button intervals initialized to zero
unsigned int tapIntervalsIndex = 0; //store current position we care about so we can loop only three array indexes

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long interval = 1000;           // interval at which to blink (milliseconds)
unsigned long previousInterval = 0;
unsigned long loopLatency = 0; //how many ms the loop takes to execute so we can adjust our delay timing to compensate

void setup() {
	//Serial.begin(31250); //this rate was in a midi example
	Serial.begin(38400); //this rate matches one of the options in the arduino serial monitor
	//noInterrupts(); //turn these off to make timing more consistent since we don't use them.

	// set the digital pin as output:
	pinMode(ledPin, OUTPUT);
	pinMode(ledPin2, OUTPUT);
	pinMode(tapPin, OUTPUT);
	pinMode(tapPin2, OUTPUT);
	pinMode(tempoLED, OUTPUT);
	pinMode(tempoButton, INPUT_PULLUP);
	pinMode(presetButton, INPUT_PULLUP);
	//attachInterrupt(digitalPinToInterrupt(tempoButton), processTapButton, LOW);
}

void loop()
{
	// here is where you'd put code that needs to be running all the time.

	// check to see if it's time to blink the LED; that is, if the
	// difference between the current time and last time you blinked
	// the LED is bigger than the interval at which you want to
	// blink the LED.
	unsigned long currentMillis = millis(); // record timestamp
	unsigned long deltaMillis = currentMillis - previousMillis; // ms since last quarter tap
	deltaMillis = deltaMillis * 1.022;  //compensate for calculated time diff from nova delay clock 22ms in 1000ms
	unsigned long deltaTapButtonMillis = 0;

	//int tapButtonValue = analogRead(tempoButton);
	processTaps = false;
	int tapButtonValue = digitalRead(tempoButton);   // read values from the tap tempo momentary switch
	if(tapButtonValue == LOW){
		receivingTaps = 1;
		processTaps = true;  
		pulse = 0; //use button press to reset midi pulse count
	} else if(Serial.available() > 0) {
		serialData = Serial.read();
		if(serialData == midi_start || serialData == midi_continue) {
			play_flag = 1;
			pulse = 0; //count next pulse as zero after start or stop
		} else if(serialData == midi_stop) {
			play_flag = 0;
			pulse = 0; //count next pulse as zero after start or stop
			//} else if((serialData == midi_clock) && (play_flag == 1)) {
	} else if(serialData == midi_clock) {
		//do stuff, maybe we don't care about play flag...
		if(pulse >= ppqn){
			pulse = 0; // when pulse is 24 we restart the 0-23 interation pulse counter
			processTaps = true;  
		} 
		pulse++;
	}
	} 

	deltaTapButtonMillis = currentMillis - previousTapButtonMillis; // ms since last quarter tap

	if(processTaps == true){
		if(tempoButtonState != 1 && tapping != 1){
			//initial press
			tempoButtonState = 1; //button is pressed
			//Serial.println(deltaTapButtonMillis);

			if(deltaTapButtonMillis > minViableQuarter){
				if(deltaTapButtonMillis < 3000){
					//use an array to store and average previous tap intervals
					tapIntervalsIndex = tapIntervalsIndex % tapsToAverage;
					tapIntervals[tapIntervalsIndex++] = deltaTapButtonMillis; //add the current interval to the back of the vector storage
					cummulativeButtonTapsMillis = 0;
					numButtonTaps = 0;
					//Serial.print("\n");
					for(int i=0; i<tapsToAverage; i++) {
						if(tapIntervals[i] != 0){
							cummulativeButtonTapsMillis += tapIntervals[i];
							numButtonTaps++;
							//Serial.print(i);  Serial.print(". "); 
							Serial.print(tapIntervals[i]);    Serial.print(" + ");
						}
					}
					if(numButtonTaps > 0){
						interval = cummulativeButtonTapsMillis/numButtonTaps;
					}

					Serial.print(" = "); Serial.print( cummulativeButtonTapsMillis ); 
					Serial.print(" / "); Serial.print( numButtonTaps ); Serial.print(" = ");  Serial.println(interval);

				} else {
					Serial.print("\n deltaTapButtonMillis was greater than 3000, resetting tapIntervals to zeros \n");
					for(int i=0; i<tapsToAverage;i++){ tapIntervals[i] = 0; } //reset intervals array
					numButtonTaps = 0; //reset
					receivingTaps = 0;
				}

				previousTapButtonMillis =  currentMillis;
				//Serial.println(interval);
			}
		}
	} else {
		if(tempoButtonState == 1){
			tempoButtonState = 0;
			//Serial.println(tempoButtonState);
		}
		//if we've allowed time for at least 3 taps, allow tapSend to happen.
		if(deltaTapButtonMillis > (3 * interval)){
			receivingTaps = 0; 
		}
	}
	//if(tapping == 1 && deltaMillis > tap_hold_ms) {
	if(deltaMillis > tap_hold_ms) {
		//release tap
		digitalWrite(tapPin, LOW);
		digitalWrite(tapPin2, LOW);
		digitalWrite(ledPin, LOW);
		digitalWrite(tempoLED, LOW);
		tapping = 0;
	}
	if(deltaMillis >= interval) {
		previousMillis = currentMillis;     // save the last time you tapped the tempo
		if(receivingTaps == 0 && interval != previousInterval){    
			tapsSent = 0; 
			//Serial.println(millis());
			Serial.print("new interval: "); Serial.print(interval); 
			Serial.print("ms. "); Serial.print(ms2bpm(interval)); Serial.print("bpm. ");
			previousInterval = interval;
		}
		digitalWrite(ledPin, HIGH);  //flash led in tempo
		digitalWrite(tempoLED, HIGH);  //flash outer led
		if(tapsSent <= tapsToAverage){
			// begin tap down
			digitalWrite(tapPin, HIGH);
			digitalWrite(tapPin2, HIGH);
			tapsSent++;
			tapping = 1;
			Serial.print(" * ");
			//Serial.println(millis()/interval);
			//Serial.println(loopLatency);
		}
	}
	/*
	   int presetButtonValue = digitalRead(presetButton);
	//Serial.println(presetButtonValue);
	if(presetButtonValue < 50){
	if(presetButtonState != 1){
//initial press
presetButtonState = 1; //button is pressed
Serial.println(presetButton);
}
}
else if(presetButtonState == 1){
presetButtonState = 0;
//Serial.println(presetButtonState);
}
*/
loopLatency = millis() - currentMillis;
}

unsigned int ms2bpm(int ms){
	return ( 60000 / ms );
}
//unsigned int bpm2ms(int bpm){
//  return( bpm * 60 / 1000 );
//}

