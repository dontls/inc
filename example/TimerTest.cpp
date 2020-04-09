#include "common/Time.h"
#include "common/Timer.h"
void display()
{
    printf("Hello: %s\n", NowUnixString().c_str());
}

void displayInterval()
{
    printf("Hello Interval: %s\n", NowUnixString().c_str());
}

int main(int argc, char const *argv[])
{
    CommTimer ctr;
    ctr.SetInterval(displayInterval, 1000);
    ctr.SetTimeout(display, 2000);
    for (;;)
        ;
    return 0;
}
