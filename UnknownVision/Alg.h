#pragma once
//#include <memory>
class DXRenderer;
class MainClass;


void CubeMapGen(DXRenderer*, MainClass*);

void DeepGBuffer(DXRenderer*, MainClass*);

void ScreenSpaceReflection(DXRenderer*, MainClass*);

/*
0. 先渲染反射面
1. 准备贴图，作为反射面的矩形的变换矩阵M
2. 渲染剩余场景，将顶点传递到ps中
3. PS中将当前顶点位置通过M转换到反射面空间中
4. 计算反射面中心到顶点的方向向量D
5. 将D结合反射面长宽换算成UV计算出反射结果
*/
void ImageBasedLighting(DXRenderer*, MainClass*);

void BruteForce(DXRenderer*, MainClass*);

void MyAlg(DXRenderer*, MainClass*);