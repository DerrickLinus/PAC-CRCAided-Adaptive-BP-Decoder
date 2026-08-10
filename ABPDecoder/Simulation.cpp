#include "define.h"
#include <signal.h>
#include <vector>

// 全局变量，用于信号处理函数访问
static time_t g_start_time = 0;
static volatile sig_atomic_t g_program_running = 0;

const char* g_outputFilename = "Performance_v1.3.txt";

// 记录程序结束时间和运行时长（可被正常结束和信号处理共同调用）
void WriteEndTimeAndDuration(const char* reason)
{
	if (g_start_time == 0) return;  // 还未开始计时

	time_t end_time = time(NULL);
	double elapsed_seconds = difftime(end_time, g_start_time);
	int hours = (int)(elapsed_seconds / 3600);
	int minutes = (int)((elapsed_seconds - hours * 3600) / 60);
	int seconds = (int)(elapsed_seconds - hours * 3600 - minutes * 60);

	printf("\nProgram  ends    at: %s", ctime(&end_time));
	printf("Termination reason: %s\n", reason);
	printf("Total running time: %d hours %d minutes %d seconds\n", hours, minutes, seconds);
	printf("******************************************************************************\n");

	// 同时写入到文件
	FILE* outfile;
	if ((outfile = fopen(g_outputFilename, "a+")) != NULL)
	{
		fprintf(outfile, "\nProgram  ends    at: %s", ctime(&end_time));
		fprintf(outfile, "Termination reason: %s\n", reason);
		fprintf(outfile, "Total running time: %d hours %d minutes %d seconds\n", hours, minutes, seconds);
		fprintf(outfile, "******************************************************************************\n");
		fclose(outfile);
	}

	g_start_time = 0;  // 防止重复调用
}

// 信号处理函数 - 捕获 Ctrl+C
void SignalHandler(int signum)
{
	if (g_program_running)
	{
		WriteEndTimeAndDuration("User terminated (Ctrl+C)");
		g_program_running = 0;
	}
	exit(signum);
}

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

	printf("****************************** ABP/SCLD Decoder ******************************\n");
	printf("* Version 1.2\n");
	printf("* N = %d, M = %d, K = %d, A = %d, shorten = %d, puncture = %d, CRC = %d, R = %d/%d, R_net = %d/%d\n", ADP->N, ADP->M, ADP->K, ADP->K - ADP->CRC_len, ADP->shorten, ADP->puncture, ADP->CRC_len, ADP->K, ADP->N - ADP->puncture - ADP->shorten, ADP->K - ADP->CRC_len, ADP->N - ADP->puncture - ADP->shorten);
	printf("* Decoding Method: ");
	if (ADP->DecodingMethod == 1)
	{
		printf("ideal-ABP/MSA(%d, %d), Deg-2 = %d, Metric Threshold = %.2f\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->ML_metric_th);
		printf("* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
		// printf("* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha1 = %.2f, Beta1 = %.2f, Alpha2 = %.2f, Beta2 = %.2f\n",
		// 	ADP->ms_type, ADP->alpha_fixed, ADP->beta_fixed, ADP->alpha_fixed2, ADP->beta_fixed2);
		printf("* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha = %.2f, Beta = %.2f\n",
			ADP->ms_type, ADP->alpha_factor, ADP->beta_factor);
		printf("* Damping Mode = %d (0:fixed,1:linear,2:Power-low), fixed = %.2f, start = %.2f, end = %.2f, p = %.2f\n",
			ADP->damp_mode, ADP->damp_fixed, ADP->damp_start, ADP->damp_end, ADP->damp_p);
	}
	else if (ADP->DecodingMethod == 2)
	{
		printf("ABP/MSA(%d, %d), Deg-2 = %d, Metric Threshold = %.2f\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->ML_metric_th);
		printf("* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
		// printf("* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha1 = %.2f, Beta1 = %.2f, Alpha2 = %.2f, Beta2 = %.2f\n",
		// 	ADP->ms_type, ADP->alpha_fixed, ADP->beta_fixed, ADP->alpha_fixed2, ADP->beta_fixed2);
		printf("* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha = %.2f, Beta = %.2f\n",
			ADP->ms_type, ADP->alpha_factor, ADP->beta_factor);
		printf("* Damping Mode = %d (0:fixed,1:linear,2:Power-low), fixed = %.2f, start = %.2f, end = %.2f, p = %.2f\n",
			ADP->damp_mode, ADP->damp_fixed, ADP->damp_start, ADP->damp_end, ADP->damp_p);
	}
	else if (ADP->DecodingMethod == 3)
		printf("SG-ABP(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 4)
		printf("List-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 5)
		printf("EC-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 6)
		printf("SCLD L = %d,system = %d\n", ADP->PAC_code->L,ADP->PAC_code->system);
	else
		printf("Error!\n");
	//printf("* Convergence Early Stop: epsilon = %g, window = %d\n", ADP->convergence_epsilon, ADP->convergence_window); // 3.14修改
	printf("* AWGN, BPSK, Source = %d, Seed = %d, %s, srand(main)\n", SP->sourceType, 173, strrchr(g_outputFilename, '\\') ? strrchr(g_outputFilename, '\\') + 1 : (strrchr(g_outputFilename, '/') ? strrchr(g_outputFilename, '/') + 1 : g_outputFilename));
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

	if ((outfile = fopen(g_outputFilename, "a+")) == NULL)
	{
		printf("Can not open performance file !\n");
		getch();
		exit(0);
	}
	fprintf(outfile, "\n****************************** ABP/SCLD Decoder ******************************\n");
	fprintf(outfile, "* Version 1.2\n");
	fprintf(outfile, "* N = %d, M = %d, K = %d, A = %d, shorten = %d, puncture = %d, CRC = %d, R = %d/%d, R_net = %d/%d\n", ADP->N, ADP->M, ADP->K, ADP->K - ADP->CRC_len, ADP->shorten, ADP->puncture, ADP->CRC_len, ADP->K, ADP->N - ADP->puncture - ADP->shorten, ADP->K - ADP->CRC_len, ADP->N - ADP->puncture - ADP->shorten);
	fprintf(outfile, "* Decoding Method: ");
	if (ADP->DecodingMethod == 1)
	{
		fprintf(outfile, "ideal-ABP/MSA(%d, %d), Deg-2 = %d, Metric Threshold = %.2f\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->ML_metric_th);
		fprintf(outfile, "* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
		// fprintf(outfile, "* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha1 = %.2f, Beta1 = %.2f, Alpha2 = %.2f, Beta2 = %.2f\n",
		// 	ADP->ms_type, ADP->alpha_fixed, ADP->beta_fixed, ADP->alpha_fixed2, ADP->beta_fixed2);
		fprintf(outfile, "* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha = %.2f, Beta = %.2f\n",
			ADP->ms_type, ADP->alpha_factor, ADP->beta_factor);
		fprintf(outfile, "* Damping Mode = %d (0:fixed,1:linear,2:Power-low), fixed = %.2f, start = %.2f, end = %.2f, p = %.2f\n",
			ADP->damp_mode, ADP->damp_fixed, ADP->damp_start, ADP->damp_end, ADP->damp_p);
	}
	else if (ADP->DecodingMethod == 2)
	{
		fprintf(outfile, "ABP/MSA(%d, %d), Deg-2 = %d, Metric Threshold = %.2f\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->ML_metric_th);
		fprintf(outfile, "* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
		// fprintf(outfile, "* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha1 = %.2f, Beta1 = %.2f, Alpha2 = %.2f, Beta2 = %.2f\n",
		// 	ADP->ms_type, ADP->alpha_fixed, ADP->beta_fixed, ADP->alpha_fixed2, ADP->beta_fixed2);
		fprintf(outfile, "* MS Type = %d (0:MS, 1:NMS, 2:OMS, 3:NMS+OMS), Alpha = %.2f, Beta = %.2f\n",
			ADP->ms_type, ADP->alpha_factor, ADP->beta_factor);
		fprintf(outfile, "* Damping Mode = %d (0:fixed,1:linear,2:Power-low), fixed = %.2f, start = %.2f, end = %.2f, p = %.2f\n",
			ADP->damp_mode, ADP->damp_fixed, ADP->damp_start, ADP->damp_end, ADP->damp_p);
	}
	else if (ADP->DecodingMethod == 3)
		fprintf(outfile, "SG-ABP(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 4)
		fprintf(outfile, "List-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 5)
		fprintf(outfile, "EC-ABP/MSA(%d, %d), Deg-2 = %d, Damping Factor = %.2f, Metric Threshold = %.2f\n* Interchange = %d, Use CRC = %d, Use Channel LLR = %d\n", ADP->N1, ADP->N2, ADP->Deg2, ADP->damp_fixed, ADP->ML_metric_th, ADP->Interchange, ADP->CRC_len_for_ABP, ADP->use_channel_LLR);
	else if (ADP->DecodingMethod == 6)
		fprintf(outfile, "SCLD L = %d,system = %d\n", ADP->PAC_code->L, ADP->PAC_code->system);
	else
		fprintf(outfile, "Error!\n");
	//fprintf(outfile, "* Convergence Early Stop: epsilon = %g, window = %d\n", ADP->convergence_epsilon, ADP->convergence_window); // 3.14修改
	fprintf(outfile, "* AWGN, BPSK, Source = %d, Seed = %d, %s, srand(main)\n", SP->sourceType, 173, strrchr(g_outputFilename, '\\') ? strrchr(g_outputFilename, '\\') + 1 : (strrchr(g_outputFilename, '/') ? strrchr(g_outputFilename, '/') + 1 : g_outputFilename));
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
	if ((outfile = fopen(g_outputFilename, "a+")) == NULL)
	{
		printf("Can not open performance file !\n");
		getch();
		exit(0);
	}

	fprintf(outfile, " %5.2f %10d %6d %6d    %5.3e   %5.3e   %5.3e   %5.3f\n", SNR, Statis->testFrames, Statis->errorFrames, Statis->undetectedErrorFrames, Statis->FER, Statis->SER, Statis->BER, Statis->avgIterTime);
	fclose(outfile);
}
void PACEncode(const int* u, int* c, int K, int N, int** G, int** T0,int* A,int* infoseq,int system,
               int* temp, int* tempcode) {
	if (system == 0) {
		Encode(u, c, N, N, G);
	}
	else {
		memset(temp, 0, sizeof(int)*K);
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
	}
}

// ==================== OpenMP 多帧并行仿真 ====================
void Simulation(struct SPStruct *SP, struct ADPStruct *ADP, struct AWGN *awgn, struct StatisStruct *Statis)
{
	int i, j;
	char ch;
	int pressflag = 0;

	double currentSNR = SP->startSNR;

	WriteLogo(SP, ADP);

	signal(SIGINT, SignalHandler);
	g_start_time = time(NULL);
	g_program_running = 1;

	// ---- 获取系统线程数 ----
	int numThreads = omp_get_max_threads();
	printf("* OpenMP Threads: %d\n", numThreads);

	int N = ADP->N, K = ADP->K, GK = ADP->G_K;
	int M_ABP = ADP->M + ADP->CRC_len_for_ABP;
	int CRCLen = ADP->CRC_len;

	// ---- 每线程独立资源 ----
	vector<int*>       thrInfoSeq(numThreads), thrInfoSeq_add0(numThreads);
	vector<int*>       thrCodeSeq(numThreads), thrChannelInBit(numThreads);
	vector<double*>    thrChannelOutBit(numThreads), thrBitsoft(numThreads);
	vector<int*>       thrDecodeResult(numThreads);
	vector<int*>       thrDeg2RandSeq(numThreads);
	vector<int*>       thrPolarCode(numThreads);
	vector<int*>       thrEncodeTempK(numThreads);  // PACEncode scratch: size K
	vector<int*>       thrEncodeTempN(numThreads);  // PACEncode scratch: size N
	vector<DecodePool> thrDecodePools(numThreads);  // Decode scratch pool
	vector<SEED>       thrSeeds(numThreads);
	vector<AWGN>       thrAWGNs(numThreads);
	vector<mt19937>    thrRNGs(numThreads);
	vector<IterStruct> thrIterDecs(numThreads);
	vector<ADPStruct>  thrADPs(numThreads);
	vector<PACStruct>  thrPACCodes(numThreads);
	vector<StatisStruct> thrStats(numThreads);

	for (int t = 0; t < numThreads; t++)
	{
		// 帧缓冲区
		thrInfoSeq[t]       = new int[K];
		thrInfoSeq_add0[t]  = new int[GK];
		thrCodeSeq[t]       = new int[N];
		thrChannelInBit[t]  = new int[N];
		thrChannelOutBit[t] = new double[N];
		thrBitsoft[t]       = new double[N];
		thrDecodeResult[t]  = new int[N];

		// AWGN 噪声（每线程独立种子）
		thrSeeds[t].ix = 173 + t * 1000;
		thrSeeds[t].iy = 173 + t * 1000;
		thrSeeds[t].iz = 173 + t * 1000;
		thrAWGNs[t].seed = &thrSeeds[t];
		thrAWGNs[t].seedmethod = awgn->seedmethod;

		// 信息比特随机生成器（每线程独立）
		thrRNGs[t].seed(731 + t * 10000);

		// Deg2RandSeq（每线程独立初始排列）
		thrDeg2RandSeq[t] = new int[M_ABP];
		for (int k = 0; k < M_ABP; k++) thrDeg2RandSeq[t][k] = k;
		for (int k = M_ABP - 1; k > 0; k--) {
			int jj = thrRNGs[t]() % (k + 1);
			swap(thrDeg2RandSeq[t][k], thrDeg2RandSeq[t][jj]);
		}

		// PolarCode 副本
		thrPolarCode[t] = new int[N];

		// PACEncode 临时缓冲区（避免每帧 new/delete）
		thrEncodeTempK[t] = new int[K];
		thrEncodeTempN[t] = new int[N];

		// 克隆 ADP：浅拷贝 + 重定向可变字段
		thrADPs[t] = *ADP;
		MallocIter(M_ABP, N, &thrIterDecs[t]);
		thrADPs[t].IterDec = &thrIterDecs[t];
		thrADPs[t].Deg2RandSeq = thrDeg2RandSeq[t];

		// 克隆 PAC_code（浅拷贝 + 独立 PolarCode）
		thrPACCodes[t] = *ADP->PAC_code;
		thrPACCodes[t].PolarCode = thrPolarCode[t];
		thrADPs[t].PAC_code = &thrPACCodes[t];

		InitDecodePool(&thrDecodePools[t], ADP);

		memset(&thrStats[t], 0, sizeof(StatisStruct));
	}

	// ==================== 主 SNR 循环 ====================
	while (currentSNR < (SP->endSNR + 0.001))
	{
		// 全局统计复位（每个 SNR 点重新统计）
		Statis->testFrames = 0; Statis->errorFrames = 0;
		Statis->errorBits = 0; Statis->errorSym = 0;
		Statis->SER = 0; Statis->IterTime = 0;
		Statis->undetectedErrorFrames = 0;

		awgn->snr = currentSNR;
		if (SP->SNRtype == 0)
			awgn->sigma = sqrt(0.5 / (ADP->rate * pow(10.0, (awgn->snr / 10.0))));
		else
			awgn->sigma = sqrt(0.5 / (pow(10.0, (awgn->snr / 10.0))));
		double SoftDeModFactor = 2.0 / (awgn->sigma * awgn->sigma);

		// 每线程 AWGN 参数同步
		for (int t = 0; t < numThreads; t++) {
			thrAWGNs[t].snr = currentSNR;
			thrAWGNs[t].sigma = awgn->sigma;
			thrAWGNs[t].isigma = 1.0 / (awgn->sigma * awgn->sigma);
		}

		// ==================== 分批并行帧仿真 ====================
		while (Statis->testFrames < SP->leastTestFrame || Statis->errorFrames < SP->leastErrorFrame)
		{
			// 确定本批帧数：至少 200 帧，但不超 displayStep
			int remainingTest = SP->leastTestFrame - Statis->testFrames;
			if (remainingTest < 0) remainingTest = 0;
			int batchSize = min(SP->displayStep, max(200, remainingTest));

			// 清零本批线程统计
			for (int t = 0; t < numThreads; t++)
				memset(&thrStats[t], 0, sizeof(StatisStruct));

#pragma omp parallel
			{
				int tid = omp_get_thread_num();
				ADPStruct* localADP = &thrADPs[tid];

#pragma omp for schedule(static)
				for (int f = 0; f < batchSize; f++)
				{
					// ---- 生成信息比特 ----
					if (SP->sourceType == 0) {
						memset(thrInfoSeq[tid], 0, sizeof(int) * K);
						memset(thrCodeSeq[tid], 0, sizeof(int) * N);
					}
					else {
						for (int i = 0; i < K - CRCLen; i++)
							thrInfoSeq[tid][i] = thrRNGs[tid]() % 2;

						CRC_ENC(thrInfoSeq[tid], thrInfoSeq[tid], CRCLen, K - CRCLen);

						if (ADP->EncodeAdd0) {
							memset(thrInfoSeq_add0[tid], 0, sizeof(int) * GK);
							for (int i = 0; i < K; i++)
								thrInfoSeq_add0[tid][ADP->A[i]] = thrInfoSeq[tid][i];
						}
						else {
							memcpy(thrInfoSeq_add0[tid], thrInfoSeq[tid], sizeof(int) * GK);
						}

						PACEncode(thrInfoSeq_add0[tid], thrCodeSeq[tid], K, N,
							ADP->PAC_code->G, ADP->PAC_code->T0, ADP->A, thrInfoSeq[tid], ADP->PAC_code->system,
							thrEncodeTempK[tid], thrEncodeTempN[tid]);
					}

					memcpy(thrPolarCode[tid], thrCodeSeq[tid], sizeof(int) * N);
					memcpy(thrChannelInBit[tid], thrCodeSeq[tid], sizeof(int) * N);

					// ---- AWGN 信道 ----
					AWGNChannel(thrChannelOutBit[tid], thrChannelInBit[tid], &thrAWGNs[tid], N);

					// ---- 软解调 ----
					SoftDemodulate(thrBitsoft[tid], thrChannelOutBit[tid], SoftDeModFactor, N);

					// ---- 打孔 / 缩短处理 ----
					if (ADP->puncture > 0) {
						for (int i = 0; i < ADP->puncture; i++) {
							thrBitsoft[tid][ADP->PunctureIndex[i]] = 0;
							thrChannelOutBit[tid][ADP->PunctureIndex[i]] = 0;
						}
					}
					if (ADP->shorten > 0) {
						for (int i = 0; i < ADP->shorten; i++) {
							thrBitsoft[tid][ADP->ShortenIndex[i]] = 10000;
							thrChannelOutBit[tid][ADP->ShortenIndex[i]] = 10000;
						}
					}

					// ---- 译码 ----
					Decode(currentSNR, thrBitsoft[tid], thrChannelOutBit[tid], thrDecodeResult[tid], localADP, &thrDecodePools[tid]);

					// ---- 误码统计 ----
					int errorNum = 0;
					for (int i = ADP->shorten; i < K; i++) {
						if (thrInfoSeq[tid][i] != thrDecodeResult[tid][i]) {
							errorNum++;
							thrStats[tid].errorBits++;
						}
					}
					thrStats[tid].testFrames++;
					thrStats[tid].IterTime += localADP->IterTime;
					if (errorNum > 0) {
						thrStats[tid].errorFrames++;
						if (localADP->check_flag == 1)
							thrStats[tid].undetectedErrorFrames++;
					}
				}
			} // end omp parallel

			// ---- 归约线程统计到全局 ----
			for (int t = 0; t < numThreads; t++) {
				Statis->testFrames  += thrStats[t].testFrames;
				Statis->errorFrames += thrStats[t].errorFrames;
				Statis->errorBits   += thrStats[t].errorBits;
				Statis->undetectedErrorFrames += thrStats[t].undetectedErrorFrames;
				Statis->IterTime    += thrStats[t].IterTime;
			}

			// 衍生统计
			int statLen = K - ADP->shorten;
			if (Statis->testFrames > 0) {
				Statis->FER = (double)Statis->errorFrames / (double)Statis->testFrames;
				Statis->BER = (double)Statis->errorBits / (double)Statis->testFrames / (double)statLen;
				Statis->UER = (double)Statis->undetectedErrorFrames / (double)Statis->testFrames;
				Statis->avgIterTime = (double)Statis->IterTime / Statis->testFrames;
			}

			// 屏幕显示
			if (Statis->testFrames % SP->displayStep == 0)
				Display(currentSNR, Statis);

			// 键盘交互
			if (kbhit() != 0)
			{
				ch = getch();
				while ((ch != 'E') && (ch != 'e') && (ch != 'C') && (ch != 'c') && (ch != 'N') && (ch != 'n'))
					ch = getch();
				if ((ch == 'E') || (ch == 'e'))
					pressflag = 1;
				else if ((ch == 'N') || (ch == 'n'))
					pressflag = 2;
				Display(currentSNR, Statis);
			}
			if (pressflag > 0) break;
		} // end 批次循环

		Display(currentSNR, Statis);
		printf("\n");
		WriteResult2File(currentSNR, Statis);

		currentSNR += SP->stepSNR;

		if (pressflag == 1) {
			WriteEndTimeAndDuration("User exit (pressed E)");
			g_program_running = 0;
			goto cleanup;
		}
	} // end SNR 循环

	WriteEndTimeAndDuration("Normal completion");
	g_program_running = 0;

cleanup:
	// ---- 释放每线程资源 ----
	for (int t = 0; t < numThreads; t++) {
		delete[] thrInfoSeq[t];       delete[] thrInfoSeq_add0[t];
		delete[] thrCodeSeq[t];       delete[] thrChannelInBit[t];
		delete[] thrChannelOutBit[t];  delete[] thrBitsoft[t];
		delete[] thrDecodeResult[t];  delete[] thrDeg2RandSeq[t];
		delete[] thrPolarCode[t];
		delete[] thrEncodeTempK[t];    delete[] thrEncodeTempN[t];
		FreeDecodePool(&thrDecodePools[t], ADP);

		// 释放 IterDec
		delete[] thrIterDecs[t].CNdegree;
		for (int i = 0; i < M_ABP; i++) {
			delete[] thrIterDecs[t].CNindex[i];
			delete[] thrIterDecs[t].R[i];
		}
		delete[] thrIterDecs[t].CNindex;
		delete[] thrIterDecs[t].R;
		delete[] thrIterDecs[t].pLLR;
	}
}
