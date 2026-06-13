#include "define.h"

void ReadProfile(struct ADPStruct *ADP, struct SPStruct *SP)
{
	FILE *profile;

	if ((profile = fopen("Profile.txt", "r")) == NULL)
	{
		printf("Can not open Profile.txt !\n");
		getch();
		exit(0);
	}

	fscanf(profile, "%*s%*s%*s");
	fscanf(profile, "%*s%s", ADP->codefile);
	fscanf(profile, "%*s%d", &(ADP->N));
	fscanf(profile, "%*s%d", &(ADP->K));
	//ADP->shorten = 0;													// shortening is not supported
	fscanf(profile, "%*s%d", &(ADP->CRC_len));
	fscanf(profile, "%*s%*s%d", &(ADP->EncodeAdd0));
	fscanf(profile, "%*s%d", &(ADP->puncture));
	fscanf(profile, "%*s%d", &(ADP->shorten));
	fscanf(profile, "%*s%*s%d", &(ADP->DecodingMethod));

	fscanf(profile, "%*s%*s%d%d", &(ADP->N1), &(ADP->N2));
	fscanf(profile, "%*s%*s%d", &(ADP->Deg2));
	fscanf(profile, "%*s%*s%d", &(ADP->Interchange));
	fscanf(profile, "%*s%*s%*s%d", &(ADP->CRC_len_for_ABP));
	fscanf(profile, "%*s%*s%*s%d", &(ADP->use_channel_LLR));
	fscanf(profile, "%*s%*s%lf", &(ADP->ML_metric_th));
	fscanf(profile, "%*s%*s%d", &(ADP->PAC_code->L));
	fscanf(profile, "%*s%*s%d", &(ADP->PAC_code->system));

	fscanf(profile, "%*s%*s%d", &(ADP->ms_type));
	fscanf(profile, "%*s%*s%lf", &(ADP->alpha_fixed));
	fscanf(profile, "%*s%*s%lf", &(ADP->beta_fixed));
	fscanf(profile, "%*s%*s%lf", &(ADP->alpha_fixed2));
	fscanf(profile, "%*s%*s%lf", &(ADP->beta_fixed2));

	fscanf(profile, "%*s%*s%lf", &(ADP->convergence_epsilon));
	fscanf(profile, "%*s%*s%d", &(ADP->convergence_window));

	fscanf(profile, "%*s%*s%d", &(ADP->damp_mode));
	fscanf(profile, "%*s%*s%lf", &(ADP->damp_fixed));
	fscanf(profile, "%*s%*s%lf", &(ADP->damp_start));
	fscanf(profile, "%*s%*s%lf", &(ADP->damp_end));
	fscanf(profile, "%*s%*s%lf", &(ADP->damp_p));

	if (ADP->damp_mode < 0 || ADP->damp_mode > 2)
	{
		fprintf(stderr, "Invalid damping mode %d. Expected 0, 1, or 2.\n", ADP->damp_mode);
		exit(EXIT_FAILURE);
	}
	if ((ADP->damp_mode == 1 || ADP->damp_mode == 2) && ADP->damp_start < 0.0)
	{
		fprintf(stderr, "Invalid equal-mean damping amplitude A = %.8f. A must be nonnegative.\n", ADP->damp_start);
		exit(EXIT_FAILURE);
	}
	if (ADP->damp_mode == 2 && ADP->damp_p < 0.0)
	{
		fprintf(stderr, "Invalid equal-mean power exponent p = %.8f. p must be nonnegative.\n", ADP->damp_p);
		exit(EXIT_FAILURE);
	}
	if ((ADP->damp_mode == 1 || ADP->damp_mode == 2) && ADP->N1 > 1)
	{
		const double exponent = (ADP->damp_mode == 1) ? 1.0 : ADP->damp_p;
		double shape_sum = 0.0;
		for (int k = 0; k < ADP->N1; k++)
		{
			const double position = 1.0 - static_cast<double>(k) / static_cast<double>(ADP->N1 - 1);
			shape_sum += pow(position, exponent);
		}
		const double shape_mean = shape_sum / static_cast<double>(ADP->N1);
		const double minimum_damping = ADP->damp_fixed - ADP->damp_start * shape_mean;
		if (minimum_damping < 0.0)
		{
			fprintf(stderr,
				"Invalid equal-mean damping parameters: mu = %.8f, A = %.8f, p = %.8f produce minimum damping %.8f.\n",
				ADP->damp_fixed, ADP->damp_start, exponent, minimum_damping);
			exit(EXIT_FAILURE);
		}
	}

	fscanf(profile, "%*s%*s%d", &(SP->SNRtype));
	fscanf(profile, "%*s%*s%lf", &(SP->startSNR));
	fscanf(profile, "%*s%*s%lf", &(SP->endSNR));
	fscanf(profile, "%*s%*s%lf", &(SP->stepSNR));
	fscanf(profile, "%*s%*s%*s%d", &(SP->leastTestFrame));
	fscanf(profile, "%*s%*s%*s%d", &(SP->leastErrorFrame));
	fscanf(profile, "%*s%*s%d", &(SP->sourceType));
	fscanf(profile, "%*s%*s%d", &(SP->displayStep));

	fclose(profile);
}

void ReadCodefile(struct ADPStruct *ADP)
{
	int i, j;

	FILE *codefile;
	if ((codefile = fopen(ADP->codefile, "r")) == NULL)
	{
		printf("Can not open Codefile.txt !\n");
		getch();
		exit(0);
	}

	// read the generator matrix G=F⊗n
	ADP->G_K = (ADP->EncodeAdd0 ? ADP->N : ADP->K);
	ADP->G = new int*[ADP->G_K];
	for (i = 0; i < ADP->G_K; i++)
		ADP->G[i] = new int[ADP->N];
	fscanf(codefile, "%*s%*s%*s");
	for (i = 0; i < ADP->G_K; i++)
	{
		for (j = 0; j < ADP->N; j++)
		{
			fscanf(codefile, "%d", &(ADP->G[i][j]));
		}
	}

	// 采用函数生成 the generator matrix G=F⊗n (for test)
	//int n = 7;
	//ADP->G_K = (ADP->EncodeAdd0 ? ADP->N : ADP->K);
	//ADP->G = new int*[ADP->G_K];
	//for (i = 0; i < ADP->G_K; i++)
	//	ADP->G[i] = new int[ADP->N];
	//ADP->G = generate_polar_matrix(n);
	//cout << "Matrix P_n:" << endl;
	////printf("Matrix P_n:\n");
	//for (int i = 0; i < ADP->N; i++) {
	//	for (int j = 0; j < ADP->N; j++) {
	//		cout << ADP->G[i][j] << " ";
	//		//printf("%d ", ADP->G[i][j]);
	//	}
	//	cout << endl;
	//	//printf("\n");
	//}

	// read the parity-check matrix H
	ADP->M = ADP->N - ADP->K;
	ADP->H = new int*[ADP->M];

	// read the precoding matrix P
	//edited by Zhangxianwen 
	//H constructed from function
	ADP->P = new int*[ADP->G_K];
	for (i = 0; i < ADP->G_K; i++)
	{
		ADP->P[i] = new int[ADP->G_K];
	}

	// read the active set
	// 使用RM 准则，忽略从文件中读取的方法
	if (ADP->EncodeAdd0)
	{
		ADP->A = new int[ADP->K];
		fscanf(codefile, "%*s%*s%*s");
		/*
		for (i = 0; i < ADP->K; i++)
		{
			fscanf(codefile, "%d", &(ADP->A[i]));
			ADP->A[i]--;
		}
		*/
	}

	fclose(codefile);
}

// 创建迭代译码所需的变量，由于ADP会改变Tanner连接，按全连接创建
void MallocIter(int M, int N, struct IterStruct *Iter)
{
	Iter->CNdegree = new int[M];
	Iter->CNindex = new int*[M];
	for (int i = 0; i < M; i++)
	{
		Iter->CNindex[i] = new int[N];
	}
	Iter->R = new double* [M];
	for (int i = 0; i < M; i++)
		Iter->R[i] = new double[N];
	Iter->pLLR = new double[N];
}

// 初始化迭代译码所需参数，M,N,H均为比特级
//void InitialIter(int M, int N, int **H, struct IterStruct *Iter) // 源代码，每次内层迭代都需要扫描两遍H矩阵
//{
//	for (int i = 0; i < M; i++) Iter->CNdegree[i] = 0;
//	for (int i = 0; i < M; i++)
//	{
//		for (int j = 0; j < N; j++)
//		{
//			if (H[i][j] == 1)
//				Iter->CNdegree[i] += 1;
//		}
//	}
//
//	for (int i = 0; i < M; i++)
//	{
//		int k = 0;
//		for (int j = 0; j < N; j++)
//		{
//			if (H[i][j] == 1)
//				Iter->CNindex[i][k++] = j;
//		}
//	}
//}
void InitialIter(int M, int N, int** H, struct IterStruct* Iter) // 性能优化版，将两遍扫描合并为一遍扫描
{
	for (int i = 0; i < M; i++) {
		int k = 0;
		for (int j = 0; j < N; j++) {
			if (H[i][j] == 1)
				Iter->CNindex[i][k++] = j;
		}
		Iter->CNdegree[i] = k;
	}
}

void InitDecodePool(struct DecodePool* pool, struct ADPStruct* ADP)
{
	int N = ADP->N, K = ADP->K, M_ABP = ADP->M + ADP->CRC_len_for_ABP;
	pool->DecodingMethod = ADP->DecodingMethod;

	if (ADP->DecodingMethod >= 1 && ADP->DecodingMethod <= 5) {
		ABPPool* p = &pool->abp;
		p->codeword          = new int[N];
		p->y_H               = new int[N];
		p->alpha             = new int[N];
		p->p1                = new double[N];
		p->adaptive_p1       = new double[N];
		p->ReliabilityOrder  = new int[N];
		p->ReliabilityOrderGE = new int[N];
		p->InterGE           = new int[N];
		p->Interchange_Buf   = new int[ADP->Interchange];

		p->adaptiveH_data = new int[M_ABP * N];
		p->adaptiveH = new int*[M_ABP];
		for (int i = 0; i < M_ABP; i++)
			p->adaptiveH[i] = p->adaptiveH_data + i * N;

		p->th  = new int[M_ABP * N];
		p->pos = new int[N];
		p->tr  = new int[N];

		p->rec_temp      = new int[K];
		p->rec_temp_code = new int[N];

		p->tanhq     = new double[N];
		p->MRB_LLR   = new double[N - ADP->M];
		p->MRB_Order = new int[N - ADP->M];
		p->temp_code = new int[N];
		p->TempLLR   = new double[N];

		p->Pr       = new double[N];
		p->Pr_Table = new double[N];
	}
	else if (ADP->DecodingMethod == 6) {
		SCLPool* p = &pool->scl;
		int L = ADP->PAC_code->L;
		int n = (int)log2((double)N);

		p->list_data = new int[L * N];
		p->list = new int*[L];
		for (int i = 0; i < L; i++)
			p->list[i] = p->list_data + i * N;

		p->olist_data = new int[L * N];
		p->olist = new int*[L];
		for (int i = 0; i < L; i++)
			p->olist[i] = p->olist_data + i * N;

		p->sheet_data = new double[L * (n + 1) * N];
		p->sheet_rows = new double*[L * (n + 1)];
		p->sheet = new double**[L];
		for (int i = 0; i < L; i++) {
			p->sheet[i] = p->sheet_rows + i * (n + 1);
			for (int j = 0; j < n + 1; j++)
				p->sheet[i][j] = p->sheet_data + (i * (n + 1) + j) * N;
		}

		p->listtemp_data = new int[2 * L * N];
		p->listtemp = new int*[2 * L];
		for (int i = 0; i < 2 * L; i++)
			p->listtemp[i] = p->listtemp_data + i * N;

		p->olisttemp_data = new int[2 * L * N];
		p->olisttemp = new int*[2 * L];
		for (int i = 0; i < 2 * L; i++)
			p->olisttemp[i] = p->olisttemp_data + i * N;

		p->sheettemp_data = new double[2 * L * (n + 1) * N];
		p->sheettemp_rows = new double*[2 * L * (n + 1)];
		p->sheettemp = new double**[2 * L];
		for (int i = 0; i < 2 * L; i++) {
			p->sheettemp[i] = p->sheettemp_rows + i * (n + 1);
			for (int j = 0; j < n + 1; j++)
				p->sheettemp[i][j] = p->sheettemp_data + (i * (n + 1) + j) * N;
		}

		p->SCL_result  = new int[N];
		p->Inter_result = new int[N];
	}
}

void FreeDecodePool(struct DecodePool* pool, struct ADPStruct* ADP)
{
	if (pool->DecodingMethod >= 1 && pool->DecodingMethod <= 5) {
		ABPPool* p = &pool->abp;
		delete[] p->codeword;
		delete[] p->y_H;
		delete[] p->alpha;
		delete[] p->p1;
		delete[] p->adaptive_p1;
		delete[] p->ReliabilityOrder;
		delete[] p->ReliabilityOrderGE;
		delete[] p->InterGE;
		delete[] p->adaptiveH_data;
		delete[] p->adaptiveH;
		delete[] p->Interchange_Buf;
		delete[] p->th;
		delete[] p->pos;
		delete[] p->tr;
		delete[] p->rec_temp;
		delete[] p->rec_temp_code;
		delete[] p->tanhq;
		delete[] p->MRB_LLR;
		delete[] p->MRB_Order;
		delete[] p->temp_code;
		delete[] p->TempLLR;
		delete[] p->Pr;
		delete[] p->Pr_Table;
	}
	else if (pool->DecodingMethod == 6) {
		SCLPool* p = &pool->scl;
		delete[] p->list_data;
		delete[] p->list;
		delete[] p->olist_data;
		delete[] p->olist;
		delete[] p->sheet_data;
		delete[] p->sheet_rows;
		delete[] p->sheet;
		delete[] p->listtemp_data;
		delete[] p->listtemp;
		delete[] p->olisttemp_data;
		delete[] p->olisttemp;
		delete[] p->sheettemp_data;
		delete[] p->sheettemp_rows;
		delete[] p->sheettemp;
		delete[] p->SCL_result;
		delete[] p->Inter_result;
	}
}

// calculate Matrix inverse
void Calculate__Inverse_Matrix(int **T, int **T_1,int N) {
	//Augmented matrix
	int** AMT= new int*[N];
	int** Temp_Matrix = new int* [N];
	int* ReliabilityOrder;
	int* ReliabilityOrderGE;
	int** adaptiveH;				//H after Gaussian Elimination
	int* InterGE;
	int K;


	ReliabilityOrder = new int[2*N];
	ReliabilityOrderGE = new int[2*N];
	adaptiveH = new int* [N];
	for (int i = 0; i < N; i++) adaptiveH[i] = new int[2*N];
	InterGE = new int[2*N];
	for (int i = 0; i < N; i++) Temp_Matrix[i] = new int[2 * N];

	
	for (int i = 0; i < N; i++)
	{
		AMT[i] = new int[N*2];
		memset(AMT[i], 0, sizeof(int) * (2*N));
	}
	//AMT=[T|I]
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			AMT[i][j] = T[i][j];
		}
	}
	for (int i = 0; i < N; i++) {
		AMT[i][i+N] = 1;
	}

	//AMT->[I|T_1]
	//SortLLR(p1, N, ReliabilityOrder);
	/*
	for (int i = 0; i < CRC->CRCLen; i++)
		ReliabilityOrder[i] = LDPC->K - i - 1;
	for (int i = CRC->CRCLen; i < LDPC->K; i++)
		ReliabilityOrder[i] = i - CRC->CRCLen;
	*/
	for (int i = 0; i < N; i++) {
		ReliabilityOrder[i] = i+N;
	}
	for (int i = N; i < 2*N; i++) {
		ReliabilityOrder[i] = i-N;
	}
	OSD_GE_H(AMT, adaptiveH, N, 2*N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
	//恢复位置
	for (int i = 0; i < N; i++)
		for (int j = 0; j < 2*N; j++)
			Temp_Matrix[i][ReliabilityOrderGE[j]] = adaptiveH[i][j];
	/*
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < 2 * N; j++) {
			printf("%d ", Temp_Matrix[i][j]);
		}
		printf("\n");
	}
	*/
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			T_1[i][j] = Temp_Matrix[i][j + N];
		}
	}
	/*
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			printf("%d ", T_1[i][j]);
		}
		printf("\n");
	}
	*/
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (int i = 0; i < N; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;
	for (int i = 0; i < N; i++)
		delete[] AMT[i];
	delete[] AMT;
	for (int i = 0; i < N; i++)
		delete[] Temp_Matrix[i];
	delete[] Temp_Matrix;
}

void Initial(struct ADPStruct *ADP, struct SPStruct *SP)
{

	//PAC 码初始化
	ADP->PAC_code = new struct PACStruct;

	ReadProfile(ADP, SP);
	ReadCodefile(ADP);
	
	//ADP->rate = (double)(ADP->K -ADP->CRC_len) / (double)(ADP->N -ADP->puncture-ADP->shorten);	// 有效信息码率R_net（从系统层有效信息传输效率视角）
	ADP->rate = (double)(ADP->K) / (double)(ADP->N - ADP->puncture - ADP->shorten);			// 编码码率R（从通信理论/5G标准/编码器视角，仿真用这个）
	//ADP->rate = 0.8;
	
	//上三角矩阵T
	ADP->PAC_code->T = new int*[ADP->N];
	for (int i = 0; i < ADP->N; i++){
		ADP->PAC_code->T[i] = new int[ADP->N];
		memset(ADP->PAC_code->T[i], 0, sizeof(int)* ADP->G_K);
	}
	for (int i = 0; i < ADP->N; i++) {
		for (int j = i; j < min(i + ADP->PAC_code->IR_size, ADP->N); j++) {
			ADP->PAC_code->T[i][j] = ADP->PAC_code->IR[j - i];	
		}
	}
	/*printf("Matrix T:\n");
	for (int i = 0; i < ADP->N; i++) {
		for (int j = 0; j < ADP->N; j++) {
			printf("%d ", ADP->PAC_code->T[i][j]);
		}
		printf("\n");
	}*/

	//矩阵T的逆矩阵
	//T^-1
	ADP->PAC_code->T_1 = new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		ADP->PAC_code->T_1[i] = new int[ADP->N];
		memset(ADP->PAC_code->T_1[i], 0, sizeof(int) * ADP->N);
	}
	//高斯消元求矩阵的逆
	Calculate__Inverse_Matrix(ADP->PAC_code->T, ADP->PAC_code->T_1,ADP->N);

	//注意：涉及到矩阵乘法的，需要赋初值
	//G=T*G_polar
	ADP->PAC_code->G = new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		ADP->PAC_code->G[i] = new int[ADP->N];
		memset(ADP->PAC_code->G[i], 0, sizeof(int)* ADP->N);
	}
	for (int i = 0; i < ADP->N; i++)
	{
		for (int j = 0; j < ADP->N; j++)
		{
			for (int k = 0; k < ADP->N; k++)
			{
				ADP->PAC_code->G[i][j] ^= (ADP->PAC_code->T[i][k] && ADP->G[k][j]);
			}
		}
	}

	//RM 信息位
	//Info_Index_256_128.txt
	//Info_Index_128_96.txt
	//蒙特卡洛优化结果
	//Info_Index_256_128_MC3.txt //目前仿真最佳的1.5dB
	
	//get puncture index before get info bit index
	//consider Puncture and Shorten; first Shorten, then Puncture 
	if (ADP->shorten > 0)
		getShortenIndex(ADP);
	if (ADP->puncture > 0)
		//getPunctureIndex(ADP);
		proposedPuncture(ADP);

	FILE* codefile;
	vector<int> infoIndex;
	unordered_set<int> s;
	//根据邹涛文章结论，极化码的打孔位置应该设置为冻结位
	//而PAC仿真结果标明，打孔位置不设置为冻结位性能更好
	if (ADP->codeMode == 0) {
		for (int i = 0; i < ADP->puncture; i++) {
			s.insert(ADP->PunctureIndex[i]);
		}
	}
	//冲激响应长度对缩短的影响，PAC为6，Polar为0
	
	int len = 0;
	if (ADP->shorten > 0) {
		if (ADP->codeMode == 1) {
			len = ADP->PAC_code->IR_size - 1;
		}
		for (int i = 0; i < ADP->shorten + len; i++) {
			s.insert(ADP->ShortenIndex[i]);
		}
	}
	if ((codefile = fopen("Index_1024_RM.txt", "r")) == NULL)
	{
		printf("Can not open txt file!\n");
		getch();
		exit(0);
	}
	
	for (int j = 0; j < 1024; j++)
	{
		int tmp;
		fscanf(codefile, "%d", &tmp);
		tmp--;
		if (tmp<ADP->N && !s.count(tmp)) {
			infoIndex.push_back(tmp);
		}
	}
	fclose(codefile);

	if (ADP->codeMode == 0) {
		sort(infoIndex.begin() + (ADP->M - ADP->puncture - ADP->shorten), infoIndex.end());
		for (int i = 0; i < ADP->K; i++) {
			ADP->A[i] = infoIndex[i - ADP->puncture - ADP->shorten + ADP->M];
			//printf("%d ", ADP->A[i]);
		}
	}
	else {
		//sort(infoIndex.begin() + (ADP->M - ADP->puncture - ADP->shorten-len), infoIndex.end());
		sort(infoIndex.begin() + (ADP->M - ADP->shorten - len), infoIndex.end());
		for (int i = 0; i < ADP->K; i++) {
			//ADP->A[i] = infoIndex[i - ADP->puncture - ADP->shorten -len+ ADP->M];
			ADP->A[i] = infoIndex[i + ADP->M - ADP->shorten - len];
			//printf("%d ", ADP->A[i]);
		}
	}
	//saveArrayToFile(ADP->A, ADP->K, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\Infobits_128.txt");
	
	//FILE* codefile;
	/*
	if ((codefile = fopen("Info_Index_128_96_RM_1.txt", "r")) == NULL)
	{
		printf("Can not open txt file!\n");
		getch();
		exit(0);
	}
	for (int j = 0; j < ADP->K; j++)
	{
		fscanf(codefile, "%d", &(ADP->A[j]));
		ADP->A[j]--;
		//printf("%d ", ADP->A[j]);
	}
	fclose(codefile);
	*/



	//非系统编码时的T0
	ADP->PAC_code->T0 = new int* [ADP->K];
	for (int i = 0; i < ADP->K; i++) {
		ADP->PAC_code->T0[i] = new int[ADP->K];
		memset(ADP->PAC_code->T0[i], 0, sizeof(int) * ADP->K);
	}
	int** T0 = new int* [ADP->K];
	for (int i = 0; i < ADP->K; i++) {
		T0[i] = new int[ADP->K];
		memset(T0[i], 0, sizeof(int) * ADP->K);
	}
	int **temp= new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		temp[i] = new int[ADP->N];
		memset(temp[i], 0, sizeof(int) * ADP->N);
	}
	//Temp=G*T
	for (int i = 0; i < ADP->N; i++)
	{
		for (int j = 0; j < ADP->N; j++)
		{
			for (int k = 0; k < ADP->N; k++)
			{
				temp[i][j] ^= (ADP->G[i][k] && ADP->PAC_code->T[k][j]);
			}
		}
	}
	for (int i = 0; i < ADP->K; i++) {
		//cout << ADP->A[i]<<" ";
		for (int j = 0; j < ADP->K; j++) {
			T0[i][j] = temp[ADP->A[i]][ADP->A[j]];
		}
	}
	Calculate__Inverse_Matrix(T0, ADP->PAC_code->T0, ADP->K);


	//极化码的H表示冻结比特对应的列 
	//H=G*T^-1
	
	ADP->PAC_code->P = new int* [ADP->N ];
	for (int i = 0; i < ADP->N ; i++) {
		ADP->PAC_code->P[i] = new int[ADP->N];
		memset(ADP->PAC_code->P[i], 0, sizeof(int) * ADP->N);
	}
	ADP->PAC_code->H = new int* [ADP->N-ADP->K];
	for (int i = 0; i < ADP->N - ADP->K; i++) {
		ADP->PAC_code->H[i] = new int[ADP->N];
		memset(ADP->PAC_code->H[i], 0, sizeof(int) * ADP->N);
	}
	for (int i = 0; i < ADP->N; i++)
	{
		for (int j = 0; j < ADP->N; j++)
		{
			for (int k = 0; k < ADP->N; k++)
			{
				ADP->PAC_code->P[i][j] ^= (ADP->G[i][k] && ADP->PAC_code->T_1[k][j]);
			}
		}
	}
	int index = 0;
	int H_index = 0;
	for (int i = 0; i < ADP->N; i++) {
		//冻结比特的位置
		if (i != ADP->A[index]) {
			for (int j = 0; j < ADP->N; j++) {
				ADP->PAC_code->H[H_index][j] = ADP->PAC_code->P[j][i];
			}
			H_index++;
		}
		else {
			index++;
		}
		if (H_index == ADP->M)
			break;
	}
	//saveMatrixToFile(ADP->PAC_code->H, ADP->M, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\PAC_H_128.txt");
	//cout << "H_I:" << endl;
	////printf("H_I:\n");
	//for(int i=0;i<ADP->M;i++){
	//	for (int j = 0; j < ADP->N; j++) {
	//		cout << ADP->PAC_code->H[i][j]<<" ";
	//	}
	//	cout << endl;
	//}

	// ABP Initial
	if (ADP->N2 > 1 && ADP->Interchange * (ADP->N2 - 1) > ADP->K)
	{
		printf("ADP->Interchange * ADP->N2 is too large.\n");
		getch();
		exit(0);
	}
	ADP->H_crc = new int* [ADP->CRC_len];
	for (int i = 0; i < ADP->CRC_len; i++)
		ADP->H_crc[i] = new int[ADP->K];
	CRC_H_initial(ADP->H_crc, ADP->CRC_len, ADP->K - ADP->CRC_len);
	//saveMatrixToFile(ADP->H_crc, ADP->CRC_len, ADP->K, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\H_crc_128.txt");
	//cout << "H_crc:" << endl;
	////printf("H_crc:\n");
	//for (int i = 0; i < ADP->CRC_len; i++) {
	//	for (int j = 0; j < ADP->K; j++) {
	//		cout << ADP->H_crc[i][j]<<" ";
	//	}
	//	cout << endl;
	//}

	ADP->H_crc_add0 = new int* [ADP->CRC_len];
	for (int i = 0; i < ADP->CRC_len; i++)
	{
		ADP->H_crc_add0[i] = new int[ADP->N];
	}
	for (int i = 0; i < ADP->CRC_len; i++)
	{
		for (int j = 0; j < ADP->N; j++)
		{
			ADP->H_crc_add0[i][j] = 0;
		}
	}
	for (int i = 0; i < ADP->K; i++)
	{
		for (int j = 0; j < ADP->CRC_len; j++)
		{
			ADP->H_crc_add0[j][ADP->A[i]] = ADP->H_crc[j][i];
		}
	}
	//saveMatrixToFile(ADP->H_crc_add0, ADP->CRC_len, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\H_crc_add0_128.txt");
	//cout << "H_crc_add0:" << endl;
	//printf("H_crc_add0:\n");
	//for (int i = 0; i < ADP->CRC_len; i++) {
	//	for (int j = 0; j < ADP->N; j++) {
	//		cout << ADP->H_crc_add0[i][j]<<" ";
	//	}
	//	cout << endl;
	//}

	ADP->Joint_check_matrix = new int*[ADP->M + ADP->CRC_len];
	for (int i = 0; i < ADP->M + ADP->CRC_len; i++) {
		ADP->Joint_check_matrix[i] = new int[ADP->N];
		memset(ADP->Joint_check_matrix[i], 0, sizeof(int)* ADP->N);
	}
	Joint_H_Initial(ADP);
	//saveMatrixToFile(ADP->Joint_check_matrix, ADP->M + ADP->CRC_len, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\Joint_H_Matrix_128.txt");
	//cout << "Joint_check_matrix:" << endl;
	/*printf("Joint_check_matrix:\n");
	for (int i = 0; i < ADP->M+ADP->CRC_len; i++) {
		for (int j = 0; j < ADP->N; j++) {
			cout << ADP->Joint_check_matrix[i][j]<<" ";
		}
		cout << endl;
	}*/

	//PAC code 
	MallocIter(ADP->M + ADP->CRC_len_for_ABP, ADP->N, ADP->IterDec);
	MallocIter(ADP->M + ADP->CRC_len, ADP->N, ADP->Tanner);
	InitialIter(ADP->M + ADP->CRC_len, ADP->N, ADP->Joint_check_matrix, ADP->Tanner);

	// 产生随机置换序列,Deg-2使用
	if (ADP->Deg2 == 1)
	{
		ADP->Deg2RandSeq = new int[ADP->M + ADP->CRC_len_for_ABP];
		for (int i = 0; i < ADP->M + ADP->CRC_len_for_ABP; i++)
			ADP->Deg2RandSeq[i] = i;
		// 方式一：交换10000次
		/*int temp;
		int pos1, pos2;
		for (int i = 0; i < 10000; i++)
		{
			pos1 = rand() % (ADP->M + ADP->CRC_len_for_ABP);
			pos2 = rand() % (ADP->M + ADP->CRC_len_for_ABP);
			temp = ADP->Deg2RandSeq[pos1];
			ADP->Deg2RandSeq[pos1] = ADP->Deg2RandSeq[pos2];
			ADP->Deg2RandSeq[pos2] = temp;
		}*/
		// 方式二：Fisher–Yates shuffle算法
		for (int i = ADP->M + ADP->CRC_len_for_ABP - 1; i > 0; i--)
		{
			int j = rand() % (i + 1);
			swap(ADP->Deg2RandSeq[i], ADP->Deg2RandSeq[j]);
		}
	}
	//saveArrayToFile(ADP->Deg2RandSeq, ADP->M + ADP->CRC_len_for_ABP, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\Deg2RandSeq_128.txt");
	
	for (int i = 0; i < ADP->N; i++)
		delete[] temp[i];
	delete[] temp;
	for (int i = 0; i < ADP->K; i++)
		delete[] T0[i];
	delete[] T0;
}

//select the puncture index according the zoutao paper
void getPunctureIndex(struct ADPStruct* ADP) {

	ADP->PunctureIndex = new int [ADP->puncture];
	int n = log2(ADP->N);

	for (int i = 0; i < ADP->puncture;i++) {
		int sum = 0;
		for (int j = 0; j < n; j++) {
			if (i & (1<<j)) {
			//if ((ADP->N-i-1) & (1 << j)) {
				sum += (1 << (n - j - 1));
			}
		}
		ADP->PunctureIndex[i] = sum;
	}
}


void proposedPuncture(struct ADPStruct* ADP) {
	//exclude the Shorten index
	unordered_set<int> shortenIndex;
	for (int i = 0; i < ADP->shorten; i++) {
		shortenIndex.insert(ADP->ShortenIndex[i]);
	}
	ADP->PunctureIndex = new int[ADP->puncture];
	FILE* codefile;
	if ((codefile = fopen("Index_1024_RM.txt", "r")) == NULL)
	{
		printf("Can not open txt file!\n");
		getch();
		exit(0);
	}
	int i = 0;
	vector<int> infoIndex;
	for (int j = 0; j < 1024; j++)
	{
		int tmp;
		fscanf(codefile, "%d", &tmp);
		tmp--;
		if (tmp < ADP->N && !shortenIndex.count(tmp)) {
			infoIndex.push_back(tmp);
			//printf("%d ", tmp);
		}
	}
	fclose(codefile);
	int infoSize = infoIndex.size();
	for (i = 0; i < ADP->puncture; i++) {
		//Proposed-1
		ADP->PunctureIndex[i] = infoIndex[infoSize - i - 1];
		//Proposed-2
		//ADP->PunctureIndex[i] = infoIndex[i];
		//novel puncture cited by the paper
		//ADP->PunctureIndex[i] = ADP->N - i - 1;
		//printf("%d ", ADP->PunctureIndex[i]);
	}
}

//select the shorten index according the zoutao paper
void getShortenIndex(struct ADPStruct* ADP) {
	//polar shorten
	int len = ADP->PAC_code->IR_size-1;
	if (ADP->codeMode == 0) {
		ADP->ShortenIndex = new int[ADP->shorten];
		for (int i = 0; i < ADP->shorten; i++) {
			ADP->ShortenIndex[i] = ADP->N - i - 1;
		}
	}
	//PAC shorten
	//只有前shorten个位置是缩短的位置，剩余的len个位置是作为冻结位，来满足缩短的特性
	else {
		ADP->ShortenIndex = new int[ADP->shorten+len];
		for (int i = 0; i < ADP->shorten+len; i++) {
			ADP->ShortenIndex[i] = ADP->N - i - 1;
			//cout << ADP->ShortenIndex[i] << " ";
		}
	}
}

// 初始化AWGN和SEED，这里为了方便，使用了固定的种子，没有从Profile中读取
void InitialAWGN(struct AWGN *awgn)
{
	awgn->seed->ix = 173;
	awgn->seed->iy = 173;
	awgn->seed->iz = 173;
	awgn->seedmethod = 1;
}


void FreeMemory(struct ADPStruct *ADP)
{

}
