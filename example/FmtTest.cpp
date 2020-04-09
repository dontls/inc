#include "common/Fmt.h"
#include <iostream>
int main(int argc, char const *argv[])
{
    Fmt us(".%06d", 1000);
    std::cout << us.Data() << "\t" << us.Length() << "\n";

    Fmt t("%d", 123456);
    std::cout << t.Data() << "\t" << t.Length() << "\n";
    return 0;
}
