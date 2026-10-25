#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace native_lighting {
// Diagnostic CPU copies made only when a PSO is first identified. No GPU work,
// per-frame copying, file I/O or blocking lock in the creation callback.
class LightingShaderSnapshot {
public:
    static constexpr size_t Capacity = 8 * 1024 * 1024;
    static constexpr size_t MaxShaderBytes = 256 * 1024;
    static constexpr unsigned MaxShaders = 512;
private:
    struct Entry { unsigned stage=0; size_t offset=0, size=0; char sha[65]{}; };
    std::array<unsigned char,Capacity> bytes_{};
    std::array<Entry,MaxShaders> entries_{};
    std::mutex writer_;
    std::atomic<unsigned> count_{0}, dropped_{0};
    size_t used_=0;
    unsigned exportSerial_=0; // Only the diagnostic UI thread exports.
public:
    void Record(unsigned stage,const void* bytes,size_t size,const char* sha) noexcept {
        if((stage!=1 && stage!=2) || !bytes || size<4 || size>MaxShaderBytes ||
            !sha || std::strlen(sha)!=64 || std::memcmp(bytes,"DXBC",4)) { ++dropped_;return; }
        std::unique_lock<std::mutex> lock(writer_,std::try_to_lock);
        if(!lock.owns_lock()){++dropped_;return;}
        const unsigned count=count_.load(std::memory_order_relaxed);
        for(unsigned i=0;i<count;++i)
            if(entries_[i].stage==stage && !std::strcmp(entries_[i].sha,sha))return;
        if(count==MaxShaders || size>Capacity-used_){++dropped_;return;}
        auto& entry=entries_[count];entry.stage=stage;entry.offset=used_;entry.size=size;
        std::memcpy(entry.sha,sha,65);std::memcpy(bytes_.data()+used_,bytes,size);used_+=size;
        count_.store(count+1,std::memory_order_release); // Published bytes never mutate.
    }
    unsigned Count() const noexcept {return count_.load(std::memory_order_acquire);}
    unsigned Dropped() const noexcept {return dropped_.load();}
    template<class Write> bool Export(Write&& write) noexcept {
        const unsigned count=Count(),serial=++exportSerial_;
        char line[512]{};
        auto emit=[&](int length){return length>0 && size_t(length)<sizeof(line) && write(line,size_t(length));};
        if(!emit(std::snprintf(line,sizeof(line),"shader_snapshot_begin export=%u count=%u dropped=%u capacity=%zu\r\n",serial,count,Dropped(),Capacity)))return false;
        constexpr char hex[]="0123456789abcdef";
        for(unsigned i=0;i<count;++i){
            const auto& entry=entries_[i];
            if(!emit(std::snprintf(line,sizeof(line),"shader_snapshot_entry export=%u index=%u stage=%u bytes=%zu sha=%s\r\n",serial,i,entry.stage,entry.size,entry.sha)))return false;
            for(size_t offset=0;offset<entry.size;){
                const size_t size=(entry.size-offset<128)?entry.size-offset:128;
                char encoded[257]{};
                for(size_t n=0;n<size;++n){const auto value=bytes_[entry.offset+offset+n];encoded[n*2]=hex[value>>4];encoded[n*2+1]=hex[value&15];}
                if(!emit(std::snprintf(line,sizeof(line),"shader_snapshot_data export=%u index=%u offset=%zu hex=%s\r\n",serial,i,offset,encoded)))return false;
                offset+=size;
            }
        }
        return emit(std::snprintf(line,sizeof(line),"shader_snapshot_end export=%u count=%u\r\n",serial,count));
    }
};
} // namespace native_lighting
