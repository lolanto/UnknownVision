/** 文档说明
 * 该hlsl头文件用于Shader文件中解析来自GeneralCamera的缓冲区，务必保证GeneralCamera的缓冲区与下面的
 * GeneralCameraDataStructure一致!!
 * 使用时尽量使用提供的辅助函数，减少对structure本身的依赖
 */

#ifndef GENERAL_CAMERA_DATA_STRUCTURE_HLSL_INC
#define GENERAL_CAMERA_DATA_STRUCTURE_HLSL_INC

struct GeneralCameraDataStructure {
    float4 position;
    float4x4 viewMat;
    float4x4 projMat;
};

float3 GetCameraPosition(GeneralCameraDataStructure input) {
    return input.position.xyz;
}

float4x4 GetViewMatrix(GeneralCameraDataStructure input) {
    return input.viewMat;
}

float4x4 GetProjectionMatrix(GeneralCameraDataStructure input) {
    return input.projMat;
}

#endif // GENERAL_CAMERA_DATA_STRUCTURE_HLSL_INC
