#include "define.h"


/*
交换矩阵的两个列
Matrix: 矩阵
rowNum: 矩阵的行数
colNum: 矩阵的列数
a, b: 交换的两列
*/
//void SwitchColumn(int **Matrix, int rowNum, int colNum, int a, int b)
//{
//	int temp;
//	int i;
//
//	if (a < 0 || b < 0 || a >= colNum || b >= colNum)
//	{
//		printf("Program Error at SwitchColumn!\n");
//		getch();
//		exit(0);
//	}
//
//	for (i = 0; i < rowNum; i++)
//	{
//		temp = Matrix[i][a];
//		Matrix[i][a] = Matrix[i][b];
//		Matrix[i][b] = temp;
//	}
//}

int SeekColumn(int *h, int *scp, int cp, int M, int N)
{
	int i = 0;
	int j = 0;
	int rp = -1;

	for (i = N - cp - 2; i >= 0; i--)
	{
		for (j = M - cp - 1; j >= 0; j--)
		{
			//if ((*(h + j * N + i)) == 1)
			if ((*(h + j * N + i)) >= 1)
			{
				rp = j;
				*scp = i;
				return rp;
			}
		}
	}

	return rp;
}
void SwitchColumn(int *m, int nc, int nr, int a, int b)
{
	int i = 0;
	int temp = 0;

	for (i = 0; i<nr; i++)
	{
		temp = *(m + i * nc + a);
		*(m + i * nc + a) = *(m + i * nc + b);
		*(m + i * nc + b) = temp;
	}
}

/*
Hi: 原始的二进制H矩阵
Ho: 高斯消元后新的标准型H矩阵
M, N: 输入，H矩阵的行列数
K: 初始的K=N-M，如果H矩阵不满秩，K会更改
ReliabilityOrder: 根据LLR可靠度排序后的bit顺序
ReliabilityOrderGE: GE过程中发生列交换，交换后的ReliabilityOrder。文献中表示为2次交织后的顺序
InterGE: ReliabilityOrder和ReliabilityOrderGE的交织关系
*/
void OSD_GE_H(int **Hi, int **Ho, int M, int N, int *K, 
	const int *ReliabilityOrder, int *ReliabilityOrderGE, int *InterGE)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int temp = 0;
	int scp = 0;
	int rp = 0;
	int fs = 0;//GE过程中发生列交换的标志
	int *th;
	int *pos;//GE过程中发生列交换，记录位置
	int *tr;//临时存放reliability order的数组

	*K = N - M;
	th = new int[M*N];
	pos = new int[N];
	tr = new int[N];

	for (i = 0; i < N; i++) pos[i] = i;

	// 根据ReliabilityOrder调整H矩阵
	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			Ho[i][j] = Hi[i][ReliabilityOrder[j]];

	// 把调整过的H赋值给临时变量th
	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			th[i*N + j] = Ho[i][j];

	// 高斯消元
	//do Gauss Elimation from bottom line up
	for (i = 0; i<M; i++)
	{
		temp = N * M - (i * N) - i - 1;
		rp = M - i - 1;

		//seek the line that currrent column bit is 1
		for (j = 0; j<M - i; j++)
		{
			if ((*(th + temp)) == 1)
			{
				break;
			}
			else
			{
				temp -= N;
				rp -= 1;
			}
		}

		if (rp == -1)	//can not find '1' in this column
		{
			rp = SeekColumn(th, &scp, i, M, N);

			//can not find any other columns with one '1' in it
			if (rp == -1)//不满秩
			{
				*K = N - i;
				break;
			}

			//switch this two column
			SwitchColumn(th, N, M, N - i - 1, scp);//从后往前找，与H矩阵按可靠度降序排列一致
			fs += 1;

			temp = *(pos + N - i - 1);
			*(pos + N - i - 1) = *(pos + scp);
			*(pos + scp) = temp;
		}

		if (rp != M - i - 1)	//current column position not equal to '1'
		{
			//add the rp row to current row to make currunt column position equal to '1'
			for (j = 0; j<N; j++)
			{
				(*(th + N * (M - i - 1) + j)) ^= (*(th + N * rp + j));
			}
		}

		//elimate the other '1's in current column
		for (j = 0; j<M; j++)
		{
			if ((j != M - i - 1) && ((*(th + N * j + N - i - 1)) == 1))
			{
				for (k = 0; k<N; k++)
				{
					(*(th + N * j + k)) ^= (*(th + N * (M - i - 1) + k));
				}
			}
		}
		//printf("\r Gauss Elimination: %d", (i + 1));
	}

	memcpy(InterGE, pos, sizeof(int)*N);
	// 如果GE过程中发生了列交换，更新ReliabilityOrder
	if (fs != 0)
	{
		for (i = 0; i < N; i++) tr[i] = ReliabilityOrder[pos[i]];
		for (i = 0; i < N; i++) ReliabilityOrderGE[i] = tr[i];
	}
	else
	{
		memcpy(ReliabilityOrderGE, ReliabilityOrder, sizeof(int)*N);
	}
	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			Ho[i][j] = th[i*N + j];

	delete[] th;
	delete[] tr;
	delete[] pos;
}


void Encode(const int * u, int * c, int K, int N, int **G)
{
	for (int i = 0; i < N; ++i)
	{
		c[i] = 0;
		for (int j = 0; j < K; ++j)
			c[i] ^= (u[j] && G[j][i]);
	}
}

/*
Input:
	LLR: 初始的LLR序列
	codeSeq: OSD译码得到的序列
	length: 码长
	minSED: 当前的最小欧氏距离平方，会在函数中更改
	result: 最优译码序列，会在函数中更改
*/
void FindOptimal(const double *LLR, int *codeSeq, int length, double *minSED, int *result)
{
	double SED = 0;
	double BPSK_Signal;
	int i;

	for (i = 0; i < length; i++)
	{
		BPSK_Signal = 1.0 - 2.0 * codeSeq[i];
		if ((BPSK_Signal > 0 && LLR[i] <= 0) || (BPSK_Signal <= 0 && LLR[i] > 0)) // 这一条件是新加的
			SED += (BPSK_Signal - LLR[i]) * (BPSK_Signal - LLR[i]);
	}

	if (SED < *minSED)
	{
		*minSED = SED;
		memcpy(result, codeSeq, sizeof(int)*length);
	}
}


// Deg-2 Random Connection
// 源代码，不是真正的均匀随机，但通过仿真实验，发现这种方式误码率性能好像更好？难道这个地方就是不能太随机？
void Permute(int *seq, int length)
{
	thread_local std::mt19937 rng(173 + omp_get_thread_num() * 10000);
 	int temp;
 	int pos1, pos2;
 	for (int i = 0; i < length / 2; i++)
 	{
 		pos1 = rng() % length;
 		pos2 = rng() % length;
 		temp = seq[pos1];
 		seq[pos1] = seq[pos2];
 		seq[pos2] = temp;
 	}
}
// Fisher-Yates (正确的均匀随机置换) - dlh
//void Permute(int* seq, int length) 
//{
//	for (int i = length - 1; i > 0; i--)
//	{
//		int j = rand() % (i + 1);
//		swap(seq[i], seq[j]);
//	}
//}


/*
GF(2)域(M,N)矩阵高斯消元
Return:
0: 秩不等于M
1: 秩等于M，但GE过程中发生列交换
2: 秩等于M，且GE过程中没有列交换
*/

int GaussElimation_GF2(int **Hi, int **Ho, int M, int N, int *K, int *pos)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int temp = 0;
	int scp = 0;
	int rp = 0;
	int fs = 0;//GE过程中发生列交换的标志
	int *th;
	int min_M_N = (M < N ? M : N);

	*K = N - M;
	th = new int[M*N];

	for (i = 0; i < N; i++) pos[i] = i;

	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			Ho[i][j] = Hi[i][j];

	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			th[i*N + j] = Ho[i][j];

	// 高斯消元
	//do Gauss Elimation from bottom line up
	for (i = 0; i < min_M_N; i++)
	{
		temp = N * M - (i * N) - i - 1;
		rp = M - i - 1;

		//seek the line that currrent column bit is 1
		for (j = 0; j<M - i; j++)
		{
			if ((*(th + temp)) == 1)
			{
				break;
			}
			else
			{
				temp -= N;
				rp -= 1;
			}
		}

		if (rp == -1)	//can not find '1' in this column
		{
			rp = SeekColumn(th, &scp, i, M, N);

			//can not find any other columns with one '1' in it
			if (rp == -1)//不满秩
			{
				*K = N - i;
				break;
			}

			//switch this two column
			SwitchColumn(th, N, M, N - i - 1, scp);//从后往前找，与H矩阵按可靠度降序排列一致
			fs += 1;

			temp = *(pos + N - i - 1);
			*(pos + N - i - 1) = *(pos + scp);
			*(pos + scp) = temp;
		}

		if (rp != M - i - 1)	//current column position not equal to '1'
		{
			//add the rp row to current row to make currunt column position equal to '1'
			for (j = 0; j<N; j++)
			{
				(*(th + N * (M - i - 1) + j)) ^= (*(th + N * rp + j));
			}
		}

		//elimate the other '1's in current column
		for (j = 0; j<M; j++)
		{
			if ((j != M - i - 1) && ((*(th + N * j + N - i - 1)) == 1))
			{
				for (k = 0; k<N; k++)
				{
					(*(th + N * j + k)) ^= (*(th + N * (M - i - 1) + k));
				}
			}
		}
		//printf("\r Gauss Elimination: %d", (i + 1));
	}

	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			Ho[i][j] = th[i*N + j];

	delete[] th;

	if (rp == -1)
		return 0;
	else if (fs > 0)
		return 1;
	else
		return 2;
}

/*
降序排列
order1: 降序排列后的序列为a[order1[0]], a[order1[1]], ...
order2: a[i]按从大到小排在第order2[i]位
*/
void DesSort(const double *bitsoft, int N, int *order1, int *order2)
{
	int i, j;
	double temp_float;
	int temp_int;
	double *llr_temp = new double[N];
	memcpy(llr_temp, bitsoft, sizeof(double)*N);
	for (i = 0; i < N; i++) order1[i] = i;
	for (i = 0; i < N - 1; i++)
	{
		for (j = 0; j < N - 1 - i; j++)
		{
			if (llr_temp[j] < llr_temp[j + 1])
			{
				temp_float = llr_temp[j];
				llr_temp[j] = llr_temp[j + 1];
				llr_temp[j + 1] = temp_float;
				temp_int = order1[j];
				order1[j] = order1[j + 1];
				order1[j + 1] = temp_int;
			}
		}
	}
	for (i = 0; i < N; i++)
	{
		order2[order1[i]] = i;
	}

	delete[] llr_temp;
}


void CRC_ENC(int* sourceseq, int* codeseq, int CRCLEN, int SRCLEN)
{
	if (CRCLEN <= 0)
	{
		memcpy(codeseq, sourceseq, sizeof(int)*SRCLEN);
		return;
	}
	// g_poly = {g0, g1,..., g_n-k}
	int g_poly_m3[4] = { 1,1,0,1 };
	int g_poly_m6[7] = { 1,0,0,0,0,1,1 };
	int g_poly_m11[12] = { 1,0,0,0,0,1,0,0,0,1,1,1 };
	int g_poly_m16[17] = { 1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1 };
	int g_poly_m24[25] = { 1,1,1,0,1,0,0,0,1,0,0,0,1,1,0,1,0,1,0,0,1,1,0,1,1 }; // CRC24C

	int r[24];
	int g[25];

	for (int i = 0; i < 24; ++i) r[i] = 0;
	if (CRCLEN == 3)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m3[i];
	}
	else if (CRCLEN == 6)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m6[i];
	}
	else if (CRCLEN == 11)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m11[i];
	}
	else if (CRCLEN == 16)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m16[i];
	}
	else if (CRCLEN == 24)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m24[i];
	}
	else
	{
		printf("CRC length is not supported!\n");
	}

	for (int i = 0; i < SRCLEN; ++i)
	{
		int temp = (r[CRCLEN - 1] ^ sourceseq[i]);
		int reg1 = 0, reg2 = 0;
		for (int j = 0; j < CRCLEN; ++j)
		{
			reg2 = reg1;
			reg1 = r[j];
			if (j == 0)
				r[j] = 0;
			else
				r[j] = reg2;
		}
		for (int j = 0; j < CRCLEN; ++j)
		{
			r[j] ^= (g[j] & temp);
		}
	}

	for (int i = 0; i < SRCLEN; ++i)
	{
		codeseq[i] = sourceseq[i];
	}
	for (int i = 0; i < CRCLEN; ++i)
	{
		codeseq[i + SRCLEN] = r[CRCLEN - 1 - i];
	}
}

int CRC_DEC(int* codeseq, int CRCLEN, int CODELEN)
{
	if (CRCLEN <= 0)
	{
		return 0;
	}
	// g_poly = {g0, g1,..., g_n-k}
	int g_poly_m3[4] = { 1,1,0,1 };
	int g_poly_m6[7] = { 1,0,0,0,0,1,1 };
	int g_poly_m11[12] = { 1,0,0,0,0,1,0,0,0,1,1,1 };
	int g_poly_m16[17] = { 1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1 };
	int g_poly_m24[25] = { 1,1,1,0,1,0,0,0,1,0,0,0,1,1,0,1,0,1,0,0,1,1,0,1,1 }; // CRC24C

	int r[24];
	int g[25];

	for (int i = 0; i < 24; ++i) r[i] = 0;
	if (CRCLEN == 3)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m3[i];
	}
	else if (CRCLEN == 6)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m6[i];
	}
	else if (CRCLEN == 11)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m11[i];
	}
	else if (CRCLEN == 16)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m16[i];
	}
	else if (CRCLEN == 24)
	{
		for (int i = 0; i < CRCLEN + 1; ++i)
			g[i] = g_poly_m24[i];
	}
	else
	{
		printf("CRC length is not supported!\n");
	}

	for (int i = 0; i < CODELEN; ++i)
	{
		int temp = r[CRCLEN - 1];
		int reg1 = 0, reg2 = 0;
		for (int j = 0; j < CRCLEN; ++j)
		{
			reg2 = reg1;
			reg1 = r[j];
			if (j == 0)
				r[j] = codeseq[i];
			else
				r[j] = reg2;
		}
		for (int j = 0; j < CRCLEN; ++j)
		{
			r[j] ^= (g[j] & temp);
		}
	}

	for (int i = 0; i < CRCLEN; ++i)
	{
		if (r[i] > 0)
			return 1;
	}
	return 0;
}

void CRC_H_initial(int **H, int CRCLEN, int k)
{
	// g_poly = {g0, g1,..., g_n-k}
	int g_poly_m3[4] = { 1,1,0,1 };
	int g_poly_m6[7] = { 1,0,0,0,0,1,1 };
	int g_poly_m11[12] = { 1,0,0,0,0,1,0,0,0,1,1,1 };
	int g_poly_m16[17] = { 1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1 };
	int g_poly_m24[25] = { 1,1,1,0,1,0,0,0,1,0,0,0,1,1,0,1,0,1,0,0,1,1,0,1,1 }; // CRC24C
	int g_poly[25];

	if (CRCLEN <= 0)
		return;
	else
	{
		if (CRCLEN == 3)
		{
			for (int i = 0; i < CRCLEN + 1; ++i)
				g_poly[i] = g_poly_m3[i];
		}
		else if (CRCLEN == 6)
		{
			for (int i = 0; i < CRCLEN + 1; ++i)
				g_poly[i] = g_poly_m6[i];
		}
		else if (CRCLEN == 11)
		{
			for (int i = 0; i < CRCLEN + 1; ++i)
				g_poly[i] = g_poly_m11[i];
		}
		else if (CRCLEN == 16)
		{
			for (int i = 0; i < CRCLEN + 1; ++i)
				g_poly[i] = g_poly_m16[i];
		}
		else if (CRCLEN == 24)
		{
			for (int i = 0; i < CRCLEN + 1; ++i)
				g_poly[i] = g_poly_m24[i];
		}
		else
		{
			printf("CRC length is not supported!\n");
		}
	}

	int **g = new int*[k];
	for (int i = 0; i < k; i++)
	{
		g[i] = new int[k + CRCLEN];
		memset(g[i], 0, sizeof(int)*(k + CRCLEN));
	}

	for (int i = 0; i < k; i++)
	{
		for (int j = 0; j < CRCLEN + 1; j++)
		{
			g[i][j + i] = g_poly[j];
		}
	}

	int *pos = new int[k + CRCLEN];
	int K;
	GaussElimation_GF2(g, g, k, k + CRCLEN, &K, pos);

	for (int i = 0; i < CRCLEN; i++)
	{
		memset(H[i], 0, sizeof(int)*(CRCLEN + k));
	}

	// CRC_ENC和CRC_DEC函数比特反序，这里要调整H的列
	for (int i = 0; i < CRCLEN; i++)
	{
		for (int j = 0; j < k; j++)
		{
			H[i][(CRCLEN + k - 1) - (j + CRCLEN)] = g[j][i];
		}
	}
	for (int i = 0; i < CRCLEN; i++)
	{
		H[i][(CRCLEN + k - 1) - i] = 1;
	}


	for (int i = 0; i < k; i++)
		delete[] g[i];
	delete[] g;
	delete[] pos;
}

// ************ 极化码生成矩阵F⊗n ************
// 计算 2^n
int power_of_2(int n) {
	return 1 << n;
}

// Kronecker积：A (sizeA x sizeA) ⊗ F (2x2) -> C (sizeA*2 x sizeA*2)
void kron_gf2(int** A, int sizeA, int** C) {
	// 基础矩阵 F = [1,0; 1,1]
	int F[2][2] = { {1, 0}, {1, 1} };

	for (int ia = 0; ia < sizeA; ia++) {
		for (int ja = 0; ja < sizeA; ja++) {
			int a_val = A[ia][ja];
			for (int ib = 0; ib < 2; ib++) {
				for (int jb = 0; jb < 2; jb++) {
					int ic = ia * 2 + ib;
					int jc = ja * 2 + jb;
					C[ic][jc] = a_val & F[ib][jb];  // GF(2)乘法
				}
			}
		}
	}
}

// 分配 size x size 的二维矩阵
int** alloc_matrix(int size) {
	int** mat = (int**)malloc(size * sizeof(int*));
	for (int i = 0; i < size; i++) {
		mat[i] = (int*)malloc(size * sizeof(int));
	}
	return mat;
}

// 释放矩阵
void free_matrix(int** mat, int size) {
	for (int i = 0; i < size; i++) {
		free(mat[i]);
	}
	free(mat);
}

// 生成极化码生成矩阵，返回 N x N 的二维矩阵
int** generate_polar_matrix(int n) {
	int N = power_of_2(n);

	// 初始化 P_n = F
	int** P_n = alloc_matrix(2);
	P_n[0][0] = 1; P_n[0][1] = 0;
	P_n[1][0] = 1; P_n[1][1] = 1;

	int current_size = 2;

	// 迭代计算 P_n = P_n ⊗ F
	for (int i = 1; i < n; i++) {
		int new_size = current_size * 2;
		int** temp = alloc_matrix(new_size);

		kron_gf2(P_n, current_size, temp);

		free_matrix(P_n, current_size);
		P_n = temp;
		current_size = new_size;
	}

	return P_n;
}

// ************ 保存二维矩阵 ************
void saveMatrixToFile(int** G, int rows, int cols, const char* filename) {
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("无法打开文件\n");
		return;
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			fprintf(fp, "%d", G[i][j]);
			if (j < cols - 1) fprintf(fp, " ");
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

// ************ 保存一维int类型矩阵 ************
void saveArrayToFile(int* A, int len, const char* filename) {
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("无法打开文件\n");
		return;
	}

	for (int i = 0; i < len; i++) {
		fprintf(fp, "%d\n", A[i]);  // 每行一个元素
	}
	fclose(fp);
}

// ************ 保存一维double类型矩阵 ************
void saveArrayDoubleToFile(double* A, int len, const char* filename) {
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("无法打开文件\n");
		return;
	}

	for (int i = 0; i < len; i++) {
		fprintf(fp, "%.15e\n", A[i]);  // %f 默认只保存到小数点后六位
	}
	fclose(fp);
}

// ************ 打印矩阵 ************
void printMatrix(int** G, int rows, int cols) {
	for(int i=0; i<rows; i++) {
		for(int j = 0; j < cols; j++) {
			printf("%d ", G[i][j]);
		}
		printf("\n");
	}
}