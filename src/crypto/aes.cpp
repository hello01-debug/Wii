#include "aes.h"

#include <vector>
#include <cstdio>
#include <cstdlib>

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

std::vector<uint32_t> key_fifo;
std::vector<uint32_t> iv_fifo;

uint32_t src_address, dst_address;

void PushKeyFifo(uint32_t val)
{
	if (key_fifo.size() == 4)
	{
		printf("Reseting key FIFO\n");
		key_fifo.clear();
	}
	else if (key_fifo.size() == 3)
	{
		printf("[AES]: Key: 0x%08x%08x%08x%08x\n", key_fifo[0], key_fifo[1], key_fifo[2], val);
	}

	key_fifo.push_back(val);
}

void PushIVFifo(uint32_t val)
{
	if (iv_fifo.size() == 4)
	{
		printf("Reseting IV FIFO\n");
		iv_fifo.clear();
	}
	else if (iv_fifo.size() == 3)
	{
		printf("[AES]: IV: 0x%08x%08x%08x%08x\n", iv_fifo[0], iv_fifo[1], iv_fifo[2], val);
	}

	iv_fifo.push_back(val);
}

void AES::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d020000:
		aes_ctrl.value = value;
		if (value & (1 << 31))
		{
			printf("[AES]: Starting AES transfer\n");
			exit(1);
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
