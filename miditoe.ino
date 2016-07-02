int counter = 0;
int output_pin = 40; // set output pin
byte midi_start = 0xfa;
byte midi_stop = 0xfc; 
byte midi_clock = 0xf8; //midi clock pulse
byte midi_continue = 0xfb;
int play_flag = 0; //current state of the track
byte data;
int pulse = 0; // track the midi PPQ pulses aka ticks
// for now we just assume midi tempo has 24 pulses (aka ticks) per quarter note
int last_pulse = 23; // zero indexed 24 pulses per quarter note. possibly configurable in the master clock?
int eighth_note_pulse = last_pulse/2;

void setup() {
  Serial.begin(31250);
  pinMode(output_pin, OUTPUT);
}

void loop() {
  if(Serial.available() > 0) {
    data = Serial.read();
    
    if(data == midi_start || data == midi_continue) {
      play_flag = 1;
    } else if(data == midi_stop) {
      play_flag = 0;
      pulse = 0; //restart pulse counting when the music stops
    } else if((data == midi_clock) && (play_flag == 1)) {
      SyncTap();
    }

    /*
    else if(data == program_change){
      //might need multiple chunks of data
      SwitchToPreset(program_change_value);   
    } else {
      //log unsupported data message type
    }
    */

  }
}

void SyncTap() {
  if(pulse == 0) {
    digitalWrite(output_pin, HIGH); // send a tap via the transistor
  } else if(pulse == eighth_note_pulse) {
    // possibly one pulse isn't long enough for the pedal to register a tap
    // if pedal reacts to tap start we can "toe up" anytime so use eighth notes just like my foot does
    digitalWrite(output_pin, LOW); //stop the tap
  } 
  if(pulse < ppqn) {
    pulse = pulse + 1;
  } else {
    pulse = 0;
  }
}

