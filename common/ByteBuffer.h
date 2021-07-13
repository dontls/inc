// buffer.h
#ifndef __BYTES_BUFFER_H__
#define __BYTES_BUFFER_H__

#include "ThMutex.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>

namespace bytes {

static const int KBlockSize = 4096;

class Buffer {
public:
    Buffer() : _offset(0)
    {
        _allocSize = KBlockSize;
        _allocPtr = new char[_allocSize];
        _totalSize = _allocSize;
    }
    Buffer(size_t n) : _offset(0)
    {
        _allocSize = KBlockSize;
        _allocPtr = new char[_allocSize];
        _totalSize = n;
    }
    ~Buffer()
    {
        if (_allocPtr) {
            delete[] _allocPtr;
            _allocPtr = NULL;
        }
    }

    bool Empty()
    {
        return _offset == 0;
    }

    size_t Cap()
    {
        return _allocSize;
    }

    size_t Len()
    {
        return _offset;
    }

    char* Bytes()
    {
        return _allocPtr;
    }

    int Write(char* p, size_t n)
    {
        assert(NULL != p || 0 > 0);
        return tryAllocAndWrite(p, n);
    }

    int Read(char* p, size_t n)
    {
        assert(NULL != p && n > 0);
        return tryRead(p, n);
    }

    int WriteString(std::string s)
    {
        if (s.length() <= 0) {
            return 0;
        }
        return Write(( char* )s.c_str(), s.length());
    }

    int WriteByte(char c)
    {
        return Write(&c, 1);
    }

    int Remove(size_t n)
    {
        CommRWLock pMutex(&_lock);
        int        readSize = n < _offset ? n : _offset;
        _offset -= readSize;
        ::memmove(_allocPtr, _allocPtr + readSize, _offset);
        return readSize;
    }

private:
    int tryAllocAndWrite(char* p, int n)
    {
        int nsize = _offset + n;
        if (nsize > _totalSize) {
            return -1;
        }
        CommRWLock pMutex(&_lock);
        if (_allocSize < nsize) {
            _allocPtr = ( char* )realloc(_allocPtr, nsize);
            assert(_allocPtr);
            _allocSize = nsize;
        }
        ::memcpy(_allocPtr + _offset, p, n);
        _offset = nsize;
        return n;
    }

    int tryRead(char* p, int n)
    {
        CommRWLock pMutex(&_lock);
        int        readSize = n < _offset ? n : _offset;
        memcpy(p, _allocPtr, readSize);
        _offset -= readSize;
        ::memmove(_allocPtr, _allocPtr + readSize, _offset);
        return readSize;
    }

private:
    CommMutex _lock;
    char*     _allocPtr;
    size_t    _allocSize;
    size_t    _totalSize;
    size_t    _offset;
};

}  // namespace bytes
#endif