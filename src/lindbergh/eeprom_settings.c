#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eeprom_settings.h"

uint32_t crc32_table[255];

typedef struct
{
    uint16_t offset;
    uint16_t size;
} eepromOffsets;

typedef enum
{
    STATIC,
    NETWORK_TYPE,
    ETH0,
    ETH1,
    CREDIT,
    BACKUP
} eepromSection;

eepromOffsets eepromOffsetTable[] = {
    {0x0000, 0x0020},  // amSysDataRecord_Static
    {0x00A0, 0x0010},  // amSysDataRecord_NetworkType
    {0x00B0, 0x0020},  // amSysDataRecord_Eth0
    {0x00D0, 0x0020},  // amSysDataRecord_Eth1
    {0x0100, 0x0040},  // amSysDataRecord_Credit
    {0x0400, 0x0060}}; // amSysDataRecord_Backup

unsigned char eepromBuffer[512];

void build_crc32_table()
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t ch = i;
        uint32_t crc = 0;
        for (size_t j = 0; j < 8; j++)
        {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b)
                crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
        crc32_table[i] = crc;
    }
}

uint32_t gen_crc(int section, size_t n)
{
    unsigned char *buff = &eepromBuffer[eepromOffsetTable[section].offset + 4];
    unsigned long crc = 0xfffffffful;
    size_t i;
    for (i = 0; i < n; i++)
        crc = crc32_table[*buff++ ^ (crc & 0xff)] ^ (crc >> 8);
    return (crc);
}

void addCRCtoBuffer(int section)
{
    uint32_t crc = gen_crc(section, eepromOffsetTable[section].size - 4);
    memcpy(&eepromBuffer[eepromOffsetTable[section].offset], &crc, sizeof(uint32_t));
}

int fillBuffer(FILE *file)
{
    fseek(file, 0, SEEK_SET);
    int b = fread(eepromBuffer, 1, 512, file);
    if (b < 512)
        return 1;
    return 0;
}

int checkCRCinBuffer(int section)
{
    uint32_t crc;
    memcpy(&crc, &eepromBuffer[eepromOffsetTable[section].offset], 4);
    if (crc != gen_crc(section, eepromOffsetTable[section].size - 4))
        return 1;
    return 0;
}

int createStaticSection()
{
    unsigned char *buff = &eepromBuffer[eepromOffsetTable[STATIC].offset];
    memset(buff, 0, eepromOffsetTable[STATIC].size);
    buff[14] = 0;
    memcpy(buff + 15, "AAGX-01A00009999", 16);
    return 0;
}

int createNetworkTypeSection()
{
    memset(&eepromBuffer[eepromOffsetTable[NETWORK_TYPE].offset], 0,
           eepromOffsetTable[NETWORK_TYPE].size);
    return 0;
}

int createEthSection(int section)
{
    unsigned char *buff = &eepromBuffer[eepromOffsetTable[section].offset];
    memset(buff, 0, eepromOffsetTable[section].size);
    uint32_t value = 1;
    memcpy(buff + 8, &value, sizeof(uint32_t));
    value = inet_addr("10.0.0.1");
    if (section == ETH1)
        value += (1 << 24);
    memcpy(buff + 12, &value, sizeof(uint32_t));
    value = inet_addr("255.255.255.0");
    memcpy(buff + 16, &value, sizeof(uint32_t));
    value = inet_addr("0.0.0.0");
    memcpy(buff + 20, &value, sizeof(uint32_t));
    memcpy(buff + 24, &value, sizeof(uint32_t));
    memcpy(buff + 28, &value, sizeof(uint32_t));
    return 0;
}

int createCreditSection()
{
    unsigned char *buff = &eepromBuffer[eepromOffsetTable[CREDIT].offset];
    memset(buff, 0, eepromOffsetTable[CREDIT].size);
    buff[32] = 99;
    buff[33] = 9;
    buff[34] = 4;
    buff[35] = 0; // Coin chute type [COMMON (Default) / INDIVIDUAL]
    buff[36] = 1; // Service Type [COMMON / INDIVIDUAL (Default)]
    buff[38] = 1; //
    buff[39] = 0; // Freeplay set to 1
    buff[40] = 1; // Credits Chute #1
    buff[41] = 1; // Credits Chute #1
    buff[42] = 0; //
    buff[43] = 1; // Coins
    for (size_t i = 0; i < 8; i++)
    {
        buff[44 + i] = 1;
    }
    return 0;
}

int writeSectiontoFile(FILE *file, int section)
{
    unsigned char *buff = &eepromBuffer[eepromOffsetTable[section].offset];
    // Original
    if (fseek(file, eepromOffsetTable[section].offset, SEEK_SET) != 0)
        return 1;
    if (fwrite(buff, eepromOffsetTable[section].size, 1, file) != 1)
        return 1;

    // Duplicate
    if (fseek(file, eepromOffsetTable[section].offset + 0x200, SEEK_SET) != 0)
        return 1;
    if (fwrite(buff, eepromOffsetTable[section].size, 1, file) != 1)
        return 1;
    return 0;
}

int getRegion()
{
    return eepromBuffer[14];
}

int setRegion(FILE *eeprom, int region)
{
    eepromBuffer[14] = region;
    addCRCtoBuffer(STATIC);
    if (writeSectiontoFile(eeprom, STATIC) != 0)
    {
        printf("Error writing to eeprom.");
        return 1;
    }
    return 0;
}

int getFreeplay()
{
    return eepromBuffer[eepromOffsetTable[CREDIT].offset + 39];
}

int setFreeplay(FILE *eeprom, int freeplay)
{
    if (createCreditSection() != 0)
    {
        printf("Error setting Free Play.");
        return 1;
    }
    eepromBuffer[eepromOffsetTable[CREDIT].offset + 39] = freeplay;
    addCRCtoBuffer(CREDIT);
    if (writeSectiontoFile(eeprom, CREDIT) != 0)
    {
        printf("Error writing to eeprom.");
        return 1;
    }
    return 0;
}

int fixCreditSection(FILE *eeprom)
{
    eepromBuffer[eepromOffsetTable[CREDIT].offset + 36] = 0;
    eepromBuffer[eepromOffsetTable[CREDIT].offset + 39] = 0;
    addCRCtoBuffer(CREDIT);
    if (writeSectiontoFile(eeprom, CREDIT) != 0)
    {
        printf("Error writing to eeprom.");
        return 1;
    }
    return 0;
}

int eepromSettingsInit( FILE *eeprom)
{
    build_crc32_table();

    eeprom = fopen("eeprom.bin", "r+b");
    if (eeprom == NULL)
    {
        printf("eeprom.bin cannot be opened, let's create a new one.\n");
        eeprom = fopen("eeprom.bin", "w+b");
    }
    fseek(eeprom, 0, SEEK_END);
    int size = ftell(eeprom);
    fseek(eeprom, 0, SEEK_SET);
    if (size >= 832) // eeprom initialized at least by SEGABOOT
    {
        fillBuffer(eeprom); // Fills buffer with 1st 512 bytes of the eeprom file
        if (checkCRCinBuffer(STATIC) != 0)
        {
            // Create section from scratch
            if (createStaticSection() != 0)
            {
                printf("Error creating Static Section.\n");
                return 1;
            }
            else
            {
                addCRCtoBuffer(STATIC);
                if (writeSectiontoFile(eeprom, STATIC) != 0)
                {
                    printf("Error writing to eeprom.bin [STATIC].");
                    return 1;
                }
            }
        }
        if (checkCRCinBuffer(NETWORK_TYPE) != 0)
        {
            // Create section from scratch
            if (createNetworkTypeSection() != 0)
            {
                printf("Error creating NetworkType Section.\n");
                return 1;
            }
            else
            {
                addCRCtoBuffer(NETWORK_TYPE);
                if (writeSectiontoFile(eeprom, NETWORK_TYPE) != 0)
                {
                    printf("Error writing to eeprom.bin [NETWORK_TYPE].");
                    return 1;
                }
            }
        }
        if (checkCRCinBuffer(ETH0) != 0)
        {
            // Create section from scratch
            if (createEthSection(ETH0) != 0)
            {
                printf("Error creating Eth0 Section.\n");
                return 1;
            }
            else
            {
                addCRCtoBuffer(ETH0);
                if (writeSectiontoFile(eeprom, ETH0) != 0)
                {
                    printf("Error writing to eeprom.bin [ETH0].");
                    return 1;
                }
            }
        }
        if (checkCRCinBuffer(ETH1) != 0)
        {
            // Create section from scratch
            if (createEthSection(ETH1) != 0)
            {
                printf("Error creating Eth0 Section.\n");
                return 1;
            }
            else
            {
                addCRCtoBuffer(ETH1);
                if (writeSectiontoFile(eeprom, ETH1) != 0)
                {
                    printf("Error writing to eeprom.bin [ETH1].");
                    return 1;
                }
            }
        }
        if (checkCRCinBuffer(CREDIT) != 0)
        {
            // Create section from scratch
            if (createCreditSection() != 0)
            {
                printf("Error creating CREDIT Section.\n");
                return 1;
            }
            else
            {
                addCRCtoBuffer(CREDIT);
                if (writeSectiontoFile(eeprom, NETWORK_TYPE) != 0)
                {
                    printf("Error writing to eeprom.bin [CREDIT]].");
                    return 1;
                }
            }
        }
    }
    else
    {
        // Create all sections from scratch
        if ((createStaticSection() != 0) || (createNetworkTypeSection() != 0) ||
            (createEthSection(ETH0) != 0) || (createEthSection(ETH1) != 0) || (createCreditSection() != 0))
        {
            printf("Error creating section from scratch.\n");
            return 1;
        }
        addCRCtoBuffer(STATIC);
        addCRCtoBuffer(NETWORK_TYPE);
        addCRCtoBuffer(ETH0);
        addCRCtoBuffer(ETH1);
        addCRCtoBuffer(CREDIT);
        if ((writeSectiontoFile(eeprom, STATIC) != 0) || (writeSectiontoFile(eeprom, NETWORK_TYPE) != 0) ||
            (writeSectiontoFile(eeprom, ETH0) != 0) || (writeSectiontoFile(eeprom, ETH1) != 0) ||
            (writeSectiontoFile(eeprom, CREDIT) != 0))
        {
            printf("Error writing section from scratch.\n");
            return 1;
        }
    }
    return 0;
}
