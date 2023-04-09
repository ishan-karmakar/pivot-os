#include "string.h"

size_t strlen(const char *str) {
	const char *s;
	for (s = str; *s; ++s)
		;
	return (s - str);
}
int
itoa(long long num, char* str, int len, int base)
{
	long long sum = num;
	int i = 0;
	int digit;
	if (len == 0)
		return -1;
	do
	{
		digit = sum % base;
		if (digit < 0xA)
			str[i++] = '0' + digit;
		else
			str[i++] = 'A' + digit - 0xA;
		sum /= base;
	}while (sum && (i < (len - 1)));
	if (i == (len - 1) && sum)
		return -1;
	str[i] = '\0';
	strrev(str);
	return 0;
}
void
strrev(char *str)
{
	int i;
	int j;
	char a;
	unsigned len = strlen(str);
	for (i = 0, j = len - 1; i < j; i++, j--)
	{
		a = str[i];
		str[i] = str[j];
		str[j] = a;
	}
}