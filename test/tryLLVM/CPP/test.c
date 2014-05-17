#include<stdio.h>

int main()
{
	int a[10];
	char x[10][10][10];
	a[0] = 0;
	x[0][0][0] = a[0];
	printf ("%d %d\n", a[0], x[0][0][0]);
	return 0;
}
