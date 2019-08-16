#include "grouptext.h"

static const char en85[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z',
	'!', '#', '$', '%', '&', '(', ')', '*', '+', '-',
	';', '<', '=', '>', '?', '@', '^', '_',	'`', '{',
	'|', '}', '~'
};

static char de85[256];
static void prep_base85(void)
{
	int i;
	if (de85['Z'])
		return;
	for (i = 0; i < ARRAY_SIZE(en85); i++) {
		int ch = en85[i];
		de85[ch] = i + 1;
	}
}

int decode_85(char *dst, const char *buffer, int len, int * outLen)
{
	prep_base85();

	while (len) {
		unsigned acc = 0;
		int de, cnt = 4;
		unsigned char ch;
		do {
			ch = *buffer++;
			de = de85[ch];
			if (--de < 0)
				return ch;
			acc = acc * 85 + de;
		} while (--cnt);
		ch = *buffer++;
		de = de85[ch];
		if (--de < 0)
			return ch;
		/* Detect overflow. */
		if (0xffffffff / 85 < acc ||
		    0xffffffff - de < (acc *= 85))
			return buffer-5;
		acc += de;

		cnt = (len < 4) ? len : 4;
		len -= cnt;
		do {
			acc = (acc << 8) | (acc >> 24);
			*dst++ = acc;
			*outLen += 1;
		} while (--cnt);
	}

	return 0;
}

void encode_85(char *buf, const unsigned char *data, int bytes)
{
	while (bytes) {
		unsigned acc = 0;
		int cnt;
		for (cnt = 24; cnt >= 0; cnt -= 8) {
			unsigned ch = *data++;
			acc |= ch << cnt;
			if (--bytes == 0)
				break;
		}
		for (cnt = 4; cnt >= 0; cnt--) {
			int val = acc % 85;
			acc /= 85;
			buf[cnt] = en85[val];
		}
		buf += 5;
	}

	*buf = 0;
}
