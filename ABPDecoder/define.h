#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <random>
#include <iostream>
#include <unordered_set>
#include <omp.h>
using namespace std;
#include "Struct.h"

using namespace std;

#define MAXVALUE 1E20
#define MINVALUE 1E-20
#define PI        3.1415926535897932384626433832795


void Initial(struct ADPStruct *ADP, struct SPStruct *SP);
void InitialAWGN(struct AWGN *awgn);
void InitialIter(int M, int N, int **H, struct IterStruct *Iter);
void MallocIter(int M, int N, struct IterStruct *Iter);
void OSD_GE_H(int **Hi, int **Ho, int M, int N, int *K,
	const int *ReliabilityOrder, int *ReliabilityOrderGE, int *InterGE);
void Permute(int *seq, int length);
int GaussElimation_GF2(int **Hi, int **Ho, int M, int N, int *K, int *pos);
void CRC_ENC(int* sourceseq, int* codeseq, int CRCLEN, int SRCLEN);
int CRC_DEC(int* codeseq, int CRCLEN, int CODELEN);
void CRC_H_initial(int **H, int CRCLEN, int k);
void Encode(const int * u, int * c, int K, int N, int **G);
void AWGNChannel(double *receiveseq, int *codeseq, struct AWGN *awgn, int length);
void SoftDemodulate(double *bitsoft, double *receiveseq, double factor, int length);
void Decode(double snr, double *bitsoft, double *y, int *result, struct ADPStruct *ADP);
int CheckCode(const int *codeseq, int M, struct IterStruct *Iter);
void Simulation(struct SPStruct *SP, struct ADPStruct *ADP, struct AWGN *awgn, struct StatisStruct *Statis);
void FindOptimal(const double* LLR, int* codeSeq, int length, double* minSED, int* result);
void SortLLR(const double* bitsoft, int N, int* ReliabilityOrder);
void Calculate__Inverse_Matrix(int** T, int** T_1, int N);
void Joint_H_Initial(struct ADPStruct* ADP);
void Recover_Info(const int* codeword, int* info, struct ADPStruct* ADP);
void getPunctureIndex(struct ADPStruct* ADP);
void proposedPuncture(struct ADPStruct* ADP);
void getShortenIndex(struct ADPStruct* ADP);
int** generate_polar_matrix(int n);
void saveMatrixToFile(int** G, int rows, int cols, const char* filename);
void saveArrayToFile(int* A, int len, const char* filename);
void printMatrix(int** G, int rows, int cols);
void saveArrayDoubleToFile(double* A, int len, const char* filename);