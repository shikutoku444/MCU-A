#include "mbed.h"
#include <cstdint>
#include <cstring>

#define BUFFSIZE      64  // Buffer size for serial communications

BufferedSerial pc(USBTX, USBRX, 115200);
BufferedSerial xbee(PA_9, PA_10, 9600);
AnalogIn potentiometer(A5);

char msgBuff[BUFFSIZE] = {0};
char xbeeBuff[BUFFSIZE] = {0};

char calculateChecksum(const char* message, int length) {
    int sum = 0;
    // Calculate sum of bytes between length and checksum (bytes 3 to n-2)
    for (int i = 3; i < length - 1; i++) {
        sum += static_cast<unsigned char>(message[i]);
    }
    return (0xFF - (sum & 0xFF));  // Return 8-bit checksum
}

void BuildMessage(char *xbeeMsg, char *msg, int len) {
    // API Frame Components
    const char startByte = 0x7E;
    const char frameType = 0x10;  // Transmit Request
    const char frameID = 0x01;
    const char destAddr[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}; // Broadcast
    const char destAdd2[2] = {0xFF, 0xFE};
    const char broadcastRad = 0x00;
    const char options = 0x00;

    // Calculate frame length (14 bytes fixed + payload + checksum)
    const int payloadLength = len;
    const int frameLength = 14 + payloadLength + 1; // +1 for checksum
    
    // Build frame header
    xbeeMsg[0] = startByte;
    xbeeMsg[1] = static_cast<char>((frameLength - 4) >> 8);  // Length MSB (excludes start, length, checksum)
    xbeeMsg[2] = static_cast<char>((frameLength - 4) & 0xFF); // Length LSB
    xbeeMsg[3] = frameType;
    xbeeMsg[4] = frameID;
    
    // Copy address components
    memcpy(&xbeeMsg[5], destAddr, 8);
    memcpy(&xbeeMsg[13], destAdd2, 2);
    xbeeMsg[15] = broadcastRad;
    xbeeMsg[16] = options;
    
    // Add payload
    memcpy(&xbeeMsg[17], msg, payloadLength);
    
    // Calculate and append checksum
    xbeeMsg[frameLength - 1] = calculateChecksum(xbeeMsg, frameLength);
    
    // Send complete frame
    xbee.write(xbeeMsg, frameLength);
}

float readPotentiometer() {
    return potentiometer.read();
}

int main() {
    pc.set_format(8, BufferedSerial::None, 1);
    xbee.set_format(8, BufferedSerial::None, 1);

    while (true) {
        float potValue = readPotentiometer();
        int potValueInt = static_cast<int>(potValue * 1023);

        // Safe formatted output using snprintf
        int msg_len = snprintf(msgBuff, BUFFSIZE, "%d", potValueInt);
        
        // Validate and send debug output
        if (msg_len > 0 && msg_len < BUFFSIZE) {
            char debug_msg[BUFFSIZE];
            int debug_len = snprintf(debug_msg, BUFFSIZE, "Payload to send: %s\n", msgBuff);
            if (debug_len > 0 && debug_len < BUFFSIZE) {
                pc.write(debug_msg, debug_len);
            }
        }
        
        BuildMessage(xbeeBuff, msgBuff, msg_len);
        ThisThread::sleep_for(2s);
    }
}
