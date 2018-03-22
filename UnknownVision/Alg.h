#pragma once
//#include <memory>
class DXRenderer;
class MainClass;


void CubeMapGen(DXRenderer*, MainClass*);

void DeepGBuffer(DXRenderer*, MainClass*);

void ScreenSpaceReflection(DXRenderer*, MainClass*);

/*
0. ����Ⱦ������
1. ׼����ͼ����Ϊ������ľ��εı任����M
2. ��Ⱦʣ�ೡ���������㴫�ݵ�ps��
3. PS�н���ǰ����λ��ͨ��Mת����������ռ���
4. ���㷴�������ĵ�����ķ�������D
5. ��D��Ϸ����泤�����UV�����������
*/
void ImageBasedLighting(DXRenderer*, MainClass*);

void BruteForce(DXRenderer*, MainClass*);

void MyAlg(DXRenderer*, MainClass*);