#include <future>

#include "PcoCamera.h"

int main()
{
    auto fut = std::async(std::launch::async, [] {
        std::string sn = "61011182";
        lima::Pco::Camera cam(sn);
    });

    fut.wait();

    return 0;
}