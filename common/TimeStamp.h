#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <sys/time.h>

class CommTimeStamp {
private:
    /* data */
public:
    CommTimeStamp() : _microSecondsSinceEpoch(0) {}

    int64_t MicroSecondsSinceEpoch() const
    {
        return _microSecondsSinceEpoch;
    }
    //@param microSecondsSinceEpoch
    explicit CommTimeStamp(int64_t microSecondsSinceEpoch) : _microSecondsSinceEpoch(microSecondsSinceEpoch) {}

    //
    // Get time of now.
    //
    std::string ToString() const
    {
        char    buf[32] = { 0 };
        int64_t seconds = _microSecondsSinceEpoch / kMicroSecondsPerSecond;
        int64_t microseconds = _microSecondsSinceEpoch % kMicroSecondsPerSecond;
        snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
        return buf;
    }

    bool Valid() const
    {
        return _microSecondsSinceEpoch > 0;
    }

    static CommTimeStamp Now()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);
        int64_t seconds = tv.tv_sec;
        return CommTimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
    }
    static CommTimeStamp Invalid()
    {
        return CommTimeStamp();
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t _microSecondsSinceEpoch;
};

inline CommTimeStamp AddTime(CommTimeStamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * CommTimeStamp::kMicroSecondsPerSecond);
    return CommTimeStamp(timestamp.MicroSecondsSinceEpoch() + delta);
}

inline bool operator<(CommTimeStamp lhs, CommTimeStamp rhs)
{
    return lhs.MicroSecondsSinceEpoch() < rhs.MicroSecondsSinceEpoch();
}

inline bool operator==(CommTimeStamp lhs, CommTimeStamp rhs)
{
    return lhs.MicroSecondsSinceEpoch() == rhs.MicroSecondsSinceEpoch();
}

#endif
