/** 该文件仅对一般数学类进行封装，为的是隐藏其背后真正的数学库细节 */
#pragma once

namespace IMath {
    template<typename T>
    struct IVEC2 {
        union {
            struct { T x, y; };
            T data[2];
        };
    };

    template<typename T>
    struct IVEC3 {
        union {
            struct { T x, y, z; };
            struct { T r, g, b; };
            T data[3];
        };
    };

    template<typename T>
    struct IVEC4 {
        union
        {
            struct { T x, y, z, w; };
            struct { T r, g, b, a; };
            T data[4];
        };
    };

    template<typename T>
    struct IVEC4X4 {
        union {
            struct {
                T _00, _01, _02, _03;
                T _10, _11, _12, _13;
                T _20, _21, _22, _23;
                T _30, _31, _32, _33;
            };
            T data[16];
        };
    };

    using IFLOAT2 = IVEC2<float>;
    using IFLOAT3 = IVEC3<float>;
    using IFLOAT4 = IVEC4<float>;
    using IFLOAT4X4 = IVEC4X4<float>;

    using IDOUBLE2 = IVEC2<double>;
};
