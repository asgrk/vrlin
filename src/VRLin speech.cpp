// um I guess windows defined a min and max but then c++ did and now you have to like undefine it if you want to use <windows.h>
#define NOMINMAX

// midiout.cpp

#include <iostream>
#include <cstdlib>
#include "RtMidi.h"

#include <windows.h>
#include "openvr.h"
#include <math.h>

#include <string>

#include <chrono>
#include <thread>

#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

// sends a midi message at a good rate.
using namespace std::chrono_literals;

#define SEND_MIDI()                                                         \
    {                                                                       \
        std::this_thread::sleep_until(next_midi_message_time);              \
        next_midi_message_time = std::chrono::steady_clock::now() + 1000us; \
        midiout->sendMessage(&message);                                     \
    }

int main(int argc, char *argv[])
{
    auto next_midi_message_time = std::chrono::steady_clock::now();

    float offset[3] = {0.0, 0.8, 0.0};
    bool offset_is_set = false;
    // um strtof and atof don't like let you know if they didn't enter a number, they just return 0.0 in that case, so we're just
    // not going to check if the numbers are valid er whatever. We just call strtof on 3 arguments if there's 3
    // if the arg count isn't 3, we just don't set the x, y, z offsets, keeping them to the default
    if (argc == 4)
    {
        for (int i = 0; i < 3; i++)
        {
            offset[i] = atof(argv[i + 1]);
        }
        offset_is_set = true;
    }

    // um I think just dont use the header file outside the .lib just like... delete it or
    // or use <> instead of quotes
    vr::IVRSystem *vrSystem = nullptr;
    vr::EVRInitError eError = vr::VRInitError_None;

    vrSystem = vr::VR_Init(&eError, vr::VRApplication_Background);

    // ERM MIDI SETUP
    RtMidiOut *midiout = new RtMidiOut();
    std::vector<unsigned char> message;

    // Check available ports.
    unsigned int nPorts = midiout->getPortCount();
    if (nPorts == 0)
    {
        std::cout << "No ports available!\n";
        vr::VR_Shutdown();
        delete midiout;
        return 0;
    }

    // Print out port names
    std::string portName;
    std::cout << "\nThere are " << nPorts << " MIDI output ports available.\n";
    for (unsigned int i = 0; i < nPorts; i++)
    {
        try
        {
            portName = midiout->getPortName(i);
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
        std::cout << "  Output Port " << i << ": " << portName << '\n';
    }
    std::cout << '\n';

    // choosing a midi port to send output to
    int selectedPortNumber;
    std::cout << "Erm give me the number of the port you want: ";
    std::cin >> selectedPortNumber;
    if (selectedPortNumber >= nPorts || selectedPortNumber < 0)
    {
        std::cout << "ummmm that's out of range :(";
        vr::VR_Shutdown();
        delete midiout;
    }
    if (std::cin.fail())
    {
        std::cout << "erm that's not a number :(";
        vr::VR_Shutdown();
        delete midiout;
    }

    midiout->openPort(selectedPortNumber);

    // Program change: 192, 5
    message.push_back(192);
    message.push_back(5);
    SEND_MIDI();

    // Control Change: 176, 7, 100 (volume)
    message[0] = 176;
    // change on control 7
    message[1] = 7;
    message.push_back(100);
    SEND_MIDI();

    // error handling
    if (eError != vr::VRInitError_None)
    {
        vrSystem = nullptr;
        printf("Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return -1;
    }

    bool right_trigger_is_down = false;
    bool left_trigger_is_down = false;
    float left_hand_x_pos;
    float left_hand_y_pos;
    float left_hand_z_pos;
    int consonant_note = 0;

    // loop of stAte you NEED a LOOP rather than a handler :( otherwise it'll be complicated to mix handlore w loop ;(I:(:(:(:))))
    vr::VRControllerState_t controllerState;
    vr::TrackedDevicePose_t controllerPose;

    // I also want to like get device info, like how many devices there are n whatnot, what's the largest connected device index
    // so I don't have to loop through k_unMaxTrackedDeviceCount times
    char buf[128];
    vr::ETrackedPropertyError trackedPropertyError;
    int biggest_connected_device_index = 0;
    for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
    {
        if (vrSystem->IsTrackedDeviceConnected(unDevice))
        {
            std::cout << "Device with index " << unDevice << ":" << std::endl;
            vrSystem->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ControllerType_String, buf, sizeof(buf), &trackedPropertyError);
            std::cout << buf << std::endl;
            vrSystem->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, buf, sizeof(buf), &trackedPropertyError);
            std::cout << buf << "\n"
                      << std::endl;

            // cuz undevice always gets bigger, the last connected device will have the biggest index
            biggest_connected_device_index = unDevice;
        }
    }

    std::cout << "Biggest connected device index is: " << biggest_connected_device_index << ". Erm also max devices is: " << vr::k_unMaxTrackedDeviceCount << std::endl;
    int left_hand_device_index = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
    int right_hand_device_index = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
    std::cout << "Left hand index: " << left_hand_device_index << "\n"
              << "Right hand index: " << right_hand_device_index << std::endl;

    if (!offset_is_set)
    {
        std::cout << "Erm an offset wasn't given. Choose the center of your vrlin space by positioning the left controller where you want it, and pressing any button on the right controller.\n";
    }
    while (!offset_is_set)
    {
        for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice <= biggest_connected_device_index; unDevice++)
        {
            if (vrSystem->GetControllerStateWithPose(vr::TrackingUniverseRawAndUncalibrated, unDevice, &controllerState, sizeof(controllerState), &controllerPose))
            {
                if (unDevice == left_hand_device_index && right_trigger_is_down)
                {
                    offset[0] = -controllerPose.mDeviceToAbsoluteTracking.m[0][3];
                    offset[1] = -controllerPose.mDeviceToAbsoluteTracking.m[1][3];
                    offset[2] = -controllerPose.mDeviceToAbsoluteTracking.m[2][3];
                    offset_is_set = true;
                    std::cout << "xyz center is set to:\n" << offset[0] << " " << offset[1] << " " << offset[2] << "\nIf you want to keep using this same offset, write those values down somewhere and run this with those 3 numbers as arguments";
                }
                if (unDevice == right_hand_device_index && controllerState.ulButtonPressed)
                {
                    right_trigger_is_down = true;
                }
            }
        }
    }

    while (true)
    {

        // loop through each device
        for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice <= biggest_connected_device_index; unDevice++)
        {

            // 3906 bytes per second is like the upper limit to how many messages a midi can send
            // so this loop shouldn't happen more than 3906 times per second
            // needs to take at least 1/3906 of a second before looping again
            // orrr roughly a quarter of a milisecond
            // just gonna do 1 milisecond lol
            //  Sleep(1);

            // getting state of current device
            if (vrSystem->GetControllerStateWithPose(vr::TrackingUniverseRawAndUncalibrated, unDevice, &controllerState, sizeof(controllerState), &controllerPose))
            {
                // Before checking if a button is pressed, reguardless we update the sliders. So people can see what position the pitch bend n whaever is in before they play something

                // getting left hand position n doing stuff w that. Updating pitch bend and generic controllers that correspond
                if (unDevice == left_hand_device_index)
                {
                    vr::HmdMatrix34_t left_controller_matrix = controllerPose.mDeviceToAbsoluteTracking;

                    if (left_trigger_is_down && right_trigger_is_down)
                    {

                        // printing the position of the left controller. To see a good position to set the offset to.
                        // std::cout << "Left controller position: x: " << left_controller_matrix.m[0][3] << ",    y: " << left_controller_matrix.m[1][3] << ",    z: " << left_controller_matrix.m[2][3] << "\n";
                    }
                    //  Erm pitch bend code: 224 0xe0 or the code for a pitch bend on channel 0
                    //  message[0] = 224;

                    // old "finger position" thing. Just controlled ctrl 17
                    // // but erm for now I'm going to make it another midi controller that I'll map to the pitch bend
                    // // Control Change: 176
                    // message[0] = 176;

                    // // change on control 17 (general purpose controller 2 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                    // message[1] = 17;

                    // seeing which one is height and where it should be
                    //  std::cout << "\t"+std::to_string(left_controller_matrix.m[0][3])+"A\n";
                    //  std::cout << "\t"+std::to_string(left_controller_matrix.m[1][3])+"B\n";
                    //  std::cout << "\t"+std::to_string(left_controller_matrix.m[2][3])+"C\n";
                    //[1][3] is vertical and a comfortable left hand height is -0.7, which is the default value for the y offset (offset[1])
                    left_hand_y_pos = left_controller_matrix.m[1][3] + offset[1];

                    // yea idk we're just gonna have to experiment with these numbers
                    //.44 AND  -1.62 works for me as an offset. Maybe make it so you can start the program with offset values x y z
                    float left_xz_plane_size = 0.2;
                    left_hand_x_pos = ((left_controller_matrix.m[0][3] + offset[0]) / left_xz_plane_size + 1.0) / 2.0;
                    left_hand_z_pos = ((left_controller_matrix.m[2][3] + offset[2]) / left_xz_plane_size + 1.0) / 2.0;

                    // to um stretch a value from 0.0-1.0f to 0-127 (integer)
                    // std::min(std::max((int)(/* generic controller value */), 0), 127)
                    //  Control change on channel 0: 176
                    message[0] = 176;
                    // change on control 17 (general purpose controller 1 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                    message[1] = 17;
                    message[2] = std::min(std::max((int)(left_hand_x_pos * 127), 0), 127);
                    SEND_MIDI();
                    // Control change on channel 0: 176
                    message[0] = 176;
                    // change on control 18 (general purpose controller 1 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                    message[1] = 18;
                    message[2] = std::min(std::max((int)(left_hand_z_pos * 127), 0), 127);
                    SEND_MIDI();

                    // old CC message
                    // int left_controller_message = (int)(left_hand_vertical_offset * 127 + 63);
                    // // range is from 0 to 127. From seeing the ranges bring printed out and how
                    // // it maps in fl studio
                    // left_controller_message = std::min(std::max(left_controller_message, 0), 127);
                    // // // and let's just for now assume range of roughly -1 to 1 (it's not close to that but aoeisfjai)
                    // // message[2] = left_controller_message;

                    // so for a pitch bend number between 0-16383 n, msb is n/128 (which should only go up to 127 since 16383 is 1 less than being able to divide by 128 128 times)
                    // and lsb is n%128 (which should also only go up to 127 cuz that's how modulo works)

                    // scaling, flooring, and limiting it to values between 0 and 16383
                    int pitch_bend_number = std::min(std::max((int)(left_hand_y_pos * 16383 + 8191), 0), 16383);

                    // Pitch bend on channel 0: 224
                    message[0] = 224;
                    // bend lsb
                    message[1] = pitch_bend_number % 128;
                    // bend msb
                    message[2] = pitch_bend_number / 128;

                    // sooo I guess ummm probably lsb and msb both take numbers 0-127 (7 bits) and together they form a 14-bit number 0-16383
                    SEND_MIDI();
                }

                // if a button is pressed
                if (controllerState.ulButtonPressed)
                {
                    // if that button is the trigger button
                    if (vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger))
                    {

                        // if it's from the right controller
                        if (unDevice == right_hand_device_index)
                        {

                            // if right_trigger_is_down isn't already set to true
                            if (!right_trigger_is_down)
                            {
                                // Note 60 On on channel 0 at volume 90/255: 144, 64, 90
                                // 144 in hex is 90. Look at message formats: https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm
                                // 60 is middle c
                                // idk why I made the volume 90, it was arbitrary
                                message[0] = 144;
                                message[1] = 60;
                                message[2] = 90;
                                SEND_MIDI();

                                // Checking the height when the right trigger was pressed.
                                // For playing consonants

                                // play a note on another channel, note depends on the height of the right controller
                                // it'll always map to a valid note, so like we'll just have a note in the FPC in fl studio map to no sound
                                // that way there's like no additional logic for like playing and stopping this note.
                                vr::HmdMatrix34_t right_controller_matrix = controllerPose.mDeviceToAbsoluteTracking;
                                float right_hand_y_pos = right_controller_matrix.m[1][3] + offset[1];

                                // it's the interval between notes played by the right hand in meters.
                                float note_interval = .1;

                                consonant_note = 60 + (int)std::floor(right_hand_y_pos / note_interval);

                                // Note consonant_note On on channel 1 at volume 90/255: 145, consonant_note, 90
                                message[0] = 145;
                                message[1] = consonant_note;
                                message[2] = 90;
                                SEND_MIDI();

                                // set right_trigger_is_down to true
                                std::cout << "right trigger is down\n";
                                right_trigger_is_down = true;
                            }
                            // get the velocity on one of the axiseseses
                            float speed = abs(controllerPose.vVelocity.v[0]);

                            // print that out :v to see that :V::VV:V::V
                            //  std::cout << std::to_string(speed)+"\n";

                            // Control change on channel 0: 176
                            message[0] = 176;
                            // change on control 16 (general purpose controller 1 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                            message[1] = 16;
                            // set the value to speed*128+128 floored to nearest integer
                            //  looks like speed is usually between -1 and 1, and messages to controllers are 1 byte (256 numbers) so yer.
                            //  message[2]=(unsigned char)(256-speed*256);
                            // HAS TO BE BETWEEN 0 AND 127 otherwise it breaks stuff :(
                            // yer it ignores the last bit sooo yer
                            message[2] = (int)(127 - exp(-speed) * 127);

                            SEND_MIDI();
                        }
                        else if (unDevice == left_hand_device_index)
                        { // otherwise if it's from the LEFT controller
                            // if left_trigger_is_down isn't already set to true
                            if (!left_trigger_is_down)
                            {
                                // Control change on channel 0: 176
                                message[0] = 176;
                                // change on control 19 (general purpose controller 4 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                                message[1] = 19;

                                // HAS TO BE BETWEEN 0 AND 127 otherwise it breaks stuff :(
                                // and we want the left controller button to just trigger the "s" sound so like volume of noise all the way up
                                // so we're just brining the slider all the way up when the left controller is pressed
                                message[2] = 127;

                                SEND_MIDI();

                                // set right_trigger_is_down to true
                                std::cout << "left trigger is down\n";
                                left_trigger_is_down = true;
                            }
                        }
                    }
                }
                else
                { // otherwise, if a button ISN'T being pressed
                    // if it's the right controller that isn't being pressed and right_trigger_is_down hasn't already been set to false
                    if (unDevice == right_hand_device_index)
                    {
                        if (right_trigger_is_down)
                        {
                            //  Note Off: 128, 64, 40
                            message[0] = 128;
                            message[1] = 60;
                            message[2] = 40;
                            SEND_MIDI();

                            // Note consonant_note Off on channel 1: 145, consonant_note, 40
                            message[0] = 129;
                            message[1] = consonant_note;
                            message[2] = 40;
                            SEND_MIDI();

                            // set right_trigger_is_down to false
                            std::cout << "right trigger is up\n";
                            right_trigger_is_down = false;
                        }
                    }
                    // if it's the left controller that isn't being pressed and left_trigger_is_down hasn't already been set to false
                    else if (unDevice == left_hand_device_index)
                    {
                        if (left_trigger_is_down)
                        {

                            // Control change on channel 0: 176
                            message[0] = 176;
                            // change on control 19 (general purpose controller 4 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                            message[1] = 19;

                            // HAS TO BE BETWEEN 0 AND 127 otherwise it breaks stuff :(
                            // and we want the left controller button to just trigger the "s" sound so like volume of noise all the way up
                            // bringing the slider all the way down, back to 0
                            message[2] = 0;

                            SEND_MIDI();

                            // set right_trigger_is_down to false
                            std::cout << "left trigger is up\n";
                            left_trigger_is_down = false;
                        }
                    }
                }
            }
        }
    }

    // Clean up
    vr::VR_Shutdown();
    delete midiout;

    return 0;
}