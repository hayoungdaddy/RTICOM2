#ifndef MYCRC_H
#define MYCRC_H

/*
 * External Prototypes
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE	0
#define TRUE	!FALSE
#endif

/*
 * Select the CRC standard from the list that follows.
 */
/*
 * #define CRC_CCITT
 */

#define CRC32


#if defined(CRC_CCITT)

typedef unsigned short  crc;

#define CRC_NAME			"CRC-CCITT"
#define POLYNOMIAL			0x1021
#define INITIAL_REMAINDER	0xFFFF
#define FINAL_XOR_VALUE		0x0000
#define REFLECT_DATA		FALSE
#define REFLECT_REMAINDER	FALSE
#define CHECK_VALUE			0x29B1

#elif defined(CRC16)

typedef unsigned short  crc;

#define CRC_NAME			"CRC-16"
#define POLYNOMIAL			0x8005
#define INITIAL_REMAINDER	0x0000
#define FINAL_XOR_VALUE		0x0000
#define REFLECT_DATA		TRUE
#define REFLECT_REMAINDER	TRUE
#define CHECK_VALUE			0xBB3D

#elif defined(CRC32)

// LIM for 64 bits OS
/*
#ifdef _LINUX
typedef unsigned int  crc;
#else
typedef unsigned long  crc;
#endif
*/

typedef unsigned long  crc;


#define CRC_NAME			"CRC-32"
#define POLYNOMIAL			0x04C11DB7
#define INITIAL_REMAINDER	0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF
#define REFLECT_DATA		TRUE
#define REFLECT_REMAINDER	TRUE
#define CHECK_VALUE			0xCBF43926

#else

#error "One of CRC_CCITT, CRC16, or CRC32 must be #define'd."

#endif


void  crcInit(void);
crc   crcFast(unsigned char const message[], int nBytes);

#ifdef __cplusplus
}
#endif

#endif // MYCRC_H
