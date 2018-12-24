// last mod 23/12/2018 - Alan Smith

/*
Program size: 8,168 bytes (used 100% of a 8,192 byte maximum)
Minimum Memory Usage: 408 bytes (80% of a 512 byte maximum)
 */

#include <SendOnlySoftwareSerial.h>
SendOnlySoftwareSerial midiSerial(0); // Tx PB 0 pin 5

const byte
lightSensitivity = 1, // the lower the more sensitive the light change detection;
arraySizeSet = 32,
chordArraySizeSet = 8,
keyArraySizeSet = 8,
channels = 16,
highHat = 42,
kick = 35,
snare = 38,
drumChan = 9,
changeRatePin = 3,
tempoPin = 2,
LDRPin = 1;

const unsigned int scales[10] = {
	0xAD5AU,
	0xB5ABU,
	0x9729U,
	0xB6DBU,
	0xA94AU,
	0xAADAU,
	0xB55BU,
	0xCDACU,
	0xB59BU,
	0xDDADU
};

/*
Here are how chords are arranged

C C#D D#E F F#G G#A A#B C C#D D# Chord HEX Value
1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 Note 8000
1 0 0 0 0 1 0 1 0 0 0 0 0 0 0 0 Sus 8500
1 0 0 0 0 1 0 1 0 0 1 0 0 0 0 0 7Sus 8520
1 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0 Aug  8880
1 0 0 0 1 0 0 1 0 0 0 0 0 0 0 0 Maj  8900
1 0 0 0 1 0 0 1 0 0 0 0 1 0 0 0 Maj 9th 8908
1 0 0 0 1 0 0 1 0 0 0 1 0 0 0 0 Maj 7th 8910
1 0 0 0 1 0 0 1 0 0 1 0 0 0 0 0 7th  8920
1 0 0 0 1 0 0 1 0 1 0 0 0 0 0 0 6th  8940
1 0 0 1 0 0 0 1 0 0 0 0 0 0 0 0 Min  9100
1 0 0 1 0 0 0 1 0 0 1 0 0 0 0 0 Min 7th 9120
1 0 0 1 0 0 0 1 0 1 0 0 0 0 0 0 Min 6th 9140
1 0 0 1 0 0 1 0 0 0 0 0 0 0 0 0 Dim  9200
1 0 0 1 1 0 0 1 0 1 1 1 0 0 0 0 Const 9970
1 0 1 0 0 0 0 1 0 0 0 0 0 0 0 0 Sus2 A100
1 0 1 0 0 0 0 1 0 0 1 0 0 0 0 0 7Sus2 A120
1 0 1 1 0 0 0 1 0 0 1 0 1 0 0 0 Min 9th B128
*/

const unsigned int chords[15] = {
	0x8900U, 0x8940U, 0x8920U, 0x8910U, 0x8908U, //maj chords
	0x9100U, 0x9140U, 0x9120U, 0xB128U, // min chords
0x9200U, 0x8880U, 0xA100U, 0x8500U, 0xA120U, 0x8520U // others
};// , chord = 0x8520U;   

unsigned int
loopCount = 0, tickCount = 0, tickCount2 = 0, straightCount = 0, scale = scales[0];

int
beat = 1, playControl = 0, tuneSpeed = 0, tuneSpeed2 = 0, lightLevel = 0, randomSpeed = 0, oldLightLevel = 0;

unsigned long
bassPatt = 0, pianoPatt = 0, windPatt = 0, synthPatt = 0, kickPatt = 0, hhPatt = 0,
snarePatt = 0, notePatt = 0, sc = 0, chordPatt = 0, tickTime2 = 0, tickTime = 0;

byte
arraySize = arraySizeSet, keyArraySize = keyArraySizeSet, chordArraySize = chordArraySizeSet,
articnote = 0,
filter1 = 64,//cutoff frequency of VCF
flr1 = 10,   //filter lower range
fur1 = 100,  //filter upper range
filter2 = 64,//cutoff frequency of VCF
flr2 = 10,   //filter lower range
fur2 = 100,  //filter upper range
filter3 = 64,//cutoff frequency of VCF
flr3 = 10,   //filter lower range
fur3 = 100,  //filter upper range
rcc = 0,    //random articulation channel
bassChan = 0, //midi chan 1
bassNote = 0, pianoIndex = 0, chordIndex = 0,
pianoChan = 3, //midi chan 4
pianoNote = 0, pianoChord = 0, pianoChordIndex = 0, pianoChordType = 0, bassIndex = 0, windNote = 0, windIndex = 0,
windChan = 1,  // midi chan 2
synthChan = 2, // midi chan 3
synthNote = 0, synthNoteM = 0, tickNote = 0, tickNoteM = 0, tickChan = 3, lightNote = 0, lightNoteM = 0,
lightChan = 11, keyIndex = 0, changeCount = 0, lightIndex = 0, ta = 0, ta2 = 0, tc = 0, cc = 0,
llp = 0, //light level pattern type pointer
abeat = 1, bbeat = 1, wbeat = 1, drum = 0;
char keys[keyArraySizeSet];
byte pianoArray[arraySizeSet];
byte bassArray[arraySizeSet];
byte windArray[arraySizeSet];
byte chordArray[chordArraySizeSet];
byte tv[channels];
char  key = 0, kn = 0, bassDir = 0, filterDir1 = 1, filterDir2 = 1, filterDir3 = 1, llc = 1, lc = 0;

void setup() {
	pinMode(2, INPUT); //the tuneSpeed speed pot - attiny85 PB2 pin 7
	pinMode(3, INPUT); //the note/lightLevel offset pot - attiny85 PB3 pin 2
	pinMode(1, OUTPUT);//output for the beat/activity LED - attiny85 PB1 pin 6
	pinMode(4, INPUT); //the LDR input for the lightLevel - attiny85 PB4 pin 3
	midiSerial.begin(31250); // Start serial port at the midi 31250 baud - out on attiny85 PB0 pin 5
	gsReset();  // reset the Boss DR-330 synth and switch to multitimberal mode
	delay(1000); //GS Reset needs a delay 


	//set up channel stuff
	for (byte x = 0; x < channels; x++) {
		CC(x, 123, 0);  //all notes off
		//CC(x, 121, 0);  //reset controllers
		//ProgChange(x, random(119));
		makeChanges();
		//keys[x] = x;
		randomSeed(analogRead(LDRPin));
	}
	ProgChange(drumChan, rp() % 7 * 8);
	randomSpeed = rp() % 10 + 3;
	//playControl = random(0xFFFFU);
}



void loop() {
	//reset light change comparison
	oldLightLevel = lightLevel;

	//check the next light change //read the LDR value smoothed by map 
	lightLevel = analogRead(LDRPin);//map(analogRead(LDRPin), 0, 1023, 0, 256);//
	llc = (lightLevel - oldLightLevel);   //light level change - brighter is +ve,  darker is -ve

//the main musical timed tempo based event loop (fast times)
	if (millis() > tickTime2) {
		tuneSpeed = (unsigned long)(map(analogRead(tempoPin), 0, 1023, 4, 860) + randomSpeed);  // 4/4 tempo control
		tuneSpeed2 = (unsigned long)((tuneSpeed >> 1) - (tuneSpeed / (beat << 2)));//// tempo control fast beats and syncopation 

		//various loop pointers
		ta = (byte)(byte(tickCount) % arraySize);  // dynamic
		ta2 = (byte)(byte(tickCount2) % arraySizeSet); // dynamic
		sc = (byte)(byte(straightCount) % arraySizeSet); // static // main counter for periodic changes
		kn = keys[keyIndex % keyArraySize];  //dynamic // defines the key changes from the array of key changes

		tmv();  // adjust track mixer volumes

		if (abs(llc) > (char)lightSensitivity) {   // if it changes above or below the threshhold...
			analogWrite(1, abs(llc) * 30);   // indicate with LED the change
			chordIndex++;
			if (llc > 0) {  // positive
				chordArray[chordIndex % chordArraySize] = tr();// major chords //update the chord array pointer
				bassDir = 1;
				keys[keyIndex % keyArraySize] ++;
			}
			else {  // must be -ve
				chordArray[chordIndex % chordArraySize] = tr() + 5;// random(5, 10); // minor and other chords //update the chord array pointer
				bassDir = -1;
				keys[keyIndex % keyArraySize] --;
			}
			lc = llc;

		}

		if (millis() > tickTime) {
			// MIDI NOTES SOUNDED FROM THE PATTERNS AND ARRAYS //
			// DRUMS lead the way
			// high hats

			//playControl = 0x0200u;   // diagnostics

			if ((bitRead(playControl, 0) && (hhPatt >> ta2 % 32ul & 1))) {
				drum = highHat + (pianoArray[ta2] % 3 * 2) * 3;
				NoteOn(drumChan, drum, hr());
				NoteOff(drumChan, drum);
			}
			// snare
			if (bitRead(playControl, 2) && (snarePatt >> sc % 32ul & 1)) {
				drum = snare + ((beat == 1) ? ((windArray[sc] % 3) * 4) : (bassArray[ta] % 3) * 4);
				NoteOn(drumChan, drum, ((sc % (7 + beat) == 0) ? 120 : lr()));
				NoteOff(drumChan, drum);
				//rpan(bassChan, channels);
			}
			else if (bitRead(playControl, 3) && ta2 % 4 == 0) {  // snare straight pattern
				drum = snare + windArray[ta2] % 2;
				NoteOn(drumChan, drum, hr());
				NoteOff(drumChan, drum);
			}
			// kick  
			if ((ta % (beat + 3) == 0) || (bitRead(playControl, 4) && (kickPatt >> sc % 32ul & 1))) {
				drum = kick + pianoArray[sc] % 2;
				NoteOn(drumChan, drum, hr());
				NoteOff(drumChan, drum);
			}

			//others end loop fills
			else if ((ta2 > (arraySize - 4))) {
				drum = kick + ((lightLevel + bassArray[sc]) % 24);
				NoteOn(drumChan, drum, ((ta % (2 + beat) == 0) ? 120 : hr()));
				NoteOff(drumChan, drum);
				//makeChanges();
				//playControl++;
			}

			// NOW THE FIVE INSURUMENT PLAYERS  
			tr() ? 0 : pianoChordIndex++;
			//PIANO - ch 4
			if ((sc % ((unsigned long)(beat + 7)) == 0) || (bitRead(playControl, 6) && (pianoPatt >> ta % 32ul & 1))) {  // 
				playChord(chords[pianoChord], pianoChan, pianoNote, 0, 0, pianoChordType);
				pianoNote = kn + ScaleFilter(scale, 36 + ((pianoArray[(pianoIndex += beat) % arraySize] += cllc(lc)) % 48), kn);
				pianoChord = chordArray[pianoChordIndex % chordArraySize]; //(pianoIndex) 
				pianoChordType = rp();//
				playChord(chords[pianoChord], pianoChan, pianoNote, hr() - 10, 1, pianoChordType); //**************NOTE ON*********************
			}
			//BASS - ch 1
			if ((bitRead(playControl, 7)) && ((sc % ((unsigned long)(bbeat << 1)) == 0) || (bassPatt >> ta2 % 32ul & 1))) {  //
				DoFilter(bassChan, 90, filter1);  // (pianoNote % 12)*filterDir2 +
				NoteOff(bassChan, bassNote);
				bassNote = kn + ScaleFilter(scale, (24 + ((ta2 % keyArraySize)*bassDir) +
					(bassArray[(bassIndex += bbeat) % arraySize] += cllc(lc)) % 36), kn);
				NoteOn(bassChan, bassNote, hr()); //**************NOTE ON*************
				filter1 += filterDir1;
				if (filter1 >= fur1) {
					filterDir1 = -1;
					flr1 = lr();
				}
				if (filter1 <= flr1) {
					filterDir1 = 1;
					fur1 = hr();
				}
			}

			// Filter2 and other per beat updates...
			filter2 += filterDir2;
			if (filter2 >= fur2) {
				filterDir2 = -1;
				flr2 = lr();
			}
			if (filter2 <= flr2) {
				filterDir2 = 1;
				fur2 = hr();
				//keyIndex++;
			}
			if (playControl > 0x0400) {
				playControl++;
			}
			DoFilter(windChan, 90, filter2); // for ds330 
			CC(bassChan, channels, filter1); // controller send
			CC(synthChan, channels, filter2); // controller send
			CC(tickCount % 16, 123, 0);  // snc - stuck note control :)
			tickCount++;//  += beat;//

			analogWrite(1, 0);  // turn off the light change LED
			tickTime = (unsigned long)(millis() + (tuneSpeed));
		} // end of timed loop1 if TuneSpeed1 - the slow parts

		if ((!(bitRead(playControl, 0)) && bitRead(playControl, 1) && ta2 % (beat + 1) == 0)) {  // hh straight pattern
			drum = highHat + (bassArray[ta2] % 3) * 2;
			NoteOn(drumChan, drum, hr());
			NoteOff(drumChan, drum);
		}

		if ((!(bitRead(playControl, 4))) && bitRead(playControl, 5) && ta2 % 8 == 0) {  // kick straight pattern 
			drum = kick + pianoArray[ta2] % 2;
			NoteOn(drumChan, drum, hr());
			NoteOff(drumChan, drum);
		}

		//WIND - ch 2
		if ((bitRead(playControl, 8)) && ((sc % ((unsigned long)(wbeat << 2)) == 0) || (windPatt >> sc % 32ul & 1))) {//
			NoteOff(windChan, windNote); //
			windNote = kn + ScaleFilter(scale, 36 +
				((windArray[(windIndex += wbeat) % arraySizeSet] += ((pianoNote + 7) % 12)) % 50), kn);
			NoteOn(windChan, windNote, hr());  //*******************NOTE ON*********************
		}

		//TICKNOTE run notes
		if ((bitRead(playControl, 10)) && (((~kickPatt)) >> ta2 % 32ul & 1)) { // 
			tickNote = ScaleFilter(scale, kn + 48 +
				byte((bassNote % chordArraySize) + ((((ta2) % chordArraySize)*filterDir2) + (pianoNote % 12)) % 60), kn);
			if (tickNote != tickNoteM) {
				NoteOff(tickChan, tickNoteM);
				tickNoteM = tickNote;
				DoFilter(tickChan, 90, filter2); //
				NoteOn(tickChan, tickNote, hr());//**************NOTE ON*********************
			}
		}

		////Fast LIGHTNOTE
		if (bitRead(playControl, 11) || llc != 0) {
			lightNote = ScaleFilter(scale, kn + 48 + (((ta*filterDir2) + lightLevel) % 60), kn);
			if (lightNote != lightNoteM && (scale << 16ul + chords[pianoChord] >> ta2 % 32ul & 1)) { //  
				NoteOff(lightChan, lightNoteM);
				lightNoteM = lightNote;
				NoteOn(lightChan, lightNote, hr());
			}
		}

		//SYNTH - ch 3   scale based melodies
		if ((bitRead(playControl, 9)) && (((~snarePatt) | synthPatt) >> ta2 % 32ul & 1)) { // (byte(sc % 4) == 0) ||  
			synthNote = ScaleFilter(scale, kn + (48 + (((windArray[ta]) % 12)*filterDir2
				+ ((bassArray[ta]) % 12)
				+ filter3 + ((pianoArray[pianoIndex % arraySize]) % 12)) % 48), kn);
			if (synthNote != synthNoteM) {
				NoteOff(synthChan, synthNoteM);
				synthNoteM = synthNote;
				NoteOn(synthChan, synthNote, hr());//**************NOTE ON*********************
			}
		}

		straightCount++;
		tickCount2 += abeat;//++;//+= (pianoNote % beat);//
		tickTime2 = (unsigned long)(millis() + (tuneSpeed2)); //  the other Time

	}  // end of fast loop if - tunespeed 2 - the fast parts

   //PERIODIC CHANGES/////////
	if (straightCount % (unsigned int)(analogRead(changeRatePin)) == 0) {  // just a whole load of complicated stuff done about 10 times!
		if (!rp()) { straightCount++; }	// to reduce multiple changes
		//DCB();
		makeChanges();
		//straightCount++;
	}
	if (straightCount % (unsigned int)(keyArraySize << 4) == 0) {  // key change
		keyIndex++;
	}
	// fast filter sweeps
	if (loopCount % (unsigned int)(keyArraySize << 4u) == 0) {
		//DCB();
		filter3 += filterDir3;
		if (filter3 >= fur3) {
			filterDir3 = -1;
			flr3 = lr();
		}
		if (filter3 <= flr3) {
			filterDir3 = 1;
			fur3 = hr();

		}
		CC(randomChan(), rp() % 22 + channels, filter3); // controller send
		DoFilter(synthChan, 90, filter3); // fast filter for ds330
	}

	loopCount++;
}  // end of megga fast loop

///////////////////////////// macros and helpers /////////////////////////////////

void DCB() { // diagnostic cowbell (ride cymbal)
	NoteOn(drumChan, 59, 120); NoteOff(drumChan, 59);
}


char cllc(char check) {  //check light level change 
	char cll = 0;
	(check > 0 ? cll = 1 : check < 0 ? cll = -1 : cll = 0);
	return cll;
}

void tmv() {
	//track mix volumes
	rcc = sc % 16;
	(tv[rcc] >= 40 && tv[rcc] <= 70) ? (tv[rcc] += (tr() - 1)) : (tv[rcc] = 65);//
	CC(rcc, 7, rcc == 9 ? tv[rcc] - 10 : tv[rcc]);  // keep drum volumes down 10 behind rest of instruments
}


byte randomChan() {
	byte rc = rp() % channels;
	while (rc == drumChan) { // rc < 4 || 
		rc = rp() % channels;
	}
	return rc;
}

void makeChanges() {

	digitalWrite(1, 1);
	changeCount++;

	CC(changeCount % 16, 123, 0);  // snc - stuck note control :)
	llp = byte(analogRead(LDRPin) % 3);  // 0, 1 or 2 read light level and use it to power the kind of pattern produced
	switch (random(48)) {  // slightly oversubscribed to allow for build up
	case 0:
		chordArray[chordIndex % chordArraySize] = rp() % 15;  // all available chords
		break;
	case 1:
		ProgChange(tickChan, random(96));
		break;
	case 2:
		pianoPatt = randomPatt(llp);
		break;
	case 3:
		snarePatt = randomPatt(llp);
		break;
	case 4:
		hhPatt = randomPatt(llp);
		break;
	case 5:
		playControl = random(0xFFFFU);
		break;
	case 6:
		ProgChange(bassChan, rp() + 33); //
		break;
	case 7:
		ProgChange(pianoChan, lr() + rp());
		break;
	case 8:
		ProgChange(windChan, lr() + lr());//random(56, 95)
		break;
	case 9:
		ProgChange(drumChan, rp() % 7 * 8);
		break;
	case 10:
		windPatt = randomPatt(llp);
		break;
	case 11:
		synthPatt = randomPatt(llp);
		break;
	case 12:
		playControl++;
		break;
	case 13:
		keyArraySize = random(4, keyArraySizeSet); // 1 to ArraySize (8)	
		break;
	case 14:
		chordArraySize = random(4, chordArraySizeSet); // 1 to ArraySize (8)
		break;
	case 15:
		randomSpeed += (random(11) - 5);
		break;
	case 16:
		scale = scales[random(10)]; // all available scales
		break;
	case 17:
		playControl = ~playControl;  // invert
		break;
	case 18:
		bassPatt = randomPatt(llp);
		break;
	case 19:
		kickPatt = randomPatt(llp);
		break;
	case 20:
		NoteOff(tickChan, tickNoteM); //need to stop current note
		tickChan = randomChan();   //because we change the channel
		break;
	case 21:
		arraySize = random(8, arraySizeSet + 1); // 
		break;
	case 22:
		NoteOff(synthChan, synthNoteM);
		synthChan = randomChan();
		ProgChange(synthChan, lr() + hr()); // random(58, 96)
		break;
	case 23:
		abeat = rp() % 8 + 1;
		break;
	case 24:
		NoteOff(windChan, windNote); //need to stop current note
		windChan = randomChan();   //because we change the channel
		break;
	case 25:
		playControl = 1;
		break;
	case 27:
		bassPatt |= 1 << rp();
		break;
	case 28:
		windPatt |= 1 << rp();
		break;
	case 29:
		NoteOff(lightChan, lightNoteM); //need to stop current note
		lightChan = randomChan();   //because we change the channel
		break;
	case 30:
		arraySize = random(16, arraySizeSet + 1); // 8 to 31
		break;
	case 31:
		beat = tr() + 1;  //random(arraySize >> 1) + 1;// (lr() % 14) + 1;  //
		break;
	case 32:
		bbeat = tr() + 1;  //random(arraySize >> 1) + 1;//
		break;
	case 33:
		wbeat = tr() + 1;  //random(arraySize >> 1) + 1;//tr() + 1;  //
		break;
	case 34:
		scale = llc > 0 ? scales[0] : llc < 0 ? scales[1] : scales[rp() % 10]; // minor if darker major if brighter or all available scales
		break;

	default:
		playControl |= (1 << (rp() % 16)); // build up players
		CC(randomChan(), 1, rp());  //slight modulation
		CC(randomChan(), 5, tr() << 1);  //portamento time
		CC(randomChan(), 65, wr());  //portamento on / off
		CC(randomChan(), 0x5B, wr()); // reverb send
		CC(randomChan(), 0x5D, wr()); // chorus send
		rpan(randomChan(), lr());
		rpan(bassChan, channels);
		DoArticulations();
		scale = scales[changeCount % 10];
		break;
	}/// end of switch / case
}



byte tr() {
	return random(3);
}


byte wr() {  // wide range random values for asdr and filter settings
	return random(0x0e, 0x72);
}

void rpan(byte chan, byte scope) {  // random pan 
	CC(chan, 10, random(64 - scope, 64 + scope));
}

byte hr() {  // random high values for settings
	return random(65, 127);
}

byte lr() {  // random low values for settings
	return random(0, 64);
}

byte rp() {  // random bit pointers
	return random(32);
}

unsigned long randomPatt(byte r) { // random long patterns formed from nibbles, bytes or ints :)
	unsigned long n = random(1, 0xFFFFU);
	unsigned int y = random(1, 0xFFFFU);
	if (r == 0) {
		n = (((n & 0x0F) << 28) + ((n & 0x0F) << 24) + ((n & 0x0F) << 20) +
			((n & 0x0F) << 16) + ((n & 0x0F) << 12) + ((n & 0x0F) << 8) +
			((n & 0x0F) << 4) + (n & 0x0F));
	}
	else if (r == 1) {
		n = (((n & 0xFF) << 24) + ((n & 0xFF) << 16) + ((n & 0xFF) << 8) + (n & 0xFF));
	}
	else {
		n = ((n << 16) + y);
	}
	return n;
}



// send note on stuff
void NoteOn(byte chan, byte note, byte vel) {
	if (note > 23) {
		midiSerial.write((chan + 0x90));
		midiSerial.write(note & 0x7F);
		midiSerial.write(vel & 0x7F);
	}
}
// send note off stuff
void NoteOff(byte chan, byte note) {
	//chan &= 0x0F;
	midiSerial.write((chan + 0x80));
	midiSerial.write(note & 0x7F);
	midiSerial.write(byte(0));
}

//real filterting
//byte ScaleFilter(unsigned int s, byte n, char k) {
//	if (bitRead(s, 15u - (((n % 12u) + k) % 12))) {
//		return n;
//	}
//	else {  
//		return 0;
//	}
//}

//// scale correction - the input note is moved until it belongs to the current scale
//byte ScaleFilter(unsigned int s, byte n, char k) {
//	while (bitRead(s, (15u - ((n % 12u + k) % 12u))) == 0u) {
//		n++;
//	}
//	return n;
//}

// scale filtering - passed if the note belongs to the current scale, else grab the lightlevel and make it a current chord note
byte ScaleFilter(unsigned int s, byte n, char k) {
	if (bitRead(s, 15u - (((n % 12u) + k) % 12))) {
		return n;
	}
	else {  // play chord notes
		s = chords[pianoChord];
		n = 24 + (analogRead(LDRPin) % (rp() << 1));
		while (bitRead(s, (15u - (((n % 12u) + k) % 12u))) == 0u) {
			n++;
		}
		return n;
	}
}

//chord plays the scale corrected chord
void playChord(unsigned int cord, byte chan, byte note, byte vel, byte cont, byte type) {
	// cont is either play or kill
	//chan &= 0x0f;
	for (byte c = 0; c < 15; c++) {
		if (bitRead(cord, 15u - c)) { // && ((cord << c) & 0x8000U) > 0U
			//delay(100);
			if (cont) {
				//if (bitRead(scale << 4, 15 - ((note + c) % 12))) {
				NoteOn((type ? chan : (chan + c) % 16), ScaleFilter(scale, note + c, kn), vel); //note + c
			//}
			}
			else {
				//CC(type ? chan : ((chan + c) % 16), 123, 0);
				NoteOff((type ? chan : (chan + c) % 16), ScaleFilter(scale, note + c, kn)); //note + c
			}
			//if (cord == 0x8000U) { // no point going any further
			//	break;
			//}
		}
	}
}

//specificaly NRPN for BOSS DR-Synth 330
void MasterTune(byte chan, byte b) {
	//chan &= 0x0F;
	CC(chan, 0x65, 0);
	CC(chan, 0x64, 2); // should be 2 - 0 for pitch bend range
	CC(chan, 6, b);
}

//Basic Channel contro message
void CC(byte chan, byte cont, byte val) {
	//chan &= 0x0F;
	midiSerial.write((chan + 0xB0));
	midiSerial.write(cont & 0x7F);
	midiSerial.write(val & 0x7F);
}

//specificaly NRPN for BOSS DR-Synth 330
void DoFilter(byte ch, byte res, byte coff) {
	//ch &= 0x0F;
	CC(ch, 0x63, 0x01);
	CC(ch, 0x62, 0x21);
	CC(ch, 6, res & 0x7F); //resonance can go to 0x72
	CC(ch, 0x63, 0x01);
	CC(ch, 0x62, 0x20);
	CC(ch, 6, coff & 0x7F);//cut off frequency
}

void DoArticulations() {  // for Kontact
	articnote = random(23, 127);
	rcc = randomChan();
	NoteOn(rcc, articnote, 1);
	NoteOff(rcc, articnote);
}

//specificaly sysex for BOSS DR-Synth DS330
void gsReset() {
	byte gs[11] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
	for (byte g = 0; g < 11; g++) {
		midiSerial.write(gs[g]);
	}
}

// Program change for midi channel
void ProgChange(byte chan, byte prog) {
	//chan &= 0x0F;
	midiSerial.write((chan + 0xC0));
	midiSerial.write(prog & 0x7F);
}