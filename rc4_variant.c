/*
 * main.c
 *
 * Created: 5/10/2021 12:19:27 PM
 *  Author: patri
 */ 

#include <xc.h>
#include <avr/pgmspace.h>

void swap(uint8_t *a, uint8_t *b) {
	uint8_t tmp = *a;
	*a = *b;
	*b = tmp;
}

int main(void)
{
	// RC4 Setup
	
	uint8_t key[40] = {3, 7, 4, 12, 13, 24, 14, 17, 14, 4, 6, 2, 24, 15, 17, 27, 7, 17, 21, 6, 12, 13, 9, 9, 20, 30, 12, 8, 24, 2, 3, 25, 21, 9, 24, 30, 18, 17, 12, 16};
	uint8_t keylength = 40;
	
	// KSA
	uint8_t S[256] = {0};
	uint8_t i = 0;
	for (i = 0;; i++) // Initialize S to the identity permutation
	{
		S[i] = i;
		if (i == 255) break;
	}
	uint8_t j = 0;
	for (i = 0;; i++)
	{
		j = (j + S[i] + key[i % keylength]) % 256;
		swap(&S[i], &S[j]);
		if (i == 255) break;
	}
	
	// PRGA
	unsigned long m = 50000; // Number of loop iterations
	unsigned int y = 0; // Generic loop counter
	i = 0;
	j = 0;
	// Discard the first 256 bytes
	while (y < 256)
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(&S[i], &S[j]);
		y++;
	}
	
	uint8_t C[8] = {0};
	for (y = 0; y < 8; y++)
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(&S[i], &S[j]);
		C[y] = S[(S[i] + S[j]) % 256]; // C is our output stream of first 8 bytes (Initialize Checksum vector)
	}
	
	unsigned long k = 0; // Generic loop counter for the main loop
	uint8_t* A = 0x0000; // 16-bit Address for program memory access
	int b = 0; // j in the pseudocode
	
	// Initialize RC4_(i-1) and RC4_i
	uint8_t prev_rc4;
	uint8_t curr_rc4;
	i = (i + 1) % 256;
	j = (j + S[i]) % 256;
	swap(&S[i], &S[j]);
	prev_rc4 = S[(S[i] + S[j]) % 256];
	i = (i + 1) % 256;
	j = (j + S[i]) % 256;
	swap(&S[i], &S[j]);
	curr_rc4 = S[(S[i] + S[j]) % 256];
	
	for (k = 0; k < m; ++k)
	{
		
		A = (uint8_t *)(curr_rc4 << 8) + C[(b - 1) % 8]; // Build 16-bit address upper and lower bytes
		
		if (A == (uint8_t *)0x0492)
		{
			pgm_read_byte(A);
		}
		
		// Update checksum byte
		C[b] = C[b] + ((pgm_read_byte(A) ^ C[(b - 2) % 8]) + prev_rc4); // Read program memory from the calculated address
		C[b] = C[b] << 1; // Rotate left 1 bit
		
		// Update checksum index
		b = (b + 1) % 8;
		
		// Update RC4_(i-1) and RC4_i
		prev_rc4 = curr_rc4;
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(&S[i], &S[j]);
		curr_rc4 = S[(S[i] + S[j]) % 256];
	}
	
	int h = 0;
	while(1)
	{
		h++;
		PORTE = h;
	}
	return 0;
}