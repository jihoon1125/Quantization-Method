# Quantization Method
* 다양한 양자화 기법을 통해 이미지 압축 및 복원을 해보며 각 기법의 장단점을 분석한다.
* PSNR(10 * log(1024^2 / MSE)) 값을 기준으로 복원률 계산
* Uniquantization, Lloyd-Max quantization, Vector quantization 기법들을 사용한다. 


---

## Uniquantization

* Step size를 4로 설정한 후 각 Decision level들을 균일하게 설정하였다. 

* 픽셀 범위 0 ~ 256 -> 0 ~ 64, 2bit 이득.

* Reconstruction level은 각 step의 시작 값으로 임의로 설정해주었다.(어느 값을 level로 설정하든 합리적인 근거가 없기 때문)

*  Uniquantization으로 효과를 얻기 위해선 이미지의 분포가 균일해야 할 것.

* 역양자화 시, Reconstruction level에 4만 곱해주면 쉽게 복원할 수 있다.

* Result & Analysis

![image](https://user-images.githubusercontent.com/67624104/118350132-19efdc00-b590-11eb-862d-91c1aa1daa74.png)



* 평균적으로 각 step마다 양자화된 픽셀값이 실제 픽셀값과 약 1.5정도 차이가 난다는 것을 알 수 있다.

---

## Lloyd-Max quantization

* 균일 양자화와 다르게 픽셀 분포에 유동적인 decision level 설정이 가능한 non-uniform 양자화 기법

* 구간 초기화시 확률 밀도가 0인 구간들을 다른 구간에 합쳐주는 과정 필요

* Next Reconstruction level = 어떤 구간의 픽셀값 평균 / 구간 픽셀분포값

* Next Decision level = 현재와 이전 Reconstruction level 값의 절반 (임의로 선택한 것)

* 위의 두 식을 기준으로 계속 양자화 에러율을 계산하면서 반복적으로 iterate 하며 에러가 최소가 되는 타이밍의 reconstruction level값들을 최종 값들로 선택

* 각 level들을 64개의 값들로 encoding하는 과정 필요(codebook.txt)

* Decoding 시에는 codebook.txt를 참조하여 픽셀값 복원

* Result & Analysis


![image](https://user-images.githubusercontent.com/67624104/118350645-e498bd80-b592-11eb-90a9-098499690442.png)

* 이미지 분포에 상관없이 균일 양자화에 비해 성능이 매우 향상되었다.

* 계속 소수점 부분을 버려가면서 iterate하면 오차가 누적되어 비정상적인 step들이 나올 수 있기 때문에 주의해야한다.


---


## Vector quantization

* Sample들을 벡터화 한 non-uniform 양자화 기법(similar as K-NN algorithm)

* 8bit만큼의 벡터 개수 할당, 각 벡터의 사이즈는 2x2

* 즉 이미지의 데이터의 크기를 1/4로 줄이는 것을 목표로 함

* 각 input vector들은 자신과 평균 거리가 가장 가까운 reconstruction vector를 선택하고 reconstruction vector는 이에 맞춰 업데이트 하는 과정을 반복

* 압축률이 높을수록 복원이 깔끔하지 않은 것은 당연하고, 다른 양자화 기법들에 비해 알고리즘이 무거워짐

* 단순 PSNR로 성능이 안좋다고 평가하기에는 무리가 있음(높은 압축률 고려 필요)

* Result

![image](https://user-images.githubusercontent.com/67624104/118351285-09426480-b596-11eb-81e2-b1273d420c1f.png)





