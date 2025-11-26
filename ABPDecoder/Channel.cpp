#include "define.h"


double RandomModule(struct SEED *seed)
{
	double temp = 0.0;

	seed->ix = (seed->ix * 249) % 61967;
	seed->iy = (seed->iy * 251) % 63443;
	seed->iz = (seed->iz * 252) % 63599;
	temp = (((double)seed->ix) / ((double)61967)) + (((double)seed->iy) / ((double)63443))
		+ (((double)seed->iz) / ((double)63599));
	temp -= (int)temp;

	return (temp);
}

void RandomSFR(double *u1, double *u2, struct SEED *seed)
{
	unsigned long   b1, b2;

	b1 = (((seed->ix) << 13) ^ (seed->ix)) >> 19;
	seed->ix = (((seed->ix) & 0xfffffffe) << 12) ^ b1;
	b1 = (((seed->iy) << 2) ^ (seed->iy)) >> 25;
	seed->iy = (((seed->iy) & 0xfffffff8) << 4) ^ b1;
	b1 = (((seed->iz) << 3) ^ (seed->iz)) >> 11;
	seed->iz = (((seed->iz) & 0xfffffff0) << 17) ^ b1;
	b1 = (seed->ix) ^ (seed->iy) ^ (seed->iz);

	b2 = (((seed->ixx) << 13) ^ (seed->ixx)) >> 19;
	seed->ixx = (((seed->ixx) & 0xfffffffe) << 12) ^ b2;
	b2 = (((seed->iyy) << 2) ^ (seed->iyy)) >> 25;
	seed->iyy = (((seed->iyy) & 0xfffffff8) << 4) ^ b2;
	b2 = (((seed->izz) << 3) ^ (seed->izz)) >> 11;
	seed->izz = (((seed->izz) & 0xfffffff0) << 17) ^ b2;
	b2 = (seed->ixx) ^ (seed->iyy) ^ (seed->izz);

	*u1 = ((b2 & 0xffff0000) >> 16) / ((double)(unsigned long)0x10000);
	*u1 = (*u1 + b1) / ((double)(unsigned long)0x80000000) / 2;
	*u2 = (b2 & 0xffff) / ((double)(unsigned long)0x10000);
}


void AWGNChannel(double *receiveseq, int *codeseq, struct AWGN *awgn, int length)
{
	int i = 0;
	double u1;
	double u2;
	double temp = 0;

	for (i = 0; i<length; i++)
	{
		switch (awgn->seedmethod)
		{
		case 1: u1 = RandomModule(awgn->seed);
			u2 = RandomModule(awgn->seed);
			break;
		case 2: RandomSFR(&u1, &u2, awgn->seed);
			break;
		}
		temp = (double)sqrt((double)(-2) * log((double)1 - u1));
		*(receiveseq + i) = (awgn->sigma) * sin(2 * PI * u2) * temp + (1 - (*(codeseq + i)) - (*(codeseq + i)));
	}
}

void SoftDemodulate(double *bitsoft, double *receiveseq, double factor, int length)
{
	int i = 0;

	//BPSK Soft Demodulate
	for (i = 0; i<length; i++)
	{
		*(bitsoft + i) = factor * (*(receiveseq + i));
	}
}

// 在解调后处理似然比，模拟shorten
void FakeShorten(double *bitsoft, int shorten)
{
	int i = 0;
	for (i = 0; i < shorten; i++)
	{
		bitsoft[i] = 1000.0;   //filled bits = 0 --> BPSK +1, 防止OSD算欧式距离出错, 没有设为MAXVALUE
	}
}