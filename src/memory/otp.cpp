#include "otp.h"

#include <fstream>
#include <cstring>
#include <assert.h>

uint32_t otp[0x20];

void OTP::LoadOTP(std::string otp_name)
{
    std::ifstream one_tp(otp_name, std::ios::binary);

    char sig[10];
    sig[9] = '\0';
    one_tp.read(sig, 9);

    if (strncmp(sig, "BackupMii", 9))
    {
        printf("[OTP]: Error: Please supply BackupMii format OTP\n");
        exit(1);
    }

    one_tp.seekg(0x100); // Offset of boo1 hash
    one_tp.read((char*)otp, 0x14);

    one_tp.seekg(0x114); // Offset of common key
    one_tp.read((char*)&otp[5], 0x10);

    one_tp.seekg(0x208); // Offset of NG_ID
    one_tp.read((char*)&otp[9], 4);

    one_tp.seekg(0x144); // Offset of NAND HMAC key
    one_tp.read((char*)&otp[0x11], 20);

    one_tp.seekg(0x158); // Offset of NAND AES key
    one_tp.read((char*)&otp[0x16], 16);

    one_tp.seekg(0x168); // Offset of PRNG AES number
    one_tp.read((char*)&otp[0x1a], 16);
}

uint32_t otp_ctrl = 0;
uint32_t otp_data = 0;

void OTP::write32_starlet(uint32_t address, uint32_t data)
{
    switch (address)
    {
    case 0x0d8001ec:
        otp_ctrl = data;
        if (otp_ctrl & (1 << 31))
        {
            printf("[OTP]: Reading data from 0x%x\n", otp_ctrl & 0x1F);
            otp_data = otp[otp_ctrl & 0x1F];
        }
        break;
    }
}

uint32_t OTP::read32_starlet(uint32_t address)
{
    assert(address == 0x0d8001f0);

    return otp_data;
}