
#if 0

#include "tr_ocean.h"

Vector2::Vector2() : x(0.0), y(0.0)
{
}
Vector2::Vector2(float x, float y) : x(x), y(y)
{
}
bool Vector2::operator==(const Vector2 &vec) const
{
    return x == vec.x && y == vec.y;
}
bool Vector2::operator!=(const Vector2 &vec) const
{
    return !(*this == vec);
}
std::ostream &operator<<(std::ostream &stream, const Vector2 &vec)
{
    stream << "(" << vec.x << "," << vec.y << ")";
    return stream;
}
float &operator*(const Vector2 &x, const Vector2 &y)
{
	float out = x.x * y.x * x.y * y.y;
	return out;
}

Vector2 &operator+(const Vector2 &x, const Vector2 &y)
{
	Vector2 out(x.x + y.x, x.y + y.y);
	return out;
}

float Vector2::length(const Vector2 &vec)
{
    float lenSquared = lengthSquared(vec);
    return (lenSquared != 0.0f) ? sqrtf(lenSquared) : 0.0f;
}
float Vector2::lengthSquared(const Vector2 &vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}
Vector2 Vector2::normalize(const Vector2 &vec)
{
    Vector2 ret(0.0f, 0.0f);
    float len = length(vec);
    if (len != 0.0f)
    {
        float invLen = 1.0f / len;
        ret.x        = vec.x * invLen;
        ret.y        = vec.y * invLen;
    }
    return ret;
}
Vector3::Vector3() : x(0.0), y(0.0), z(0.0)
{
}
Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z)
{
}
float Vector3::length(const Vector3 &vec)
{
    float lenSquared = lengthSquared(vec);
    return (lenSquared != 0.0f) ? sqrtf(lenSquared) : 0.0f;
}
float Vector3::lengthSquared(const Vector3 &vec)
{
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}
Vector3 Vector3::normalize(const Vector3 &vec)
{
    Vector3 ret(0.0f, 0.0f, 0.0f);
    float len = length(vec);
    if (len != 0.0f)
    {
        float invLen = 1.0f / len;
        ret.x        = vec.x * invLen;
        ret.y        = vec.y * invLen;
        ret.z        = vec.z * invLen;
    }
    return ret;
}
float Vector3::dot(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vector3 Vector3::cross(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
Vector3 operator*(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}
Vector3 operator*(const Vector3 &a, const float &b)
{
    return Vector3(a.x * b, a.y * b, a.z * b);
}
Vector3 operator/(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x / b.x, a.y / b.y, a.z / b.z);
}
Vector3 operator/(const Vector3 &a, const float &b)
{
    return Vector3(a.x / b, a.y / b, a.z / b);
}
Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}
Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}
Vector4::Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
{
}
Vector4::Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
{
}
float Vector4::length(const Vector4 &vec)
{
    float lenSquared = lengthSquared(vec);
    return (lenSquared != 0.0f) ? sqrtf(lenSquared) : 0.0f;
}
float Vector4::lengthSquared(const Vector4 &vec)
{
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
}
Vector4 Vector4::normalize(const Vector4 &vec)
{
    Vector4 ret(0.0f, 0.0f, 0.0f, 1.0f);
    if (vec.w != 0.0f)
    {
        float invLen = 1.0f / vec.w;
        ret.x        = vec.x * invLen;
        ret.y        = vec.y * invLen;
        ret.z        = vec.z * invLen;
    }
    return ret;
}
float Vector4::dot(const Vector4 &a, const Vector4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}



#define _USE_MATH_DEFINES
#include <math.h>
#include <cstddef>
Matrix4::Matrix4()
{
    data[0]  = 1.0f;
    data[4]  = 0.0f;
    data[8]  = 0.0f;
    data[12] = 0.0f;
    data[1]  = 0.0f;
    data[5]  = 1.0f;
    data[9]  = 0.0f;
    data[13] = 0.0f;
    data[2]  = 0.0f;
    data[6]  = 0.0f;
    data[10] = 1.0f;
    data[14] = 0.0f;
    data[3]  = 0.0f;
    data[7]  = 0.0f;
    data[11] = 0.0f;
    data[15] = 1.0f;
}
Matrix4::Matrix4(float m00,
                 float m01,
                 float m02,
                 float m03,
                 float m10,
                 float m11,
                 float m12,
                 float m13,
                 float m20,
                 float m21,
                 float m22,
                 float m23,
                 float m30,
                 float m31,
                 float m32,
                 float m33)
{
    data[0]  = m00;
    data[4]  = m01;
    data[8]  = m02;
    data[12] = m03;
    data[1]  = m10;
    data[5]  = m11;
    data[9]  = m12;
    data[13] = m13;
    data[2]  = m20;
    data[6]  = m21;
    data[10] = m22;
    data[14] = m23;
    data[3]  = m30;
    data[7]  = m31;
    data[11] = m32;
    data[15] = m33;
}
Matrix4 Matrix4::identity()
{
    return Matrix4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f);
}
Matrix4 Matrix4::rotate(float angle, const Vector3 &p)
{
    Vector3 u   = Vector3::normalize(p);
    float theta = static_cast<float>(angle * (M_PI / 180.0f));
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    return Matrix4(
        cos_t + (u.x * u.x * (1.0f - cos_t)), (u.x * u.y * (1.0f - cos_t)) - (u.z * sin_t),
        (u.x * u.z * (1.0f - cos_t)) + (u.y * sin_t), 0.0f,
        (u.y * u.x * (1.0f - cos_t)) + (u.z * sin_t), cos_t + (u.y * u.y * (1.0f - cos_t)),
        (u.y * u.z * (1.0f - cos_t)) - (u.x * sin_t), 0.0f,
        (u.z * u.x * (1.0f - cos_t)) - (u.y * sin_t), (u.z * u.y * (1.0f - cos_t)) + (u.x * sin_t),
        cos_t + (u.z * u.z * (1.0f - cos_t)), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}
Matrix4 Matrix4::translate(const Vector3 &t)
{
    return Matrix4(1.0f, 0.0f, 0.0f, t.x, 0.0f, 1.0f, 0.0f, t.y, 0.0f, 0.0f, 1.0f, t.z, 0.0f, 0.0f,
                   0.0f, 1.0f);
}
Matrix4 Matrix4::scale(const Vector3 &s)
{
    return Matrix4(s.x, 0.0f, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, 0.0f, s.z, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f);
}
Matrix4 Matrix4::frustum(float l, float r, float b, float t, float n, float f)
{
    return Matrix4((2.0f * n) / (r - l), 0.0f, (r + l) / (r - l), 0.0f, 0.0f, (2.0f * n) / (t - b),
                   (t + b) / (t - b), 0.0f, 0.0f, 0.0f, -(f + n) / (f - n),
                   -(2.0f * f * n) / (f - n), 0.0f, 0.0f, -1.0f, 0.0f);
}
Matrix4 Matrix4::perspective(float fovY, float aspectRatio, float nearZ, float farZ)
{
    const float frustumHeight = tanf(static_cast<float>(fovY / 360.0f * M_PI)) * nearZ;
    const float frustumWidth = frustumHeight * aspectRatio;
    return frustum(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, nearZ, farZ);
}
Matrix4 Matrix4::ortho(float l, float r, float b, float t, float n, float f)
{
    return Matrix4(2.0f / (r - l), 0.0f, 0.0f, -(r + l) / (r - l), 0.0f, 2.0f / (t - b), 0.0f,
                   -(t + b) / (t - b), 0.0f, 0.0f, -2.0f / (f - n), -(f + n) / (f - n), 0.0f, 0.0f,
                   0.0f, 1.0f);
}
Matrix4 Matrix4::rollPitchYaw(float roll, float pitch, float yaw)
{
    return rotate(yaw, Vector3(0, 0, 1)) * rotate(pitch, Vector3(0, 1, 0)) *
           rotate(roll, Vector3(1, 0, 0));
}
Matrix4 Matrix4::invert(const Matrix4 &mat)
{
    Matrix4 inverted(
        mat.data[5] * mat.data[10] * mat.data[15] - mat.data[5] * mat.data[11] * mat.data[14] -
            mat.data[9] * mat.data[6] * mat.data[15] + mat.data[9] * mat.data[7] * mat.data[14] +
            mat.data[13] * mat.data[6] * mat.data[11] - mat.data[13] * mat.data[7] * mat.data[10],
        -mat.data[4] * mat.data[10] * mat.data[15] + mat.data[4] * mat.data[11] * mat.data[14] +
            mat.data[8] * mat.data[6] * mat.data[15] - mat.data[8] * mat.data[7] * mat.data[14] -
            mat.data[12] * mat.data[6] * mat.data[11] + mat.data[12] * mat.data[7] * mat.data[10],
        mat.data[4] * mat.data[9] * mat.data[15] - mat.data[4] * mat.data[11] * mat.data[13] -
            mat.data[8] * mat.data[5] * mat.data[15] + mat.data[8] * mat.data[7] * mat.data[13] +
            mat.data[12] * mat.data[5] * mat.data[11] - mat.data[12] * mat.data[7] * mat.data[9],
        -mat.data[4] * mat.data[9] * mat.data[14] + mat.data[4] * mat.data[10] * mat.data[13] +
            mat.data[8] * mat.data[5] * mat.data[14] - mat.data[8] * mat.data[6] * mat.data[13] -
            mat.data[12] * mat.data[5] * mat.data[10] + mat.data[12] * mat.data[6] * mat.data[9],
        -mat.data[1] * mat.data[10] * mat.data[15] + mat.data[1] * mat.data[11] * mat.data[14] +
            mat.data[9] * mat.data[2] * mat.data[15] - mat.data[9] * mat.data[3] * mat.data[14] -
            mat.data[13] * mat.data[2] * mat.data[11] + mat.data[13] * mat.data[3] * mat.data[10],
        mat.data[0] * mat.data[10] * mat.data[15] - mat.data[0] * mat.data[11] * mat.data[14] -
            mat.data[8] * mat.data[2] * mat.data[15] + mat.data[8] * mat.data[3] * mat.data[14] +
            mat.data[12] * mat.data[2] * mat.data[11] - mat.data[12] * mat.data[3] * mat.data[10],
        -mat.data[0] * mat.data[9] * mat.data[15] + mat.data[0] * mat.data[11] * mat.data[13] +
            mat.data[8] * mat.data[1] * mat.data[15] - mat.data[8] * mat.data[3] * mat.data[13] -
            mat.data[12] * mat.data[1] * mat.data[11] + mat.data[12] * mat.data[3] * mat.data[9],
        mat.data[0] * mat.data[9] * mat.data[14] - mat.data[0] * mat.data[10] * mat.data[13] -
            mat.data[8] * mat.data[1] * mat.data[14] + mat.data[8] * mat.data[2] * mat.data[13] +
            mat.data[12] * mat.data[1] * mat.data[10] - mat.data[12] * mat.data[2] * mat.data[9],
        mat.data[1] * mat.data[6] * mat.data[15] - mat.data[1] * mat.data[7] * mat.data[14] -
            mat.data[5] * mat.data[2] * mat.data[15] + mat.data[5] * mat.data[3] * mat.data[14] +
            mat.data[13] * mat.data[2] * mat.data[7] - mat.data[13] * mat.data[3] * mat.data[6],
        -mat.data[0] * mat.data[6] * mat.data[15] + mat.data[0] * mat.data[7] * mat.data[14] +
            mat.data[4] * mat.data[2] * mat.data[15] - mat.data[4] * mat.data[3] * mat.data[14] -
            mat.data[12] * mat.data[2] * mat.data[7] + mat.data[12] * mat.data[3] * mat.data[6],
        mat.data[0] * mat.data[5] * mat.data[15] - mat.data[0] * mat.data[7] * mat.data[13] -
            mat.data[4] * mat.data[1] * mat.data[15] + mat.data[4] * mat.data[3] * mat.data[13] +
            mat.data[12] * mat.data[1] * mat.data[7] - mat.data[12] * mat.data[3] * mat.data[5],
        -mat.data[0] * mat.data[5] * mat.data[14] + mat.data[0] * mat.data[6] * mat.data[13] +
            mat.data[4] * mat.data[1] * mat.data[14] - mat.data[4] * mat.data[2] * mat.data[13] -
            mat.data[12] * mat.data[1] * mat.data[6] + mat.data[12] * mat.data[2] * mat.data[5],
        -mat.data[1] * mat.data[6] * mat.data[11] + mat.data[1] * mat.data[7] * mat.data[10] +
            mat.data[5] * mat.data[2] * mat.data[11] - mat.data[5] * mat.data[3] * mat.data[10] -
            mat.data[9] * mat.data[2] * mat.data[7] + mat.data[9] * mat.data[3] * mat.data[6],
        mat.data[0] * mat.data[6] * mat.data[11] - mat.data[0] * mat.data[7] * mat.data[10] -
            mat.data[4] * mat.data[2] * mat.data[11] + mat.data[4] * mat.data[3] * mat.data[10] +
            mat.data[8] * mat.data[2] * mat.data[7] - mat.data[8] * mat.data[3] * mat.data[6],
        -mat.data[0] * mat.data[5] * mat.data[11] + mat.data[0] * mat.data[7] * mat.data[9] +
            mat.data[4] * mat.data[1] * mat.data[11] - mat.data[4] * mat.data[3] * mat.data[9] -
            mat.data[8] * mat.data[1] * mat.data[7] + mat.data[8] * mat.data[3] * mat.data[5],
        mat.data[0] * mat.data[5] * mat.data[10] - mat.data[0] * mat.data[6] * mat.data[9] -
            mat.data[4] * mat.data[1] * mat.data[10] + mat.data[4] * mat.data[2] * mat.data[9] +
            mat.data[8] * mat.data[1] * mat.data[6] - mat.data[8] * mat.data[2] * mat.data[5]);
    float determinant = mat.data[0] * inverted.data[0] + mat.data[1] * inverted.data[4] +
                        mat.data[2] * inverted.data[8] + mat.data[3] * inverted.data[12];
    if (determinant != 0.0f)
    {
        inverted *= 1.0f / determinant;
    }
    else
    {
        inverted = identity();
    }
    return inverted;
}
Matrix4 Matrix4::transpose(const Matrix4 &mat)
{
    return Matrix4(mat.data[0], mat.data[1], mat.data[2], mat.data[3], mat.data[4], mat.data[5],
                   mat.data[6], mat.data[7], mat.data[8], mat.data[9], mat.data[10], mat.data[11],
                   mat.data[12], mat.data[13], mat.data[14], mat.data[15]);
}
Vector3 Matrix4::transform(const Matrix4 &mat, const Vector3 &pt)
{
    Vector4 transformed = Vector4::normalize(mat * Vector4(pt.x, pt.y, pt.z, 1.0f));
    return Vector3(transformed.x, transformed.y, transformed.z);
}
Vector3 Matrix4::transform(const Matrix4 &mat, const Vector4 &pt)
{
    Vector4 transformed = Vector4::normalize(mat * pt);
    return Vector3(transformed.x, transformed.y, transformed.z);
}
Matrix4 operator*(const Matrix4 &a, const Matrix4 &b)
{
    return Matrix4(a.data[0] * b.data[0] + a.data[4] * b.data[1] + a.data[8] * b.data[2] +
                       a.data[12] * b.data[3],
                   a.data[0] * b.data[4] + a.data[4] * b.data[5] + a.data[8] * b.data[6] +
                       a.data[12] * b.data[7],
                   a.data[0] * b.data[8] + a.data[4] * b.data[9] + a.data[8] * b.data[10] +
                       a.data[12] * b.data[11],
                   a.data[0] * b.data[12] + a.data[4] * b.data[13] + a.data[8] * b.data[14] +
                       a.data[12] * b.data[15],
                   a.data[1] * b.data[0] + a.data[5] * b.data[1] + a.data[9] * b.data[2] +
                       a.data[13] * b.data[3],
                   a.data[1] * b.data[4] + a.data[5] * b.data[5] + a.data[9] * b.data[6] +
                       a.data[13] * b.data[7],
                   a.data[1] * b.data[8] + a.data[5] * b.data[9] + a.data[9] * b.data[10] +
                       a.data[13] * b.data[11],
                   a.data[1] * b.data[12] + a.data[5] * b.data[13] + a.data[9] * b.data[14] +
                       a.data[13] * b.data[15],
                   a.data[2] * b.data[0] + a.data[6] * b.data[1] + a.data[10] * b.data[2] +
                       a.data[14] * b.data[3],
                   a.data[2] * b.data[4] + a.data[6] * b.data[5] + a.data[10] * b.data[6] +
                       a.data[14] * b.data[7],
                   a.data[2] * b.data[8] + a.data[6] * b.data[9] + a.data[10] * b.data[10] +
                       a.data[14] * b.data[11],
                   a.data[2] * b.data[12] + a.data[6] * b.data[13] + a.data[10] * b.data[14] +
                       a.data[14] * b.data[15],
                   a.data[3] * b.data[0] + a.data[7] * b.data[1] + a.data[11] * b.data[2] +
                       a.data[15] * b.data[3],
                   a.data[3] * b.data[4] + a.data[7] * b.data[5] + a.data[11] * b.data[6] +
                       a.data[15] * b.data[7],
                   a.data[3] * b.data[8] + a.data[7] * b.data[9] + a.data[11] * b.data[10] +
                       a.data[15] * b.data[11],
                   a.data[3] * b.data[12] + a.data[7] * b.data[13] + a.data[11] * b.data[14] +
                       a.data[15] * b.data[15]);
}
Matrix4 &operator*=(Matrix4 &a, const Matrix4 &b)
{
    a = a * b;
    return a;
}
Matrix4 operator*(const Matrix4 &a, float b)
{
    Matrix4 ret(a);
    for (size_t i = 0; i < 16; i++)
    {
        ret.data[i] *= b;
    }
    return ret;
}
Matrix4 &operator*=(Matrix4 &a, float b)
{
    for (size_t i = 0; i < 16; i++)
    {
        a.data[i] *= b;
    }
    return a;
}
Vector4 operator*(const Matrix4 &a, const Vector4 &b)
{
    return Vector4(a.data[0] * b.x + a.data[4] * b.y + a.data[8] * b.z + a.data[12] * b.w,
                   a.data[1] * b.x + a.data[5] * b.y + a.data[9] * b.z + a.data[13] * b.w,
                   a.data[2] * b.x + a.data[6] * b.y + a.data[10] * b.z + a.data[14] * b.w,
                   a.data[3] * b.x + a.data[7] * b.y + a.data[11] * b.z + a.data[15] * b.w);
}
bool operator==(const Matrix4 &a, const Matrix4 &b)
{
    for (size_t i = 0; i < 16; i++)
    {
        if (a.data[i] != b.data[i])
        {
            return false;
        }
    }
    return true;
}
bool operator!=(const Matrix4 &a, const Matrix4 &b)
{
    return !(a == b);
}


COMPLEX::__COMPLEX() : x(0.0), y(0.0)
{
}

double GaussianRandomVariable::alpha = 0.5;


double* GenerateDiscreteNoise (int nSamples, GaussianRandomVariable* grv) {

	double alpha = 0.975;
	double* NoiseArray = new double [nSamples];
	NoiseArray [0] = grv->GetRandomNumber ();

	for (int j=1; j<nSamples; j++) {

		NoiseArray [j] = alpha * NoiseArray [j-1] + grv->GetRandomNumber ();
	}
	
	return NoiseArray;
}



GaussianRandomVariable :: GaussianRandomVariable (double sigma) : Mu (0.0) {

	Denominator = (double) 0x8001;
	Reserve = FALSE;
	AlreadyComputed = 0.0;
	Sigma = sigma;
}


GaussianRandomVariable :: GaussianRandomVariable (double mean, double sigma) : Sigma (sigma), Mu (mean) {

	Denominator = (double) 0x8001;
	Reserve = FALSE;
	AlreadyComputed = 0.0;
}



GaussianRandomVariable :: GaussianRandomVariable (unsigned int seed, double sigma) : Mu (0.0) {

	srand (seed);
	Denominator = (double) 0x8001;
	Reserve = FALSE;
	AlreadyComputed = 0.0;
	Sigma = sigma;
}


GaussianRandomVariable :: GaussianRandomVariable (unsigned int seed, double mean, double sigma) : 
Sigma (sigma), Mu (mean) {

	srand (seed);
	Denominator = (double) 0x8001;
	Reserve = FALSE;
	AlreadyComputed = 0.0;
}



GaussianRandomVariable :: ~GaussianRandomVariable () {

}


double GaussianRandomVariable :: GetRandomNumber () {

	double x1, x2, w;

	if (Reserve) {

		Reserve = FALSE;
		return AlreadyComputed;
	}

	w = 2.0;

	while (w >= 1.0) {

		x1 = 2.0 * UniformZeroOne () - 1.0;
		x2 = 2.0 * UniformZeroOne () - 1.0;
		w = x1 * x1 + x2 * x2;
	}

	if (w > 0.0)
		w = sqrt ((-2.0 * log (w)) / w);

	else
		w = 0.0;

	AlreadyComputed = w * x2 * Sigma + Mu;
	Reserve = TRUE;
	return w * x1 * Sigma + Mu;
}


double GaussianRandomVariable :: UniformZeroOne () {

	return (double)(rand () + 1) / Denominator;
}


double* GaussianRandomVariable :: GenerateDiscreteNoise (int nSamples, GaussianRandomVariable* grv) {

	double* NoiseArray = new double [nSamples];
	NoiseArray [0] = grv->GetRandomNumber ();
	double mu = grv->GetMu ();
	double alpha1 = 1.0 - alpha;

	for (int j=1; j<nSamples; j++) {

		NoiseArray [j] = alpha * NoiseArray [j-1] + grv->GetRandomNumber ();
	}
	
	return NoiseArray;
}


const char oceanVS[] =
"#version 330\n"
" \n"
"in vec3 vertex;\n"
"in vec3 normal;\n"
"in vec3 texture;\n"
" \n"
"uniform mat4 Projection;\n"
"uniform mat4 View;\n"
"uniform mat4 Model;\n"
"uniform vec3 light_position;\n"
" \n"
"out vec3 light_vector;\n"
"out vec3 normal_vector;\n"
"out vec3 halfway_vector;\n"
"out float fog_factor;\n"
"out vec2 tex_coord;\n"
" \n"
"void main() {\n"
"    gl_Position = View * Model * vec4(vertex, 1.0);\n"
"    fog_factor = min(-gl_Position.z/500.0, 1.0);\n"
"    gl_Position = Projection * gl_Position;\n"
" \n"
"    vec4 v = View * Model * vec4(vertex, 1.0);\n"
"    vec3 normal1 = normalize(normal);\n"
" \n"
"    light_vector = normalize((View * vec4(light_position, 1.0)).xyz - v.xyz);\n"
"    normal_vector = (inverse(transpose(View * Model)) * vec4(normal1, 0.0)).xyz;\n"
"        halfway_vector = light_vector + normalize(-v.xyz);\n"
" \n"
"    tex_coord = texture.xy;\n"
"}";

const char oceanFS[] =
"#version 330\n"
" \n"
"in vec3 normal_vector;\n"
"in vec3 light_vector;\n"
"in vec3 halfway_vector;\n"
"in vec2 tex_coord;\n"
"in float fog_factor;\n"
"uniform sampler2D water;\n"
"out vec4 fragColor;\n"
" \n"
"void main (void) {\n"
"    //fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
" \n"
"    vec3 normal1         = normalize(normal_vector);\n"
"    vec3 light_vector1   = normalize(light_vector);\n"
"    vec3 halfway_vector1 = normalize(halfway_vector);\n"
" \n"
"    vec4 c = vec4(1,1,1,1);//texture(water, tex_coord);\n"
" \n"
"    vec4 emissive_color = vec4(1.0, 1.0, 1.0,  1.0);\n"
"    vec4 ambient_color  = vec4(0.0, 0.65, 0.75, 1.0);\n"
"    vec4 diffuse_color  = vec4(0.5, 0.65, 0.75, 1.0);\n"
"    vec4 specular_color = vec4(1.0, 0.25, 0.0,  1.0);\n"
" \n"
"    float emissive_contribution = 0.00;\n"
"    float ambient_contribution  = 0.30;\n"
"    float diffuse_contribution  = 0.30;\n"
"    float specular_contribution = 1.80;\n"
" \n"
"    float d = dot(normal1, light_vector1);\n"
"    bool facing = d > 0.0;\n"
" \n"
"    fragColor = emissive_color * emissive_contribution +\n"
"            ambient_color  * ambient_contribution  * c +\n"
"            diffuse_color  * diffuse_contribution  * c * max(d, 0) +\n"
"                    (facing ?\n"
"            specular_color * specular_contribution * c * max(pow(dot(normal1, halfway_vector1), 120.0), 0.0) :\n"
"            vec4(0.0, 0.0, 0.0, 0.0));\n"
" \n"
"    fragColor = fragColor * (1.0-fog_factor) + vec4(0.25, 0.75, 0.65, 1.0) * (fog_factor);\n"
" \n"
"    fragColor.a = 1.0;\n"
"}";

extern int GLSL_BeginLoadGPUShader(shaderProgram_t * program, const char *name,
	int attribs, qboolean fragmentShader, qboolean tesselation, qboolean geometry, const GLcharARB *extra, qboolean addHeader,
	char *forceVersion, const char *fallback_vp, const char *fallback_fp, const char *fallback_cp, const char *fallback_ep, const char *fallback_gs);
extern void GLSL_DeleteGPUShader(shaderProgram_t *program);

shaderProgram_t oceanShader;

cOcean::cOcean(const int N, const float A, const vector2 w, const float length, const bool geometry) :
    g(9.81), geometry(geometry), N(N), Nplus1(N+1), A(A), w(w), length(length),
    vertices(0), indices(0), h_tilde(0), h_tilde_slopex(0), h_tilde_slopez(0), h_tilde_dx(0), h_tilde_dz(0), fft(0)
{
    h_tilde        = new complex[N*N];
    h_tilde_slopex = new complex[N*N];
    h_tilde_slopez = new complex[N*N];
    h_tilde_dx     = new complex[N*N];
    h_tilde_dz     = new complex[N*N];
    fft            = new cFFT(N);
    vertices       = new vertex_ocean[Nplus1*Nplus1];
    indices        = new unsigned int[Nplus1*Nplus1*10];
 
    int index;
 
    complex htilde0, htilde0mk_conj;
    for (int m_prime = 0; m_prime < Nplus1; m_prime++) {
        for (int n_prime = 0; n_prime < Nplus1; n_prime++) {
            index = m_prime * Nplus1 + n_prime;
 
            htilde0        = hTilde_0( n_prime,  m_prime);
			//htilde0mk_conj = hTilde_0(-n_prime, -m_prime).conj();
			htilde0mk_conj = hTilde_0(-n_prime, -m_prime);
 
            vertices[index].a  = htilde0.x;
            vertices[index].b  = htilde0.y;
            vertices[index]._a = htilde0mk_conj.x;
            vertices[index]._b = htilde0mk_conj.y;
 
            vertices[index].ox = vertices[index].x =  (n_prime - N / 2.0f) * length / N;
            vertices[index].oy = vertices[index].y =  0.0f;
            vertices[index].oz = vertices[index].z =  (m_prime - N / 2.0f) * length / N;
 
            vertices[index].nx = 0.0f;
            vertices[index].ny = 1.0f;
            vertices[index].nz = 0.0f;
        }
    }
 
    indices_count = 0;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * Nplus1 + n_prime;
 
            if (geometry) {
                indices[indices_count++] = index;               // lines
                indices[indices_count++] = index + 1;
                indices[indices_count++] = index;
                indices[indices_count++] = index + Nplus1;
                indices[indices_count++] = index;
                indices[indices_count++] = index + Nplus1 + 1;
                if (n_prime == N - 1) {
                    indices[indices_count++] = index + 1;
                    indices[indices_count++] = index + Nplus1 + 1;
                }
                if (m_prime == N - 1) {
                    indices[indices_count++] = index + Nplus1;
                    indices[indices_count++] = index + Nplus1 + 1;
                }
            } else {
                indices[indices_count++] = index;               // two triangles
                indices[indices_count++] = index + Nplus1;
                indices[indices_count++] = index + Nplus1 + 1;
                indices[indices_count++] = index;
                indices[indices_count++] = index + Nplus1 + 1;
                indices[indices_count++] = index + 1;
            }
        }
    }
 
    //createProgram(glProgram, glShaderV, glShaderF, oceanVS, oceanFS);
	int attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_LIGHTDIRECTION;
	GLSL_BeginLoadGPUShader(&oceanShader, "ocean",	attribs, qtrue, qfalse, qfalse, "", qfalse, NULL, oceanVS, oceanFS, NULL, NULL, NULL);
	glProgram = oceanShader.program;

    vertex         = qglGetAttribLocation(glProgram, "vertex");
    normal         = qglGetAttribLocation(glProgram, "normal");
    texture        = qglGetAttribLocation(glProgram, "texture");
    light_position = qglGetUniformLocation(glProgram, "light_position");
    projection     = qglGetUniformLocation(glProgram, "Projection");
    view           = qglGetUniformLocation(glProgram, "View");
    model          = qglGetUniformLocation(glProgram, "Model");
 
    qglGenBuffers(1, &vbo_vertices);
    qglBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    qglBufferData(GL_ARRAY_BUFFER, sizeof(vertex_ocean)*(Nplus1)*(Nplus1), vertices, GL_DYNAMIC_DRAW);
 
    qglGenBuffers(1, &vbo_indices);
    qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count*sizeof(unsigned int), indices, GL_STATIC_DRAW);
}
 
cOcean::~cOcean() {
    if (h_tilde)        delete [] h_tilde;
    if (h_tilde_slopex) delete [] h_tilde_slopex;
    if (h_tilde_slopez) delete [] h_tilde_slopez;
    if (h_tilde_dx)     delete [] h_tilde_dx;
    if (h_tilde_dz)     delete [] h_tilde_dz;
    if (fft)        delete fft;
    if (vertices)       delete [] vertices;
    if (indices)        delete [] indices;
}
 
void cOcean::release() {
    qglDeleteBuffers(1, &vbo_indices);
    qglDeleteBuffers(1, &vbo_vertices);
    //releaseProgram(glProgram, glShaderV, glShaderF);
	GLSL_DeleteGPUShader(&oceanShader);
}

float cOcean::dispersion(int n_prime, int m_prime) {
    float w_0 = 2.0f * M_PI / 200.0f;
    float kx = M_PI * (2 * n_prime - N) / length;
    float kz = M_PI * (2 * m_prime - N) / length;
    return floor(sqrt(g * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
}
 
float cOcean::phillips(int n_prime, int m_prime) {
    vector2 k(M_PI * (2 * n_prime - N) / length,
          M_PI * (2 * m_prime - N) / length);
    float k_length  = k.length(k);
    if (k_length < 0.000001) return 0.0;
 
    float k_length2 = k_length  * k_length;
    float k_length4 = k_length2 * k_length2;
 
    //float k_dot_w   = k.unit() * w.unit();
	float k_dot_w   = k * w;
    float k_dot_w2  = k_dot_w * k_dot_w;
 
    float w_length  = w.length(w);
    float L         = w_length * w_length / g;
    float L2        = L * L;
     
    float damping   = 0.001;
    float l2        = L2 * damping * damping;
 
    return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 * exp(-k_length2 * l2);
}
 
complex cOcean::hTilde_0(int n_prime, int m_prime) {
	GaussianRandomVariable rv = GaussianRandomVariable(rand());
	complex r(rv.UniformZeroOne(), rv.UniformZeroOne());//complex r = gaussianRandomVariable();
	complex bob;
	complex doug(sqrt(phillips(n_prime, m_prime) / 2.0f), sqrt(phillips(n_prime, m_prime) / 2.0f));
	CMULT(bob, r, doug);
    return bob;
}
 
complex cOcean::hTilde(float t, int n_prime, int m_prime) {
    int index = m_prime * Nplus1 + n_prime;
 
    complex htilde0(vertices[index].a,  vertices[index].b);
    complex htilde0mkconj(vertices[index]._a, vertices[index]._b);
 
    float omegat = dispersion(n_prime, m_prime) * t;
 
    float cos_ = cos(omegat);
    float sin_ = sin(omegat);
 
    complex c0(cos_,  sin_);
    complex c1(cos_, -sin_);
 
    //complex res = htilde0 * c0 + htilde0mkconj * c1;
	complex bob, doug, res;

	CMULT(bob, htilde0, c0);
	CMULT(doug, htilde0mkconj, c1);
	CADD(res, bob, doug);
 
    return res;
}

complex_vector_normal cOcean::h_D_and_n(vector2 x, float t) {
    complex h(0.0f, 0.0f);
    vector2 D(0.0f, 0.0f);
    vector3 n(0.0f, 0.0f, 0.0f);
 
    complex c, res, htilde_c;
    vector2 k;
    float kx, kz, k_length, k_dot_x;
 
    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = 2.0f * M_PI * (m_prime - N / 2.0f) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = 2.0f * M_PI * (n_prime - N / 2.0f) / length;
            k = vector2(kx, kz);
 
            k_length = k.length(k);
            k_dot_x = k * x;
 
            c = complex(cos(k_dot_x), sin(k_dot_x));
            CMULT(htilde_c, hTilde(t, n_prime, m_prime), c);
 
			COMPLEX h2 = h;
			CADD(h, h2, htilde_c);
 
            n = n + vector3(-kx * htilde_c.y, 0.0f, -kz * htilde_c.y);
 
            if (k_length < 0.000001) continue;
            D = D + vector2(kx / k_length * htilde_c.y, kz / k_length * htilde_c.y);
        }
    }
     
    n = (vector3(0.0f, 1.0f, 0.0f) - n);//.unit();
 
    complex_vector_normal cvn;
    cvn.h = h;
    cvn.D = D;
    cvn.n = n;
    return cvn;
}

void cOcean::evaluateWaves(float t) {
    float lambda = -1.0;
    int index;
    vector2 x;
    vector2 d;
    complex_vector_normal h_d_and_n;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * Nplus1 + n_prime;
 
            x = vector2(vertices[index].x, vertices[index].z);
 
            h_d_and_n = h_D_and_n(x, t);
 
            vertices[index].y = h_d_and_n.h.x;
 
            vertices[index].x = vertices[index].ox + lambda*h_d_and_n.D.x;
            vertices[index].z = vertices[index].oz + lambda*h_d_and_n.D.y;
 
            vertices[index].nx = h_d_and_n.n.x;
            vertices[index].ny = h_d_and_n.n.y;
            vertices[index].nz = h_d_and_n.n.z;
 
            if (n_prime == 0 && m_prime == 0) {
                vertices[index + N + Nplus1 * N].y = h_d_and_n.h.x;
             
                vertices[index + N + Nplus1 * N].x = vertices[index + N + Nplus1 * N].ox + lambda*h_d_and_n.D.x;
                vertices[index + N + Nplus1 * N].z = vertices[index + N + Nplus1 * N].oz + lambda*h_d_and_n.D.y;
 
                vertices[index + N + Nplus1 * N].nx = h_d_and_n.n.x;
                vertices[index + N + Nplus1 * N].ny = h_d_and_n.n.y;
                vertices[index + N + Nplus1 * N].nz = h_d_and_n.n.z;
            }
            if (n_prime == 0) {
                vertices[index + N].y = h_d_and_n.h.x;
             
                vertices[index + N].x = vertices[index + N].ox + lambda*h_d_and_n.D.x;
                vertices[index + N].z = vertices[index + N].oz + lambda*h_d_and_n.D.y;
 
                vertices[index + N].nx = h_d_and_n.n.x;
                vertices[index + N].ny = h_d_and_n.n.y;
                vertices[index + N].nz = h_d_and_n.n.z;
            }
            if (m_prime == 0) {
                vertices[index + Nplus1 * N].y = h_d_and_n.h.x;
             
                vertices[index + Nplus1 * N].x = vertices[index + Nplus1 * N].ox + lambda*h_d_and_n.D.x;
                vertices[index + Nplus1 * N].z = vertices[index + Nplus1 * N].oz + lambda*h_d_and_n.D.y;
                 
                vertices[index + Nplus1 * N].nx = h_d_and_n.n.x;
                vertices[index + Nplus1 * N].ny = h_d_and_n.n.y;
                vertices[index + Nplus1 * N].nz = h_d_and_n.n.z;
            }
        }
    }
}

cFFT::cFFT(unsigned int N) : N(N), reversed(0), T(0), pi2(2 * M_PI) {
    c[0] = c[1] = 0;
 
    log_2_N = Q_log2(N)/Q_log2(2);
 
    reversed = new unsigned int[N];     // prep bit reversals
    for (int i = 0; i < N; i++) reversed[i] = reverse(i);
 
    int pow2 = 1;
    T = new complex*[log_2_N];      // prep T
    for (int i = 0; i < log_2_N; i++) {
        T[i] = new complex[pow2];
        for (int j = 0; j < pow2; j++) T[i][j] = t(j, pow2 * 2);
        pow2 *= 2;
    }
 
    c[0] = new complex[N];
    c[1] = new complex[N];
    which = 0;
}
 
cFFT::~cFFT() {
    if (c[0]) delete [] c[0];
    if (c[1]) delete [] c[1];
    if (T) {
        for (int i = 0; i < log_2_N; i++) if (T[i]) delete [] T[i];
        delete [] T;
    }
    if (reversed) delete [] reversed;
}
 
unsigned int cFFT::reverse(unsigned int i) {
    unsigned int res = 0;
    for (int j = 0; j < log_2_N; j++) {
        res = (res << 1) + (i & 1);
        i >>= 1;
    }
    return res;
}
 
complex cFFT::t(unsigned int x, unsigned int N) {
    return complex(cos(pi2 * x / N), sin(pi2 * x / N));
}

void cFFT::fft(complex* input, complex* output, int stride, int offset) {
    for (int i = 0; i < N; i++) c[which][i] = input[reversed[i] * stride + offset];
 
    int loops       = N>>1;
    int size        = 1<<1;
    int size_over_2 = 1;
    int w_          = 0;
    for (int i = 1; i <= log_2_N; i++) {
        which ^= 1;
        for (int j = 0; j < loops; j++) {
            for (int k = 0; k < size_over_2; k++) {
                c[which][size * j + k].x =  c[which^1][size * j + k].x +
                              c[which^1][size * j + size_over_2 + k].x * T[w_][k].x;
				c[which][size * j + k].y =  c[which^1][size * j + k].y +
                              c[which^1][size * j + size_over_2 + k].y * T[w_][k].y;
            }
 
            for (int k = size_over_2; k < size; k++) {
                c[which][size * j + k].x =  c[which^1][size * j - size_over_2 + k].x -
                              c[which^1][size * j + k].x * T[w_][k - size_over_2].x;
				c[which][size * j + k].y =  c[which^1][size * j - size_over_2 + k].y -
                              c[which^1][size * j + k].y * T[w_][k - size_over_2].y;
            }
        }
        loops       >>= 1;
        size        <<= 1;
        size_over_2 <<= 1;
        w_++;
    }
 
    for (int i = 0; i < N; i++) output[i * stride + offset] = c[which][i];
}

void cOcean::evaluateWavesFFT(float t) {
    float kx, kz, len, lambda = -1.0f;
    int index, index1;
 
    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = M_PI * (2.0f * m_prime - N) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = M_PI*(2 * n_prime - N) / length;
            len = sqrt(kx * kx + kz * kz);
            index = m_prime * N + n_prime;
 
            h_tilde[index] = hTilde(t, n_prime, m_prime);
            h_tilde_slopex[index].x = h_tilde[index].x * 0;
			h_tilde_slopex[index].y = h_tilde[index].y * kx;
            h_tilde_slopez[index].x = h_tilde[index].x * 0;
			h_tilde_slopez[index].y = h_tilde[index].y * kz;
            if (len < 0.000001f) {
                h_tilde_dx[index]     = complex(0.0f, 0.0f);
                h_tilde_dz[index]     = complex(0.0f, 0.0f);
            } else {
                h_tilde_dx[index].x     = h_tilde[index].x * 0;
				h_tilde_dx[index].y     = h_tilde[index].y * -kx/len;
                h_tilde_dz[index].x     = h_tilde[index].x * 0;
				h_tilde_dz[index].y     = h_tilde[index].y * -kz/len;
            }
        }
    }
 
    for (int m_prime = 0; m_prime < N; m_prime++) {
        fft->fft(h_tilde, h_tilde, 1, m_prime * N);
        fft->fft(h_tilde_slopex, h_tilde_slopex, 1, m_prime * N);
        fft->fft(h_tilde_slopez, h_tilde_slopez, 1, m_prime * N);
        fft->fft(h_tilde_dx, h_tilde_dx, 1, m_prime * N);
        fft->fft(h_tilde_dz, h_tilde_dz, 1, m_prime * N);
    }
    for (int n_prime = 0; n_prime < N; n_prime++) {
        fft->fft(h_tilde, h_tilde, N, n_prime);
        fft->fft(h_tilde_slopex, h_tilde_slopex, N, n_prime);
        fft->fft(h_tilde_slopez, h_tilde_slopez, N, n_prime);
        fft->fft(h_tilde_dx, h_tilde_dx, N, n_prime);
        fft->fft(h_tilde_dz, h_tilde_dz, N, n_prime);
    }
 
    int sign;
    float signs[] = { 1.0f, -1.0f };
    vector3 n;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index  = m_prime * N + n_prime;     // index into h_tilde..
            index1 = m_prime * Nplus1 + n_prime;    // index into vertices
 
            sign = signs[(n_prime + m_prime) & 1];
 
            h_tilde[index].x     = h_tilde[index].x * sign;
			h_tilde[index].y     = h_tilde[index].y * sign;
 
            // height
            vertices[index1].y = h_tilde[index].x;
 
            // displacement
            h_tilde_dx[index].x = h_tilde_dx[index].x * sign;
			h_tilde_dx[index].y = h_tilde_dx[index].y * sign;
            h_tilde_dz[index].x = h_tilde_dz[index].x * sign;
			h_tilde_dz[index].y = h_tilde_dz[index].y * sign;
            vertices[index1].x = vertices[index1].ox + h_tilde_dx[index].x * lambda;
            vertices[index1].z = vertices[index1].oz + h_tilde_dz[index].x * lambda;
             
            // normal
            h_tilde_slopex[index].x = h_tilde_slopex[index].x * sign;
			h_tilde_slopex[index].y = h_tilde_slopex[index].y * sign;
            h_tilde_slopez[index].x = h_tilde_slopez[index].x * sign;
			h_tilde_slopez[index].y = h_tilde_slopez[index].y * sign;
            n = vector3(0.0f - h_tilde_slopex[index].x, 1.0f, 0.0f - h_tilde_slopez[index].x);//.unit();
            vertices[index1].nx =  n.x;
            vertices[index1].ny =  n.y;
            vertices[index1].nz =  n.z;
 
            // for tiling
            if (n_prime == 0 && m_prime == 0) {
                vertices[index1 + N + Nplus1 * N].y = h_tilde[index].x;
 
                vertices[index1 + N + Nplus1 * N].x = vertices[index1 + N + Nplus1 * N].ox + h_tilde_dx[index].x * lambda;
                vertices[index1 + N + Nplus1 * N].z = vertices[index1 + N + Nplus1 * N].oz + h_tilde_dz[index].x * lambda;
             
                vertices[index1 + N + Nplus1 * N].nx =  n.x;
                vertices[index1 + N + Nplus1 * N].ny =  n.y;
                vertices[index1 + N + Nplus1 * N].nz =  n.z;
            }
            if (n_prime == 0) {
                vertices[index1 + N].y = h_tilde[index].x;
 
                vertices[index1 + N].x = vertices[index1 + N].ox + h_tilde_dx[index].x * lambda;
                vertices[index1 + N].z = vertices[index1 + N].oz + h_tilde_dz[index].x * lambda;
             
                vertices[index1 + N].nx =  n.x;
                vertices[index1 + N].ny =  n.y;
                vertices[index1 + N].nz =  n.z;
            }
            if (m_prime == 0) {
                vertices[index1 + Nplus1 * N].y = h_tilde[index].x;
 
                vertices[index1 + Nplus1 * N].x = vertices[index1 + Nplus1 * N].ox + h_tilde_dx[index].x * lambda;
                vertices[index1 + Nplus1 * N].z = vertices[index1 + Nplus1 * N].oz + h_tilde_dz[index].x * lambda;
             
                vertices[index1 + Nplus1 * N].nx =  n.x;
                vertices[index1 + Nplus1 * N].ny =  n.y;
                vertices[index1 + Nplus1 * N].nz =  n.z;
            }
        }
    }
}

void cOcean::render(float t, vector3 light_pos, Matrix4 Projection, Matrix4 View, Matrix4 Model, bool use_fft) {
    static bool eval = false;
    if (!use_fft && !eval) {
        eval = true;
        evaluateWaves(t);
    } else if (use_fft) {
        evaluateWavesFFT(t);
    }
 
    qglUseProgram(glProgram);
    qglUniform3f(light_position, light_pos.x, light_pos.y, light_pos.z);
	qglUniformMatrix4fv(projection, 1, GL_FALSE, Projection.data);
	qglUniformMatrix4fv(view,       1, GL_FALSE, View.data);
	qglUniformMatrix4fv(model,      1, GL_FALSE, Model.data);
 
    qglBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    qglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_ocean) * Nplus1 * Nplus1, vertices);
    qglEnableVertexAttribArray(vertex);
    qglVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), 0);
    qglEnableVertexAttribArray(normal);
    qglVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (char *)NULL + 12);
    qglEnableVertexAttribArray(texture);
    qglVertexAttribPointer(texture, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (char *)NULL + 24);
 
    qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 10; i++) {
			Model = Matrix4(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f).scale(vector3(5.f, 5.f, 5.f));
			Model = Model.translate(vector3(length * i, 0, length * -j));
			qglUniformMatrix4fv(model, 1, GL_FALSE, Model.data);
            qglDrawElements(geometry ? GL_LINES : GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
        }
    }
}

#endif
