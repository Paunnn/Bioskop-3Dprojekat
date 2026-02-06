// Minimal GLM-compatible math library for 3D graphics
#pragma once

#include <cmath>
#include <algorithm>

namespace glm {

// Forward declarations
struct vec2;
struct vec3;
struct vec4;
struct mat4;

// ============ vec2 ============
struct vec2 {
    float x, y;

    vec2() : x(0), y(0) {}
    vec2(float s) : x(s), y(s) {}
    vec2(float x, float y) : x(x), y(y) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator*(float s) const { return vec2(x * s, y * s); }
    vec2 operator/(float s) const { return vec2(x / s, y / s); }
    vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
};

// ============ vec3 ============
struct vec3 {
    float x, y, z;

    vec3() : x(0), y(0), z(0) {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec4& v);

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }
    vec3 operator-() const { return vec3(-x, -y, -z); }
    vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};

inline vec3 operator*(float s, const vec3& v) { return v * s; }

// ============ vec4 ============
struct vec4 {
    float x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator-(const vec4& v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
    vec4 operator/(float s) const { return vec4(x / s, y / s, z / s, w / s); }
};

inline vec3::vec3(const vec4& v) : x(v.x), y(v.y), z(v.z) {}

// ============ mat3 ============
struct mat3 {
    vec3 cols[3];

    mat3() {
        cols[0] = vec3(1, 0, 0);
        cols[1] = vec3(0, 1, 0);
        cols[2] = vec3(0, 0, 1);
    }

    mat3(const mat4& m);

    vec3& operator[](int i) { return cols[i]; }
    const vec3& operator[](int i) const { return cols[i]; }

    vec3 operator*(const vec3& v) const {
        return vec3(
            cols[0].x * v.x + cols[1].x * v.y + cols[2].x * v.z,
            cols[0].y * v.x + cols[1].y * v.y + cols[2].y * v.z,
            cols[0].z * v.x + cols[1].z * v.y + cols[2].z * v.z
        );
    }
};

// ============ mat4 ============
struct mat4 {
    vec4 cols[4];

    mat4() {
        cols[0] = vec4(1, 0, 0, 0);
        cols[1] = vec4(0, 1, 0, 0);
        cols[2] = vec4(0, 0, 1, 0);
        cols[3] = vec4(0, 0, 0, 1);
    }

    mat4(float s) {
        cols[0] = vec4(s, 0, 0, 0);
        cols[1] = vec4(0, s, 0, 0);
        cols[2] = vec4(0, 0, s, 0);
        cols[3] = vec4(0, 0, 0, s);
    }

    vec4& operator[](int i) { return cols[i]; }
    const vec4& operator[](int i) const { return cols[i]; }

    mat4 operator*(const mat4& m) const {
        mat4 result;
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                result[c][r] =
                    cols[0][r] * m[c][0] +
                    cols[1][r] * m[c][1] +
                    cols[2][r] * m[c][2] +
                    cols[3][r] * m[c][3];
            }
        }
        return result;
    }

    vec4 operator*(const vec4& v) const {
        return vec4(
            cols[0].x * v.x + cols[1].x * v.y + cols[2].x * v.z + cols[3].x * v.w,
            cols[0].y * v.x + cols[1].y * v.y + cols[2].y * v.z + cols[3].y * v.w,
            cols[0].z * v.x + cols[1].z * v.y + cols[2].z * v.z + cols[3].z * v.w,
            cols[0].w * v.x + cols[1].w * v.y + cols[2].w * v.z + cols[3].w * v.w
        );
    }
};

inline mat3::mat3(const mat4& m) {
    cols[0] = vec3(m[0].x, m[0].y, m[0].z);
    cols[1] = vec3(m[1].x, m[1].y, m[1].z);
    cols[2] = vec3(m[2].x, m[2].y, m[2].z);
}

// ============ Functions ============

inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float dot(const vec4& a, const vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float length(const vec3& v) {
    return sqrtf(dot(v, v));
}

inline vec3 normalize(const vec3& v) {
    float len = length(v);
    if (len > 0.0f) return v / len;
    return vec3(0);
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline vec3 reflect(const vec3& I, const vec3& N) {
    return I - N * 2.0f * dot(N, I);
}

inline float radians(float degrees) {
    return degrees * 3.14159265358979323846f / 180.0f;
}

inline float clamp(float x, float minVal, float maxVal) {
    return std::min(std::max(x, minVal), maxVal);
}

inline mat4 transpose(const mat4& m) {
    mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] = m[j][i];
        }
    }
    return result;
}

inline mat4 inverse(const mat4& m) {
    float coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    float coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
    float coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
    float coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
    float coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
    float coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
    float coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
    float coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
    float coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
    float coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
    float coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
    float coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
    float coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

    vec4 fac0(coef00, coef00, coef02, coef03);
    vec4 fac1(coef04, coef04, coef06, coef07);
    vec4 fac2(coef08, coef08, coef10, coef11);
    vec4 fac3(coef12, coef12, coef14, coef15);
    vec4 fac4(coef16, coef16, coef18, coef19);
    vec4 fac5(coef20, coef20, coef22, coef23);

    vec4 vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
    vec4 vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
    vec4 vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
    vec4 vec3_v(m[1][3], m[0][3], m[0][3], m[0][3]);

    vec4 inv0(vec1.x*fac0.x - vec2.x*fac1.x + vec3_v.x*fac2.x,
              vec1.y*fac0.y - vec2.y*fac1.y + vec3_v.y*fac2.y,
              vec1.z*fac0.z - vec2.z*fac1.z + vec3_v.z*fac2.z,
              vec1.w*fac0.w - vec2.w*fac1.w + vec3_v.w*fac2.w);
    vec4 inv1(vec0.x*fac0.x - vec2.x*fac3.x + vec3_v.x*fac4.x,
              vec0.y*fac0.y - vec2.y*fac3.y + vec3_v.y*fac4.y,
              vec0.z*fac0.z - vec2.z*fac3.z + vec3_v.z*fac4.z,
              vec0.w*fac0.w - vec2.w*fac3.w + vec3_v.w*fac4.w);
    vec4 inv2(vec0.x*fac1.x - vec1.x*fac3.x + vec3_v.x*fac5.x,
              vec0.y*fac1.y - vec1.y*fac3.y + vec3_v.y*fac5.y,
              vec0.z*fac1.z - vec1.z*fac3.z + vec3_v.z*fac5.z,
              vec0.w*fac1.w - vec1.w*fac3.w + vec3_v.w*fac5.w);
    vec4 inv3(vec0.x*fac2.x - vec1.x*fac4.x + vec2.x*fac5.x,
              vec0.y*fac2.y - vec1.y*fac4.y + vec2.y*fac5.y,
              vec0.z*fac2.z - vec1.z*fac4.z + vec2.z*fac5.z,
              vec0.w*fac2.w - vec1.w*fac4.w + vec2.w*fac5.w);

    vec4 signA(+1, -1, +1, -1);
    vec4 signB(-1, +1, -1, +1);

    mat4 inv;
    inv[0] = vec4(inv0.x * signA.x, inv0.y * signB.x, inv0.z * signA.x, inv0.w * signB.x);
    inv[1] = vec4(inv1.x * signA.y, inv1.y * signB.y, inv1.z * signA.y, inv1.w * signB.y);
    inv[2] = vec4(inv2.x * signA.z, inv2.y * signB.z, inv2.z * signA.z, inv2.w * signB.z);
    inv[3] = vec4(inv3.x * signA.w, inv3.y * signB.w, inv3.z * signA.w, inv3.w * signB.w);

    vec4 row0(inv[0][0], inv[1][0], inv[2][0], inv[3][0]);
    float det = dot(m[0], row0);

    if (fabsf(det) < 0.00001f) return mat4(1.0f);

    float oneOverDet = 1.0f / det;
    for (int i = 0; i < 4; i++) {
        inv[i] = inv[i] * oneOverDet;
    }
    return inv;
}

inline mat4 translate(const mat4& m, const vec3& v) {
    mat4 result = m;
    result[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
    return result;
}

inline mat4 scale(const mat4& m, const vec3& v) {
    mat4 result;
    result[0] = m[0] * v.x;
    result[1] = m[1] * v.y;
    result[2] = m[2] * v.z;
    result[3] = m[3];
    return result;
}

inline mat4 rotate(const mat4& m, float angle, const vec3& v) {
    float c = cosf(angle);
    float s = sinf(angle);
    vec3 axis = normalize(v);
    vec3 temp = axis * (1.0f - c);

    mat4 rot;
    rot[0][0] = c + temp.x * axis.x;
    rot[0][1] = temp.x * axis.y + s * axis.z;
    rot[0][2] = temp.x * axis.z - s * axis.y;
    rot[1][0] = temp.y * axis.x - s * axis.z;
    rot[1][1] = c + temp.y * axis.y;
    rot[1][2] = temp.y * axis.z + s * axis.x;
    rot[2][0] = temp.z * axis.x + s * axis.y;
    rot[2][1] = temp.z * axis.y - s * axis.x;
    rot[2][2] = c + temp.z * axis.z;

    mat4 result;
    result[0] = m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2];
    result[1] = m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2];
    result[2] = m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2];
    result[3] = m[3];
    return result;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 result(1.0f);
    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    return result;
}

inline mat4 perspective(float fovy, float aspect, float zNear, float zFar) {
    float tanHalfFovy = tanf(fovy / 2.0f);

    mat4 result;
    result[0] = vec4(0);
    result[1] = vec4(0);
    result[2] = vec4(0);
    result[3] = vec4(0);

    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / tanHalfFovy;
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0f;
    result[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return result;
}

inline mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    mat4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -2.0f / (zFar - zNear);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return result;
}

// Value ptr for passing to OpenGL
inline float* value_ptr(mat4& m) {
    return &m[0][0];
}

inline float* value_ptr(vec3& v) {
    return &v[0];
}

inline float* value_ptr(vec4& v) {
    return &v[0];
}

inline const float* value_ptr(const mat4& m) {
    return &m[0][0];
}

inline const float* value_ptr(const vec3& v) {
    return &v[0];
}

inline const float* value_ptr(const vec4& v) {
    return &v[0];
}

} // namespace glm
