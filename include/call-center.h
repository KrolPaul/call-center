#pragma once

#include <stddef.h>

#include <map>
#include <list>
#include <string>
#include <memory>
#include <chrono>
#include <shared_mutex>

#include "json.hpp"

#include "cdr.h"
#include "unique-queue.h"

using namespace cdr;

class CallCenter{
public:
    CallCenter();
    static std::shared_ptr<CallCenter> getCallCenter(const std::string & confFileName = "");

    void run();
    bool configure();
    void pushCall(Cdr & cdr);

    bool setMinResponseTime(const size_t minResponseTime);
    bool setMaxResponseTime(const size_t maxResponseTime);
    bool setMinMaxResponseTime(const size_t minResponseTime,
                               const size_t maxResponseTime);
    size_t getMinResponseTime() const;
    size_t getMaxResponseTime() const;

    bool setMinCallDuration(const size_t minCallDuration);
    bool setMaxCallDuration(const size_t maxCallDuration);
    bool setMinMaxCallDuration(const size_t minCallDuration,
                               const size_t maxCallDuration);
    size_t getMinCallDuration() const;
    size_t getMaxCallDuration() const;

    bool setMaxCallQueueSize(const size_t maxCallQueueSize);
    size_t getMaxCallQueueSize() const;

    bool setRejectRepeatedCalls(bool);
    bool getRejectRepeatedCalls();

    bool setNOperators(const size_t nOperators);
    size_t getNOperators() const;

private:
    // Minimum call duration (seconds)
    size_t minCallDuration;
    // Maximum call duration (seconds)
    size_t maxCallDuration;
    // Minimum call response time (seconds)
    size_t minResponseTime;
    // Maximum call response time (seconds)
    size_t maxResponseTime;
    // Number of operators
    size_t nOperators;
    // Current calls (handled by operators)
    // Ordered by call end DT
    std::multimap<std::chrono::time_point<std::chrono::steady_clock>, Cdr> servicedCalls;
    // Free operators ids
    std::list<size_t> freeOperators;
    std::unique_ptr<UniqueQueue<Cdr>> callQueue;
    // Behavior when receiving call from phone number that is already in queue:
    // 1 - Reject
    // 0 - Delete old call and place new one in queue
    //bool rejectRepeatedCalls; -> placed in callQueue->...
    // Configuration file name
    std::string confFileName;
    // Default configuration file name
    std::string defaultConfFileName;
    // Blocking mtx while configuring
    mutable std::shared_mutex mtx;

    bool tryServeCall(Cdr & cdr);
    bool serveCall(Cdr & cdr);
    void serveCall();
    bool tryEndCall(decltype(servicedCalls)::iterator callIt);
    void endCallByTimeout(Cdr & cdr);
    void releaseOperator(const size_t operatorId);
    void initializeCdr(Cdr & cdr);

    bool getConf(nlohmann::json & conf) const;
    static std::string getConfPath(const std::string & fN);
    static bool readConf(const std::string & fN,
                                  nlohmann::json & conf);
    static bool setConfParams(const nlohmann::json & conf,
                                       CallCenter & callCenter);

};


inline size_t CallCenter::getMinResponseTime() const{
    return minResponseTime;
}
inline size_t CallCenter::getMaxResponseTime() const{
    return maxResponseTime;
}

inline size_t CallCenter::getMinCallDuration() const{
    return minCallDuration;
}
inline size_t CallCenter::getMaxCallDuration() const{
    return maxCallDuration;
}

inline size_t CallCenter::getMaxCallQueueSize() const{
    return callQueue->getMaxSize();
}

inline size_t CallCenter::getNOperators() const{
    return nOperators;
}
inline void CallCenter::releaseOperator(const size_t operatorId){
    std::shared_lock<std::shared_mutex> lck(mtx);
    if (operatorId <= nOperators)
        freeOperators.push_back(operatorId);
}