#pragma once
#include <string>
#include <memory>

class CallCenter;

class HttpServer{
public:
    bool listen(const std::string &host, const int port, std::shared_ptr<CallCenter> callCenter);
};