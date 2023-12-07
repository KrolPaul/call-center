#include "cdr.h"

using namespace cdr;

const static std::unordered_map<CallStatus, std::string> strs{
    {CallStatus::ok, "ok"},
    {CallStatus::overload, "overload"},
    {CallStatus::alreadyInQueue, "alreadyInQueue"},
    {CallStatus::callDuplication, "callDuplication"},
    {CallStatus::timeout, "timeout"}
};

std::string cdr::toString(CallStatus cS){
    return strs.find(cS)->second;
}