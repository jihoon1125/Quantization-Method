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

		for (int i = 0; i < 1024; i++)//백분율로 변환
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
	
	/* decision level 초기화 */
	for (int ch = 0; ch < M_ICH; ch++) {
		dec_num[ch] = 0;
		dec_level[ch][0] = 0; 
		for (int i = 0; i < 256; i++)
		{
			double interval_per = 0;
			for (int j = 0; j < 4; j++)
			{
				interval_per += histogram[ch][4 * i + j];//각 구간에 histogram값 0 아닌지 확인
			}
			if (interval_per > 0) {//만약 0 보다 크면 구간으로 인정
				dec_num[ch]++;//level 개수 증가
				dec_level[ch][dec_num[ch]] = 4*(i + 1);//누적된 구간 다 삽입				
			}
		}		
	}

	while (1) {
		/* reconstrucion level 갱신 */
		for (int ch = 0; ch < M_ICH; ch++) {
			if (quantize_more[ch] == false)
				continue;

			for (int i = 0; i < dec_num[ch]; i++) {
				s_avg = 0; p_avg = 0;
				int start_quantize;

				if (dec_level[ch][i] > (int)dec_level[ch][i]) start_quantize = (int)dec_level[ch][i] + 1;//각 level의 시작점 정의, 올림연산
				else start_quantize = (int)dec_level[ch][i];

				for (int j = start_quantize; j < dec_level[ch][i + 1]; j++) {//rc_level값 계산
					s_avg += j * histogram[ch][j];
					p_avg += histogram[ch][j];
				}
				rc_level[ch][i + 1] = s_avg / p_avg;
			}
		}

		/* quantize error 계산 및 threshold와 비교 */
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

		for (int ch = 0; ch < M_ICH; ch++) {//이전의 에러가 더 높으면 계속 진행하고 아니면 그 채널의 양자화는 중지
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

		if (quantize_complete == true) break;//세 채널 모두 양자화 끝났으면 break

		/*decision level 갱신*/
		for (int ch = 0; ch < M_ICH; ch++) {			
			for (int i = 1; i < dec_num[ch]; i++)
				dec_level[ch][i] = (rc_level[ch][i] + rc_level[ch][i + 1]) / 2;
		}

		/* quantize_error 기록 */
		for (int ch = 0; ch < M_ICH; ch++)
			prev_quantize_error[ch] = quantize_error[ch];
		
	}

	/* m_ui8Out에 값 담기 */
	for (int ch = 0; ch < M_ICH; ch++)
	{
		for (int i = 0; i < m_size[ch]; i++)
		{
			for (int j = 0; j < dec_num[ch]; j++)
			{
				if (m_ui16Comp[ch][i] > dec_level[ch][j + 1])//구간 찾기
					continue;
				else {
					m_ui8Out[ch][i] = j + 1;
					break;
				}
			}
		}
	}

	/* codebook.txt에 reconstruction level 저장 */
	FILE* cb;
	cb = fopen("codebook.txt", "wt");
	if (!cb) cout << "codebook.txt 열기 실패!" << endl;
	
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
	cb = fopen("codebook.txt", "rt");//codebook.txt 읽어오기

	for (int ch = 0; ch < M_ICH; ch++)//codebook에서 reconstruction level들 차례로 읽어오기
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
			m_ui16Out[ch][i] = rc_level[ch][m_ui8Comp[ch][i]];//reconstruction 수행
		}
			
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
		Get_Histogram(histogram);

		fclose(fp_InputImg);

		nonuni_quantization(histogram, 0.0);	// 10bit -> 8bit 양자화	

		fp_outputImg = fopen(output_name[i], "wb"); // 결과 저장하기
		for (int ch = 0; ch < M_ICH; ch++) {
			fwrite(&m_ui8Out[ch][0], sizeof(UCHAR), m_size[ch], fp_outputImg);
		}

		fclose(fp_outputImg);

		fp_InputImg = fopen(output_name[i], "rb");
		readOneFrame(fp_InputImg, 8);//8-bit 이미지 데이터 저장
		fclose(fp_InputImg);

		inverse_nonuniquant();//균일 양자화 실행
		

		cout << output_name[i] << "의 PSNR 출력" << endl;
		PRINT_PSNR();//PSNR 출력		
	}
	
	return 0;
}