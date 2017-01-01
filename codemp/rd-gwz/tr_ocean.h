#if 0

#include "tr_local.h"
#include <ostream>

#ifndef UTIL_VECTOR_H
#define UTIL_VECTOR_H

struct Vector2
{
    Vector2();
    Vector2(float x, float y);
    bool operator==(const Vector2 &vec) const;
    bool operator!=(const Vector2 &vec) const;
    static float length(const Vector2 &vec);
    static float lengthSquared(const Vector2 &vec);
    static Vector2 normalize(const Vector2 &vec);
    float *data() { return &x; }
    const float *data() const { return &x; }
    float x, y;
};
std::ostream &operator<<(std::ostream &stream, const Vector2 &vec);
float &operator*(const Vector2 &x, const Vector2 &y);
Vector2 &operator+(const Vector2 &x, const Vector2 &y);
struct Vector3
{
    Vector3();
    Vector3(float x, float y, float z);
    static float length(const Vector3 &vec);
    static float lengthSquared(const Vector3 &vec);
    static Vector3 normalize(const Vector3 &vec);
    static float dot(const Vector3 &a, const Vector3 &b);
    static Vector3 cross(const Vector3 &a, const Vector3 &b);
    float *data() { return &x; }
    const float *data() const { return &x; }
    float x, y, z;
};
Vector3 operator*(const Vector3 &a, const Vector3 &b);
Vector3 operator*(const Vector3 &a, const float &b);
Vector3 operator/(const Vector3 &a, const Vector3 &b);
Vector3 operator/(const Vector3 &a, const float &b);
Vector3 operator+(const Vector3 &a, const Vector3 &b);
Vector3 operator-(const Vector3 &a, const Vector3 &b);
struct Vector4
{
    Vector4();
    Vector4(float x, float y, float z, float w);
    static float length(const Vector4 &vec);
    static float lengthSquared(const Vector4 &vec);
    static Vector4 normalize(const Vector4 &vec);
    static float dot(const Vector4 &a, const Vector4 &b);
    float *data() { return &x; }
    const float *data() const { return &x; }
    float x, y, z, w;
};
#endif  // UTIL_VECTOR_H

#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

struct Matrix4
{
    float data[16];
    Matrix4();
    Matrix4(float m00,
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
            float m33);
    static Matrix4 identity();
    static Matrix4 rotate(float angle, const Vector3 &p);
    static Matrix4 translate(const Vector3 &t);
    static Matrix4 scale(const Vector3 &s);
    static Matrix4 frustum(float l, float r, float b, float t, float n, float f);
    static Matrix4 perspective(float fov, float aspectRatio, float n, float f);
    static Matrix4 ortho(float l, float r, float b, float t, float n, float f);
    static Matrix4 rollPitchYaw(float roll, float pitch, float yaw);
    static Matrix4 invert(const Matrix4 &mat);
    static Matrix4 transpose(const Matrix4 &mat);
    static Vector3 transform(const Matrix4 &mat, const Vector3 &pt);
    static Vector3 transform(const Matrix4 &mat, const Vector4 &pt);
};
Matrix4 operator*(const Matrix4 &a, const Matrix4 &b);
Matrix4 &operator*=(Matrix4 &a, const Matrix4 &b);
Matrix4 operator*(const Matrix4 &a, float b);
Matrix4 &operator*=(Matrix4 &a, float b);
Vector4 operator*(const Matrix4 &a, const Vector4 &b);
bool operator==(const Matrix4 &a, const Matrix4 &b);
bool operator!=(const Matrix4 &a, const Matrix4 &b);
#endif  // UTIL_MATRIX_H

#ifndef _COMPLEX_H
#define _COMPLEX_H

/* COMPLEX.H header file		
 * use for complex arithmetic in C 
 (part of them are from "C Tools for Scientists and Engineers" by L. Baker)
*/
extern double cos(double);
extern double sin(double);
struct __COMPLEX
{ 
	__COMPLEX();
	__COMPLEX(double x, double y) { x = x; y = y; };
	double x,y; 
};

typedef struct __COMPLEX COMPLEX;

/* for below, X, Y are complex structures, and one is returned */

/*//real part of the complex multiplication */
#define CMULTR(X,Y) ((X).x*(Y).x-(X).y*(Y).y)
/*//image part of the complex multiplication */
#define CMULTI(X,Y) ((X).y*(Y).x+(X).x*(Y).y)
/*// used in the Division : real part of the division */
#define CDRN(X,Y) ((X).x*(Y).x+(Y).y*(X).y)
/*// used in the Division : image part of the division */
#define CDIN(X,Y) ((X).y*(Y).x-(X).x*(Y).y)
/*// used in the Division : denumerator of the division */
#define CNORM(X) ((X).x*(X).x+(X).y*(X).y)
/*//real part of the complex */
#define CREAL(X) ((double)((X).x))
/*//conjunction value */
#define CONJG(z,X) {(z).x=(X).x;(z).y= -(X).y;}
/*//conjunction value */
#define CONJ(X) {(X).y= -(X).y;}
/*//muliply : z could not be same variable as X or Y, same rule for other Macro */
#define CMULT(z,X,Y) {(z).x=CMULTR((X),(Y));(z).y=CMULTI((X),(Y));}
/*//division */
#define CDIV(z,X,Y){double d=CNORM(Y); (z).x=CDRN(X,Y)/d; (z).y=CDIN(X,Y)/d;}
/*//addition */
#define CADD(z,X,Y) {(z).x=(X).x+(Y).x;(z).y=(X).y+(Y).y;}
/*//subtraction */
#define CSUB(z,X,Y) {(z).x=(X).x-(Y).x;(z).y=(X).y-(Y).y;}
/*//assign */
#define CLET(to,from) {(to).x=(from).x;(to).y=(from).y;}
/*//abstract value(magnitude) */
#define cabs(X) sqrt((X).y*(X).y+(X).x*(X).x)
/*//real to complex */
#define CMPLX(X,real,imag) {(X).x=(real);(X).y=(imag);}
/*//multiply with real */
#define CTREAL(z,X,real) {(z).x=(X).x*(real);(z).y=(X).y*(real);}

#define CEXP(z,phase) {(z).x = cos(phase); (z).y = sin(phase); }
/* implementation using function : for compatibility */
/*COMPLEX compdiv(COMPLEX ne,COMPLEX de);
COMPLEX compexp(double theta);
COMPLEX compmult(double scalar,COMPLEX compnum);
COMPLEX compprod(COMPLEX compnum1, COMPLEX compnum2);
COMPLEX comp2sum(COMPLEX summand1, COMPLEX summand2);
COMPLEX comp3sum(COMPLEX summand1, COMPLEX summand2, COMPLEX summand3);
COMPLEX compsubtract(COMPLEX complexA, COMPLEX complexB);
double  REAL(COMPLEX compnum);*/

#endif



#ifndef _GAUSSIANRANDOMVARIABLE_H_
#define _GAUSSIANRANDOMVARIABLE_H_


class GaussianRandomVariable;

double* GenerateDiscreteNoise (int nSamples, GaussianRandomVariable* grv);


class GaussianRandomVariable {

public:
	GaussianRandomVariable (double sigma);
	GaussianRandomVariable (double mean, double sigma);
	GaussianRandomVariable (unsigned int seed, double sigma);
	GaussianRandomVariable (unsigned int seed, double mean, double sigma);
	~GaussianRandomVariable ();

	double GetRandomNumber ();
	double UniformZeroOne ();

	double GetMu () const { return Mu; }

	static void SetAlpha (double newAlpha) { alpha = newAlpha; }
	static double GetAlpha () { return alpha; }
	static double* GenerateDiscreteNoise (int nSamples, GaussianRandomVariable* grv);

protected:
	double Denominator;
	bool Reserve;
	double AlreadyComputed;
	double Sigma;
	double Mu;

	static double alpha;
};


#endif  /*  _GAUSSIANRANDOMVARIABLE_H_  */

//#define vector2 vec2_t
//#define vector3 vec3_t
//#define vector4 vec4_t
//#define complex _complex

#define vector2 Vector2
#define vector3 Vector3
#define vector4 Vector4
//#define complex std::complex<double>
#define complex COMPLEX


#ifndef FFT_H
#define FFT_H
 

class cFFT {
  private:
    unsigned int N, which;
    unsigned int log_2_N;
    float pi2;
    unsigned int *reversed;
    complex **T;
    complex *c[2];
  protected:
  public:
    cFFT(unsigned int N);
    ~cFFT();
    unsigned int reverse(unsigned int i);
    complex t(unsigned int x, unsigned int N);
    void fft(complex* input, complex* output, int stride, int offset);
};
 
#endif

struct vertex_ocean {
    GLfloat   x,   y,   z; // vertex
    GLfloat  nx,  ny,  nz; // normal
    GLfloat   a,   b,   c; // htilde0
    GLfloat  _a,  _b,  _c; // htilde0mk conjugate
    GLfloat  ox,  oy,  oz; // original position
};

struct complex_vector_normal {  // structure used with discrete fourier transform
    complex h;      // wave height
    vector2 D;      // displacement
    vector3 n;      // normal
};

class cOcean {
  private:
    bool geometry;                          // flag to render geometry or surface
 
    float g;                                // gravity constant
    int N, Nplus1;                          // dimension -- N should be a power of 2
    float A;                                // phillips spectrum parameter -- affects heights of waves
    vector2 w;                              // wind parameter
    float length;                           // length parameter
    complex *h_tilde,                       // for fast fourier transform
        *h_tilde_slopex, *h_tilde_slopez,
        *h_tilde_dx, *h_tilde_dz;
    cFFT *fft;                              // fast fourier transform
 
    vertex_ocean *vertices;                 // vertices for vertex buffer object
    unsigned int *indices;                  // indicies for vertex buffer object
    unsigned int indices_count;             // number of indices to render
    GLuint vbo_vertices, vbo_indices;       // vertex buffer objects
 
    GLuint glProgram, glShaderV, glShaderF; // shaders
    GLint vertex, normal, texture, light_position, projection, view, model; // attributes and uniforms
 
  protected:
  public:
    cOcean(const int N, const float A, const vector2 w, const float length, bool geometry);
    ~cOcean();
    void release();
 
    float dispersion(int n_prime, int m_prime);     // deep water
    float phillips(int n_prime, int m_prime);       // phillips spectrum
    complex hTilde_0(int n_prime, int m_prime);
    complex hTilde(float t, int n_prime, int m_prime);
    complex_vector_normal h_D_and_n(vector2 x, float t);
    void evaluateWaves(float t);
    void evaluateWavesFFT(float t);
    void render(float t, vector3 light_pos, Matrix4 Projection, Matrix4 View, Matrix4 Model, bool use_fft);
};

#endif
