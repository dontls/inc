#ifndef __FMT_H__
#define __FMT_H__

#include <assert.h>
#include <stdio.h>

class Fmt {
private:
    /* data */
public:
    template <typename T>
    Fmt(const char* fmt, T v);
    const char* Data() const
    {
        return _buf;
    }

    int Length() const
    {
        return _length;
    }

private:
    char _buf[32];
    int  _length;
};

template <typename T>
Fmt::Fmt(const char* fmt, T v)
{
    _length = snprintf(_buf, sizeof(_buf), fmt, v);
    assert(static_cast<size_t>(_length) < sizeof(_buf));
}

template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

#endif