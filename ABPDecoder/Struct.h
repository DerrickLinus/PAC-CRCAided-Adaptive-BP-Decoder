#pragma once
#include "define.h"

// Simulation Parameter struct
struct SPStruct
{
	int SNRtype;        //0--Eb/No, 1--Es/No
	double startSNR;
	double endSNR;
	double stepSNR;

	int leastErrorFrame;
	int leastTestFrame;
	int sourceType;
	int displayStep;
};

//random seed struct
struct SEED
{
	unsigned long ix;					//long seed
	unsigned long iy;					//long seed
	unsigned long iz;					//long seed
	unsigned long ixx;					//long seed
	unsigned long iyy;					//long seed
	unsigned long izz;					//long seed
};

//AWGN channel struct
struct AWGN
{
	struct SEED *seed;			//random seed to generate Gauss variable
	int seedmethod;             //random seed generate method,1--Module, 2--Shift register
	double  snr;				//channel SNR--dB
	double  sigma;				//variation of Gauss process
	double  isigma;				//1/(sigma*sigma)
};

struct StatisStruct
{
	int testFrames;
	int errorFrames;
	int errorBits;
	int errorSym;
	int undetectedErrorFrames;		// undetectable error
	double FER;
	double SER;
	double BER;
	double UER;
	long long IterTime;
	double avgIterTime;
};

struct IterStruct
{
	int **CNindex;
	int *CNdegree;
	double *pLLR;
	double** R;
};

// ABP 译码方法每线程预分配缓冲池（方法 1-5）
struct ABPPool
{
	// 所有 ABP 方法通用
	int* codeword;             // [N]
	int* y_H;                  // [N]
	int* alpha;                // [N]
	double* p1;                // [N]
	double* adaptive_p1;       // [N]
	int* ReliabilityOrder;     // [N]
	int* ReliabilityOrderGE;   // [N]
	int* InterGE;              // [N]
	int** adaptiveH;           // [M_ABP][N] 行指针
	int* adaptiveH_data;       // [M_ABP * N] 扁平数据
	int* Interchange_Buf;      // [Interchange]

	// 方法特定（统一预分配）
	double* tanhq;             // [N] — SG_ABP
	double* MRB_LLR;           // [N-M] — SG_ABP
	int* MRB_Order;            // [N-M] — SG_ABP
	int* temp_code;            // [N] — List_ABP
	double* TempLLR;           // [N] — EC_ABP

	// OSD_GE_H 辅助（每帧调用 N1×N2 次）
	int* th;                   // [M_ABP * N]
	int* pos;                  // [N]
	int* tr;                   // [N]

	// Recover_Info 辅助
	int* rec_temp;             // [K]
	int* rec_temp_code;        // [N]

	// StochasticGrouping 辅助
	double* Pr;                // [N]
	double* Pr_Table;          // [N]
};

// SCL 译码方法每线程预分配缓冲池（方法 6）
struct SCLPool
{
	int** list;                // [L][N]
	int* list_data;            // [L * N]
	int** olist;               // [L][N]
	int* olist_data;           // [L * N]
	double*** sheet;           // [L][n+1][N]
	double* sheet_data;        // [L * (n+1) * N]
	double** sheet_rows;       // [L * (n+1)] 中间指针
	int** listtemp;            // [2L][N]
	int* listtemp_data;        // [2L * N]
	int** olisttemp;           // [2L][N]
	int* olisttemp_data;       // [2L * N]
	double*** sheettemp;       // [2L][n+1][N]
	double* sheettemp_data;    // [2L * (n+1) * N]
	double** sheettemp_rows;   // [2L * (n+1)] 中间指针
	int* SCL_result;           // [N]
	int* Inter_result;         // [N]
};

// 每线程译码缓冲池（仅分配当前 DecodingMethod 所需部分）
struct DecodePool
{
	int DecodingMethod;
	union {
		ABPPool abp;           // DecodingMethod 1-5
		SCLPool scl;           // DecodingMethod 6
	};
};


struct PACStruct 
{
	//Impulse response
	int IR_size = 7;
	int IR[7] = { 1,0,1,1,0,1,1 };
	//int IR[7] = { 1,0,0,0,0,0,0 };
	int* PolarCode;
	int** T;
	int** T_1;		//T^-1
	int** G;		//G=T*G_polar
	int** P;		//P=G*T^-1
	int** H;		//校验矩阵H
	int L;			//SCL list size
	//ABP 译码时，暂时不考虑系统形式
	int system;		//1:系统码,0:非系统码
	int** T0;		//T0:GMatrix*T中A所在的行和列，然后取逆

};

struct ADPStruct
{
	char codefile[80];
	double rate;
	int N;						// code length
	int K;						// message length
	int M;						// M = N - K

	int codeMode = 1;			// 0:polar code; 1:PAC code; 注意此次修改需要修改对应的IR序列
	int DecodingMethod;			// SPA or MSA
	int **H;					// parity-check matrix
	int **G;					// generator matrix
	int G_K;					// the number of rows of G, K or N
	int **P;					// procoding matrix, make the G to a systematic form
	
	int puncture;				// punctured bits
	int* PunctureIndex;			// index for puncture 
	
	int shorten;				// shortened bits
	int* ShortenIndex;			// index for puncture 

	int *A;						// the active positions, for Polar
	int N1;						// inner iteration
	int N2;						// outer iteration
	int Deg2;
	double IterTime;
	int HDD;
	int *Deg2RandSeq;
	int Interchange;

	int check_flag;
	int CRC_len;
	//int Gen[9] = { 1,1,0,0,1,1,0,1,1 };
	int EncodeAdd0;				// for Polar codes
	int **H_crc;				// the parity-check matrix of the CRC code
	int** H_crc_add0;			//the parity-check matrix of the CRC code with zero (N*N)
	int CRC_len_for_ABP;		// the number of CRC bits used in ABP
	int **H_cc;					// the full parity-check matrix of the CRC-coded concatenated code
	int use_channel_LLR;
	double ML_metric_th;
	int** Joint_check_matrix;
	struct IterStruct *IterDec;		// the adaptived Tanner
	struct IterStruct *Tanner;		// the original Tanner

	struct PACStruct* PAC_code;
	int SG_Scheme = 2;
	double ReliableFactor = 1;

	// Neural Min-Sum 参数
	double alpha_fixed;				// NMS缩放因子，1.0为标准MS
	double beta_fixed;				// OMS偏移量，0.0为标准MS
	double alpha_fixed2;			// NMS缩放因子(不可靠比特组)
	double beta_fixed2;				// OMS偏移量(不可靠比特组)
	int ms_type;					// 0:标准MS, 1:NMS, 2:OMS, 3:NMS+OMS

	double alpha_factor;
	double beta_factor;

	// 收敛速率早停参数
	double convergence_epsilon;		// 相对变化阈值，metric变化小于此比例视为停滞 - 3.14修改
	int convergence_window;			// 连续停滞次数阈值，达到则触发早停 - 3.14修改

	int damp_mode;				// 0:固定阻尼, 1:线性变化, 2:幂函数变化
	double damp_fixed;			// 固定阻尼因子
	double damp_start;			// 线性/幂函数变化起始阻尼因子
	double damp_end;			// 线性/幂函数变化结束阻尼因子
	double damp_p;				// 幂函数变化的指数
};


