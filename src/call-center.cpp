#include <unistd.h>

#include <string>
#include <random>
#include <limits>
#include <thread>
#include <fstream>
#include <filesystem>

#include "easylogging++.h"

#include "call-center.h"
#include "rand-generator.hpp"

CallCenter::CallCenter() :
    minCallDuration{0},
    maxCallDuration{0},
    minResponseTime{1},
    maxResponseTime{1},
    nOperators{0},
    defaultConfFileName{"default-call-center.json"}
{
    callQueue = std::make_unique<UniqueQueue<Cdr>>();
}

std::shared_ptr<CallCenter> CallCenter::getCallCenter(
    const std::string & confFileName){
    LOG(INFO) << "Get call center. Config file name: " << confFileName; 
    auto callCenter = std::make_shared<CallCenter>();
    callCenter->confFileName = confFileName;
    callCenter->configure();
    return callCenter;
}

bool CallCenter::configure(){
    nlohmann::json conf;
    getConf(conf);
    
    CallCenter newCallCenter;
    if (!setConfParams(conf, newCallCenter)){
        LOG(ERROR) << "Invalid params";
        LOG(ERROR) << "Configuring not done";
        return false;
    }

    LOG(INFO) << "Configuring. Blocking call center to set new params";
    setConfParams(conf, *(this));
    LOG(INFO) << "Configuring done. Unblocking call center";
    
    return true;
}

bool CallCenter::getConf(nlohmann::json & conf) const{
    nlohmann::json defaultConf;
    LOG(INFO) << "Getting configuration";
    LOG_IF(!readConf(defaultConfFileName, defaultConf), FATAL) <<
        "Can't find default configuration";

    if (confFileName != ""){
        if (!readConf(confFileName, conf))
            LOG(ERROR) << "Can't find configuration";
        else{
            LOG(DEBUG) << "Merging configurations";
            defaultConf.merge_patch(conf);
        }
    }
    conf = defaultConf;
    return true;
}

bool CallCenter::readConf(const std::string & fN,
                                   nlohmann::json & conf){
    auto absolutePath = getConfPath(fN);
    LOG(INFO) << "Configuring. Absolute config file path: " << absolutePath;
    std::ifstream f(absolutePath);
    if (!f.is_open()){
        LOG(ERROR) << "Can't open file: " << absolutePath;
        return false;
    }
    try {
        conf = nlohmann::json::parse(f);
    }
    catch(...){
        LOG(ERROR) << "Can't parse json: " << absolutePath;
        return false;
    }
    return true;
}

std::string CallCenter::getConfPath(const std::string &fN){
    return std::filesystem::current_path().string() + "/../" + fN;
}

bool CallCenter::setConfParams(const nlohmann::json &conf,
                                        CallCenter &callCenter){
    if (!callCenter.setMinMaxResponseTime(conf["minResponseTime"],
                                          conf["maxResponseTime"]))
        return false;
    if (!callCenter.setMinMaxCallDuration(conf["minCallDuration"],
                                          conf["maxCallDuration"]))
        return false;
    if (!callCenter.setNOperators(conf["nOperators"]))
        return false;
    callCenter.callQueue->setRejectRepeated(conf["rejectRepeatedCalls"]);
    if (!callCenter.setMaxCallQueueSize(conf["maxCallQueueSize"]))
        return false;
    return true;
}

void CallCenter::run(){
    LOG(INFO) << "Call center running";
    while (true){
        // Ending serving calls
        if (servicedCalls.size() > 0){
            auto earliestEndCall = servicedCalls.begin();
            // Earliest ending call not ended?
            tryEndCall(earliestEndCall);
        }
        // Starting serving calls
        serveCall();
    }

}

bool CallCenter::tryEndCall(decltype(servicedCalls)::iterator callIt){
    // Call ended?
    if (callIt->first <= std::chrono::steady_clock::now()){
        LOG(INFO) << "Call with callId: " <<
            callIt->second.callId << " ended";
        LOG(INFO) << "Releasing operator with operatorId: " <<
            callIt->second.operatorId;

        releaseOperator(callIt->second.operatorId);
        servicedCalls.erase(callIt);
        return true;
    }
    return false;
}

void CallCenter::serveCall(){
    static bool cdrEmpty = true;
    static Cdr cdr;
    // Getting call from call queue and
    // serving it if minResponseTime elapsed
    if (!callQueue->isEmpty() && cdrEmpty){
        cdr = callQueue->pop();
        cdrEmpty = false;
    }
    if (!cdrEmpty)
        cdrEmpty = tryServeCall(cdr);
}

bool CallCenter::tryServeCall(Cdr & cdr){
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - cdr.receiveDT);
    // If elapsed time > maxResponseTime -> call ending by timeout
    if (elapsedTime > std::chrono::duration<int64_t>(maxResponseTime)){
        endCallByTimeout(cdr);
        return true;
    }
    // If elapsed time > minResponseTime -> serving call
    // and > 1 free operator
    if (std::chrono::duration<int64_t>(minResponseTime) < elapsedTime){
        if (serveCall(cdr))
            return true;
        LOG_EVERY_N(100000000, DEBUG) << "All operators are busy";
    }
    return false;
}

void CallCenter::endCallByTimeout(Cdr &cdr){
    cdr.callStatus = CallStatus::timeout;
    initializeCdr(cdr);
    LOG(INFO) << "Call with callId: " << cdr.callId <<
        " ending by timeout. Elapsed time: " <<
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - cdr.receiveDT).count() <<
        ", maxResponseTime: " << maxResponseTime <<
        ", receiveTime: " << cdr.receiveDT.time_since_epoch().count();
}

bool CallCenter::serveCall(Cdr &cdr){
    std::shared_lock<std::shared_mutex> lck(mtx);
    if (freeOperators.size() < 1)
        return false;
    cdr.operatorId = freeOperators.front();
    freeOperators.pop_front();
    lck.unlock();
    LOG(DEBUG) << "Operator assigned";

    cdr.callStatus = CallStatus::ok;
    initializeCdr(cdr);
    LOG(DEBUG) << "Cdr initialized";
    servicedCalls.emplace(cdr.endDT, cdr);
    LOG(INFO) << "Call serving started. CallId: " << cdr.callId <<
        ", operatorId: " << cdr.operatorId;
    return true;
}

void CallCenter::initializeCdr(Cdr & cdr){
    switch (cdr.callStatus){
    case CallStatus::ok:
        cdr.callDuration = std::chrono::duration<long long>(
            rndgen::genRandUniform(minCallDuration, maxCallDuration));
        cdr.endDT = cdr.receiveDT + cdr.callDuration;
        cdr.callStatus = CallStatus::ok;
        cdr.responseDT = std::chrono::steady_clock::now();
        LOG(DEBUG) << "Initialized cdr with fields:" <<
            ", call id: " << cdr.callId <<
            ", call duration:" << cdr.callDuration.count() <<
            ", operator id: " << cdr.operatorId;
        break;

    case CallStatus::timeout:
        cdr.endDT = std::chrono::steady_clock::now();
        break;

    default:
        break;
    }
}

void CallCenter::pushCall(Cdr & cdr){
    // Phone number format checks can be here
    cdr.callId = rndgen::genRandUniform(
         static_cast<decltype(cdr.callId)>(1),
         std::numeric_limits<decltype(cdr.callId)>::max());

    auto ec = callQueue->push(cdr);

    using EC = UniqueQueue<Cdr>::EC;
    using CS = CallStatus;
    switch (ec){
        case EC::inserted:
        case EC::reassigned:
            cdr.callStatus = CS::ok;
            LOG(INFO) << "Pushed call with call id: " << cdr.callId;
            break;
            
        case EC::overload:
            cdr.callStatus = CS::overload;
            LOG(INFO) << "Tried push call with call id: " << cdr.callId <<
                ". Call queue overloaded";
            break;

        case EC::alreadyInQueue:
            cdr.callStatus = CS::alreadyInQueue;
            LOG(INFO) << "Call with call id: " << cdr.callId <<
                " already in queue";
            break;
    }
}


const static std::string successfulSetPar =
    "Successful attempt setting parameter. New ";
const static std::string unsuccessfulSetPar =
    "Unsuccessful attempt setting parameter. Invalid ";

bool CallCenter::setMinResponseTime(const size_t minResponseTime){
    static auto parName = "minResponseTime: ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    if (minResponseTime > maxResponseTime){
        LOG(DEBUG) << unsuccessfulSetPar << parName << minResponseTime;
        return false;
    }
    this->minResponseTime = minResponseTime;
    LOG(DEBUG) << successfulSetPar << parName << minResponseTime;
    return true;
}

bool CallCenter::setMaxResponseTime(const size_t maxResponseTime){
    static auto parName = "maxResponseTime: ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    if (maxResponseTime < minResponseTime){
        LOG(DEBUG) << unsuccessfulSetPar << parName << maxResponseTime;
        return false;
    }
    this->maxResponseTime = maxResponseTime;
    LOG(DEBUG) << successfulSetPar << parName << minResponseTime;
    return true;
}

bool CallCenter::setMinMaxResponseTime(const size_t minResponseTime,
                                       const size_t maxResponseTime){
    static auto parName = "minMaxResponseTime: ";        
    std::unique_lock<std::shared_mutex> lck(mtx);                  
    if (minResponseTime > maxResponseTime){
        LOG(DEBUG) << unsuccessfulSetPar << parName <<
            "min: " << minResponseTime << " max: " << maxResponseTime;
        return false;
    }
    this->minResponseTime = minResponseTime;
    this->maxResponseTime = maxResponseTime;
    LOG(DEBUG) << successfulSetPar << parName <<
        "new min: " << minResponseTime << " new max: " << maxResponseTime;
    return true;
}

bool CallCenter::setMinCallDuration(const size_t minCallDuration){
    static auto parName = "minCallDuration: ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    if (minCallDuration > maxCallDuration ||
        minCallDuration < 1){
        LOG(DEBUG) << unsuccessfulSetPar << parName << minCallDuration;
        return false;
    }
    LOG(DEBUG) << successfulSetPar << parName << minCallDuration;
    this->minCallDuration = minCallDuration;
    return true;
}

bool CallCenter::setMaxCallDuration(const size_t maxCallDuration){
    static auto parName = "maxCallDuration ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    if (maxCallDuration < minCallDuration ||
        maxCallDuration < 1){
        LOG(DEBUG) << successfulSetPar << parName << maxCallDuration;
        return false;
    }
    this->maxCallDuration = maxCallDuration;
    LOG(DEBUG) << successfulSetPar << parName << minResponseTime;
    return true;
}

bool CallCenter::setMinMaxCallDuration(const size_t minCallDuration,
                                       const size_t maxCallDuration){
    static auto parName = "minMaxCallDuration: ";
    std::unique_lock<std::shared_mutex> lck(mtx);  
    if (minCallDuration > maxCallDuration || minCallDuration < 1){
        LOG(DEBUG) << unsuccessfulSetPar << parName <<
            "min: " << minCallDuration << " max: " << maxCallDuration;
        return false;
    }
    this->minCallDuration = minCallDuration;
    this->maxCallDuration = maxCallDuration;
    LOG(DEBUG) << successfulSetPar << parName <<
        "new min: " << minCallDuration << " new max: " << maxCallDuration;
    return true;
}

bool CallCenter::setMaxCallQueueSize(const size_t maxCallQueueSize){
    auto parName = "maxCallQueueSize: ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    auto res = callQueue->setMaxSize(maxCallQueueSize);
    lck.unlock();
    if (res)
        LOG(DEBUG) << successfulSetPar << parName << maxCallQueueSize;
    else 
        LOG(DEBUG) << unsuccessfulSetPar << parName << maxCallQueueSize;
    return res;
}

bool CallCenter::setNOperators(const size_t nOperators){
    static auto parName = "nOperators: ";
    std::unique_lock<std::shared_mutex> lck(mtx);     
    if (nOperators < 1){
        LOG(DEBUG) << unsuccessfulSetPar << parName << nOperators;
        return false;
    }

    const int64_t difference = this->nOperators - nOperators;
    LOG(DEBUG) << "Old and new nOperators difference: " << difference;
    // Add new free operators if new nOperators > old newOperators
    if (difference < 0){
        for (auto i = difference * (-1); i > 0; --i)
            freeOperators.push_back(this->nOperators + i);
    }
    // Delete free operators if new nOperators < old newOperators
    if (difference > 0){
        freeOperators.remove_if([&nOperators](auto operatorId){
            return operatorId > nOperators;
        });
    }

    this->nOperators = nOperators;
    LOG(DEBUG) << successfulSetPar << parName << nOperators;
    return true;
}

bool CallCenter::setRejectRepeatedCalls(bool rejectRepeatedCalls){
    LOG(DEBUG) << "Old rejectRepeatedCalls: " << callQueue->getRejectRepeated();
    LOG(DEBUG) << "New rejectRepeatedCalls: " << rejectRepeatedCalls;
    callQueue->setRejectRepeated(rejectRepeatedCalls);
    return true;
}

bool CallCenter::getRejectRepeatedCalls(){
    return callQueue->getRejectRepeated();
}
