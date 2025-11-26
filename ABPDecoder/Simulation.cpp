#include "define.h"

/*
统计误码率
codeseq: 原始编码序列
decodeseq: 解码序列
length: 总长度N或K
shorten: 最前面shorten个符号不统计
*/
void CalculateError(int *codeSeq, int *decodeSeq, int length, int shorten, struct StatisStruct *Statis, int check_flag)
{
	int i, j;
	int errorNum = 0;

	for (i = shorten; i < length; i++)
	{
		if (codeSeq[i] != decodeSeq[i])
		{
			errorNum += 1;
			Statis->errorBits += 1;
		}
	}

	if (errorNum > 0)
	{
		Statis->errorFrames += 1;
		if (check_flag == 1)
			Statis->undetectedErrorFrames += 1;
	}

	Statis->FER = (double)(Statis->errorFrames) / (double)(Statis->testFrames);
	Statis->BER = (double)(Statis->errorBits) / (double)(Statis->testFrames) / (double)(length - shorten);
	Statis->UER = (double)(Statis->undetectedErrorFrames) / (double)(Statis->testFrames);

}

void WriteLogo(struct SPStruct *SP, struct ADPStruct *ADP)
{
	int i = 0;
	FILE *outfile;
	time_t  start;				//start time


	printf("\r                                                                             ");
	printf("\r");

	printf("******************************** ABP/SCLD Decoder ******************************\n");
	printf("* Version 1.2\n");
	printf("* N = %d, M = %d, K = %d, shorten = %d, puncture = %d, CRC = %d, rate = %5.3f\n", ADP->N, ADP->M, ADP->K, ADP->shorten, ADP->puncture, ADP->CRC_len, ADP->rate);
	printf("* Decoding Method: ");
	if (ADP->DecodingMethod == 1)
		printf("ideal-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 2)
		printf("ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 3)
		printf("SG-ABP(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 4)
		printf("List-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 5)
		printf("EC-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 6)
		printf("SCLD L = %d,system = %d\n", ADP->PAC_code->L,ADP->PAC_code->system);
	else
		printf("Error!\n");
	printf("* AWGN, BPSK, Source = %d, Seed = %d\n", SP->sourceType, 173);
	printf("******************************************************************************\n");

	start = time(NULL);
	printf("Program  starts  at: %s\n", ctime(&start));
	if (SP->SNRtype == 0)
	{
		printf(" Eb/No       NTF     NEF     NUF    FER         SER         BER         IT\n");
	}
	else
	{
		printf(" Es/No       NTF     NEF     NUF    FER         SER         BER         IT\n");
	}

	if ((outfile = fopen("Performance.txt", "a+")) == NULL)
	{
		printf("Can not open performance file !\n");
		getch();
		exit(0);
	}
	fprintf(outfile, "\n******************************** ABP/SCLD Decoder ******************************\n");
	fprintf(outfile, "* Version 1.2\n");
	fprintf(outfile, "* N = %d, M = %d, K = %d, shorten = %d, puncture = %d, CRC = %d, rate = %5.3f\n", ADP->N, ADP->M, ADP->K, ADP->shorten, ADP->puncture, ADP->CRC_len, ADP->rate);
	fprintf(outfile, "* Decoding Method: ");
	if (ADP->DecodingMethod == 1)
		fprintf(outfile, "ideal-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 2)
		fprintf(outfile, "ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 3)
		fprintf(outfile, "SG-ABP(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 4)
		fprintf(outfile, "List-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 5)
		fprintf(outfile, "EC-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %6.4f, Metric Threshold = %5.3f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damping_factor, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 6)
		fprintf(outfile, "SCLD L = %d,system = %d\n", ADP->PAC_code->L, ADP->PAC_code->system);
	else
		fprintf(outfile, "Error!\n");
	fprintf(outfile, "* AWGN, BPSK, Source = %d, Seed = %d\n", SP->sourceType, 173);
	fprintf(outfile, "******************************************************************************\n");


	fprintf(outfile, "Program  starts  at: %s\n", ctime(&start));
	if (SP->SNRtype == 0)
	{
		fprintf(outfile, " Eb/No       NTF     NEF     NUF    FER         SER         BER         IT\n");
	}
	else
	{
		fprintf(outfile, " Es/No       NTF     NEF     NUF    FER         SER         BER         IT\n");
	}
	fclose(outfile);
}

void Display(double SNR, struct StatisStruct *Statis)
{
	printf("\r");
	printf(" %5.2f %10d %6d %6d    %5.3e   %5.3e   %5.3e   %5.3f", SNR, Statis->testFrames, Statis->errorFrames, Statis->undetectedErrorFrames, Statis->FER, Statis->SER, Statis->BER, Statis->avgIterTime);
}

void WriteResult2File(double SNR, struct StatisStruct *Statis)
{
	FILE *outfile;
	if ((outfile = fopen("Performance.txt", "a+")) == NULL)
	{
		printf("Can not open performance file !\n");
		getch();
		exit(0);
	}

	fprintf(outfile, " %5.2f %10d %6d %6d    %5.3e   %5.3e   %5.3e   %5.3f\n", SNR, Statis->testFrames, Statis->errorFrames, Statis->undetectedErrorFrames, Statis->FER, Statis->SER, Statis->BER, Statis->avgIterTime);
	fclose(outfile);
}
void PACEncode(const int* u, int* c, int K, int N, int** G, int** T0,int* A,int* infoseq,int system) {
	if (system == 0) {
		Encode(u, c, N, N, G);
	}
	else {
		int* temp = new int[K];
		memset(temp, 0, sizeof(int)*K);
		int* tempcode = new int[N];
		memset(tempcode,0,sizeof(int)*N);
		
		for (int i = 0; i < K; i++)
		{
			for (int j = 0; j < K; j++)
			{
				temp[i]^= (infoseq[j] && T0[j][i]);
			}
		}
		for (int i = 0; i < K; i++)
			tempcode[A[i]] = temp[i];
		Encode(tempcode, c, N, N, G);
		delete[] temp;
		delete[] tempcode;
	}
}

void Simulation(struct SPStruct *SP, struct ADPStruct *ADP, struct AWGN *awgn, struct StatisStruct *Statis)
{
	int i, j;
	char ch;
	int pressflag = 0;

	double currentSNR = SP->startSNR;

	int *InfoSeq = new int[ADP->K];
	int *InfoSeq_add0 = new int[ADP->G_K];
	int *CodeSeq = new int[ADP->N];
	int *ChannelInBit = new int[ADP->N];//只支持BPSK
	double *ChannelOutBit = new double[ADP->N];
	double *bitsoft = new double[ADP->N];
	int *DecodeResult = new int[ADP->N];
	double SoftDeModFactor;
	ADP->PAC_code->PolarCode = new int[ADP->N];
	WriteLogo(SP, ADP);

	//计算距离度量
	//double maxWeight = 0;
	//double averageWeight = 0;
	//double curWeight = 0;

	while (currentSNR < (SP->endSNR + 0.001))
	{
		Statis->testFrames = 0;
		Statis->errorFrames = 0;
		Statis->errorBits = 0;
		Statis->errorSym = 0;
		Statis->SER = 0;
		Statis->IterTime = 0;
		Statis->undetectedErrorFrames = 0;
		awgn->snr = currentSNR;
		if (SP->SNRtype == 0)
			awgn->sigma = sqrt(0.5 / (ADP->rate * pow(10.0, (awgn->snr / 10.0))));
		else
			awgn->sigma = sqrt(0.5 / (pow(10.0, (awgn->snr / 10.0))));
		SoftDeModFactor = 2.0 / ((awgn->sigma) * (awgn->sigma));
		while (Statis->testFrames < SP->leastTestFrame || Statis->errorFrames < SP->leastErrorFrame)
		{
			Statis->testFrames += 1;

			if (SP->sourceType == 0)
			{
				// 使用全0码测试
				memset(InfoSeq, 0, sizeof(int)*ADP->K);
				memset(CodeSeq, 0, sizeof(int)*ADP->N);
			}
			else
			{

				for (int i = 0; i < ADP->K - ADP->CRC_len; i++)
					InfoSeq[i] = rand() % 2;
				CRC_ENC(InfoSeq, InfoSeq, ADP->CRC_len, ADP->K - ADP->CRC_len);

				//cout << CRC_DEC(InfoSeq, ADP->CRC_len, ADP->K) << endl;
				if (ADP->EncodeAdd0)
				{
					memset(InfoSeq_add0, 0, sizeof(int)*ADP->G_K);
					for (i = 0; i < ADP->K; i++)
					{
						InfoSeq_add0[ADP->A[i]] = InfoSeq[i];
					}
				}
				else
				{
					memcpy(InfoSeq_add0, InfoSeq, sizeof(int)*ADP->G_K);
				}
				//Encode(InfoSeq_add0, CodeSeq, ADP->G_K, ADP->N, ADP->G);
				//Encode(InfoSeq_add0, CodeSeq, ADP->G_K, ADP->N, ADP->PAC_code->G);
				
				PACEncode(InfoSeq_add0, CodeSeq, ADP->K, ADP->N, ADP->PAC_code->G, ADP->PAC_code->T0, ADP->A, InfoSeq, ADP->PAC_code->system);
			}
			memcpy(ADP->PAC_code->PolarCode, CodeSeq, sizeof(int) * ADP->N);
			memcpy(ChannelInBit, CodeSeq, sizeof(int)*ADP->N);

			// check
			/*
			if (CheckCode(ChannelInBit, ADP->M +ADP->CRC_len, ADP->Tanner) == 0)
				printf("check ok!\n");
			else {
				printf("check error!\n");
				getch();
			}
			*/
			
			// 过AWGN信道
			AWGNChannel(ChannelOutBit, ChannelInBit, awgn, ADP->N);

			// 软解调
			SoftDemodulate(bitsoft, ChannelOutBit, SoftDeModFactor, ADP->N);
			/*
			for (int i = 0; i < ADP->N; i++)
				printf("%f ", bitsoft[i]);
			*/
			if (ADP->puncture > 0)
			{
				//memset(bitsoft, 0, sizeof(double)*ADP->puncture);			// puncture the demodulated LLR
				//memset(ChannelOutBit, 0, sizeof(double)*ADP->puncture);		// puncture the received signal
				for (int i = 0; i < ADP->puncture; i++) {
					bitsoft[ADP->PunctureIndex[i]] = 0;
					ChannelOutBit[ADP->PunctureIndex[i]] = 0;
				}
			}
			if (ADP->shorten > 0)
			{
				//memset(bitsoft, 0, sizeof(double)*ADP->puncture);			// puncture the demodulated LLR
				//memset(ChannelOutBit, 0, sizeof(double)*ADP->puncture);		// puncture the received signal
				for (int i = 0; i < ADP->shorten; i++) {
					bitsoft[ADP->ShortenIndex[i]] = 10000;
					ChannelOutBit[ADP->ShortenIndex[i]] = 10000;
				}
			}
			/*
			for (int i = 0; i < ADP->K; i++)
				cout << ADP->A[i] << " ";
			cout << endl;
			for (int i = 0; i < ADP->K; i++) {
				cout << InfoSeq[i] << " ";
			}
			cout << endl;
			for (int i = 0; i < ADP->N; i++) {
				cout << CodeSeq[i] << " ";
			}
			cout << endl;
			for (int i = 0; i < ADP->N; i++) {
				cout << bitsoft[i] << " ";
			}
			*/
			
			// 解码
			Decode(bitsoft, ChannelOutBit, DecodeResult, ADP);

			// 计算weight
			//if (ADP->check_flag == 1) {
			//	for (int i = 0; i < ADP->N; i++) {
			//		DecodeResult[i] = bitsoft[i] > 0 ? 0 : 1;
			//	}
			//	curWeight = 0;
			//	for (int i = 0; i < ADP->N; i++) {
			//		curWeight = curWeight + abs(bitsoft[i]) * ((CodeSeq[i] + DecodeResult[i]) % 2);
			//	}
			//	averageWeight += curWeight;
			//	if (maxWeight < curWeight) {
			//		maxWeight = curWeight;
			//	}
			//}

			//check crc rule
			//for (int i = 0; i < ADP->N; i++)
			//	CodeSeq[i] = bitsoft[i] > 0 ? 0 : 1;
			//Recover_Info(CodeSeq, DecodeResult, ADP);
			//ADP->check_flag = 0;
			//if (CRC_DEC(DecodeResult, ADP->CRC_len, ADP->K) == 0)
			//	ADP->check_flag = 1;

			// 计算迭代次数
			Statis->IterTime += ADP->IterTime;
			Statis->avgIterTime = (double)Statis->IterTime / Statis->testFrames;

			//DSCL
			//int errorNum = 0;
			//for (int i = 0; i < ADP->K; i++)
			//{
			//	if (InfoSeq[i] != DecodeResult[i])
			//	{
			//		errorNum += 1;
			//		break;
			//	}
			//}
			//if (errorNum!=0) {
			//	ADP->PAC_code->L = 2048;
			//	Decode(bitsoft, ChannelOutBit, DecodeResult, ADP);
			//	ADP->PAC_code->L = 32;
			//}

			//if (Statis->undetectedErrorFrames >= 30)
			//	break;
			// 计算误码率
			CalculateError(InfoSeq, DecodeResult, ADP->K, 0, Statis, ADP->check_flag);

			// 记录误码率并打印到屏幕
			if (Statis->testFrames % SP->displayStep == 0) Display(currentSNR, Statis);
			if (kbhit() != 0)
			{
				printf("\r                                                                              ");
				printf("\r Continue(C), Next Point(N) or Exit(E): ");
				ch = getch();
				ch = getch();
				while ((ch != 'E') && (ch != 'e') && (ch != 'C') && (ch != 'c') && (ch != 'N') && (ch != 'n'))
				{
					ch = getch();
				}
				pressflag = 0;
				if ((ch == 'E') || (ch == 'e'))
				{
					pressflag = 1;
				}
				else
				{
					if ((ch == 'N') || (ch == 'n'))
					{
						pressflag = 2;
					}
				}
				Display(currentSNR, Statis);
				if (pressflag > 0)
				{
					break;
				}
			}
		}

		Display(currentSNR, Statis);
		printf("\n");
		WriteResult2File(currentSNR, Statis);

		currentSNR += SP->stepSNR;
		//show the result
		//averageWeight = averageWeight / Statis->testFrames;
		//printf("the maxWeight: %f, the averageWeight: %f\n", maxWeight, averageWeight);

		if (pressflag == 1)
		{
			delete[] InfoSeq;
			delete[] CodeSeq;
			delete[] ChannelInBit;
			delete[] ChannelOutBit;
			delete[] bitsoft;
			delete[] DecodeResult;
			delete[] InfoSeq_add0;
			printf("\nPress Enter to exit!");
			getch();
			exit(0);
		}
	}
	delete[] InfoSeq;
	delete[] CodeSeq;
	delete[] ChannelInBit;
	delete[] ChannelOutBit;
	delete[] bitsoft;
	delete[] DecodeResult;
	delete[] InfoSeq_add0;
}