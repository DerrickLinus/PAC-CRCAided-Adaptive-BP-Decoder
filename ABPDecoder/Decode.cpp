#include "define.h"
#include <algorithm>
#include <numeric>
// #include <iostream>
using namespace std;
/*
Check the code if it satisfies the parity-check matrix
return 0:		is satisfied
return 1:		not satisfied
*/
int CheckCode(const int *codeseq, int M, struct IterStruct *Iter) // M = ADP->M + ADP->CRC_len
{
	int i = 0;
	int j = 0;
	int temp = 0;
	int sum = 0;

	for (i = 0; i<M; i++)
	{
		temp = 0;
		for (j = 0; j<Iter->CNdegree[i]; j++)
		{
			temp ^= (*(codeseq + Iter->CNindex[i][j]));
		}

		if (temp != 0)
		{
			sum++;
		}
	}

	return sum;
}

/*
Sort the abs |double values| by descending order
*/
//void SortLLR(const double *bitsoft, int N, int *ReliabilityOrder) // 源代码版本，复杂度为O(N^2)
//{
//	int i, j;
//	double temp_float;
//	int temp_int;
//	double *llr_temp = new double[N];
//	memcpy(llr_temp, bitsoft, sizeof(double)*N);
//	for (i = 0; i < N; i++) ReliabilityOrder[i] = i;
//	for (i = 0; i < N - 1; i++)
//	{
//		for (j = 0; j < N - 1 - i; j++)
//		{
//			if (fabs(llr_temp[j]) < fabs(llr_temp[j + 1]))
//			{
//				temp_float = llr_temp[j];
//				llr_temp[j] = llr_temp[j + 1];
//				llr_temp[j + 1] = temp_float;
//				temp_int = ReliabilityOrder[j];
//				ReliabilityOrder[j] = ReliabilityOrder[j + 1];
//				ReliabilityOrder[j + 1] = temp_int;
//			}
//		}
//	}
//	delete[] llr_temp;
//}
void SortLLR(const double* bitsoft, int N, int* ReliabilityOrder) // 性能优化版，复杂度为O(NlogN)
{ 
	for (int i = 0; i < N; i++) ReliabilityOrder[i] = i;
	std::sort(ReliabilityOrder, ReliabilityOrder + N,
		[bitsoft](int a, int b) {
			return fabs(bitsoft[a]) > fabs(bitsoft[b]);
		});
}

double FaiFunction(double x)
{
	double temp;
	//for BP docoding,shorten bits' bitsoft value 10000 will be overflowed
	temp = exp(x);
	if (temp > MAXVALUE)
		temp = MAXVALUE;
	double t = (temp + 1) / (temp - 1);


	if (t > MAXVALUE)
	{
		return (log(MAXVALUE));
	}
	else
	{
		if (t < MINVALUE)
		{
			return (log(MINVALUE));
		}
	}

	return (log(t));
}


/*
Recover the information bits from a non-systematic codeword
uP^-1 * PG = c, PG = G_sys = [I|P], so uP^-1 = the information bits of c and then u = the information bits of c * P
*/
void Recover_Info(const int *codeword, int *info, struct ADPStruct *ADP)
{
	int i, j;
	int *temp = new int[ADP->K];
	int* temp_code = new int[ADP->N];
	//Encode(codeword, info, ADP->G_K, ADP->G_K, ADP->P);
	Encode(codeword, temp_code, ADP->G_K, ADP->G_K, ADP->G);
	Encode(temp_code,info , ADP->G_K, ADP->G_K, ADP->PAC_code->T_1);
	if (ADP->EncodeAdd0)
	{
		for (i = 0; i < ADP->K; i++)
		{
			temp[i] = info[ADP->A[i]];
		}
		memcpy(info, temp, sizeof(int)*ADP->K);
	}
	else
	{
		// assume the first K bits are information btis, and do nothing	
	}
	delete[] temp_code;
	delete[] temp;
}

/*
Judge the codeword by ML metric
*/
int JudgeCodeword(int *y_H, int *C, double *y, int length, double threshold)
{
	int i;
	double metric = 0;

	for (i = 0; i < length; i++)
	{
		metric += (fabs(y[i]) * (y_H[i] ^ C[i]));
	}

	return (metric <= threshold);
}
//fake check
int Fake_Check(int* r, int* result, struct ADPStruct* ADP, int* PolarCode)
{
	int sum = 0;
	for (int i = 0; i < ADP->N; i++)
	{
		if (r[i] != PolarCode[i])
			sum++;
		if (sum != 0)
			break;
	}
	if (sum == 0)
	{
		memcpy(result, PolarCode, sizeof(int) * ADP->N);
		return 1;
	}
	else
	{
		memcpy(result, r, sizeof(int) * ADP->N);
		return 0;
	}
}


void List_ABP_MSA(double* bitsoft, double* y, int** H, int N, int M, int* outseq,
	struct IterStruct* Iter, struct IterStruct* Tanner, struct ADPStruct* ADP)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	int sign = -1;
	double tempd = 0;
	double min1 = 0;
	double min2 = 0;
	int pos = 0;
	int* alpha;
	double* p1;
	double* adaptive_p1;
	int* ReliabilityOrder;
	int* ReliabilityOrderGE;
	int** adaptiveH;				//H after Gaussian Elimination
	int* InterGE;
	int* codeword;
	int* y_H;
	int K;
	int outer_it = 0;
	int* Interchange_Buf;
	double R;
	double minSED = MAXVALUE;//minimum squared Euclidean distance
	ADP->check_flag = 0;
	ADP->IterTime = 0;
	int Inner_index = 0;
	int Outer_index = 0;
	int* temp_code = new int[N];
	//hard decision
	codeword = new int[N];
	for (i = 0; i < N; i++)
	{
		if (*(bitsoft + i) > 0)
		{
			*(codeword + i) = 0;
		}
		else
		{
			*(codeword + i) = 1;
		}
	}
	if (CheckCode(codeword, M, Tanner) == 0)
		//if(Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
	{
		//memcpy(ADP->Result_List_Outer[Outer_index], codeword, sizeof(int) * N);
		//Outer_index++;
		memcpy(temp_code, codeword, sizeof(int) * N);
		Recover_Info(codeword, outseq, ADP);
		ADP->check_flag = 1;
		//delete[] codeword;
		//return;
		//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
	}
	y_H = new int[N];
	alpha = new int[N];
	p1 = new double[N];
	adaptive_p1 = new double[N];
	ReliabilityOrder = new int[N];
	ReliabilityOrderGE = new int[N];
	adaptiveH = new int* [M];
	for (i = 0; i < M; i++) adaptiveH[i] = new int[N];
	InterGE = new int[N];
	Interchange_Buf = new int[ADP->Interchange];

	memcpy(y_H, codeword, sizeof(int) * N);
	for (outer_it = 0; outer_it < ADP->N2; outer_it++)
	{
		memcpy(Iter->pLLR, bitsoft, sizeof(double) * N);
		for (k = 0; k < ADP->N1; k++)
		{
			Inner_index = 0;
			ADP->IterTime += 1;
			memcpy(p1, Iter->pLLR, sizeof(double) * N);
			memset(Iter->pLLR, 0, sizeof(double) * N);

			// adaptive the PCM
			SortLLR(p1, N, ReliabilityOrder);
			if (k == 0 && outer_it > 0)	// outer iterations, various grouping
			{
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M - outer_it * ADP->Interchange + i];
				}
				memcpy(ReliabilityOrder + N - M - outer_it * ADP->Interchange, ReliabilityOrder + N - M - (outer_it - 1) * ADP->Interchange, sizeof(int) * (M + (outer_it - 1) * ADP->Interchange));
				memcpy(ReliabilityOrder + N - ADP->Interchange, Interchange_Buf, sizeof(int) * ADP->Interchange);
			}
			OSD_GE_H(H, adaptiveH, M, N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
			if (ADP->Deg2 == 1)
			{
				Permute(ADP->Deg2RandSeq, M);
				for (i = 0; i < M - 1; i++)
				{
					for (j = 0; j < N; j++)
					{
						adaptiveH[ADP->Deg2RandSeq[i]][j] ^= adaptiveH[ADP->Deg2RandSeq[i + 1]][j];
					}
				}
			}
			for (i = 0; i < N; i++) adaptive_p1[i] = p1[ReliabilityOrderGE[i]];
			InitialIter(M, N, adaptiveH, Iter);

			for (m = 0; m < M; m++)
			{
				sign = 1;
				pos = -1;
				min1 = MAXVALUE;
				min2 = MAXVALUE;

				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					tempd = adaptive_p1[Iter->CNindex[m][i]];
					if (tempd < 0)
					{
						*(alpha + i) = -1;
						sign = 0 - sign;
						tempd = 0 - tempd;
					}
					else
					{
						*(alpha + i) = 1;
					}
					if (tempd < min1)
					{
						min2 = min1;
						min1 = tempd;
						pos = i;
					}
					else
					{
						if (tempd < min2)
						{
							min2 = tempd;
						}
					}
				}
				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					if (i == pos)
					{
						R = min2;
					}
					else
					{
						R = min1;
					}
					R *= (sign * alpha[i]);
					Iter->pLLR[Iter->CNindex[m][i]] += R;
				}
			}
			// recover the LLRs' orders
			for (i = 0; i < N; i++)
				adaptive_p1[ReliabilityOrderGE[i]] = Iter->pLLR[i];
			memcpy(Iter->pLLR, adaptive_p1, sizeof(double) * N);
			// hard decision
			for (i = 0; i < N; i++)
			{
				if (ADP->use_channel_LLR)
					Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + bitsoft[i];
				else
					Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + p1[i];

				if (Iter->pLLR[i] > 0)
				{
					*(codeword + i) = 0;
				}
				else
				{
					*(codeword + i) = 1;
				}
			}

			if (CheckCode(codeword, M, Tanner) == 0)
				//if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
			{
				FindOptimal(bitsoft, codeword, N, &minSED, temp_code);
				//memcpy(ADP->Result_List_Inner[Inner_index], codeword, sizeof(int) * N);
				//Inner_index++;
				Recover_Info(codeword, outseq, ADP);
				//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0 && JudgeCodeword(y_H, codeword, y, N, ADP->ML_metric_th) == 1)
				ADP->check_flag = 1;
				//break;	
			}
			/*
			if (ADP->check_flag == 1) {
				int count;
				for (count = 0; count < ADP->N; count++) {
					if (outseq[count] != 0)
						break;
				}
				if (count == ADP->N)
					ADP->check_flag = 0;
			}*/
		}
		//minSED = MAXVALUE;
		/*
		for (i = 0; i < Inner_index; i++)
		{
			FindOptimal(bitsoft, ADP->Result_List_Inner[i], N, &minSED, codeword);
		}
		memcpy(ADP->Result_List_Outer[Outer_index], codeword, sizeof(int)* N);
		Outer_index++;
		*/
		// Auto stop
		if (ADP->check_flag == 1)
		{
			//break;
		}
	}
	/*
	minSED = MAXVALUE;
	for (i = 0; i < Outer_index; i++)
	{
		FindOptimal(bitsoft, ADP->Result_List_Outer[i], N, &minSED, codeword);
	}
	Recover_Info(codeword, outseq, ADP);
	//memcpy(codeword, ADP->Result_List_Outer[Outer_index],  sizeof(int) * N);
	*/
	Recover_Info(temp_code, outseq, ADP);
	if (ADP->check_flag == 0)
	{
		for (i = 0; i < N; i++)
		{
			if (*(bitsoft + i) > 0)
			{
				*(codeword + i) = 0;
			}
			else
			{
				*(codeword + i) = 1;
			}
		}
		Recover_Info(codeword, outseq, ADP);
	}

	delete[] y_H;
	delete[] alpha;
	delete[] p1;
	delete[] adaptive_p1;
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (i = 0; i < M; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;
	delete[] Interchange_Buf;
	delete[] codeword;
	delete[] temp_code;
}
/*
ABP/MSA(N1, N2)
bitsoft:		bit LLR
y:				undemodulated signal
H:				the parity check matrix
N:				code length
M:				the number of check nodes
outseq:			decoder-output, only first K information bits are valid, the last (N-K) bits are meaningless
非系统的PAC可以适用于各种译码方法
系统的PAC只适用于ABP-MSA，SCLD，其它算法暂时未修改
*/
void ideal_ABP_MSA(double *bitsoft, double *y, int **H, int N, int M, int *outseq,
	struct IterStruct *Iter, struct IterStruct *Tanner, struct ADPStruct *ADP)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	int sign = -1;
	double tempd = 0;
	double min1 = 0;
	double min2 = 0;
	int pos = 0;
	int *alpha;
	double *p1;
	double *adaptive_p1;
	int *ReliabilityOrder;
	int *ReliabilityOrderGE;
	int **adaptiveH;				//H after Gaussian Elimination
	int *InterGE;
	int *codeword;
	int *y_H;
	int K; 
	int outer_it = 0;
	int *Interchange_Buf;
	double R;
	double minSED = MAXVALUE;//minimum squared Euclidean distance
	double min_val = 0.0;
	int vn_index = 0;
	bool is_reliable = false;
	double alpha_factor = 1.0;
	double beta_factor = 0.0;
	ADP->check_flag = 0;
	ADP->IterTime = 0;

	//hard decision
	codeword = new int[N];
	for (i = 0; i < N; i++)
	{
		if (*(bitsoft + i) > 0)
		{
			*(codeword + i) = 0;
		}
		else
		{
			*(codeword + i) = 1;
		}
	}
	//if (CheckCode(codeword, M, Tanner) == 0)
	if(Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
	{
		if (ADP->PAC_code->system == 0) {
			Recover_Info(codeword, outseq, ADP);
		}
		else {
			for (int i = 0; i < ADP->K; i++)
				outseq[i] = codeword[ADP->A[i]];
		}
		ADP->check_flag = 1;
		delete[] codeword;
		return;
		//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
	}
	y_H = new int[N];
	alpha = new int[N];
	p1 = new double[N];
	adaptive_p1 = new double[N];
	ReliabilityOrder = new int[N];
	ReliabilityOrderGE = new int[N];
	adaptiveH = new int*[M];
	for (i = 0; i < M; i++) adaptiveH[i] = new int[N];
	InterGE = new int[N];
	Interchange_Buf = new int[ADP->Interchange];

	memcpy(y_H, codeword, sizeof(int)*N);
	for (outer_it = 0; outer_it < ADP->N2; outer_it++)
	{
		memcpy(Iter->pLLR, bitsoft, sizeof(double)*N);
		for (k = 0; k < ADP->N1; k++)
		{
			ADP->IterTime += 1;
			memcpy(p1, Iter->pLLR, sizeof(double) * N);
			memset(Iter->pLLR, 0, sizeof(double) * N);

			// adaptive the PCM
			SortLLR(p1, N, ReliabilityOrder);
			if (k == 0 && outer_it > 0)	// outer iterations, various grouping
			{
				//固定可靠比特，逐次变换不可靠比特
				/*
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M + (outer_it - 1)*ADP->Interchange + i];// 选取LSB集合边缘的几个位置
				}
				memcpy(ReliabilityOrder + ADP->Interchange, ReliabilityOrder, sizeof(int)*(N - M + (outer_it - 1)*ADP->Interchange));
				memcpy(ReliabilityOrder, Interchange_Buf, sizeof(int)*ADP->Interchange);// 交换的位置放到最左边(最可靠)，防止GE过程因为列交换而被选到
				*/
				//固定不可靠比特，逐次变换可靠比特
				
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M - outer_it*ADP->Interchange + i];
				}
				memcpy(ReliabilityOrder + N - M - outer_it*ADP->Interchange, ReliabilityOrder + N - M - (outer_it - 1)*ADP->Interchange, sizeof(int)*(M + (outer_it - 1)*ADP->Interchange));
				memcpy(ReliabilityOrder + N - ADP->Interchange, Interchange_Buf, sizeof(int)*ADP->Interchange);
				
			}
			
			OSD_GE_H(H, adaptiveH, M, N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
			if (ADP->Deg2 == 1)
			{
				Permute(ADP->Deg2RandSeq, M);
				for (i = 0; i < M - 1; i++)
				{
					for (j = 0; j < N; j++)
					{
						adaptiveH[ADP->Deg2RandSeq[i]][j] ^= adaptiveH[ADP->Deg2RandSeq[i + 1]][j];
					}
				}
			}
			
			for (i = 0; i < N; i++) adaptive_p1[i] = p1[ReliabilityOrderGE[i]];
			
			InitialIter(M, N, adaptiveH, Iter);
			
			//for(int i=0;i<N;i++)
				//printf("%f ", adaptive_p1[i]);
			//int minposition = min_element(adaptive_p1, adaptive_p1 + N) - adaptive_p1;
			//printf("%d ", minposition);
			
			for (m = 0; m < M; m++)
			{
				sign = 1;
				pos = -1;
				min1 = MAXVALUE;
				min2 = MAXVALUE;

				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					tempd = adaptive_p1[Iter->CNindex[m][i]];
					if (tempd < 0)
					{
						*(alpha + i) = -1;
						sign = 0 - sign;
						tempd = 0 - tempd;
					}
					else
					{
						*(alpha + i) = 1;
					}
					if (tempd < min1)
					{
						min2 = min1;
						min1 = tempd;
						pos = i;
					}
					else
					{
						if (tempd < min2)
						{
							min2 = tempd;
						}
					}
				}
				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					min_val = (i == pos) ? min2 : min1;
					vn_index = Iter->CNindex[m][i];
					is_reliable = (vn_index < (N - M));
					alpha_factor = is_reliable ? ADP->alpha_fixed : ADP->alpha_fixed2;
					beta_factor = is_reliable ? ADP->beta_fixed : ADP->beta_fixed2;

					// ms_type: 0-标准MS, 1-NMS, 2-OMS, 3-NMS+OMS
					if (ADP->ms_type == 0)
					{
						// 标准Min-Sum
						R = min_val;
					}
					else if (ADP->ms_type == 1)
					{
						// NMS: R = alpha * min
						R = alpha_factor * min_val;
					}
					else if (ADP->ms_type == 2)
					{
						// OMS: R = max(min - beta, 0)
						R = max(min_val - beta_factor, 0.0);
					}
					else
					{
						// NMS + OMS: R = alpha * max(min - beta, 0)
						R = alpha_factor * max(min_val - beta_factor, 0.0);
					}

					R *= (sign * alpha[i]);
					Iter->pLLR[Iter->CNindex[m][i]] += R;
				}
			}

			// recover the LLRs' orders
			for (i = 0; i < N; i++)
				adaptive_p1[ReliabilityOrderGE[i]] = Iter->pLLR[i];
			memcpy(Iter->pLLR, adaptive_p1, sizeof(double)*N);

			// hard decision
			for (i = 0; i < N; i++)
			{
				if (ADP->use_channel_LLR)
					Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + bitsoft[i];
				else
					Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + p1[i];
				
				if (Iter->pLLR[i] > 0)
				{
					*(codeword + i) = 0;
				}
				else
				{
					*(codeword + i) = 1;
				}
			}

			//if (CheckCode(codeword, M, Tanner) == 0)
			if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
			{
				//Recover_Info(codeword, outseq, ADP);
				if (ADP->PAC_code->system == 0) {
					Recover_Info(codeword, outseq, ADP);
				}
				else {
					for (int i = 0; i < ADP->K; i++)
						outseq[i] = codeword[ADP->A[i]];
				}
				//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0 && JudgeCodeword(y_H, codeword, y, N, ADP->ML_metric_th) == 1)
				ADP->check_flag = 1;
				break;	
			}
			/*
			if (ADP->check_flag == 1) {
				int count;
				for (count = 0; count < ADP->N; count++) {
					if (outseq[count] != 0)
						break;
				}
				if (count == ADP->N)
					ADP->check_flag = 0;
			}*/
		}

		// Auto stop
		if (ADP->check_flag == 1)
		{
			break;
		}
	}
	if (ADP->check_flag == 0)
	{
		for (i = 0; i < N; i++)
		{
			if (*(bitsoft + i) > 0)
			{
				*(codeword + i) = 0;
			}
			else
			{
				*(codeword + i) = 1;
			}
		}
		//Recover_Info(codeword, outseq, ADP);
		if (ADP->PAC_code->system == 0) {
			Recover_Info(codeword, outseq, ADP);
		}
		else {
			for (int i = 0; i < ADP->K; i++)
				outseq[i] = codeword[ADP->A[i]];
		}
	}
	delete[] y_H;
	delete[] alpha;
	delete[] p1;
	delete[] adaptive_p1;
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (i = 0; i < M; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;
	delete[] Interchange_Buf;
	delete[] codeword;
}

/*
ABP/MSA
*/
void ABP_MSA(double snr, double* bitsoft, double* y, int** H, int N, int M, int* outseq,
	struct IterStruct* Iter, struct IterStruct* Tanner, struct ADPStruct* ADP)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	int sign = -1;
	double tempd = 0;
	double min1 = 0;
	double min2 = 0;
	int pos = 0;
	int* alpha;
	double* p1;
	double* adaptive_p1;
	int* ReliabilityOrder;
	int* ReliabilityOrderGE;
	int** adaptiveH;				//H after Gaussian Elimination
	int* InterGE;
	int* codeword;
	int* y_H;
	int K;
	int outer_it = 0;
	int* Interchange_Buf;
	double R;
	double minSED = MAXVALUE;//minimum squared Euclidean distance
	double min_val = 0.0;
	int vn_index = 0;
	bool is_reliable = false;
	double alpha_factor = 1.0;
	double beta_factor = 0.0;
	/*double alpha_factor = ADP->alpha_factor;
	double beta_factor = ADP->beta_factor;*/
	ADP->check_flag = 0;
	ADP->IterTime = 0;

	// 1. 定义线性衰减参数
	double damp_start = 0.12;
	double damp_end = 0.04;
	double current_damping = 1;

	//saveMatrixToFile(H, M+ADP->CRC_len - ADP->CRC_len_for_ABP, N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\H_128.txt");

	//hard decision
	codeword = new int[N];
	for (i = 0; i < N; i++)
	{
		if (bitsoft[i] > 0)
		{
			codeword[i] = 0;
		}
		else
		{
			codeword[i] = 1;
		}
	}
	//saveArrayToFile(codeword, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\codeword_128.txt");
	
	int check_errors;
	check_errors = CheckCode(codeword, M + ADP->CRC_len - ADP->CRC_len_for_ABP, Tanner);
	if (CheckCode(codeword, M+ADP->CRC_len-ADP->CRC_len_for_ABP, Tanner) == 0) // M = ADP->M+ADP->CRC_len_for_ABP
	//if (CheckCode(codeword, M , Tanner) == 0)
	{
		if (ADP->PAC_code->system == 0) {
			Recover_Info(codeword, outseq, ADP);
			//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
				//ADP->check_flag = 1;
		}
		else {
			for (int i = 0; i < ADP->K; i++)
				outseq[i] = codeword[ADP->A[i]];
		}
		ADP->check_flag = 1;
		delete[] codeword;
		return;
		//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
	}

	y_H = new int[N];
	alpha = new int[N];
	p1 = new double[N];
	adaptive_p1 = new double[N];
	ReliabilityOrder = new int[N];
	ReliabilityOrderGE = new int[N];
	adaptiveH = new int* [M];
	for (i = 0; i < M; i++) adaptiveH[i] = new int[N];
	InterGE = new int[N];
	Interchange_Buf = new int[ADP->Interchange];

	memcpy(y_H, codeword, sizeof(int) * N);
	double Min_ML_metric = 10000.0;
	double prev_metric = 1e10;	// 3.14修改
	int stall_count = 0;		// 3.14修改

	for (outer_it = 0; outer_it < ADP->N2; outer_it++)
	{
		memcpy(Iter->pLLR, bitsoft, sizeof(double) * N);
		for (k = 0; k < ADP->N1; k++)
		{
			ADP->IterTime += 1;
			memcpy(p1, Iter->pLLR, sizeof(double) * N);
			memset(Iter->pLLR, 0, sizeof(double) * N);

			// adaptive the PCM
			SortLLR(p1, N, ReliabilityOrder);
			//saveArrayToFile(ReliabilityOrder, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\ReliabilityOrder_128.txt");

			if (k == 0 && outer_it > 0)	// outer iterations, various grouping
			{
				//固定可靠比特，逐次变换不可靠比特
				/*
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M + (outer_it - 1)*ADP->Interchange + i];// 选取LSB集合边缘的几个位置
				}
				memcpy(ReliabilityOrder + ADP->Interchange, ReliabilityOrder, sizeof(int)*(N - M + (outer_it - 1)*ADP->Interchange));
				memcpy(ReliabilityOrder, Interchange_Buf, sizeof(int)*ADP->Interchange);// 交换的位置放到最左边(最可靠)，防止GE过程因为列交换而被选到
				*/
				
				//固定不可靠比特，逐次变换可靠比特
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M - outer_it * ADP->Interchange + i];
				}
				memcpy(ReliabilityOrder + N - M - outer_it * ADP->Interchange, ReliabilityOrder + N - M - (outer_it - 1) * ADP->Interchange, sizeof(int) * (M + (outer_it - 1) * ADP->Interchange));
				memcpy(ReliabilityOrder + N - ADP->Interchange, Interchange_Buf, sizeof(int) * ADP->Interchange);
				//saveArrayToFile(ReliabilityOrder, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\ReliabilityOrder_Interchange_128.txt");
			}
			
			OSD_GE_H(H, adaptiveH, M, N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
			//saveMatrixToFile(adaptiveH, M, N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\adaptiveH_128.txt");
			//saveArrayToFile(ReliabilityOrderGE, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\ReliabilityOrderGE_128.txt");
			//saveArrayToFile(InterGE, ADP->N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\InterGE_128.txt");
			
			if (ADP->Deg2 == 1)
			{
				//saveArrayToFile(ADP->Deg2RandSeq, M, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\Deg2RandSeq_beforePermuted_128.txt");
				Permute(ADP->Deg2RandSeq, M);
				//saveArrayToFile(ADP->Deg2RandSeq, M, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\Deg2RandSeq_Permuted_128.txt");
				for (i = 0; i < M - 1; i++)
				{
					for (j = 0; j < N; j++)
					{
						adaptiveH[ADP->Deg2RandSeq[i]][j] ^= adaptiveH[ADP->Deg2RandSeq[i + 1]][j];
					}
				}
			}
			//saveMatrixToFile(adaptiveH, M, N, "D:\\D_SCI_Research\\PAC Code\\Code\\pac-dlh\\ABPDecoder_MATLAB\\c_result\\adaptiveH_Deg2_128.txt");
			
			// 根据adaptiveH的列顺序（即置换后的顺序）调整LLR顺序
			for (i = 0; i < N; i++) adaptive_p1[i] = p1[ReliabilityOrderGE[i]];
			
			// 基于adaptiveH建立图连接
			InitialIter(M, N, adaptiveH, Iter);

			//for(int i=0;i<N;i++)
				//printf("%f ", adaptive_p1[i]);
			//int minposition = min_element(adaptive_p1, adaptive_p1 + N) - adaptive_p1;
			//printf("%d ", minposition);

			// 校验节点更新
			for (m = 0; m < M; m++)
			{
				sign = 1;
				pos = -1;			
				min1 = MAXVALUE;	// 最小值
				min2 = MAXVALUE;	// 次小值

				//double tanh_product = 1.0;  // Sum-Product: tanh乘积

				// 计算最小值和次小值以及符号部分
				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					tempd = adaptive_p1[Iter->CNindex[m][i]];
					if (tempd < 0)
					{
						alpha[i] = -1;
						sign = 0 - sign;
						tempd = 0 - tempd;
					}
					else
					{
						alpha[i] = 1;
					}
					if (tempd < min1)
					{
						min2 = min1;
						min1 = tempd;
						pos = i;
					}
					else
					{
						if (tempd < min2)
						{
							min2 = tempd;
						}
					}

					//// Sum-Product: 计算tanh(|L|/2)的乘积
					//double tanh_val = tanh(tempd / 2.0);
					//*(alpha + Iter->CNdegree[m] + i) = tanh_val;  // 存储每个节点的tanh值，复用alpha数组后半部分
					//tanh_product *= tanh_val;
				}
				// 计算幅度部分并组合符号
				for (i = 0; i < Iter->CNdegree[m]; i++)
				{
					min_val = (i == pos) ? min2 : min1;
					vn_index = Iter->CNindex[m][i];
					is_reliable = (vn_index < (N - M));
					alpha_factor = is_reliable ? ADP->alpha_fixed : ADP->alpha_fixed2;
					beta_factor = is_reliable ? ADP->beta_fixed : ADP->beta_fixed2;

					// ms_type: 0-标准MS, 1-NMS, 2-OMS, 3-NMS+OMS
					if (ADP->ms_type == 0)
					{
						// 标准Min-Sum
						R = min_val;
					}
					else if (ADP->ms_type == 1)
					{
						// NMS: R = alpha * min
						R = alpha_factor * min_val;
					}
					else if (ADP->ms_type == 2)
					{
						// OMS: R = max(min - beta, 0)
						R = max(min_val - beta_factor, 0.0);
					}
					else
					{
						// NMS + OMS: R = alpha * max(min - beta, 0)
						R = alpha_factor * max(min_val - beta_factor, 0.0);
					}
					R *= (sign * alpha[i]);
					Iter->pLLR[Iter->CNindex[m][i]] += R; // 实际是在计算 Sum of C2V（所有连接到该变量节点的校验节点传来的消息之和）

					// 标准和积 (SPA) 校验节点更新: R = 2 * atanh( Π tanh(L/2) )
					//{
					//	double prod = 1.0;
					//	for (j = 0; j < Iter->CNdegree[m]; j++)
					//	{
					//		if (j == i) continue;
					//		tempd = adaptive_p1[Iter->CNindex[m][j]];
					//		prod *= tanh(0.5 * tempd);
					//	}
					//	// 避免 atanh 数值溢出
					//	if (prod > 0.999999) prod = 0.999999;
					//	else if (prod < -0.999999) prod = -0.999999;
					//	R = 2.0 * atanh(prod);
					//}
					//Iter->pLLR[Iter->CNindex[m][i]] += R;

					//// Sum-Product: R = 2 * atanh(乘积 / 本节点的tanh值) * 符号
					//double tanh_i = *(alpha + Iter->CNdegree[m] + i);
					//double product_exclude_i = (fabs(tanh_i) > 1e-10) ? (tanh_product / tanh_i) : tanh_product;
					//// 限制product_exclude_i的范围，避免atanh溢出
					//if (product_exclude_i > 0.9999999) product_exclude_i = 0.9999999;
					//if (product_exclude_i < -0.9999999) product_exclude_i = -0.9999999;
					//R = 2.0 * atanh(product_exclude_i);
					//R *= (sign * alpha[i]);
					//Iter->pLLR[Iter->CNindex[m][i]] += R;

				}
			}

			// recover the LLRs' orders 恢复原始LLR顺序（因为后面要加bitsoft或p1，这两个对应原始的码字顺序）
			for (i = 0; i < N; i++)
				adaptive_p1[ReliabilityOrderGE[i]] = Iter->pLLR[i];
			memcpy(Iter->pLLR, adaptive_p1, sizeof(double) * N);

			// 变量节点更新及硬判决（这里的变量节点更新没有排除目标校验节点的信息）
			// 2. 计算当前迭代 k 的动态阻尼因子
			// k 是当前内层迭代次数 (0 到 ADP->N1 - 1)
			// 信噪比差异化线性衰减阻尼因子
			//if (snr < 2.5){
			//	current_damping = 0.08;
			//}
			//else {
			//	if (ADP->N1 > 1) {
			//		current_damping = damp_start - ((double)k / (double)(ADP->N1 - 1)) * (damp_start - damp_end);
			//	}
			//	else {
			//		current_damping = damp_start; // 防止除以0
			//	}
			//}
			// 统一线性衰减阻尼因子（不区分信噪比）
			//if (ADP->N1 > 1) {
			//	current_damping = damp_start - ((double)k / (double)(ADP->N1 - 1)) * (damp_start - damp_end);
			//}
			//else {
			//	current_damping = damp_start; // 防止除以0
			//}
			for (i = 0; i < N; i++)
			{
				if (ADP->use_channel_LLR)
					// 使用初始信道LLR（一般BP做法）	
					// Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + bitsoft[i]; // 源代码这里应该有问题，使用初始信道LLR时，不能使用阻尼因子，否则会无法收敛
					Iter->pLLR[i] = Iter->pLLR[i] + bitsoft[i];	// dlh修正
				else
					// 使用上次迭代得到的LLR						
					Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + p1[i]; // 固定阻尼因子
					//Iter->pLLR[i] = Iter->pLLR[i] * current_damping + p1[i];	   // 线性衰减阻尼因子

				if (Iter->pLLR[i] > 0)
				{
					codeword[i] = 0;
				}
				else
				{
					codeword[i] = 1;
				}
			}

			// 计算ML度量
			double metric = 0;
			for (int i = 0; i < N; i++)
			{
				metric += (fabs(bitsoft[i]) * (y_H[i] ^ codeword[i]));
			} // dlh test 注释掉

			// 收敛速率检测（与校验结果无关，每次迭代都追踪）- 3.14修改
			/*double relative_change = (prev_metric > 1e-10) ?
				fabs(prev_metric - metric) / prev_metric : 1.0;
			if (relative_change < ADP->convergence_epsilon) {
				stall_count++;
			}
			else {
				stall_count = 0;
			}
			prev_metric = metric;*/

			// 码字校验+ML阈值判断
			if (CheckCode(codeword, M + ADP->CRC_len - ADP->CRC_len_for_ABP, Tanner) == 0 && metric < Min_ML_metric) // dlh test 注释掉
			//if (CheckCode(codeword, M + ADP->CRC_len - ADP->CRC_len_for_ABP, Tanner) == 0) // dlh test
			//if (CheckCode(codeword, M, Tanner) == 0)
			{
				Min_ML_metric = metric; // dlh test 注释掉
				//Recover_Info(codeword, outseq, ADP);
				if (ADP->PAC_code->system == 0) {
					Recover_Info(codeword, outseq, ADP);
					//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
					//	ADP->check_flag = 1;
				}
				else {
					for (int i = 0; i < ADP->K; i++)
						outseq[i] = codeword[ADP->A[i]];
				}
				
				//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0 && JudgeCodeword(y_H, codeword, y, N, ADP->ML_metric_th) == 1)

				if (metric <= ADP->ML_metric_th) { // 3.14修改，注释掉
				//if (metric <= ADP->ML_metric_th || stall_count >= ADP->convergence_window) { // 3.14修改，添加收敛速率检测的自动停机条件
					ADP->check_flag = 1;
					break;
				} // dlh test 注释掉

				//ADP->check_flag = 1; // dlh test
				//break;				 // dlh test

			}
			/*
			if (ADP->check_flag == 1) {
				int count;
				for (count = 0; count < ADP->N; count++) {
					if (outseq[count] != 0)
						break;
				}
				if (count == ADP->N)
					ADP->check_flag = 0;
			}*/
		} // 内循环结束

		// Auto stop
		if (ADP->check_flag == 1)
		{
			break;
		}
	} // 外循环结束

	if (ADP->check_flag == 0)
	{
		for (i = 0; i < N; i++)
		{
			if (bitsoft[i] > 0)
			{
				codeword[i] = 0;
			}
			else
			{
				codeword[i] = 1;
			}
		}
		//Recover_Info(codeword, outseq, ADP);
		if (ADP->PAC_code->system == 0) {
			Recover_Info(codeword, outseq, ADP);
		}
		else {
			for (int i = 0; i < ADP->K; i++)
				outseq[i] = codeword[ADP->A[i]];
		}
	}

	delete[] y_H;
	delete[] alpha;
	delete[] p1;
	delete[] adaptive_p1;
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (i = 0; i < M; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;
	delete[] Interchange_Buf;
	delete[] codeword;
}



void StochasticGrouping(double* LLR, int N, int* ReOrder, double ReFactor,
	default_random_engine& eng, uniform_real_distribution<double>& uniform)
{
	int i, j;
	double* Pr = new double[N];
	double* Pr_Table = new double[N];
	double Pr_sum = 0;
	double rand_alpha;

	//for (i = 0; i < N; i++)
	//{
	//	printf("%5.2f ", LLR[i]);
	//}
	//printf("\n");

	/*for (i = 0; i < N; i++)
	{
		Pr[i] = 1.0 / pow(fabs(LLR[i]), ReFactor);
		Pr_sum += Pr[i];
	}*/

	for (i = 0; i < N; i++)
	{
		Pr[i] = 1.0 / (1 + exp(LLR[i]));
		Pr[i] = Pr[i] > (1 - Pr[i]) ? Pr[i] : (1 - Pr[i]);
		Pr[i] = pow(1 / (Pr[i] - 0.5) - 2, ReFactor);
		Pr_sum += Pr[i];
	}

	/*printf("Pr: ");
	for (i = 0; i < N; i++)
	{
		printf("%5.4f ", Pr[i]);
	}
	printf("\n");
	printf("Pr_sum = %7.5e\n", Pr_sum);
	getch();*/


	Pr_Table[0] = 0;
	for (i = 1; i < N; i++)
	{
		Pr_Table[i] = Pr_Table[i - 1] + Pr[i - 1];
	}

	for (i = 0; i < N; i++)
	{
		rand_alpha = (double)rand() / (RAND_MAX + 1E-10) * Pr_sum;   // uniform distribution [0, Pr_sum)
		/*rand_alpha = (uniform(eng));
		rand_alpha *= Pr_sum;*/
		for (j = N - 1; j >= 0; j--)
		{
			if (Pr_Table[j] <= rand_alpha)
			{
				ReOrder[N - 1 - i] = j;
				break;
			}
		}

		for (j = ReOrder[N - 1 - i] + 1; j < N; j++)
		{
			Pr_Table[j] -= Pr[ReOrder[N - 1 - i]];
			if (Pr_Table[j] < 0)
				Pr_Table[j] = 0;
		}
		Pr_sum -= Pr[ReOrder[N - 1 - i]];
		if (Pr_sum < 0)
			Pr_sum = 0;

		/*printf("%.8f\n", Pr_sum);
		if (Pr_sum < 0)
		{
			for (int i = 0; i < N; i++)
			{
				printf("%d ", ReOrder[i]);
			}
			printf("\n\n");
			getch();
		}*/
	}

	delete[] Pr;
	delete[] Pr_Table;


	/*for (i = 0; i < N; i++)
	{
		printf("%d ", ReOrder[i]);
	}
	printf("\n\n");
	getch();*/
	//getch();
}
/*
ABP with stochastic grouping
*/
void SG_ABP(double* bitsoft, int** H, int N, int M, int* outseq,
	struct IterStruct* Iter, struct IterStruct* Tanner, struct ADPStruct* ADP, default_random_engine& rng, uniform_real_distribution<double>& URD)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	double sign, sum;
	double tempq;
	int pos = 0;
	int* alpha;
	double* tanhq;
	double* p1;
	double* adaptive_p1;
	int* ReliabilityOrder;
	int* ReliabilityOrderGE;
	int** adaptiveH; //根据可靠度作GE后，标准形的H矩阵
	int* InterGE;
	int K; //H矩阵经过GE后，实际的K

	double minSED = MAXVALUE;//minimum squared Euclidean distance
	int flag = 0;//check满足时置为1
	int outer_it = 0;
	int* Interchange_Buf;
	double* MRB_LLR;
	int* MRB_Order;
	int* codeword=new int[N];

	ADP->check_flag = 0;
	ADP->IterTime = 0;
	//hard decision
	for (i = 0; i < N; i++)
	{
		if (*(bitsoft + i) > 0)
		{
			*(codeword + i) = 0;
		}
		else
		{
			*(codeword + i) = 1;
		}
	}
	if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
	{
		Recover_Info(codeword, outseq, ADP);
		ADP->check_flag = 1;
		delete[] codeword;
		return;
		//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
	}

	tanhq = new double[N];
	alpha = new int[N];
	p1 = new double[N];
	adaptive_p1 = new double[N];
	ReliabilityOrder = new int[N];
	ReliabilityOrderGE = new int[N];
	adaptiveH = new int* [M];
	for (i = 0; i < M; i++) adaptiveH[i] = new int[N];
	InterGE = new int[N];

	Interchange_Buf = new int[ADP->Interchange];
	MRB_LLR = new double[N - M];
	MRB_Order = new int[N - M];
	for (outer_it = 0; outer_it < ADP->N2; outer_it++)
	{
		for (i = 0; i < M; i++)
		{
			memset(Iter->R[i], 0, sizeof(double) * N);
		}
		memcpy(Iter->pLLR, bitsoft, sizeof(double) * N);
		for (k = 0; k < ADP->N1; k++)
		{
			ADP->IterTime += 1;
			memcpy(p1, Iter->pLLR, sizeof(double) * N);
			memset(Iter->pLLR, 0, sizeof(double) * N);

			// adaptive the PCM
			if (ADP->SG_Scheme == 0)		// SG作用于全局
				StochasticGrouping(p1, N, ReliabilityOrder, ADP->ReliableFactor, rng, URD);
			else if (ADP->SG_Scheme == 1)	// SG作用于除第一次外迭代
			{
				if (outer_it > 0)
					StochasticGrouping(p1, N, ReliabilityOrder, ADP->ReliableFactor, rng, URD);
				else
					SortLLR(p1, N, ReliabilityOrder);
			}
			else if (ADP->SG_Scheme == 2)	// SG作用于除第一次外迭代之后的每第一次BP迭代
			{
				if (k == 0 && outer_it > 0)
					StochasticGrouping(p1, N, ReliabilityOrder, ADP->ReliableFactor, rng, URD);
				else
					SortLLR(p1, N, ReliabilityOrder);
			}
			else if (ADP->SG_Scheme == 3)	// SG只作用于MRB，选取一些比特交换到LRB中
			{
				SortLLR(p1, N, ReliabilityOrder);
				if (k == 0 && outer_it > 0)
					//if (outer_it > 0)
				{
					for (i = 0; i < N - M; i++)
					{
						MRB_LLR[i] = p1[ReliabilityOrder[i]];
					}
					StochasticGrouping(MRB_LLR, N - M, MRB_Order, ADP->ReliableFactor, rng, URD);
					for (i = 0; i < N - M; i++)
					{
						ReliabilityOrder[i] = ReliabilityOrder[MRB_Order[i]];
					}
					for (i = 0; i < ADP->Interchange; i++)
					{
						Interchange_Buf[i] = ReliabilityOrder[N - M - ADP->Interchange + i];
					}
					memcpy(ReliabilityOrder + N - M - ADP->Interchange, ReliabilityOrder + N - M, sizeof(int) * M);
					memcpy(ReliabilityOrder + N - ADP->Interchange, Interchange_Buf, sizeof(int) * ADP->Interchange);
				}
			}
			OSD_GE_H(H, adaptiveH, M, N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
			if (ADP->Deg2 == 1)
			{
				Permute(ADP->Deg2RandSeq, M);
				for (i = 0; i < M - 1; i++)
				{
					for (j = 0; j < N; j++)
					{
						adaptiveH[ADP->Deg2RandSeq[i]][j] ^= adaptiveH[ADP->Deg2RandSeq[i + 1]][j];
					}
				}
			}
			for (i = 0; i < N; i++) adaptive_p1[i] = p1[ReliabilityOrderGE[i]];
			InitialIter(M, N, adaptiveH, Iter);

			for (i = 0; i < M; i++)
			{
				sum = 0;
				sign = 1;
				for (j = 0; j < Iter->CNdegree[i]; j++)
				{
					tempq = adaptive_p1[Iter->CNindex[i][j]];
					if (tempq < 0)
					{
						alpha[j] = -1;
						sign = 0 - sign;
						tanhq[j] = FaiFunction(0 - tempq);
					}
					else
					{
						alpha[j] = 1;
						tanhq[j] = FaiFunction(tempq);
					}
					sum += tanhq[j];
				}
				for (j = 0; j < Iter->CNdegree[i]; j++)
				{
					Iter->R[i][j] = sign * alpha[j] * FaiFunction(sum - tanhq[j]);
					Iter->pLLR[Iter->CNindex[i][j]] += Iter->R[i][j];
				}
			}
			// 恢复pLLR的顺序
			for (i = 0; i < N; i++)
				adaptive_p1[ReliabilityOrderGE[i]] = Iter->pLLR[i];
			memcpy(Iter->pLLR, adaptive_p1, sizeof(double) * N);
			// hard decision
			for (i = 0; i < N; i++)
			{
				Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + p1[i];

				if (Iter->pLLR[i] > 0)
				{
					*(codeword + i) = 0;
				}
				else
				{
					*(codeword + i) = 1;
				}
			}

			if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
			{
				Recover_Info(codeword, outseq, ADP);
				ADP->check_flag = 1;
				flag = 1;
				break;
			}
		}
		// Auto stop
		if (flag == 1)
		{
			break;
		}
	}

	if (flag == 0)
	{
		for (i = 0; i < N; i++)
		{
			if (*(bitsoft + i) > 0)
			{
				*(outseq + i) = 0;
			}
			else
			{
				*(outseq + i) = 1;
			}
		}
	}

	delete[] alpha;
	delete[] p1;
	delete[] adaptive_p1;
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (i = 0; i < M; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;

	delete[] Interchange_Buf;
	delete[] tanhq;
	delete[] MRB_LLR;
	delete[] MRB_Order;
}
/*
翻转似然比最小的比特
ABP/MSA(N1, N2)
bitsoft:		bit LLR
y:				undemodulated signal
H:				the parity check matrix
N:				code length
M:				the number of check nodes
outseq:			decoder-output, only first K information bits are valid, the last (N-K) bits are meaningless
*/
void EC_ABP_MSA(double* bitsoft, double* y, int** H, int N, int M, int* outseq,
	struct IterStruct* Iter, struct IterStruct* Tanner, struct ADPStruct* ADP)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int m = 0;
	int sign = -1;
	double tempd = 0;
	double min1 = 0;
	double min2 = 0;
	int pos = 0;
	int* alpha;
	double* p1;
	double* adaptive_p1;
	int* ReliabilityOrder;
	int* ReliabilityOrderGE;
	int** adaptiveH;				//H after Gaussian Elimination
	int* InterGE;
	int* codeword;
	int* y_H;
	int K;
	int outer_it = 0;
	int* Interchange_Buf;
	double R;
	double minSED = MAXVALUE;//minimum squared Euclidean distance
	ADP->check_flag = 0;
	ADP->IterTime = 0;

	//hard decision
	codeword = new int[N];
	for (i = 0; i < N; i++)
	{
		if (*(bitsoft + i) > 0)
		{
			*(codeword + i) = 0;
		}
		else
		{
			*(codeword + i) = 1;
		}
	}
	//if (CheckCode(codeword, M, Tanner) == 0)
	if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
	{
		Recover_Info(codeword, outseq, ADP);
		ADP->check_flag = 1;
		delete[] codeword;
		return;
		//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0)
	}
	y_H = new int[N];
	alpha = new int[N];
	p1 = new double[N];
	adaptive_p1 = new double[N];
	ReliabilityOrder = new int[N];
	ReliabilityOrderGE = new int[N];
	adaptiveH = new int* [M];
	for (i = 0; i < M; i++) adaptiveH[i] = new int[N];
	InterGE = new int[N];
	Interchange_Buf = new int[ADP->Interchange];

	double* TempLLR = new double[N];
	memcpy(y_H, codeword, sizeof(int) * N);
	for (outer_it = 0; outer_it < ADP->N2; outer_it++)
	{
		memcpy(Iter->pLLR, bitsoft, sizeof(double) * N);
		for (k = 0; k < ADP->N1; k++)
		{
			ADP->IterTime += 1;
			memcpy(p1, Iter->pLLR, sizeof(double) * N);
			memset(Iter->pLLR, 0, sizeof(double) * N);

			// adaptive the PCM
			SortLLR(p1, N, ReliabilityOrder);
			if (k == 0 && outer_it > 0)	// outer iterations, various grouping
			{
				//固定可靠比特，逐次变换不可靠比特
				/*
				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M + (outer_it - 1)*ADP->Interchange + i];// 选取LSB集合边缘的几个位置
				}
				memcpy(ReliabilityOrder + ADP->Interchange, ReliabilityOrder, sizeof(int)*(N - M + (outer_it - 1)*ADP->Interchange));
				memcpy(ReliabilityOrder, Interchange_Buf, sizeof(int)*ADP->Interchange);// 交换的位置放到最左边(最可靠)，防止GE过程因为列交换而被选到
				*/
				//固定不可靠比特，逐次变换可靠比特

				for (i = 0; i < ADP->Interchange; i++)
				{
					Interchange_Buf[i] = ReliabilityOrder[N - M - outer_it * ADP->Interchange + i];
				}
				memcpy(ReliabilityOrder + N - M - outer_it * ADP->Interchange, ReliabilityOrder + N - M - (outer_it - 1) * ADP->Interchange, sizeof(int) * (M + (outer_it - 1) * ADP->Interchange));
				memcpy(ReliabilityOrder + N - ADP->Interchange, Interchange_Buf, sizeof(int) * ADP->Interchange);

			}
			OSD_GE_H(H, adaptiveH, M, N, &K, ReliabilityOrder, ReliabilityOrderGE, InterGE);
			if (ADP->Deg2 == 1)
			{
				Permute(ADP->Deg2RandSeq, M);
				for (i = 0; i < M - 1; i++)
				{
					for (j = 0; j < N; j++)
					{
						adaptiveH[ADP->Deg2RandSeq[i]][j] ^= adaptiveH[ADP->Deg2RandSeq[i + 1]][j];
					}
				}
			}
			for (i = 0; i < N; i++) adaptive_p1[i] = p1[ReliabilityOrderGE[i]];
			InitialIter(M, N, adaptiveH, Iter);

			//翻转最不可靠的比特
			//相当于做三次BP
			//EC_ABP
			//int minposition = min_element(adaptive_p1,adaptive_p1+N)-adaptive_p1;
			//printf("%d ", minposition);
			memcpy(TempLLR, adaptive_p1, sizeof(double) * ADP->N);
			int MAX_EC = 100;
			for (int EC = 0; EC <= MAX_EC; EC++) {
				memcpy(adaptive_p1, TempLLR, sizeof(double) * ADP->N);
				memset(Iter->pLLR, 0, sizeof(double) * N);
				if (EC < MAX_EC) {
					if (EC % 2 == 1 )
						adaptive_p1[M+(EC / 2)] = -10000;
					else
						adaptive_p1[M+(EC / 2)] = 10000;
				}

				//ABP中BP
				for (m = 0; m < M; m++)
				{
					sign = 1;
					pos = -1;
					min1 = MAXVALUE;
					min2 = MAXVALUE;

					for (i = 0; i < Iter->CNdegree[m]; i++)
					{
						tempd = adaptive_p1[Iter->CNindex[m][i]];
						if (tempd < 0)
						{
							*(alpha + i) = -1;
							sign = 0 - sign;
							tempd = 0 - tempd;
						}
						else
						{
							*(alpha + i) = 1;
						}
						if (tempd < min1)
						{
							min2 = min1;
							min1 = tempd;
							pos = i;
						}
						else
						{
							if (tempd < min2)
							{
								min2 = tempd;
							}
						}
					}
					for (i = 0; i < Iter->CNdegree[m]; i++)
					{
						if (i == pos)
						{
							R = min2;
						}
						else
						{
							R = min1;
						}
						R *= (sign * alpha[i]);
						Iter->pLLR[Iter->CNindex[m][i]] += R;
					}
				}
				// recover the LLRs' orders
				for (i = 0; i < N; i++)
					adaptive_p1[ReliabilityOrderGE[i]] = Iter->pLLR[i];
				memcpy(Iter->pLLR, adaptive_p1, sizeof(double) * N);
				// hard decision
				for (i = 0; i < N; i++)
				{
					if (ADP->use_channel_LLR)
						Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + bitsoft[i];
					else
						Iter->pLLR[i] = Iter->pLLR[i] * ADP->damping_factor + p1[i];

					if (Iter->pLLR[i] > 0)
					{
						*(codeword + i) = 0;
					}
					else
					{
						*(codeword + i) = 1;
					}
				}

				//if (CheckCode(codeword, M, Tanner) == 0)
				if (Fake_Check(codeword, outseq, ADP, ADP->PAC_code->PolarCode))
				{
					Recover_Info(codeword, outseq, ADP);
					//if (CRC_DEC(outseq, ADP->CRC_len, ADP->K) == 0 && JudgeCodeword(y_H, codeword, y, N, ADP->ML_metric_th) == 1)
					ADP->check_flag = 1;
					break;
				}
				
			}
			// Auto stop
			if (ADP->check_flag == 1)
			{
				break;
			}
			/*
			if (ADP->check_flag == 1) {
				int count;
				for (count = 0; count < ADP->N; count++) {
					if (outseq[count] != 0)
						break;
				}
				if (count == ADP->N)
					ADP->check_flag = 0;
			}*/
		}

		// Auto stop
		if (ADP->check_flag == 1)
		{
			break;
		}
	}
	if (ADP->check_flag == 0)
	{
		for (i = 0; i < N; i++)
		{
			if (*(bitsoft + i) > 0)
			{
				*(codeword + i) = 0;
			}
			else
			{
				*(codeword + i) = 1;
			}
		}
		Recover_Info(codeword, outseq, ADP);
	}

	delete[] y_H;
	delete[] alpha;
	delete[] p1;
	delete[] adaptive_p1;
	delete[] InterGE;
	delete[] ReliabilityOrder;
	delete[] ReliabilityOrderGE;
	for (i = 0; i < M; i++)
		delete[] adaptiveH[i];
	delete[] adaptiveH;
	delete[] Interchange_Buf;
	delete[] codeword;
}
//PAC 译码辅助函数
int cal2(int** T,int* v, int index) {
	//vector<int> poly = { 1,1,0,1,1,0,1 };
	int sum = v[index];
	for (int i = index - 1; i >= 0; i--) {
		if (T[i][index] != 0)
			sum += v[i];
	}
	return sum % 2;
}
//PAC: codeword=u*T*G
//output: u*T
void Decode_PLVA_SC(double* LLR, int N, int K, int L, int** GMatrix, int* A, int** T, int* DecodingSeq, struct ADPStruct* ADP) {
	ADP->IterTime = 0;
	ADP->check_flag = 0;
	int n = (int)log2(N);
	//cout << n;
	int** list = new int* [L];
	for (int i = 0; i < L; i++) {
		list[i] = new int[N];
		memset(list[i], 0, sizeof(int) * N);
	}

	int** olist = new int* [L];
	for (int i = 0; i < L; i++) {
		olist[i] = new int[N];
		memset(olist[i], 0, sizeof(int) * N);
	}
	//BinMatrix list(L, N);
	//BinMatrix olist(L, N);

	double*** sheet = new double** [L];
	for (int i = 0; i < L; i++) {
		sheet[i] = new double* [n + 1];
		for (int j = 0; j < n + 1; j++) {
			sheet[i][j] = new double[N];
			memset(sheet[i][j], 0, sizeof(double) * N);
		}
		for (int k = 0; k < N; k++)//初始化最右侧软值
			sheet[i][n][k] = LLR[k];
	}
	vector<double> PM(L);//路径度量
	vector<int> base(n + 1);
	base[0] = 1;
	for (int q = 1; q <= n; q++)
		base[q] = 2 << (q - 1);
	int q = n - 1;//列序号
	vector<int> p(n + 1);//行序号
	p[n] = N;

	int** listtemp = new int* [2 * L];
	for (int i = 0; i < 2 * L; i++) {
		listtemp[i] = new int[N];
		memset(listtemp[i], 0, sizeof(int) * N);
	}

	int** olisttemp = new int* [2 * L];
	for (int i = 0; i < 2 * L; i++) {
		olisttemp[i] = new int[N];
		memset(olisttemp[i], 0, sizeof(int) * N);
	}
	double*** sheettemp = new double** [2 * L];
	for (int i = 0; i < 2 * L; i++) {
		sheettemp[i] = new double* [n + 1];
		for (int j = 0; j < n + 1; j++) {
			sheettemp[i][j] = new double[N];
			memset(sheettemp[i][j], 0, sizeof(double) * N);
		}
	}
	//double* PMtemp=new double[2 * L];
	vector<double> PMtemp(2 * L);
	int validL = 1;//有效List大小
	int* SCL_result = new int[N];
	int* Inter_result = new int[N];
	
	while (q < n) {
		while (q >= 0) {
			for (int pq = p[q]; pq < p[q] + base[q]; pq++) {
				// f运算
				if (pq % base[q] == pq % base[q + 1]) {
					for (int l = 0; l < validL; l++) {
						double com1 = sheet[l][q + 1][pq], com2 = sheet[l][q + 1][pq + base[q]];
						double minimum = (fabs(com1) < fabs(com2)) ? fabs(com1) : fabs(com2);
						double sign1 = com1 > 0 ? 1.0 : -1.0;
						double sign2 = com2 > 0 ? 1.0 : -1.0;
						sheet[l][q][pq] = sign1 * sign2 * minimum;
						// sheet(pq, q, l) = log((exp(sheet(pq, q + 1, l) + sheet(pq + base(q), q + 1, l)) + 1) / (exp(sheet(pq, q + 1, l)) + exp(sheet(pq + base(q), q + 1, l))));
					}
				}
				// g运算
				else {
					vector<unsigned int> marks(base[q]);
					vector<int> Usum(validL);
					for (int v = 0; v < base[q]; v++) {
						marks[v] = GMatrix[v][pq % base[q]];
						if (marks[v] == 1) {
							for (int l = 0; l < validL; l++)
								Usum[l] = (Usum[l] + list[l][v + pq / base[q + 1] * base[q + 1]]) % 2;//
						}
					}
					for (int l = 0; l < validL; l++) {
						sheet[l][q][pq] = sheet[l][q + 1][pq - base[q]] * (1 - 2 * Usum[l]) + sheet[l][q + 1][pq];
					}
				}
				if (q == 0) {
					//if (find(A.begin(), A.end(), pq + 1) - A.end()) {//非冻结比特
					//for (int i = 0; i < K; i++)
						//cout << A[i] << " ";
					//auto temp = find(A, A + K, pq );
					int temp = count(A, A + K, pq);
					//cout << temp << " ";
					if (temp != 0) {
						// ----------扩展----------
						for (int i = 0; i < validL; i++) {
							PMtemp[i] = PM[i];
							PMtemp[validL + i] = PM[i];
							//sheettemp[i] = sheet[i];
							for (int j = 0; j < n + 1; j++)
								memcpy(sheettemp[i][j], sheet[i][j], sizeof(double)*N);
							//sheettemp[validL + i] = sheet[i];
							for (int j = 0; j < n + 1; j++)
								memcpy(sheettemp[validL+i][j], sheet[i][j], sizeof(double) * N);
							//listtemp[i] = list[i];
							memcpy(listtemp[i], list[i], sizeof(int)* N);
							//listtemp[validL + i] = list[i];
							memcpy(listtemp[validL + i] ,list[i], sizeof(int) * N);
							//olisttemp[i] = olist[i];
							memcpy(olisttemp[i], olist[i] , sizeof(int) * N);
							//olisttemp[validL + i] = olist[i];
							memcpy(olisttemp[validL + i], olist[i], sizeof(int)* N);
							olisttemp[i][pq] = 0;
							olisttemp[validL + i][pq] = 1;
						}
						unsigned int ru, sym;
						for (int i = 0; i < 2 * validL; i++) {
							ru = (sheettemp[i][0][pq] < 0) ? 1 : 0;
							listtemp[i][pq] = cal2(T, olisttemp[i], pq);
							sym = 1 - (ru == listtemp[i][pq]);
							PMtemp[i] = PMtemp[i] + sym * fabs(sheettemp[i][0][pq]);
						}
						vector<unsigned int> index(PMtemp.size());
						iota(index.begin(), index.begin() + 2 * validL, 0);
						sort(index.begin(), index.begin() + 2 * validL, [&PMtemp](size_t i1, size_t i2) {return PMtemp[i1] < PMtemp[i2]; });
						// ----------竞争----------
						int minlist = (2 * validL) > L ? L : 2 * validL;
						for (int i = 0; i < minlist; i++) {
							//olist[i] = olisttemp[index[i]];
							memcpy(olist[i], olisttemp[index[i]], sizeof(int) * N);
							//list[i] = listtemp[index[i]];
							memcpy(list[i], listtemp[index[i]], sizeof(int) * N);
							//sheet[i] = sheettemp[index[i]];
							for (int j = 0; j < n + 1; j++)
								memcpy(sheet[i][j], sheettemp[index[i]][j], sizeof(double) * N);
							PM[i] = PMtemp[index[i]];
						}
						validL = minlist;
					}
					else//冻结比特
					{
						unsigned int ru, sym;
						for (int i = 0; i < validL; i++) {
							ru = (sheet[i][0][pq] < 0) ? 1 : 0;
							list[i][pq] = cal2(T, olist[i], pq);
							sym = 1 - (ru == list[i][pq]);
							PM[i] = PM[i] + sym * fabs(sheet[i][0][pq]);
						}
					}
				}
			}
			p[q] = p[q] + base[q];
			q = q - 1;
		}
		while (1) {
			q = q + 1;
			if (q == n)
				break;
			if (p[q] != p[q + 1])
				break;
		}
	}
	//DecodingSeq = list[0];
	for (int i = 0; i < N; i++) {
		SCL_result[i] = list[0][i];
		//printf("%d ", result[i]);
	}
	
	if (ADP->PAC_code->system == 0) {
		//Encode(SCL_result, Inter_result, N, N, GMatrix);
		//Recover_Info(Inter_result, DecodingSeq, ADP);

		int res = 0;
		for (int i = 0; i < L; i++) {
			res++;
			for (int j = 0; j < N; j++) {
				SCL_result[j] = list[i][j];
				//printf("%d ", result[i]);
			}
			Encode(SCL_result, Inter_result, N, N, GMatrix);
			Recover_Info(Inter_result, DecodingSeq, ADP);
			if (CRC_DEC(DecodingSeq, ADP->CRC_len, ADP->K) == 0) {
				ADP->check_flag = 1;
				break;
			}	
		}
		if (res == L && ADP->check_flag == 0) {
			for (int i = 0; i < N; i++) {
				SCL_result[i] = list[0][i];
				//printf("%d ", result[i]);
			}
			Encode(SCL_result, Inter_result, N, N, GMatrix);
			Recover_Info(Inter_result, DecodingSeq, ADP);
		}
	}
	else {
		Encode(SCL_result, Inter_result, N, N, GMatrix);
		for (int i = 0; i < K; i++)
			DecodingSeq[i] = Inter_result[A[i]];
	}


	//系统形式

	//printMatrix(list,"list.txt");
	//InfoSeq = InfoSeq * T;
	//count(sheet[0][0]);
	//return;

	/*
	vector<unsigned int>c(K);
	vector<unsigned int>rc(N);
	for (int l = 0; l < L; l++) {
		if (!System) {
			for (int i = 0; i < K; i++) {
				c[i] = olist[l][A[i] - 1];
			}
		}
		else {
			rc = list[l] * GMatrix;
			for (int i = 0; i < K; i++) {
				c[i] = rc[A[i] - 1];
			}
		}
		if (crc.CRCCheck(c) == true) {
			DecodingSeq = list[l];
			DetectSCL = true;
			break;
		}
	}
	*/
	//release room
	for (int i = 0; i < L; i++)
		delete[] list[i];
	delete[] list;
	for (int i = 0; i < L; i++)
		delete[] olist[i];
	delete[] olist;

	for (int i = 0; i < L; i++){
		for (int j = 0; j < n + 1; j++) {
			delete[] sheet[i][j];
		}
		delete[] sheet[i];
	}	
	delete[] sheet;
	for (int i = 0; i < 2*L; i++)
		delete[] listtemp[i];
	delete[] listtemp;
	for (int i = 0; i < 2*L; i++)
		delete[] olisttemp[i];
	delete[] olisttemp;
	for (int i = 0; i < 2*L; i++) {
		for (int j = 0; j < n + 1; j++) {
			delete[] sheettemp[i][j];
		}
		delete[] sheettemp[i];
	}
	delete[] sheettemp;
	delete[] SCL_result;
	delete[] Inter_result;
}

void Decode(double snr, double* bitsoft, double* y, int* result, struct ADPStruct* ADP)
{
	// C标准库 rand() 当前正在使用
	//srand(731); // srand()设置的种子是进程级别的全局变量，会影响后续的rand()调用，每帧重置

	// C++11 default_random_engine 在第1035-1036行被注释
	default_random_engine rng;
	uniform_real_distribution<double> URD(0, 1);
	rng.seed(173);

	if (ADP->DecodingMethod == 1)
	{
		ideal_ABP_MSA(bitsoft, y, ADP->Joint_check_matrix, ADP->N, ADP->M+ADP->CRC_len_for_ABP, result, ADP->IterDec, ADP->Tanner, ADP);
	}
	else if (ADP->DecodingMethod == 2)
	{
		ABP_MSA(snr, bitsoft, y, ADP->Joint_check_matrix, ADP->N, ADP->M + ADP->CRC_len_for_ABP, result, ADP->IterDec, ADP->Tanner, ADP);
	}
	else if (ADP->DecodingMethod == 3)
	{
		SG_ABP(bitsoft, ADP->PAC_code->H, ADP->N, ADP->M, result, ADP->IterDec, ADP->Tanner, ADP, rng, URD);
	}
	else if (ADP->DecodingMethod == 4)
	{
		List_ABP_MSA(bitsoft, y, ADP->PAC_code->H, ADP->N, ADP->M, result, ADP->IterDec, ADP->Tanner, ADP);
	}
	if (ADP->DecodingMethod == 5)
	{
		EC_ABP_MSA(bitsoft, y, ADP->PAC_code->H, ADP->N, ADP->M, result, ADP->IterDec, ADP->Tanner, ADP);
	}
	if (ADP->DecodingMethod == 6)
	{
		Decode_PLVA_SC(bitsoft, ADP->N, ADP->K, ADP->PAC_code->L, ADP->G, ADP->A, ADP->PAC_code->T, result,ADP);
	}
}