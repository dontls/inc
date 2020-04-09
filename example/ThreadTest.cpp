#include "common/Thread.h"
#include <iostream>
#include <unistd.h>

class ThreadTest : public CommThread {
public:
    ThreadTest(const char* name)
    {
        name_ = name;
    }

protected:
    void run()
    {
        int i = 0;
        while (i++ < 100) {
            std::cout << name_ << ":---------->" << i << " " << currentThreadId() << "\n";
            sleep(1);
        }
    }

private:
    const char* name_;
};

int main(int argc, char const* argv[])
{
    ThreadTest t("TEST 1");
    t.start();

    ThreadTest t1("TEST 2");
    t1.start();
    int i = 0;
    while (i++ < 200) {
        if (i == 30) {
            t.quit();
        } else if (i == 80) {
            t1.quit();
        }
        sleep(1);
    };
    return 0;
}
