#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <stddef.h>

namespace cdr{

enum class CallStatus{
    ok,
    overload,
    alreadyInQueue,
    callDuplication,
    timeout
};

std::string toString(CallStatus cS);

struct Cdr{
    // DT поступления вызова.
    std::chrono::time_point<std::chrono::steady_clock> receiveDT;
    // Идентификатор входящего вызова (Call ID)
    size_t callId;
    // Номер абонента
    std::string phoneNumber;
    // DT завершения вызова
    std::chrono::time_point<std::chrono::steady_clock> endDT;
    // Статус вызова (OK или причина ошибки, например timeout)
    CallStatus callStatus;  
    // DT ответа оператора (если был или пустое значение)
    std::chrono::time_point<std::chrono::steady_clock> responseDT;
    // Идентификатор оператора (пустое значение если соединение не состоялось)
    size_t operatorId;
    // Длительность разговора (пустое значение если соединение не состоялось)
    std::chrono::duration<long long> callDuration; //sec


    std::string getId() const{
        return phoneNumber;
    }
};

};