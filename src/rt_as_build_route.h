#pragma once

// One owner for an AS build. Preparation must not record GPU commands, build a
// second scene, or mutate native bindings. Once selected, a replacement owns
// failures too: there is deliberately no 'failed, now call native' return path.
namespace rt_as_build_route {
struct Coverage {
    bool deviceSupported = false;
    bool consumers = false;
    bool copiesAndPostbuild = false;
    bool resourceLifetimes = false;
    bool addressMapping = false;
    bool nativePreparationRetired = false;
    constexpr bool Complete() const noexcept {
        return deviceSupported && consumers && copiesAndPostbuild &&
               resourceLifetimes && addressMapping && nativePreparationRetired;
    }
};

enum class Choice {Native, Replacement, Abort};
inline constexpr Choice Select(bool alreadyOwned,bool coverage,bool prepared) noexcept {
 return coverage&&prepared?Choice::Replacement:(alreadyOwned?Choice::Abort:Choice::Native);
}
template<class Native,class Replacement,class Abort>
inline void Record(Choice choice,Native&& native,Replacement&& replacement,Abort&& abort) {
 switch(choice){case Choice::Native:native();return;case Choice::Replacement:replacement();return;case Choice::Abort:abort();return;}
 abort();
}

template<class Native, class Replacement>
inline void Record(bool replace, Native&& native, Replacement&& replacement) {
    if (replace) {
        replacement();
        return;
    }
    native();
}
} // namespace rt_as_build_route
