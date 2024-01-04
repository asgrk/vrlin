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

#define lowByte(w) ((uint8_t)((w)&0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

int main()
{
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
    midiout->sendMessage(&message);

    // Control Change: 176, 7, 100 (volume)
    message[0] = 176;
    // change to control 7
    message[1] = 7;
    message.push_back(100);
    midiout->sendMessage(&message);

    // error handling
    if (eError != vr::VRInitError_None)
    {
        vrSystem = nullptr;
        printf("Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return -1;
    }

    bool right_trigger_is_down = false;

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
                                // Note On: 144, 64, 90
                                message[0] = 144;
                                message[1] = 60;
                                message[2] = 90;
                                midiout->sendMessage(&message);
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
                            // change to control 16 (general purpose controller 1 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                            message[1] = 16;
                            // set the value to speed*128+128 floored to nearest integer
                            //  looks like speed is usually between -1 and 1, and messages to controllers are 1 byte (256 numbers) so yer.
                            //  message[2]=(unsigned char)(256-speed*256);
                            // HAS TO BE BETWEEN 0 AND 127 otherwise it breaks stuff :(
                            // yer it ignores the last bit sooo yer
                            message[2] = (int)(127 - exp(-speed) * 127);


                            midiout->sendMessage(&message);
                        }
                        else if (unDevice == left_hand_device_index)
                        { // otherwise if it's from the LEFT controller
                            // Erm pitch bend code: 224 0xe0 or the code for a pitch bend on channel 0
                            // message[0] = 224;

                            // old "finger position" thing. Just controlled ctrl 17
                            // // but erm for now I'm going to make it another midi controller that I'll map to the pitch bend
                            // // Control Change: 176
                            // message[0] = 176;

                            // // change to control 17 (general purpose controller 2 https://www.cs.cmu.edu/~music/cmsip/readings/davids-midi-spec.htm)
                            // message[1] = 17;

                            vr::HmdMatrix34_t left_controller_matrix = controllerPose.mDeviceToAbsoluteTracking;

                            // seeing which one is height and where it should be
                            //  std::cout << "\t"+std::to_string(left_controller_matrix.m[0][3])+"A\n";
                            //  std::cout << "\t"+std::to_string(left_controller_matrix.m[1][3])+"B\n";
                            //  std::cout << "\t"+std::to_string(left_controller_matrix.m[2][3])+"C\n";
                            //[1][3] is vertical and a comfortable left hand height is -0.7
                            float left_hand_vertical_offset = left_controller_matrix.m[1][3] + 0.7;

                            // old CC message
                            // int left_controller_message = (int)(left_hand_vertical_offset * 127 + 63);
                            // // range is from 0 to 127. From seeing the ranges bring printed out and how
                            // // it maps in fl studio
                            // left_controller_message = std::min(std::max(left_controller_message, 0), 127);
                            // // // and let's just for now assume range of roughly -1 to 1 (it's not close to that but aoeisfjai)
                            // // message[2] = left_controller_message;

                            //so for a pitch bend number between 0-16383 n, msb is n/128 (which should only go up to 127 since 16383 is 1 less than being able to divide by 128 128 times)
                            //and lsb is n%128 (which should also only go up to 127 cuz that's how modulo works)

                            //scaling, flooring, and limiting it to values between 0 and 16383
                            int pitch_bend_number = std::min(std::max((int)(left_hand_vertical_offset * 16383 + 8191),0),16383);

                            // Pitch bend on channel 0: 224
                            message[0] = 224;
                            // bend lsb
                            message[1] = pitch_bend_number%128;
                            // bend msb
                            message[2] = pitch_bend_number/128;

                            //sooo I guess ummm probably lsb and msb both take numbers 0-127 (7 bits) and together they form a 14-bit number 0-16383
                            midiout->sendMessage(&message);
                        }
                    }
                }
                else
                { // otherwise, if a button ISN'T being pressed
                    // if it's the right controller that isn't being pressed and right_trigger_is_down hasn't already been set to false
                    if (unDevice == right_hand_device_index && right_trigger_is_down)
                    {
                        //  Note Off: 128, 64, 40
                        message[0] = 128;
                        message[1] = 60;
                        message[2] = 40;
                        midiout->sendMessage(&message);
                        // set right_trigger_is_down to false
                        std::cout << "right trigger is up\n";
                        right_trigger_is_down = false;
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