#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char   undefined;

typedef unsigned char    byte;
typedef unsigned int    dword;
typedef long double    longdouble;
typedef unsigned char    uchar;
typedef unsigned char    u_char;
typedef unsigned int    uint;
typedef unsigned int    uint3;
typedef unsigned long long    ulong;

// typedef unsigned uint128 __attribute__((mode(TI)));

typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined3;
typedef unsigned int    undefined4;
typedef unsigned long    undefined5;
typedef unsigned long long    undefined8;
typedef unsigned short    ushort;
typedef unsigned short    word;
typedef unsigned long long    ulonglong;
typedef long    int7;
typedef unsigned long    uint7;


typedef unsigned int    u_int;

struct sceCdlFILE {
	uint lsn; /* File location on disc */
	uint size; /* File size */
	char name[16]; /* File name (body) */
	u_char date[8]; /* Date */
	uint flag; /* File flag */
};

enum EBankAction
{
	BANK_ACTION_3 = 3,
	BANK_ACTION_5 = 5,
	CLOSE = 4,
	LOAD = 0,
	OPEN = 6,
	READ_STREAM = 2,
	SEEK = 1
};

#include <assert.h>
#define IMPLEMENTATION_GUARD(x) assert(false);

#define edDebugPrintf(...)
#define MY_LOG(...)

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))

#define NAME_NEXT_OBJECT(...)

#endif