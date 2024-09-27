## what is this
it's a script that sends midi messages based on some VR movement. Speech2.exe is the most updated one. You need Speech2.exe and openvr_api.dll in the same directory to run it.
Position data is relative to an offset you can set via either running it with 3 arguments (x y z offsets) or, if you start it without any arguments, it'll have you position your left hand where you want 0,0,0 to be and then press any button on your right controller. It'll print out those offset values so you can run the program with those same values later.

The speed of your right hand in the x-axis is sent through ctrl 16, a generic MIDI slider. If you link that to the volume knob of a synth, it kind of plays like a bow on a violin. Left hand height controls pitch bend, so maybe more like a cello :)

The left hand x and z positions (relative to the offset) are also sent as midi messages (ctrl 17 and 18 respecitvely). I've been linking those to [Kiilohearts' formant filter](https://kilohearts.com/products/formant_filter). If you run a sawtooth wave through that with the "highs" and "lows" options unchecked, you can make pretty vowel-y noises. Having a band-pass filter on the sawtooth before the formant filter I think makes it sound a little better. Control 19 is set to 1 if a button on the left controller is pressed, and 0 if it isn't. I use that to change from playing a sawtooth wave to noise, still ran through the formant filter, it can almost sound like consonants "s" or "f". I think serum's "h-breath" noise generator works well for this.

### Midi messages:
- Pressing any button on the right hand playes a C. The octave depends on the height of your right hand.
- Right hand speed in x-axis => generic controller 1 (ctrl 16)
- Left hand height => generic controller 2 (pitch bend)
- Left hand x position (relative to offset) (ctrl 17)
- Left hand z position (relative to offset) (ctrl 18)
- Left controller button down (0 if not down, 1 if down) (ctrl 19)

It's midi output so you send it to a midi device like a synth, and that device playes the notes. If you want to send it as input to fl studio for example, you'd need a loopback midi device like [LoopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).

Feel free to ask for help setting it up, or if it's not working for you. Discord: asgrk

## some other details

It doesn't override input so like if you're in the steam vr home you can still accidentally press stuff while you're using the vrlin. For me I just disabled steamvr home.

If you're using it with a loopback midi for fl studio, make sure "Send master sync" is disabled. I mean it should be by default, but make sure because Fl studio really doesn't like that. The VRlin doesn't have any transport control. See the "MIDI Output > Send master sync" section [here](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_midi.htm), it causes unpredictable behavior or something :(, especially for a loopback I'm pretty sure it starts a feedback loop.
