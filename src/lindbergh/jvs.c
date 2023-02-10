#include "jvs.h"

#include <time.h>
#include <string.h>
#include <math.h>

/* The in and out packets used to read and write to and from*/
JVSPacket inputPacket, outputPacket;

/* The in and out buffer used to read and write to and from */
unsigned char outputBuffer[JVS_MAX_PACKET_SIZE], inputBuffer[JVS_MAX_PACKET_SIZE];

/* Holds the status of the sense line */
int senseLine = 3;

JVSIO io = {0};

/**
 * Initialise the JVS emulation
 *
 * Setup the JVS emulation on a specific device path with an
 * IO mapping provided.
 *
 * @param devicePath The linux filepath for the RS485 adapter
 * @param capabilitiesSetup The representation of the IO to emulate
 * @returns 1 if the device was initialised successfully, 0 otherwise.
 */
int initJVS()
{
    io.capabilities.switches = 14;
    io.capabilities.coins = 2;
    io.capabilities.players = 2;
    io.capabilities.analogueInBits = 8;
    io.capabilities.rightAlignBits = 0;
    io.capabilities.analogueInChannels = 20;
    io.capabilities.generalPurposeOutputs = 20;
    io.capabilities.commandVersion = 19;
    io.capabilities.jvsVersion = 48;
    io.capabilities.commsVersion = 16;
    strcpy(io.capabilities.name, "SEGA CORPORATION;I/O BD JVS;837-14572;Ver1.00;2005/10");

    if (!io.capabilities.rightAlignBits)
    {
        io.analogueRestBits = 16 - io.capabilities.analogueInBits;
        io.gunXRestBits = 16 - io.capabilities.gunXBits;
        io.gunYRestBits = 16 - io.capabilities.gunYBits;
    }

    for (int player = 0; player < (io.capabilities.players + 1); player++)
        io.state.inputSwitch[player] = 0;

    for (int analogueChannels = 0; analogueChannels < io.capabilities.analogueInChannels; analogueChannels++)
        io.state.analogueChannel[analogueChannels] = 0;

    for (int rotaryChannels = 0; rotaryChannels < io.capabilities.rotaryChannels; rotaryChannels++)
        io.state.rotaryChannel[rotaryChannels] = 0;

    for (int player = 0; player < io.capabilities.coins; player++)
        io.state.coinCount[player] = 0;

    io.analogueMax = pow(2, io.capabilities.analogueInBits) - 1;
    io.gunXMax = pow(2, io.capabilities.gunXBits) - 1;
    io.gunYMax = pow(2, io.capabilities.gunYBits) - 1;

    /* Float the sense line ready for connection */
    senseLine = 3;

    return 0;
}

/**
 * Writes a single feature to an output packet
 *
 * Writes a single JVS feature, which are specified
 * in the JVS spec, to the output packet.
 *
 * @param outputPacket The packet to write to.
 * @param capability The specific capability to write
 * @param arg0 The first argument of the capability
 * @param arg1 The second argument of the capability
 * @param arg2 The final argument of the capability
 */
void writeFeature(JVSPacket *outputPacket, char capability, char arg0, char arg1, char arg2)
{
    outputPacket->data[outputPacket->length] = capability;
    outputPacket->data[outputPacket->length + 1] = arg0;
    outputPacket->data[outputPacket->length + 2] = arg1;
    outputPacket->data[outputPacket->length + 3] = arg2;
    outputPacket->length += 4;
}

/**
 * Write the entire set of features to an output packet
 *
 * Writes the set of features specified in the JVSCapabilities
 * struct to the specified output packet.
 *
 * @param outputPacket The packet to write to.
 * @param capabilities The capabilities object to read from
 */
void writeFeatures(JVSPacket *outputPacket, JVSCapabilities *capabilities)
{
    outputPacket->data[outputPacket->length] = REPORT_SUCCESS;
    outputPacket->length += 1;

    /* Input Functions */

    if (capabilities->players)
        writeFeature(outputPacket, CAP_PLAYERS, capabilities->players, capabilities->switches, 0x00);

    if (capabilities->coins)
        writeFeature(outputPacket, CAP_COINS, capabilities->coins, 0x00, 0x00);

    if (capabilities->analogueInChannels)
        writeFeature(outputPacket, CAP_ANALOG_IN, capabilities->analogueInChannels, capabilities->analogueInBits, 0x00);

    if (capabilities->rotaryChannels)
        writeFeature(outputPacket, CAP_ROTARY, capabilities->rotaryChannels, 0x00, 0x00);

    if (capabilities->keypad)
        writeFeature(outputPacket, CAP_KEYPAD, 0x00, 0x00, 0x00);

    if (capabilities->gunChannels)
        writeFeature(outputPacket, CAP_LIGHTGUN, capabilities->gunXBits, capabilities->gunYBits, capabilities->gunChannels);

    if (capabilities->generalPurposeInputs)
        writeFeature(outputPacket, CAP_GPI, 0x00, capabilities->generalPurposeInputs, 0x00);

    /* Output Functions */

    if (capabilities->card)
        writeFeature(outputPacket, CAP_CARD, capabilities->card, 0x00, 0x00);

    if (capabilities->hopper)
        writeFeature(outputPacket, CAP_HOPPER, capabilities->hopper, 0x00, 0x00);

    if (capabilities->generalPurposeOutputs)
        writeFeature(outputPacket, CAP_GPO, capabilities->generalPurposeOutputs, 0x00, 0x00);

    if (capabilities->analogueOutChannels)
        writeFeature(outputPacket, CAP_ANALOG_OUT, capabilities->analogueOutChannels, 0x00, 0x00);

    if (capabilities->displayOutColumns)
        writeFeature(outputPacket, CAP_DISPLAY, capabilities->displayOutColumns, capabilities->displayOutRows, capabilities->displayOutEncodings);

    /* Other */

    if (capabilities->backup)
        writeFeature(outputPacket, CAP_BACKUP, 0x00, 0x00, 0x00);

    outputPacket->data[outputPacket->length] = CAP_END;
    outputPacket->length += 1;
}

/**
 * Processes and responds to an entire JVS packet
 *
 * Follows the JVS spec and proceses and responds
 * to a single entire JVS packet.
 *
 * @returns The status of the entire operation
 */
JVSStatus processPacket()
{
    readPacket(&inputPacket);

    /* Check if the packet is for us */
    if (inputPacket.destination != BROADCAST && inputPacket.destination != io.deviceID)
        return JVS_STATUS_NOT_FOR_US;

    /* Setup the output packet */
    outputPacket.length = 0;
    outputPacket.destination = BUS_MASTER;

    int index = 0;

    /* Set the entire packet success line */
    outputPacket.data[outputPacket.length++] = STATUS_SUCCESS;

    while (index < inputPacket.length - 1)
    {
        int size = 1;
        switch (inputPacket.data[index])
        {

        /* The arcade hardware sends a reset command and we clear our memory */
        case CMD_RESET:
        {
            size = 2;
            io.deviceID = -1;
            senseLine = 3;
            // printf("CMD_RESET %d\n", senseLine);
        }
        break;

        /* The arcade hardware assigns an address to our IO */
        case CMD_ASSIGN_ADDR:
        {
            size = 2;
            io.deviceID = inputPacket.data[index + 1];
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
            senseLine = 1;
            // printf("CMD_ASSIGN_ADDR %d\n", senseLine);
        }
        break;

        /* Ask for the name of the IO board */
        case CMD_REQUEST_ID:
        {
            // printf("CMD_REQUEST_ID\n");
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            memcpy(&outputPacket.data[outputPacket.length + 1], io.capabilities.name, strlen(io.capabilities.name) + 1);
            outputPacket.length += strlen(io.capabilities.name) + 2;
        }
        break;

        /* Asks for version information */
        case CMD_COMMAND_VERSION:
        {
            // printf("CMD_COMMAND_VERSION\n");
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = io.capabilities.commandVersion;
            outputPacket.length += 2;
        }
        break;

        /* Asks for version information */
        case CMD_JVS_VERSION:
        {
            // printf("CMD_JVS_VERSION\n");
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = io.capabilities.jvsVersion;
            outputPacket.length += 2;
        }
        break;

        /* Asks for version information */
        case CMD_COMMS_VERSION:
        {
            // printf("CMD_COMMS_VERSION\n");
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = io.capabilities.commsVersion;
            outputPacket.length += 2;
        }
        break;

        /* Asks what our IO board supports */
        case CMD_CAPABILITIES:
        {
            // printf("CMD_CAPABILITIES\n");
            writeFeatures(&outputPacket, &io.capabilities);
        }
        break;

        /* Asks for the status of our IO boards switches */
        case CMD_READ_SWITCHES:
        {
            // printf("CMD_READ_SWITCHES\n");
            size = 3;
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = io.state.inputSwitch[0];
            outputPacket.length += 2;
            for (int i = 0; i < inputPacket.data[index + 1]; i++)
            {
                for (int j = 0; j < inputPacket.data[index + 2]; j++)
                {
                    outputPacket.data[outputPacket.length++] = io.state.inputSwitch[i + 1] >> (8 - (j * 8));
                }
            }
        }
        break;

        case CMD_READ_COINS:
        {
            ////printf("CMD_READ_COINS\n");
            size = 2;
            int numberCoinSlots = inputPacket.data[index + 1];
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

            for (int i = 0; i < numberCoinSlots; i++)
            {
                outputPacket.data[outputPacket.length] = (io.state.coinCount[i] << 8) & 0x1F;
                outputPacket.data[outputPacket.length + 1] = io.state.coinCount[i] & 0xFF;
                outputPacket.length += 2;
            }
        }
        break;

        case CMD_READ_ANALOGS:
        {
            // printf("CMD_READ_ANALOGS\n");
            size = 2;

            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

            for (int i = 0; i < inputPacket.data[index + 1]; i++)
            {
                /* By default left align the data */
                int analogueData = io.state.analogueChannel[i] << io.analogueRestBits;
                outputPacket.data[outputPacket.length] = analogueData >> 8;
                outputPacket.data[outputPacket.length + 1] = analogueData;
                outputPacket.length += 2;
            }
        }
        break;

        case CMD_READ_ROTARY:
        {
            ////printf("CMD_READ_ROTARY\n");
            size = 2;

            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

            for (int i = 0; i < inputPacket.data[index + 1]; i++)
            {
                outputPacket.data[outputPacket.length] = io.state.rotaryChannel[i] >> 8;
                outputPacket.data[outputPacket.length + 1] = io.state.rotaryChannel[i];
                outputPacket.length += 2;
            }
        }
        break;

        case CMD_READ_KEYPAD:
        {
            ////printf("CMD_READ_KEYPAD\n");
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = 0x00;
            outputPacket.length += 2;
        }
        break;

        case CMD_READ_GPI:
        {
            ////printf("CMD_READ_GPI\n");
            size = 2;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
            for (int i = 0; i < inputPacket.data[index + 1]; i++)
            {
                outputPacket.data[outputPacket.length++] = 0x00;
            }
        }
        break;

        case CMD_REMAINING_PAYOUT:
        {
            ////printf("CMD_REMAINING_PAYOUT\n");
            size = 2;
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.data[outputPacket.length + 1] = 0;
            outputPacket.data[outputPacket.length + 2] = 0;
            outputPacket.data[outputPacket.length + 3] = 0;
            outputPacket.data[outputPacket.length + 4] = 0;
            outputPacket.length += 5;
        }
        break;

        case CMD_SET_PAYOUT:
        {
            ////printf("CMD_SET_PAYOUT\n");
            size = 4;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_WRITE_GPO:
        {
            ////printf("CMD_WRITE_GPO\n");
            size = 2 + inputPacket.data[index + 1];
            outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
            outputPacket.length += 1;
        }
        break;

        case CMD_WRITE_GPO_BYTE:
        {
            ////printf("CMD_WRITE_GPO_BYTE\n");
            size = 3;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_WRITE_GPO_BIT:
        {
            ////printf("CMD_WRITE_GPO_BIT\n");
            size = 3;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_WRITE_ANALOG:
        {
            ////printf("CMD_WRITE_ANALOG\n");
            size = inputPacket.data[index + 1] * 2 + 2;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_SUBTRACT_PAYOUT:
        {
            ////printf("CMD_SUBTRACT_PAYOUT\n");
            size = 3;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_WRITE_COINS:
        {
            ////printf("CMD_WRITE_COINS\n");
            size = 4;
            // - 1 because JVS is 1-indexed, but our array is 0-indexed
            int slot_index = inputPacket.data[index + 1] - 1;
            int coin_increment = ((int)(inputPacket.data[index + 3]) | ((int)(inputPacket.data[index + 2]) << 8));

            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

            /* Prevent overflow of coins */
            if (coin_increment + io.state.coinCount[slot_index] > 16383)
                coin_increment = 16383 - io.state.coinCount[slot_index];
            io.state.coinCount[slot_index] += coin_increment;
        }
        break;

        case CMD_WRITE_DISPLAY:
        {
            ////printf("CMD_WRITE_DISPLAY\n");
            size = (inputPacket.data[index + 1] * 2) + 2;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
        }
        break;

        case CMD_DECREASE_COINS:
        {
            ////printf("CMD_DECREASE_COINS\n");
            size = 4;
            // - 1 because JVS is 1-indexed, but our array is 0-indexed
            int slot_index = inputPacket.data[index + 1] - 1;
            int coin_decrement = ((int)(inputPacket.data[index + 3]) | ((int)(inputPacket.data[index + 2]) << 8));

            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

            /* Prevent underflow of coins */
            if (coin_decrement > io.state.coinCount[slot_index])
                coin_decrement = io.state.coinCount[slot_index];
            io.state.coinCount[slot_index] -= coin_decrement;
        }
        break;

        case CMD_CONVEY_ID:
        {
            ////printf("CMD_CONVEY_ID\n");
            size = 1;
            outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
            char idData[100];
            for (int i = 1; i < 100; i++)
            {
                idData[i] = (char)inputPacket.data[index + i];
                size++;
                if (!inputPacket.data[index + i])
                    break;
            }
            printf("CMD_CONVEY_ID = %s\n", idData);
        }
        break;

        default:
        {
            printf("Error: JVS command not supported [0x%02hhX]\n", inputPacket.data[index]);
        }
        }
        index += size;
    }

    writePacket(&outputPacket);

    return JVS_STATUS_SUCCESS;
}

/**
 * Read a JVS Packet
 *
 * A single JVS packet is read into the packet pointer
 * after it has been received, unescaped and checked
 * for any checksum errors.
 *
 * @param packet The packet to read into
 */
JVSStatus readPacket(JVSPacket *packet)
{
    int escape = 0, phase = 0, index = 0, dataIndex = 0, finished = 0;
    unsigned char checksum = 0x00;

    while (!finished)
    {
        /* If we encounter a SYNC start again */
        if (!escape && (inputBuffer[index] == SYNC))
        {
            phase = 0;
            dataIndex = 0;
            index++;
            continue;
        }

        /* If we encounter an ESCAPE byte escape the next byte */
        if (!escape && inputBuffer[index] == ESCAPE)
        {
            escape = 1;
            index++;
            continue;
        }

        /* Escape next byte by adding 1 to it */
        if (escape)
        {
            inputBuffer[index]++;
            escape = 0;
        }

        /* Deal with the main bulk of the data */
        switch (phase)
        {
        case 0: // If we have not yet got the address
            packet->destination = inputBuffer[index];
            checksum = packet->destination & 0xFF;
            phase++;
            break;
        case 1: // If we have not yet got the length
            packet->length = inputBuffer[index];
            checksum = (checksum + packet->length) & 0xFF;
            phase++;
            break;
        case 2: // If there is still data to read
            if (dataIndex == (packet->length - 1))
            {
                if (checksum != inputBuffer[index])
                    return JVS_STATUS_ERROR_CHECKSUM;
                finished = 1;
                break;
            }
            packet->data[dataIndex++] = inputBuffer[index];
            checksum = (checksum + inputBuffer[index]) & 0xFF;
            break;
        default:
            return JVS_STATUS_ERROR;
        }
        index++;
    }

    return JVS_STATUS_SUCCESS;
}

/**
 * Write a JVS Packet
 *
 * A single JVS Packet is written to the arcade
 * system after it has been escaped and had
 * a checksum calculated.
 *
 * @param packet The packet to send
 */
JVSStatus writePacket(JVSPacket *packet)
{
    /* Get pointer to raw data in packet */
    unsigned char *packetPointer = (unsigned char *)packet;

    /* Add SYNC and reset buffer */
    int checksum = 0;
    int outputIndex = 1;
    outputBuffer[0] = SYNC;

    packet->length++;

    /* Write out entire packet */
    for (int i = 0; i < packet->length + 1; i++)
    {
        if (packetPointer[i] == SYNC || packetPointer[i] == ESCAPE)
        {
            outputBuffer[outputIndex++] = ESCAPE;
            outputBuffer[outputIndex++] = (packetPointer[i] - 1);
        }
        else
        {
            outputBuffer[outputIndex++] = (packetPointer[i]);
        }
        checksum = (checksum + packetPointer[i]) & 0xFF;
    }

    /* Write out escaped checksum */
    if (checksum == SYNC || checksum == ESCAPE)
    {
        outputBuffer[outputIndex++] = ESCAPE;
        outputBuffer[outputIndex++] = (checksum - 1);
    }
    else
    {
        outputBuffer[outputIndex++] = checksum;
    }

    return JVS_STATUS_SUCCESS;
}

int getSenseLine()
{
    return senseLine;
}

int setSwitch(JVSPlayer player, JVSInput switchNumber, int value)
{
    if (player > io.capabilities.players)
        return 0;

    if (value)
    {
        io.state.inputSwitch[player] |= switchNumber;
    }
    else
    {
        io.state.inputSwitch[player] &= ~switchNumber;
    }

    return 1;
}

int incrementCoin(JVSPlayer player, int amount)
{
    if (player == SYSTEM)
        return 0;

    io.state.coinCount[player - 1] = io.state.coinCount[player - 1] + amount;
    return 1;
}

int setAnalogue(JVSInput channel, int value)
{
    io.state.analogueChannel[channel] = value;
    return 1;
}

void setSenseLine(int _senseLine) {
    senseLine = _senseLine;
}
