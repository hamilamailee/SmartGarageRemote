int rcvLED = A5;
int trsLED = 7;
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

static const uint8_t receiverPort = 1;
static const uint8_t transmitterPort = 12;

void setup()
{
    mySwitch.enableReceive(receiverPort);
    mySwitch.enableTransmit(transmitterPort);
    mySwitch.setPulseLength(352);
    pinMode(rcvLED, OUTPUT);
    pinMode(trsLED, OUTPUT);
}

void loop()
{
    if (mySwitch.available())
    {
        unsigned long decimal = mySwitch.getReceivedValue();
        if (decimal == 1)
            digitalWrite(rcvLED, HIGH);
        else
            digitalWrite(rcvLED, LOW);
    }

    mySwitch.send(HIGH);
    digitalWrite(trsLED, HIGH);
    delay(2000);

    mySwitch.send(LOW);
    digitalWrite(trsLED, LOW);
    delay(2000);
}
