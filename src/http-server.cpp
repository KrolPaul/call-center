#include "httplib.h"
#include "json.hpp"
#include "call-center.h"
#include "cdr.h"
#include "http-server.h"

bool HttpServer::listen(const std::string &host, const int port, std::shared_ptr<CallCenter> callCenter){
    httplib::Server svr;

    svr.Get("/call", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(req.body, "text/plain");
        if (req.has_param("phone_number")) {
            std::string phoneNum = req.get_param_value("phone_number");
            nlohmann::json ans;
            Cdr cdr;
            cdr.phoneNumber = phoneNum;
            cdr.receiveDT =  std::chrono::steady_clock::now();
            callCenter->pushCall(cdr);
            ans["call_id"] = cdr.callId;
            ans["call_status"] = toString(cdr.callStatus);
            res.body.append(ans.dump());    
        }
        else
            res.status = 400;
    });

    return svr.listen(host, port);
}
