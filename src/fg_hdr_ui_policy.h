#pragma once
#include <cstdint>

namespace control_fg_hdr_ui {

inline bool HasRequiredUiTags(std::uint32_t tagMask, std::uint32_t hudlessBit, std::uint32_t uiAlphaBit) noexcept {
    return (tagMask & (hudlessBit | uiAlphaBit)) == (hudlessBit | uiAlphaBit);
}

inline bool HudlessDomainReady(bool hdr10Active, std::uint32_t hudlessFormat, std::uint32_t rgb10Format) noexcept {
    return !hdr10Active || hudlessFormat == rgb10Format;
}

inline bool ShouldEnableRecomposition(bool fgEnabled,
                                      std::uint32_t tagMask,
                                      std::uint32_t hudlessBit,
                                      std::uint32_t uiAlphaBit,
                                      bool hdr10Active,
                                      std::uint32_t hudlessFormat,
                                      std::uint32_t rgb10Format) noexcept {
    return fgEnabled && HasRequiredUiTags(tagMask, hudlessBit, uiAlphaBit) &&
           HudlessDomainReady(hdr10Active, hudlessFormat, rgb10Format);
}

inline std::uint32_t HudlessOptionFormat(bool hdr10Active,
                                         bool recompositionEnabled,
                                         std::uint32_t observedHudlessFormat,
                                         std::uint32_t rgb10Format) noexcept {
    if (hdr10Active) return recompositionEnabled ? rgb10Format : 0u;
    return observedHudlessFormat;
}

} // namespace control_fg_hdr_ui
