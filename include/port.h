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

/* GS registers address */
	/*-- vertex info. reg--*/
#define SCE_GS_PRIM         0x00
#define SCE_GS_RGBAQ        0x01
#define SCE_GS_ST           0x02
#define SCE_GS_UV           0x03
#define SCE_GS_XYZF2        0x04
#define SCE_GS_XYZ2         0x05
#define SCE_GS_FOG          0x0a
#define SCE_GS_XYZF3        0x0c
#define SCE_GS_XYZ3         0x0d
#define SCE_GS_XYOFFSET_1   0x18
#define SCE_GS_XYOFFSET_2   0x19
#define SCE_GS_PRMODECONT   0x1a

	/*-- drawing attribute reg. --*/
#define SCE_GS_PRMODE       0x1b
#define SCE_GS_TEX0_1       0x06
#define SCE_GS_TEX0_2       0x07
#define SCE_GS_TEX1_1       0x14
#define SCE_GS_TEX1_2       0x15
#define SCE_GS_TEX2_1       0x16
#define SCE_GS_TEX2_2       0x17
#define SCE_GS_TEXCLUT      0x1c
#define SCE_GS_SCANMSK      0x22
#define SCE_GS_MIPTBP1_1    0x34
#define SCE_GS_MIPTBP1_2    0x35
#define SCE_GS_MIPTBP2_1    0x36
#define SCE_GS_MIPTBP2_2    0x37
#define SCE_GS_CLAMP_1      0x08
#define SCE_GS_CLAMP_2      0x09
#define SCE_GS_TEXA         0x3b
#define SCE_GS_FOGCOL       0x3d
#define SCE_GS_TEXFLUSH     0x3f

/*-- pixel operation reg. --*/
#define SCE_GS_SCISSOR_1    0x40
#define SCE_GS_SCISSOR_2    0x41
#define SCE_GS_ALPHA_1      0x42
#define SCE_GS_ALPHA_2      0x43
#define SCE_GS_DIMX         0x44
#define SCE_GS_DTHE         0x45
#define SCE_GS_COLCLAMP     0x46
#define SCE_GS_TEST_1       0x47
#define SCE_GS_TEST_2       0x48
#define SCE_GS_PABE         0x49
#define SCE_GS_FBA_1        0x4a
#define SCE_GS_FBA_2        0x4b

	/*-- buffer reg. --*/
#define SCE_GS_FRAME_1      0x4c
#define SCE_GS_FRAME_2      0x4d
#define SCE_GS_ZBUF_1       0x4e
#define SCE_GS_ZBUF_2       0x4f

	/*-- inter-buffer transfer reg. --*/
#define SCE_GS_BITBLTBUF    0x50
#define SCE_GS_TRXPOS       0x51
#define SCE_GS_TRXREG       0x52
#define SCE_GS_TRXDIR       0x53
#define SCE_GS_HWREG        0x54

	/*-- other reg. --*/
#define SCE_GS_SIGNAL       0x60
#define SCE_GS_FINISH       0x61
#define SCE_GS_LABEL        0x62
#define SCE_GS_NOP          0x7f

#define SCE_GIF_PACKED_AD 0x0e