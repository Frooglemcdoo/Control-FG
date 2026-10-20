#pragma once
#include <cstdint>
#include <string>
#include "script.h"

namespace control_native_menu {
inline std::uint64_t Hash(const std::string& s) {
    std::uint64_t h = 14695981039346656037ull;
    for (unsigned char c : s) h = (h ^ c) * 1099511628211ull;
    return h;
}
inline bool ReplaceOnce(std::string& s, const char* from, const char* to) {
    const std::string key(from);
    auto p = s.find(key);
    if (p == std::string::npos || s.find(key, p + key.size()) != std::string::npos) return false;
    s.replace(p, key.size(), to);
    return true;
}
inline bool PatchCommons(const std::string& input, const std::string& config, std::string& output) {
    // Fail closed if another game build/mod changes the shared UI implementation.
    if (input.size() != 615195 || Hash(input) != 0xb5a45dfc86f79619ull) return false;
    std::string s(input);
    if (!ReplaceOnce(s, "renderOptions(){const e=[",
        "renderOptions(){const e=[...(this.props.sectionType===\"Graphics\"?window.__controlFG.rows(n):[]),")) return false;
    if (!ReplaceOnce(s,
        "n=C(e,l.OptionsEventType.Enum,!0,this.props.sectionType,this.props.isPauseMenu),\"MENU_OPTIONS_ASSIST_MODE\"",
        "n=C(e,l.OptionsEventType.Enum,!0,this.props.sectionType,this.props.isPauseMenu),e.cfgHeader?i.createElement(i.Fragment,{key:this.props.sectionType+e.m_iPropertyID},i.createElement(\"div\",{style:{position:\"relative\",zIndex:0}},i.createElement(f.default,{className:\"options-tab__group-header\",locId:e.cfgHeader})),n):\"MENU_OPTIONS_ASSIST_MODE\"")) return false;
    if (!ReplaceOnce(s, "t.default=function(e,t={}){if(!e)return e;let n=\"\";",
        "t.default=function(e,t={}){if(!e)return e;if(e.indexOf(\"CFG_\")===0)return window.__controlFG.translate(e);let n=\"\";")) return false;
    if (!ReplaceOnce(s, "let o=_.default[e.m_strNameLocID]||void 0,r=[],s=\"\";return",
        "let o=_.default[e.m_strNameLocID]||void 0,r=[],s=e.m_strNameLocID===\"CFG_CLAMP\"?\"%\":e.m_strNameLocID===\"CFG_TARGET\"?\" FPS\":\"\";return")) return false;
    output = "window.__controlFGConfig=" + config + ";\n" + kNativeMenuScript + "\n" + s;
    return true;
}
}
