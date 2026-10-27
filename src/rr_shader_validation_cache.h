#pragma once
// Included inside RRAlbedoShader after Parse. Owned by one preparation Session;
// every lookup compares freshly copied bytes, never pointers or a hash alone.
class ValidationCache {
    struct Entry { std::vector<uint8_t> bytes; Contract contract; bool pixel; };
    std::vector<Entry> entries_;
    size_t bytes_=0;
    size_t limitBytes_,limitEntries_;
    uint64_t hits_=0,parses_=0;
public:
    explicit ValidationCache(size_t bytes=512u*1024u,size_t entries=64u) noexcept
        :limitBytes_(bytes),limitEntries_(entries){}
    bool Validate(const std::vector<uint8_t>& data,bool pixel,Contract& out,const char** reason) noexcept {
        out={};if(reason)*reason=nullptr;
        for(const auto& entry:entries_){
            if(entry.pixel==pixel&&entry.bytes==data){out=entry.contract;++hits_;return true;}
        }
        ++parses_;
        if(!Parse(data.data(),data.size(),pixel,out,reason))return false;
        // Exhaustion/allocation failure disables caching, never validation.
        if(entries_.size()<limitEntries_&&data.size()<=limitBytes_-bytes_){
            try {entries_.push_back({data,out,pixel});bytes_+=data.size();}catch(...){}
        }
        return true;
    }
    uint64_t Hits() const noexcept{return hits_;}
    uint64_t Parses() const noexcept{return parses_;}
    size_t Bytes() const noexcept{return bytes_;}
};
