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

template<class Native, class Replacement>
inline void Record(bool replace, Native&& native, Replacement&& replacement) {
    if (replace) {
        replacement();
        return;
    }
    native();
}
} // namespace rt_as_build_route
