#pragma warning(disable : 4996)

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
using namespace std;


#define PIXEL_NUM  960*540 //고정 픽셀 수
#define M_ICH 3//채널 수

vector<vector<USHORT>> m_ui16Out(3, vector<USHORT>(PIXEL_NUM));//reconstruction 결과 데이터
vector<vector<USHORT>> m_ui16Comp(3, vector<USHORT>(PIXEL_NUM));//파일로부터 읽어온 데이터 16-bit 버전
vector<vector<UCHAR>> m_ui8Out(3, vector<UCHAR>(PIXEL_NUM));//quantization 결과
vector<vector<UCHAR>> m_ui8Comp(3, vector<UCHAR>(PIXEL_NUM* 2));//파일로부터 읽어온 데이터 8-bit버전
int m_size[3] = { PIXEL_NUM , PIXEL_NUM / 4 , PIXEL_NUM / 4 };//각 채널별 픽셀 수

void readOneFrame(FILE* file, int m_ibit)//하나의 프레임을 읽어오는 함수
{
	int bitfactor = (m_ibit <= 8) ? 1 : 2;		//10-bit면 2, 8-bit면 1

	for (int ch = 0; ch < M_ICH; ch++)
	{
		fread(&m_ui8Comp[ch][0], sizeof(UCHAR), m_size[ch] * bitfactor, file);// 10-bit면 8-bit씩 각각 두번 읽어야함

		if (m_ibit == 10) {
			for (int i = 0; i < m_size[ch]; i++)
				m_ui16Comp[ch][i] = (m_ui8Comp[ch][i * 2] + (m_ui8Comp[ch][i * 2 + 1] << 8));//10-bit의 하나의 데이터로 정제
		}
	}
}

void uni_quantization(void)//균일 양자화
{
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
		 m_ui8Out[ch][i] = m_ui16Comp[ch][i] >> 2;//단순 1/4배
	}
}

void inverse_uniquantization(void)//역 양자화
{
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
			m_ui16Out[ch][i] = m_ui8Comp[ch][i] << 2;//단순 4배
	}
}

void PRINT_PSNR(void)//PSNR 출력함수
{
	int MAX = 1023;
    double MSE;

	for (int ch = 0; ch < M_ICH; ch++)
	{
		MSE = 0;
		for (int i = 0; i < m_size[ch]; i++)//각 채널별로 MSE구하고 PSNR 출력
			MSE += (m_ui16Comp[ch][i] - m_ui16Out[ch][i]) * (m_ui16Comp[ch][i] - m_ui16Out[ch][i]);
		MSE /= m_size[ch];

		if (ch == 0)
			cout << "Y의 PSNR : ";
		else if (ch == 1)
			cout << "Cb의 PSNR : ";
		else
			cout << "Cr의 PSNR : ";

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
		readOneFrame(fp_InputImg, 10);//10-bit 이미지 데이터 저장

		fclose(fp_InputImg);

		uni_quantization();	// 10bit -> 8bit 양자화	

		fp_outputImg = fopen(output_name[i], "wb"); // 결과 저장하기
		for (int ch = 0; ch < M_ICH; ch++) {
			fwrite(&m_ui8Out[ch][0], sizeof(UCHAR), m_size[ch], fp_outputImg);
		}

		fclose(fp_outputImg);

		fp_InputImg = fopen(output_name[i], "rb");
		readOneFrame(fp_InputImg, 8);//8-bit 이미지 데이터 저장
		fclose(fp_InputImg);

		inverse_uniquantization();//균일 양자화 실행

		cout << output_name[i] << "의 PSNR 출력" << endl;
		PRINT_PSNR();//PSNR 출력		
	}
	
	return 0;
}