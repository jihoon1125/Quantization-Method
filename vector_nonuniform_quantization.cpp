#pragma warning(disable : 4996)

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
#include<cstdio>
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

/* Inverse vector 양자화 */
void inverse_vector_nonuniquant(void) {
	FILE* cb;
	
	int rc_level[3][256][4];
	cb = fopen("codebook.txt", "rt");//codebook.txt 읽어오기

	for (int ch = 0; ch < M_ICH; ch++)//codebook에서 reconstruction level들 차례로 읽어오기
	{		
		for(int i=0; i<256; i++)
			fscanf(cb, "%d %d %d %d", &rc_level[ch][i][0], &rc_level[ch][i][1], &rc_level[ch][i][2], &rc_level[ch][i][3]);
	}
		
	fclose(cb);
	
	/* 읽어온 프레임들을 2x2 벡터로 만들어주는 과정이 필요하다 */
	for (int ch = 0; ch < M_ICH; ch++)
	{
		if (ch == 0) {

			for (int height = 0; height < 270; height++)
			{
				for (int width = 0; width < 480; width++)
				{
					m_ui16Out[ch][height * 1920 + 2 * width] = rc_level[ch][m_ui8Comp[ch][480 * height + width]][0];
					m_ui16Out[ch][height * 1920 + 2 * width + 1] = rc_level[ch][m_ui8Comp[ch][480 * height + width]][1];
					m_ui16Out[ch][(height + 0.5) * 1920 + 2 * width] = rc_level[ch][m_ui8Comp[ch][480 * height + width]][2];
					m_ui16Out[ch][(height + 0.5) * 1920 + 2 * width + 1] = rc_level[ch][m_ui8Comp[ch][480 * height + width]][3];					
				}
			}
		}
		else {
			for (int height = 0; height < 135; height++)
			{
				for (int width = 0; width < 240; width++)
				{
					m_ui16Out[ch][height * 960 + 2 * width] = rc_level[ch][m_ui8Comp[ch][240 * height + width]][0];
					m_ui16Out[ch][height * 960 + 2 * width + 1] = rc_level[ch][m_ui8Comp[ch][240 * height + width]][1];
					m_ui16Out[ch][(height + 0.5) * 960 + 2 * width] = rc_level[ch][m_ui8Comp[ch][240 * height + width]][2];
					m_ui16Out[ch][(height + 0.5) * 960 + 2 * width + 1] = rc_level[ch][m_ui8Comp[ch][240 * height + width]][3];
				}
			}

		}
			
	}
}

/* 벡터 양자화*/
void vector_nonuni_quantization(void)
{	
	double rc_vector[3][256][4];//reconstruction level(0~255)
	int **input_vector[3];//input_vector 
	double* input_vector_avg[3];//intput vector's average
	double rc_vector_avg[3][256];	
	int* quantized_vector[3];	
	
	int c = 0;
	/* input vector 메모리 할당 */
	for (int ch = 0; ch < 3; ch++)
	{
		input_vector[ch] = new int* [m_size[ch] / 4];
		quantized_vector[ch] = new int[m_size[ch] / 4];		
		input_vector_avg[ch] = new double[m_size[ch] / 4];

		for (int i = 0; i < m_size[ch] / 4; i++)
			input_vector[ch][i] = new int[4];
	}

	/* input vector 2x2 벡터로 초기화 */
	for (int ch = 0; ch < 3; ch++)
	{
		if (ch == 0) {

			for (int height = 0; height < 270; height++)
			{
				for (int width = 0; width < 480; width++)
				{
					input_vector[ch][height * 480 + width][0] = m_ui16Comp[ch][height * 1920 + 2 * width];
					input_vector[ch][height * 480 + width][1] = m_ui16Comp[ch][height * 1920 + 2 * width + 1];
					input_vector[ch][height * 480 + width][2] = m_ui16Comp[ch][(height + 0.5) * 1920 + 2 * width];
					input_vector[ch][height * 480 + width][3] = m_ui16Comp[ch][(height + 0.5) * 1920 + 2 * width + 1];
				}
			}
		}
		else {
			for (int height = 0; height < 135; height++)
			{
				for (int width = 0; width < 240; width++)
				{
					input_vector[ch][height * 240 + width][0] = m_ui16Comp[ch][height * 960 + 2 * width];
					input_vector[ch][height * 240 + width][1] = m_ui16Comp[ch][height * 960 + 2 * width + 1];
					input_vector[ch][height * 240 + width][2] = m_ui16Comp[ch][(height + 0.5) * 960 + 2 * width];
					input_vector[ch][height * 240 + width][3] = m_ui16Comp[ch][(height + 0.5) * 960 + 2 * width + 1];
				}
			}

		}
	}
	/* rc_vector, input_vector_avg 초기화 */
	for (int ch = 0; ch < M_ICH; ch++) {

		for (int i = 0; i < 256; i++)
		{
			rc_vector_avg[ch][i] = 2+ 4*i;

			for (int j = 0; j < 4; j++)
			{
				rc_vector[ch][i][j] = 4 * i + j;//각 벡터들의 내부 값 초기화 
			}
		}

		for (int j = 0; j < m_size[ch] / 4; j++)
			input_vector_avg[ch][j] = (input_vector[ch][j][0] + input_vector[ch][j][1] + input_vector[ch][j][2] + input_vector[ch][j][3]) / 4.0;
	}

	while (c<=3) {//세번 이상돌리게 되면 시간이 너무 오래 걸린다.

		/* vector 양자화 */
		for (int ch = 0; ch < M_ICH; ch++)
		{
			for (int i = 0; i < m_size[ch] / 4; i++)
			{
				double min_error = 1023 * 1023;

				for (int j = 0; j < 256; j++)
				{					
					if (pow(input_vector_avg[ch][i] - rc_vector_avg[ch][j], 2) < min_error) {
						min_error = pow(input_vector_avg[ch][i] - rc_vector_avg[ch][j], 2);
						quantized_vector[ch][i] = j;
					}
				}
			}
		}			

		/* rc_vector 갱신 */
		for (int ch = 0; ch < M_ICH; ch++) {			
			
			for (int i = 0; i < 256; i++)
			{
				int count = 0;
				double newrc_vector[4] = { 0 };
				for (int j = 0; j < m_size[ch] / 4; j++)
				{
					if (quantized_vector[ch][j] == i) {
						for (int k = 0; k < 4; k++)
							newrc_vector[k] += input_vector[ch][j][k];
						count++;
					}
				}
				if (count == 0) continue;

				for (int k = 0; k < 4; k++)
					rc_vector[ch][i][k] = newrc_vector[k] / count;//K-mean 알고리즘
			}
		}

		/* rc_vector의 평균값 갱신 */
		for (int ch = 0; ch < M_ICH; ch++) {
			for (int i = 0; i < 256; i++)			
				rc_vector_avg[ch][i] = (rc_vector[ch][i][0] + rc_vector[ch][i][1] + rc_vector[ch][i][2] + rc_vector[ch][i][3]) / 4.0;			
		}
		
		c++;
		
	}
	/* m_ui8Out에 값 담기 */
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch] / 4; i++)
			m_ui8Out[ch][i] = quantized_vector[ch][i];			
	}

	/* codebook.txt에 reconstruction level 저장 */
	FILE* cb;
	cb = fopen("codebook.txt", "wt");
	if (!cb) cout << "codebook.txt 열기 실패!" << endl;

	for (int ch = 0; ch < M_ICH; ch++)
	{		
		for (int i = 0; i < 256; i++)
		{
			fprintf(cb, "%d %d %d %d\n", (int)(rc_vector[ch][i][0]+0.5), (int)(rc_vector[ch][i][1]+0.5), (int)(rc_vector[ch][i][2]+0.5), (int)(rc_vector[ch][i][3]+0.5));
		}
	}

	fclose(cb);
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
		readOneFrame(fp_InputImg, 10);//10-bit 이미지 데이터 저장
		fclose(fp_InputImg);

		vector_nonuni_quantization();	// 10bit -> 8bit 양자화	

		fp_outputImg = fopen(output_name[i], "wb"); // 결과 저장하기
		for (int ch = 0; ch < M_ICH; ch++) {
			fwrite(&m_ui8Out[ch][0], sizeof(UCHAR), m_size[ch], fp_outputImg);
		}

		fclose(fp_outputImg);

		fp_InputImg = fopen(output_name[i], "rb");
		readOneFrame(fp_InputImg, 8);//8-bit 이미지 데이터 저장
		fclose(fp_InputImg);

		inverse_vector_nonuniquant();//균일 양자화 실행		

		cout << output_name[i] << "의 PSNR 출력" << endl;
		PRINT_PSNR();//PSNR 출력		
	}
	
	return 0;
}