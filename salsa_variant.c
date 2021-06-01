/*
 * main.c
 *
 * Created: 5/28/2021 1:47:57 PM
 *  Author: patri
 */

#include <xc.h>
#include <avr/pgmspace.h>

#include <stdint.h>
#define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
#define QR(a, b, c, d)(	\
b ^= ROTL(a + d, 7),	\
c ^= ROTL(b + a, 9),	\
d ^= ROTL(c + b,13),	\
a ^= ROTL(d + c,18))
#define ROUNDS 20

void salsa20_block(uint32_t out[16], uint32_t in[16])
{
	int i;
	uint32_t x[16];

	for (i = 0; i < 16; ++i)
	x[i] = in[i];
	// 10 loops of 2 rounds/loop = 20 rounds
	for (i = 0; i < ROUNDS; i += 2) {
		// Odd round
		QR(x[ 0], x[ 4], x[ 8], x[12]);	// column 1
		QR(x[ 5], x[ 9], x[13], x[ 1]);	// column 2
		QR(x[10], x[14], x[ 2], x[ 6]);	// column 3
		QR(x[15], x[ 3], x[ 7], x[11]);	// column 4
		// Even round
		QR(x[ 0], x[ 1], x[ 2], x[ 3]);	// row 1
		QR(x[ 5], x[ 6], x[ 7], x[ 4]);	// row 2
		QR(x[10], x[11], x[ 8], x[ 9]);	// row 3
		QR(x[15], x[12], x[13], x[14]);	// row 4
	}
	for (i = 0; i < 16; ++i)
	out[i] = x[i] + in[i];
}

int main(void)
{
	// Salsa Setup
	uint32_t K[16];
	// Randomly Generated State
	uint32_t const key[8] = {0x3d2721de, 0xbc95fbed, 0xbcb91104, 0x4410aacd, 0x695a571f, 0xe46d724c, 0x793fd8aa, 0xc1bb8b77};
	uint32_t initial_state[16] = {0x65787061, key[0], key[1], key[2],
		                                key[3], 0x6e642033, 0x503950bf, 0x036ae4fa,
										0x00000000, 0x00000000, 0x322d6279, key[4],
										key[5], key[6], key[7], 0x7465206b};
										
	salsa20_block(K, initial_state);

	uint8_t C[8] = {0};
	
	// Initialize using the first 64 bits of the output stream K
	uint8_t (*chunk64)[64] = (void *)&K;
	C[0] = (*chunk64)[0];
	C[1] = (*chunk64)[1];
	C[2] = (*chunk64)[2];
	C[3] = (*chunk64)[3];
	C[4] = (*chunk64)[4];
	C[5] = (*chunk64)[5];
	C[6] = (*chunk64)[6];
	C[7] = (*chunk64)[7];

	uint8_t byte_counter = 8; // We've used 8 bytes (64 bits, first 2 chunks of the stream) so far, this maxes out at 64 bytes (512 bits)
	
	unsigned long m = 50000; // Number of loop iterations
	unsigned long k = 0; // Generic loop counter for the main loop
	uint8_t* A = 0x0000; // 16-bit Address for program memory access
	int b = 0; // j in the pseudocode
	
	// Initialize Salsa_i
	uint8_t prev_salsa;
	uint8_t curr_salsa;
	prev_salsa = (*chunk64)[byte_counter];
	byte_counter += 1;
	curr_salsa = (*chunk64)[byte_counter];
	byte_counter += 1;
	
	for (k = 0; k < m; ++k)
	{
		
		A = (uint8_t *)(curr_salsa << 8) + C[(b - 1) % 8]; // Build 16-bit address upper and lower bytes
		
		if (A == (uint8_t *)0x0492)
		{
			pgm_read_byte(A);
		}

		// Update checksum byte
		C[b] = C[b] + ((pgm_read_byte(A) ^ C[(b - 2) % 8]) + prev_salsa); // Read program memory from the calculated address
		C[b] = C[b] << 1; // Rotate left 1 bit
		
		// Update checksum index
		b = (b + 1) % 8;
		
		// Update Salsa_(i-1) and Salsa_i
		prev_salsa = curr_salsa;
		curr_salsa = (*chunk64)[byte_counter];
		
		byte_counter += 1;
		if (byte_counter == 64) // Generate a new block
		{
			initial_state[6] = K[0];
			initial_state[7] = K[1];
			salsa20_block(K, initial_state);
			byte_counter = 0;
		}
	}
	
	int h = 0;
	while(1)
	{
		h++;
		PORTE = h;
	}
	return 0;
}