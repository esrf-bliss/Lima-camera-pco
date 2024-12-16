#include <future>

#include "PcoCamera.h"

int main()
{
    auto fut = std::async(std::launch::async, [] {
        std::string params = "sn=61011182";
        lima::Pco::Camera cam(params);
    });

    fut.wait();

    return 0;
}