#ifndef CANCOM_H
#define CANCOM_H

#include <Arduino.h>
#include <mcp2515.h>
#include "w211_can_b.h"

extern SemaphoreHandle_t canMutex;
extern TP_KOMBI_AGW1_t TP_KOMBI_AGW1;
extern TP_AGW_KOMBI1_t TP_AGW_KOMBI1;

//#define DEBUG
#ifdef DEBUG
  #define DPRINT(...) Serial.print(__VA_ARGS__)
  #define DPRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)
  #define DPRINTLN(...)
#endif

// CAN ID of AGW that communicates to the IC Display
#define SEND_CAN_ID 0x1A4

// CAN ID of IC Display back to AGW
#define RECEIVE_CAN_ID 0x1D0

#define CAN_C_DEF 'C'
#define CAN_B_DEF 'B'
/**
 * Class to handle reading and writing to Canbus
 */
class CANBUS_COMMUNICATOR {
    public:

        /**
         * Clock default is 16Mhz
         * @param cs_pin CS Pin of MCP2515 module
         * @param speed Speed of Canbus
         */
        CANBUS_COMMUNICATOR(uint8_t cs_pin, CAN_SPEED speed, char c);
        /**
         * @param cs_pin CS Pin of MCP2515 module
         * @param speed Speed of Canbus
         * @param clock (Default 16Mhz) Clock frequency of MCP2515 module
         */
        CANBUS_COMMUNICATOR(uint8_t cs_pin, CAN_SPEED speed, CAN_CLOCK clock, char c);
        /**
         * Sends frame to bus
         */
        void sendToBus(can_frame *send);

        /**
         * Wakes up MCP module
         */
        void wakeup();

        void setReadOnly();

        void printFrame(can_frame *f);
        /**
         * Attempts to read a frame from canbus.
         * If no frame is found during the read cycle, then a frame with DLC of 0 is returned
         */
        can_frame *read_frame();

        /**
         * Returns a pointer to a string of the can_frame parsed.
         * 
         * @param f Can Frame to print as a String
         * @param includeBinary If true, the binary string of the frame is also included in the string
         */
        String *frame_to_string(can_frame *f, bool includeBinary);

        /**
         * Sends a frame to canbus from Serial
         */
        void sendSerialFrame();

        /** writes a sentinel into project specific mailbox buffers */
        void clear_frame();

    private:
        String frame_string;
        can_frame read;

       /**
         * readMessage() adapts the API to read frame data from the project's buffer
         * instead of accessing MCP2515 directly, since MCP2515 is handled elsewhere
         */
        void readMessage(can_frame* frame);
//        MCP2515 *mcp;

        /** used to clear readMessage() stale can data */
        static constexpr uint64_t sentinel = 0xDEADBEEF00000000;

        char busID;
};
    
extern bool CAR_SLEEP;
#endif