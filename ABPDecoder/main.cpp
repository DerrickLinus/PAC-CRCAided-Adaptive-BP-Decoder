#include "define.h"

int main(int argc, char* argv[])
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

	if (argc >= 2) {
		g_outputFilename = argv[1];
	}

	Initial(ADP, SP);
	InitialAWGN(awgn);

	Simulation(SP, ADP, awgn, Statis);

	return 0;
}