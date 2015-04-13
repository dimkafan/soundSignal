/*
    Fast Fourier Transformation
    ====================================================
    Coded by Miroslav Voinarovsky, 2002
    This source is freeware.
*/

#ifndef FFT_H_
#define FFT_H_

struct Complex;
struct ShortComplex;

/*
    Fast Fourier Transformation: direct (complement= false)
    and complement (complement = true). 'x' is data source.
    'x' contains 2^T items.

*/
void fft(ShortComplex *x, int T, bool complement);

struct ShortComplex
{
    double re, im;
    inline void operator=(const Complex &y);
    inline void operator/=(double div){ re /= div; im /= div; }
};

struct Complex
{
    long double re, im;
    inline void operator= (const Complex &y);
    inline void operator= (const ShortComplex &y);
};


inline void ShortComplex::operator=(const Complex &y)    { re = (double)y.re; im = (double)y.im;  }
inline void Complex::operator= (const Complex &y)       { re = y.re; im = y.im; }
inline void Complex::operator= (const ShortComplex &y)  { re = y.re; im = y.im; }

#endif