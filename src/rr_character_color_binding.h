#pragma once
// The native NGX parameter object is reused across frames. Scope the override
// to exactly one evaluation, including failure and unwind paths.
template<class Api,class Resource,class Call>
static unsigned RRWithCharacterColor(Api& api,Resource* original,Resource* replacement,Call& evaluate,bool& bound) {
    bound=false;
    if(!replacement || replacement==original)return evaluate();
    const DWORD incoming=GetLastError();DWORD outgoing=incoming;
    unsigned result=0xBAD00001u;
    __try {
        api.Resource("Color",replacement);
        bound=api.ResourceEquals("Color",replacement);
        if(!bound)api.Resource("Color",original);
        if(bound || api.ResourceEquals("Color",original)){
            SetLastError(incoming);result=evaluate();outgoing=GetLastError();
        }
    } __finally {
        api.Resource("Color",original);
        if(!api.ResourceEquals("Color",original))result=0xBAD00001u;
        SetLastError(outgoing);
    }
    return result;
}
