#pragma warning(disable : 4996)

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
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

void uni_quantization(void)//���� ����ȭ
{
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
		 m_ui8Out[ch][i] = m_ui16Comp[ch][i] >> 2;//�ܼ� 1/4��
	}
}

void inverse_uniquantization(void)//�� ����ȭ
{
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
			m_ui16Out[ch][i] = m_ui8Comp[ch][i] << 2;//�ܼ� 4��
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

		fclose(fp_InputImg);

		uni_quantization();	// 10bit -> 8bit ����ȭ	

		fp_outputImg = fopen(output_name[i], "wb"); // ��� �����ϱ�
		for (int ch = 0; ch < M_ICH; ch++) {
			fwrite(&m_ui8Out[ch][0], sizeof(UCHAR), m_size[ch], fp_outputImg);
		}

		fclose(fp_outputImg);

		fp_InputImg = fopen(output_name[i], "rb");
		readOneFrame(fp_InputImg, 8);//8-bit �̹��� ������ ����
		fclose(fp_InputImg);

		inverse_uniquantization();//���� ����ȭ ����

		cout << output_name[i] << "�� PSNR ���" << endl;
		PRINT_PSNR();//PSNR ���		
	}
	
	return 0;
}