#include "define.h"

void main()
{	
	srand(731);
	struct SPStruct *SP;
	struct AWGN *awgn;
	struct StatisStruct *Statis;
	struct ADPStruct *ADP;

	ADP = new struct ADPStruct;
	ADP->IterDec = new struct IterStruct;
	ADP->Tanner = new struct IterStruct;
	SP = new struct SPStruct;
	awgn = new struct AWGN;
	awgn->seed = new struct SEED;
	Statis = new struct StatisStruct;

	Initial(ADP, SP);
	InitialAWGN(awgn);

	Simulation(SP, ADP, awgn, Statis);
}