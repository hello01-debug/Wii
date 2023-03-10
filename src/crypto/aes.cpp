#include "aes.h"

#include <vector>
#include <cstdio>
#include <cstdlib>

#include <src/memory/bus.h>

#include "aes_lib.h"

union AES_CTRL
{
	uint32_t value;
	struct
	{
		uint32_t blocks : 12,
		iv : 1,
		: 14,
		dec : 1,
		ena : 1,
		err : 1,
		irq : 1,
		exec : 1;
	};
} aes_ctrl;

std::vector<uint8_t> key_fifo;
std::vector<uint32_t> iv_fifo;

uint32_t src_address, dst_address;

void PushKeyFifo(uint32_t val)
{
	if (key_fifo.size() == 16)
	{
		printf("[AES]: Reseting key FIFO\n");
		key_fifo.erase(key_fifo.begin());
		key_fifo.erase(key_fifo.begin());
		key_fifo.erase(key_fifo.begin());
		key_fifo.erase(key_fifo.begin());
	}
	else if (key_fifo.size() == 12)
	{
		printf("[AES] Key is 0x");
		for (int i = 0; i < 12; i++)
			printf("%02x", key_fifo[i]);
		printf("%02x%02x%02x%02x\n", (val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
	}

#if 1
	key_fifo.push_back((val >> 24) & 0xff);
	key_fifo.push_back((val >> 16) & 0xff);
	key_fifo.push_back((val >> 8) & 0xff);
	key_fifo.push_back(val & 0xff);
#else
	key_fifo.push_back(val & 0xff);
	key_fifo.push_back((val >> 8) & 0xff);
	key_fifo.push_back((val >> 16) & 0xff);
	key_fifo.push_back((val >> 24) & 0xff);
#endif
}

void PushIVFifo(uint32_t val)
{
	if (iv_fifo.size() == 4)
	{
		printf("[AES]: Reseting IV FIFO\n");
		iv_fifo.clear();
	}
	else if (iv_fifo.size() == 3)
	{
		printf("[AES]: IV: 0x%08x%08x%08x%08x\n", iv_fifo[0], iv_fifo[1], iv_fifo[2], val);
	}

	iv_fifo.push_back(val);
}

AES_ctx aes_ctx;

void AES::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d020000:
		aes_ctrl.value = value;
		if (aes_ctrl.exec)
		{
			std::reverse(key_fifo.begin(), key_fifo.end());
			AES_init_ctx(&aes_ctx, key_fifo.data());
			if (!aes_ctrl.iv)
				AES_ctx_set_iv(&aes_ctx, (uint8_t*)iv_fifo.data());

			uint32_t len = ((aes_ctrl.value & 0xfff) + 1) * 16;

			uint8_t* buf = new uint8_t[len];

			for (int i = 0; i < len; i++)
			{
				buf[i] = Bus::read8_starlet(src_address);
				src_address++;
			}

			AES_CBC_decrypt_buffer(&aes_ctx, buf, len);

			for (int i = 0; i < len; i++)
			{
				Bus::write8_starlet(dst_address, buf[i]);
				dst_address++;
			}

			aes_ctrl.exec = 0;
		}
		else
			printf("[AES]: Resetting AES engine\n");
		return;
	case 0x0d020004:
		src_address = value;
		printf("[AES]: Source set to 0x%08x\n", value);
		return;
	case 0x0d020008:
		dst_address = value;
		printf("[AES]: Destination set to 0x%08x\n", value);
		return;
	case 0x0d02000C:
		PushKeyFifo(value);
		return;
	case 0x0d020010:
		PushIVFifo(value);
		return;
	}
}

uint32_t AES::read32_starlet(uint32_t address)
{
	switch (address)
	{
	case 0x0d020000:
		return aes_ctrl.value;
	}
}
