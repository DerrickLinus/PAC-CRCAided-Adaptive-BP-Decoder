#include "define.h"
//(P*(T^-1)φ(H1^T))^T
void Joint_H_Initial(struct ADPStruct* ADP) {
	int** H_CRC = new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		H_CRC[i] = new int[ADP->CRC_len];
		memset(H_CRC[i], 0, sizeof(int) * ADP->CRC_len);
	}
	int** H_CRC_T = new int* [ADP->CRC_len];
	for (int i = 0; i < ADP->CRC_len; i++) {
		H_CRC_T[i] = new int[ADP->N];
	}
	int** H_crc_add0_T = new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		H_crc_add0_T[i] = new int[ADP->CRC_len];
	}
	for (int i = 0; i < ADP->N; i++) {
		for (int j = 0; j < ADP->CRC_len; j++) {
			H_crc_add0_T[i][j] = ADP->H_crc_add0[j][i];
		}
	}
	int** temp_matrix = new int* [ADP->N];
	for (int i = 0; i < ADP->N; i++) {
		temp_matrix[i] = new int[ADP->N];
		memset(temp_matrix[i], 0, sizeof(int) * ADP->N);
	}
	for (int i = 0; i < ADP->N; i++) {
		for (int j = 0; j < ADP->N; j++) {
			for (int k = 0; k < ADP->N; k++) {
				temp_matrix[i][j]^= (ADP->G[i][k] && ADP->PAC_code->T_1[k][j]);
			}
		}
	}
	for (int i = 0; i < ADP->N; i++) {
		for (int j = 0; j < ADP->CRC_len; j++) {
			for (int k = 0; k < ADP->N; k++) {
				H_CRC[i][j] ^= (temp_matrix[i][k] && H_crc_add0_T[k][j]);
			}
		}
	}
	//Transpose 
	for (int i = 0; i < ADP->CRC_len; i++) {
		for (int j = 0; j < ADP->N; j++) {
			H_CRC_T[i][j] = H_CRC[j][i];
		}
	}
	//Joint check matrix
	for (int i = 0; i < ADP->M; i++) {
		for (int j = 0; j < ADP->N; j++) {
			ADP->Joint_check_matrix[i][j] = ADP->PAC_code->H[i][j];
		}
	}
	for (int i = ADP->M; i < ADP->M + ADP->CRC_len; i++) {
		for (int j = 0; j < ADP->N; j++) {
			ADP->Joint_check_matrix[i][j] = H_CRC_T[i-ADP->M][j];
		}
	}
	for (int i = 0; i < ADP->N; i++)
		delete[] H_CRC[i];
	delete[] H_CRC;
	for (int i = 0; i < ADP->CRC_len; i++)
		delete[] H_CRC_T[i];
	delete[] H_CRC_T;
	for (int i = 0; i < ADP->N; i++)
		delete[] temp_matrix[i];
	delete[] temp_matrix;
	for (int i = 0; i < ADP->N; i++)
		delete[] H_crc_add0_T[i];
	delete[] H_crc_add0_T;
}
