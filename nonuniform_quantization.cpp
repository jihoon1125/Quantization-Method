#pragma warning(disable : 4996)

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
#include<cstdio>
using namespace std;


#define PIXEL_NUM  960*540 //���� �ȼ� ��
#define M_ICH 3//ä�� ��

vector<vector<USHORT>> m_ui16Out(3, vector<USHORT>(PIXEL_NUM));//reconstruction ��� ������
vector<vector<USHORT>> m_ui16Comp(3, vector<USHORT>(PIXEL_NUM));//���Ϸκ��� �о�� ������ 16-bit ����
vector<vector<UCHAR>> m_ui8Out(3, vector<UCHAR>(PIXEL_NUM));//quantization ���
vector<vector<UCHAR>> m_ui8Comp(3, vector<UCHAR>(PIXEL_NUM* 2));//���Ϸκ��� �о�� ������ 8-bit����
int m_size[3] = { PIXEL_NUM , PIXEL_NUM / 4 , PIXEL_NUM / 4 };//�� ä�κ� �ȼ� ��

void readOneFrame(FILE* file, int m_ibit)//�ϳ��� �������� �о���� �Լ�
{
	int bitfactor = (m_ibit <= 8) ? 1 : 2;		//10-bit�� 2, 8-bit�� 1

	for (int ch = 0; ch < M_ICH; ch++)
	{
		fread(&m_ui8Comp[ch][0], sizeof(UCHAR), m_size[ch] * bitfactor, file);// 10-bit�� 8-bit�� ���� �ι� �о����

		if (m_ibit == 10) {
			for (int i = 0; i < m_size[ch]; i++)
				m_ui16Comp[ch][i] = (m_ui8Comp[ch][i * 2] + (m_ui8Comp[ch][i * 2 + 1] << 8));//10-bit�� �ϳ��� �����ͷ� ����
		}
	}
}

void Get_Histogram( double (*histogram)[1024]) {//histogram[3][1024]
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < 1024; i++)
			histogram[ch][i] = 0;
	}

	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
			histogram[ch][m_ui16Comp[ch][i]]++;

		for (int i = 0; i < 1024; i++)//������� ��ȯ
			histogram[ch][i] /= m_size[ch];
	}
}

void nonuni_quantization(double histogram[][1024], double threshold)//histogram[3][1024]
{
	double dec_level[3][257];//decision level(0~256)
	double rc_level[3][257];//reconstruction level(1~256)
	bool quantize_more[3] = { true, true, true };
	double s_avg = 0, p_avg = 0;
	double quantize_error[3] = { 0 };
	double prev_quantize_error[3] = { 0 };	
	bool quantize_complete = false;
	
	int dec_num[3];	
	
	/* decision level �ʱ�ȭ */
	for (int ch = 0; ch < M_ICH; ch++) {
		dec_num[ch] = 0;
		dec_level[ch][0] = 0; 
		for (int i = 0; i < 256; i++)
		{
			double interval_per = 0;
			for (int j = 0; j < 4; j++)
			{
				interval_per += histogram[ch][4 * i + j];//�� ������ histogram�� 0 �ƴ��� Ȯ��
			}
			if (interval_per > 0) {//���� 0 ���� ũ�� �������� ����
				dec_num[ch]++;//level ���� ����
				dec_level[ch][dec_num[ch]] = 4*(i + 1);//������ ���� �� ����				
			}
		}		
	}

	while (1) {
		/* reconstrucion level ���� */
		for (int ch = 0; ch < M_ICH; ch++) {
			if (quantize_more[ch] == false)
				continue;

			for (int i = 0; i < dec_num[ch]; i++) {
				s_avg = 0; p_avg = 0;
				int start_quantize;

				if (dec_level[ch][i] > (int)dec_level[ch][i]) start_quantize = (int)dec_level[ch][i] + 1;//�� level�� ������ ����, �ø�����
				else start_quantize = (int)dec_level[ch][i];

				for (int j = start_quantize; j < dec_level[ch][i + 1]; j++) {//rc_level�� ���
					s_avg += j * histogram[ch][j];
					p_avg += histogram[ch][j];
				}
				rc_level[ch][i + 1] = s_avg / p_avg;
			}
		}

		/* quantize error ��� �� threshold�� �� */
		for (int ch = 0; ch < M_ICH; ch++) {
			quantize_error[ch] = 0;
			for (int i = 0; i < dec_num[ch]; i++) {
				int start_quantize;

				if (dec_level[ch][i] > (int)dec_level[ch][i]) start_quantize = (int)dec_level[ch][i] + 1;
				else start_quantize = (int)dec_level[ch][i];				

				for (int j = start_quantize; j < dec_level[ch][i + 1]; j++)
					quantize_error[ch] += ((j - rc_level[ch][i + 1]) * (j - rc_level[ch][i + 1])) * histogram[ch][j];
			}
		}		

		for (int ch = 0; ch < M_ICH; ch++) {//������ ������ �� ������ ��� �����ϰ� �ƴϸ� �� ä���� ����ȭ�� ����
			if ((int)(quantize_error[ch] * 10000000) == (int)(prev_quantize_error[ch] * 10000000) && prev_quantize_error[ch] != 0)
				quantize_more[ch] = false;
		}				

		for (int ch = 0; ch < M_ICH; ch++)
		{
			if (quantize_more[ch] == true) {
				quantize_complete = false; break;
			}
			else			
				quantize_complete = true;			
		}

		if (quantize_complete == true) break;//�� ä�� ��� ����ȭ �������� break

		/*decision level ����*/
		for (int ch = 0; ch < M_ICH; ch++) {			
			for (int i = 1; i < dec_num[ch]; i++)
				dec_level[ch][i] = (rc_level[ch][i] + rc_level[ch][i + 1]) / 2;
		}

		/* quantize_error ��� */
		for (int ch = 0; ch < M_ICH; ch++)
			prev_quantize_error[ch] = quantize_error[ch];
		
	}

	/* m_ui8Out�� �� ��� */
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
		{
			for (int j = 0; j < dec_num[ch]; j++)
			{
				if (m_ui16Comp[ch][i] > dec_level[ch][j + 1])//���� ã��
					continue;
				else {
					m_ui8Out[ch][i] = j + 1;
					break;
				}
			}
		}
	}

	/* codebook.txt�� reconstruction level ���� */
	FILE* cb;
	cb = fopen("codebook.txt", "wt");
	if (!cb) cout << "codebook.txt ���� ����!" << endl;
	
	for (int ch = 0; ch < M_ICH; ch++)
	{
		fprintf(cb, "%d\n", dec_num[ch]);
		for (int i = 1; i <= dec_num[ch]; i++)
		{
			fprintf(cb, "%d\n", (int)(rc_level[ch][i]+0.5));
		}
	}

	fclose(cb);
}

void inverse_nonuniquant(void) {
	FILE* cb;
	int dec_num[3];
	int rc_level[3][257];
	cb = fopen("codebook.txt", "rt");//codebook.txt �о����

	for (int ch = 0; ch < M_ICH; ch++)//codebook���� reconstruction level�� ���ʷ� �о����
	{
		fscanf(cb, "%d", &dec_num[ch]);
		for(int i=1; i<=dec_num[ch]; i++)
			fscanf(cb, "%d", &rc_level[ch][i]);
	}
		
	fclose(cb);
	
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
		{
			m_ui16Out[ch][i] = rc_level[ch][m_ui8Comp[ch][i]];//reconstruction ����
		}
			
	}
}

void PRINT_PSNR(void)//PSNR ����Լ�
{
	int MAX = 1023;
    double MSE;

	for (int ch = 0; ch < M_ICH; ch++)
	{
		MSE = 0;
		for (int i = 0; i < m_size[ch]; i++)//�� ä�κ��� MSE���ϰ� PSNR ���
			MSE += (m_ui16Comp[ch][i] - m_ui16Out[ch][i]) * (m_ui16Comp[ch][i] - m_ui16Out[ch][i]);
		MSE /= m_size[ch];

		if (ch == 0)
			cout << "Y�� PSNR : ";
		else if (ch == 1)
			cout << "Cb�� PSNR : ";
		else
			cout << "Cr�� PSNR : ";

		cout << 10 * log10(MAX * MAX / (double)MSE) << endl;

	}
	cout << endl;
}
int main(void)
{
	FILE* fp_InputImg;
	FILE* fp_outputImg;
	double histogram[3][1024] = { 0 };
	const char* input_name[5] = { "./input/RitualDance_960x540_10bit_420_frame100.yuv",
								  "./input/RitualDance_960x540_10bit_420_frame200.yuv",
								  "./input/RitualDance_960x540_10bit_420_frame250.yuv",
								  "./input/RitualDance_960x540_10bit_420_frame300.yuv",
								  "./input/RitualDance_960x540_10bit_420_frame350.yuv" };
	const char* output_name[5] = { "./output/RitualDance_960x540_8bit_420_frame100.yuv",
								  "./output/RitualDance_960x540_8bit_420_frame200.yuv",
								  "./output/RitualDance_960x540_8bit_420_frame250.yuv",
								  "./output/RitualDance_960x540_8bit_420_frame300.yuv",
								  "./output/RitualDance_960x540_8bit_420_frame350.yuv" };	
	for (int i = 0; i < 5; i++) {
		fp_InputImg = fopen(input_name[i], "rb");
		if (!fp_InputImg) {
			printf("Can not open file.");
		}
		readOneFrame(fp_InputImg, 10);//10-bit �̹��� ������ ����
		Get_Histogram(histogram);

		fclose(fp_InputImg);

		nonuni_quantization(histogram, 0.0);	// 10bit -> 8bit ����ȭ	

		fp_outputImg = fopen(output_name[i], "wb"); // ��� �����ϱ�
		for (int ch = 0; ch < M_ICH; ch++) {
			fwrite(&m_ui8Out[ch][0], sizeof(UCHAR), m_size[ch], fp_outputImg);
		}

		fclose(fp_outputImg);

		fp_InputImg = fopen(output_name[i], "rb");
		readOneFrame(fp_InputImg, 8);//8-bit �̹��� ������ ����
		fclose(fp_InputImg);

		inverse_nonuniquant();//���� ����ȭ ����
		

		cout << output_name[i] << "�� PSNR ���" << endl;
		PRINT_PSNR();//PSNR ���		
	}
	
	return 0;
}