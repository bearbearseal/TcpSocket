#ifndef _TEST_LISTENER_HPP_
#define _TEST_LISTENER_HPP_
#include "TcpListener.h"
#include <thread>
#include <chrono>

namespace Test_Listener {

    void run() {
        TcpListener tcpListener(56789);
        tcpListener.start();

        while (1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

}

#endif