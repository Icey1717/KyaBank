#pragma once

#include <cstdint>

enum GIF_FLG
{
	GIF_FLG_PACKED = 0,
	GIF_FLG_REGLIST = 1,
	GIF_FLG_IMAGE = 2,
	GIF_FLG_IMAGE2 = 3
};

enum GIF_REG
{
	GIF_REG_PRIM = 0x00,
	GIF_REG_RGBA = 0x01,
	GIF_REG_STQ = 0x02,
	GIF_REG_UV = 0x03,
	GIF_REG_XYZF2 = 0x04,
	GIF_REG_XYZ2 = 0x05,
	GIF_REG_TEX0_1 = 0x06,
	GIF_REG_TEX0_2 = 0x07,
	GIF_REG_CLAMP_1 = 0x08,
	GIF_REG_CLAMP_2 = 0x09,
	GIF_REG_FOG = 0x0a,
	GIF_REG_INVALID = 0x0b,
	GIF_REG_XYZF3 = 0x0c,
	GIF_REG_XYZ3 = 0x0d,
	GIF_REG_A_D = 0x0e,
	GIF_REG_NOP = 0x0f,
};

struct Gif_Tag
{
	struct HW_Gif_Tag
	{
		uint16_t NLOOP : 15;
		uint16_t EOP : 1;
		uint16_t _dummy0 : 16;
		uint32_t _dummy1 : 14;
		uint32_t PRE : 1;
		uint32_t PRIM : 11;
		uint32_t FLG : 2;
		uint32_t NREG : 4;
		uint32_t REGS[2];
	} tag;

	uint32_t  nLoop;		// NLOOP left to process
	uint32_t  nRegs;		// NREG (1~16)
	uint32_t  nRegIdx;	// Current nReg Index (packed mode processing)
	uint32_t  len;		// Packet Length in Bytes (not including tag)
	uint32_t  cycles;    // Time needed to process packet data in ee-cycles
	uint8_t   regs[16];	// Regs
	bool hasAD;		// Has an A+D Write
	bool isValid;	// Tag is valid

	Gif_Tag() { Reset(); }
	Gif_Tag(uint8_t* pMem, bool analyze = false) {
		setTag(pMem, analyze);
	}

	void Reset() { memset(this, 0, sizeof(Gif_Tag)); }
	uint8_t   curReg() { return regs[nRegIdx & 0xf]; }

	void packedStep() {
		if (nLoop > 0) {
			nRegIdx++;
			if (nRegIdx >= nRegs) {
				nRegIdx = 0;
				nLoop--;
			}
		}
	}

	void setTag(uint8_t* pMem, bool analyze = false)
	{
		tag = *(HW_Gif_Tag*)pMem;
		nLoop = tag.NLOOP;
		hasAD = false;
		nRegIdx = 0;
		isValid = 1;
		len = 0; // avoid uninitialized compiler warning
		switch (tag.FLG) {
		case GIF_FLG_PACKED:
			nRegs = ((tag.NREG - 1) & 0xf) + 1;
			len = (nRegs * tag.NLOOP) * 16;
			cycles = len << 1; // Packed Mode takes 2 ee-cycles
			if (analyze) analyzeTag();
			break;
		case GIF_FLG_REGLIST:
			nRegs = ((tag.NREG - 1) & 0xf) + 1;
			len = ((nRegs * tag.NLOOP + 1) >> 1) * 16;
			cycles = len << 2; // Reg-list Mode takes 4 ee-cycles
			break;
		case GIF_FLG_IMAGE:
		case GIF_FLG_IMAGE2:
			nRegs = 0;
			len = tag.NLOOP * 16;
			cycles = len << 2; // Image Mode takes 4 ee-cycles
			tag.FLG = GIF_FLG_IMAGE;
			break;
			//jNO_DEFAULT;
		}
	}

	void analyzeTag()
	{
		hasAD = false;
		uint32_t t = tag.REGS[0];
		uint32_t i = 0;
		uint32_t j = std::min<uint32_t>(nRegs, 8);
		for (; i < j; i++) {
			regs[i] = t & 0xf;
			hasAD |= (regs[i] == GIF_REG_A_D);
			t >>= 4;
		}
		t = tag.REGS[1];
		j = nRegs;
		for (; i < j; i++) {
			regs[i] = t & 0xf;
			hasAD |= (regs[i] == GIF_REG_A_D);
			t >>= 4;
		}
	}
};

constexpr uint32_t gVifEndCode = 0x60000000;
