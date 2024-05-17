## what is this
it's a script that:
1. Takes your right hand controller speed in the x-axis, and left hand height, and sends generic midi slider and pitch bend messages from those movement values respectively to a midi device of your choice.
2. Sends the middle C on and off messages to channel 1 when pressing any button on the right controller to the selected midi device.

To run it, you have to download vrlin.exe AND openvr_api.dll and make sure they're in the same folder.
It's midi output so you send it to a midi device like a synth, and that device playes the notes. If you want to send it as input to fl studio for example, you'd need a loopback midi device like [LoopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).

Feel free to ask for help if it's not working for you. Discord: asgrk

video for setting up the vrlin in fl studio: https://www.youtube.com/watch?v=D1H2mFHmTJM

## some other details

It doesn't override input so like if you're in the steam vr home you can still accidentally press stuff while you're using the vrlin. For me I just disabled steamvr home.

If you're using it with a loopback midi in fl studio, make sure "Send master sync" is disabled. I mean it should be by default, but make sure because Fl studio really doesn't like that. The VRlin doesn't have any transport control. See the "MIDI Output > Send master sync" section [here](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_midi.htm), it causes unpredictable behavior or something :(
## roughly how to set it up in fl studio

install [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html), which again just takes whatever midi output it's given and sends it as input, choose to send the vr midi output to this loopback midi, and in a program like fl studio, enable the loopback midi device's input ONLY (not output. That'll cause feedback).

Pressing anything on your right controller plays middle C.<br>
Releasing what you pressed releases the note

And then there's two motion controls: the "bow", and the "finger placement"

The "bow" is just the velocity on the x axis (I think lol. One of the axises) of the right controller while a button on the right controller is pressed. It controls midi controller #16. You could map it to control a level knob of something. You press a button on the right controller and wave it around and it'll change that controller based on the speed in um a direction. Idk I just try to find whatever direction affects the midi output the most, or find the direction that doesn't affect it and go perpendicular to that

The  "finger placement" is the height of the left controller while a button on the left controller is pressed. It controls pitch bend

So if I wanted to link the speed of the right controller to maybe a level knob in the Serum plugin: 

![Linking "bow" to level knob in fl studio](gifs/how%20to%20link%20vr%20midi.gif)

This is assuming you have the loopMIDI setup mentioned earlier
