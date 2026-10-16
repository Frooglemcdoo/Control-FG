#pragma once
namespace PTGuideAdmission {
// Evaluate at queue submission, after earlier producer lists were selected.
constexpr unsigned Reject(bool recorded,bool armed,bool deferred,bool cancelled,
                          bool replay,bool sameQueue,bool admitted){
    return ((!recorded||(!armed&&!deferred)||cancelled)?1u:0u)|
           (replay?2u:0u)|(!sameQueue?4u:0u)|(!admitted?8u:0u);
}
}
