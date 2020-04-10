#include "common/Time.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    printf("%s\n", NowUnixString().c_str());
    printf("%s\n", NowUnixString().c_str());
    unsigned long long startTime = NowTickCount();
    if (!TimeoutMsec(startTime, 90)) {
        printf("%lld\n", startTime);
    }
    sleep(2);
    if (TimeoutMsec(startTime, 5000)) {
        printf("5s超时\n");
    } else {
        printf("-------------\n");
    }
    return 0;
}
