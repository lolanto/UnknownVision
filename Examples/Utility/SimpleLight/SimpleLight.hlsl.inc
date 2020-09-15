/** 文档说明：
 * 该hlsl头文件必须与SimpleLight模块一起使用，该头文件描述了光源数据的数据结构，该结构与c++代码必须完整对应
 * 头文件中提供访问不同类型的光源的辅助方法，请尽可能使用辅助方法访问光源属性 */
#ifndef SIMPLE_LIGHT_HLSL_INC
#define SIMPLE_LIGHT_HLSL_INC
struct LightDataBufferStructure {
    float4 position_flag1;
    float4 radiance_power;
    float4 direction_flag2;
};

/** 访问点光源相关属性的辅助函数 */
inline float3 PointLight_Position(LightDataBufferStructure input) { return input.position_flag1.xyz; }
inline float3 PointLight_RadianceMulPower(LightDataBufferStructure input) { return input.radiance_power.xyz * input.radiance_power.w; }
inline float3 PointLight_Radiance(LightDataBufferStructure input) { return input.radiance_power.xyz; }
inline float3 PointLight_Power(LightDataBufferStructure input) { return input.radiance_power.w; }

/** 访问平行光相关属性的辅助函数 */
inline float3 DirectionLight_Position(LightDataBufferStructure input) { return input.position_flag1.xyz; }
inline float3 DirectionLight_RadianceMulPower(LightDataBufferStructure input) { return input.radiance_power.xyz * input.radiance_power.w; }
inline float3 DirectionLight_Radiance(LightDataBufferStructure input) { return input.radiance_power.xyz; }
inline float3 DirectionLight_Power(LightDataBufferStructure input) { return input.radiance_power.w; }
inline float3 DirectionLight_Direction(LightDataBufferStructure input) { return input.direction_flag2.xyz; }

#endif // SIMPLE_LIGHT_HLSL_INC
