#include <thread>
#include <memory>

#include "easylogging++.h"

#include "call-center.h"
#include "http-server.h"

INITIALIZE_EASYLOGGINGPP

void runCallCenter(std::shared_ptr<CallCenter> callCenter){
    callCenter->run();
}
void reloadConf(size_t everyNSec, std::shared_ptr<CallCenter> callCenter){
    if (everyNSec == 0)
        return;
    while (true){
        std::this_thread::sleep_for(std::chrono::duration<long long>(everyNSec));
        callCenter->configure();
   }
}

int main(int argc, char *argv[]){
    if (argc < 3){
        std::cerr << "Neccessary command line parameters: " <<
        "host port.\nSample ./CallCenter 127.0.0.1 7777\n";
        return 1;
    }
    // Load configuration from file
    el::Configurations conf("../logger.conf");
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);

    // Run call center
    auto callCenter = CallCenter::getCallCenter("call-center.json");
    std::thread callCenterTh(runCallCenter, callCenter);

    // Run reload configuration thread
    if (argc > 3){
        std::stringstream ss(argv[3]);
        size_t reloadEveryNSec;
        ss >> reloadEveryNSec;
        if (!ss.fail()){
            std::thread confReaderTh(reloadConf, reloadEveryNSec, callCenter);
            confReaderTh.detach();
        }
    }

    // Run http server
    HttpServer svr;
    if (!svr.listen(std::string(argv[1]), atoi(argv[2]), callCenter)){
        std::cerr << "Invalid host or port format\n";
        return 2;
    }

    callCenterTh.join();
    return 0;
}