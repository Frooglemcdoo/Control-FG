#pragma once
#include "rr_paired_state.h"
#include "rr_paired_pixels.h"
#include <condition_variable>
#include <deque>
#include <iomanip>
#include <locale>
#include <sstream>
#include <thread>
#include <vector>
#include <wrl/client.h>
namespace rr_pair {
using Microsoft::WRL::ComPtr;
struct Frame:FrameView {
    ComPtr<ID3D12Resource> buffer;
    std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,PlaneCount> layouts{};
    std::array<ComPtr<ID3D12Resource>,PlaneCount> sources;
    uint64_t capacity=0,bytes=0;
    ID3D12GraphicsCommandList* recordedList=nullptr;
    bool outputRecorded=false, mapped=false;
    ~Frame(){Unmap();}
    void Unmap(){
        if(mapped){D3D12_RANGE empty{};buffer->Unmap(0,&empty);mapped=false;}
        for(auto& grid:g)grid.base=nullptr;
    }
    bool Map(){
        if(mapped)return true;
        D3D12_RANGE range{0,SIZE_T(bytes)};void* data=nullptr;
        if(FAILED(buffer->Map(0,&range,&data)))return false;
        for(unsigned i=0;i<PlaneCount;++i)if(g[i].w)g[i].base=static_cast<unsigned char*>(data)+layouts[i].Offset;
        mapped=true;return true;
    }
};
inline const char* Name(Plane p){
    static const char* names[]={"raw_gi","both_temporal_gi","both_final_gi","native_raw_shaded",
        "r505_rr_input","character_mask","hardware_depth","normal","motion","native_map","native_diffuse_light_at_rr","native_specular_light_at_rr","rr_output"};
    return names[p];
}
inline void WriteRGB(std::ostream& os,RGB rgb){
    os<<'[';
    for(unsigned k=0;k<3;++k){if(k)os<<',';if(std::isfinite(rgb[k]))os<<rgb[k];else os<<"null";}
    os<<']';
}
inline void WriteXY(std::ostream& os,XY v){os<<'['<<v.x<<','<<v.y<<']';}
inline std::string Crop(const Frame& f,XY p){
    std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(8);
    s<<"{\"frame\":"<<f.number<<",\"internal_xy\":";WriteXY(s,p);
    s<<",\"lighting_xy\":";WriteXY(s,f.Source(p));s<<",\"history_frames\":"<<f.warm
     <<",\"native_pass_count\":"<<f.passes<<",\"jitter\":["<<f.jitterX<<','<<f.jitterY
     <<"],\"motion_scale\":["<<f.scaleX<<','<<f.scaleY<<"],\"planes\":{";
    bool first=true;
    for(Plane k:{Raw,Temporal,Spatial,Diffuse,Specular,Scene,RRInput,RROut}){
        if(!first)s<<',';first=false;
        if(!f.g[k].base){s<<'"'<<Name(k)<<"\":null";continue;}
        XY center=(k<=Spatial||k==Diffuse||k==Specular)?f.Source(p):p;
        if(k==RROut)center={int((p.x+.5)*f.g[k].w/f.w),int((p.y+.5)*f.g[k].h/f.h)};
        s<<'"'<<Name(k)<<"\":{\"center\":";WriteXY(s,center);
        s<<",\"dxgi_format\":"<<f.g[k].format<<",\"extent\":["<<f.g[k].w<<','<<f.g[k].h<<"],\"rgb\":[";
        for(int j=-4;j<=4;++j)for(int i=-4;i<=4;++i){
            if(i!=-4||j!=-4)s<<',';WriteRGB(s,Color(f.g[k],{center.x+i,center.y+j}));
        }
        s<<"]}";
    }
    s<<"}}";return s.str();
}
// A new session owns one CREATE_NEW report. Follow-through atomically replaces
// only that report after the confirmed three-frame event is already on disk.
inline bool WriteEvent(uint64_t session,const Event& e,const std::vector<std::string>& crops,std::wstring& dest){
    wchar_t base[32768]{};auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",base,_countof(base));
    if(!n||n>=_countof(base))return false;
    std::wstring dir=std::wstring(base)+L"\\ControlFGProbe";
    if(!CreateDirectoryW(dir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
    const bool update=!dest.empty();
    const auto stem=dir+L"\\Character-Speckle-R506-"+std::to_wstring(GetCurrentProcessId())+L"-"+
        std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(session);
    const auto path=update?dest:stem+L".json";
    const auto temp=path+L".partial";
    std::ostringstream os;os.imbue(std::locale::classic());os<<std::setprecision(8);
    os<<"{\"schema\":\"control-character-speckle-r506\",\"pid\":"<<GetCurrentProcessId()
      <<",\"ray_count\":4,\"crop_width\":9,\"crop_order\":\"row_major_rgb\",\"trigger_stage\":\""<<Name(e.stage)
      <<"\",\"trigger_channel\":"<<e.channel<<",\"peak_ratio\":"<<e.score
      <<",\"rr_output_mapping\":\"nearest output pixel center; RR reconstruction correspondence approximate\""
      <<",\"both_shaded_measured\":false,\"frame_count\":"<<crops.size()<<",\"frames\":[";
    for(size_t i=0;i<crops.size();++i){if(i)os<<',';os<<crops[i];}
    os<<"]}\n";const auto data=os.str();
    HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;bool ok=WriteFile(h,data.data(),DWORD(data.size()),&written,nullptr)&&written==data.size()&&FlushFileBuffers(h);
    CloseHandle(h);if(ok)ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH|(update?MOVEFILE_REPLACE_EXISTING:0))!=FALSE;
    if(!ok)DeleteFileW(temp.c_str());else dest=path;
    return ok;
}
// Only the fenced, completed RR slot enters this worker. A slow reader applies
// backpressure outside the worker's mutex instead of silently dropping impulses.
struct Reader {
    std::mutex mutex;
    std::condition_variable ready,space;
    std::deque<std::shared_ptr<Frame>> arrivals;
    std::vector<std::shared_ptr<Frame>> pool;
    bool running=false;
    std::shared_ptr<Frame> Acquire(ID3D12Device* device,uint64_t size){
        std::unique_lock<std::mutex> lock(mutex);
        for(auto& p:pool)if(p.use_count()==1 && p->capacity>=size){
            ComPtr<ID3D12Device> d;p->buffer->GetDevice(IID_PPV_ARGS(&d));
            if(d.Get()==device){p->Unmap();return p;}
        }
        pool.erase(std::remove_if(pool.begin(),pool.end(),[](const auto& p){return p.use_count()==1;}),pool.end());
        if(pool.size()>=9 || size>640ull*1024*1024)return {};
        auto p=std::make_shared<Frame>();D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
        hp.CreationNodeMask=hp.VisibleNodeMask=1;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Width=size;d.Height=d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&p->buffer))))return {};
        p->capacity=size;pool.push_back(p);return p;
    }
    void Submit(std::shared_ptr<Frame> f){
        for(auto& s:f->sources)s.Reset();
        if(f->session!=Armed()||!f->outputRecorded)return;
        std::unique_lock<std::mutex> lock(mutex);
        if(!running){std::thread t([this]{Run();});t.detach();running=true;}
        space.wait(lock,[&]{return arrivals.size()<2||f->session!=Armed();});
        if(f->session==Armed()){arrivals.push_back(std::move(f));ready.notify_one();}
    }
    void Run() noexcept {
        try{
            std::vector<std::shared_ptr<Frame>> ordered;
            std::vector<std::string> crops;std::wstring dest;Event event{};XY tail{};
            uint64_t lastEventFrame=0,currentSession=0;
            for(;;){
                std::shared_ptr<Frame> f;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    ready.wait_for(lock,std::chrono::milliseconds(100),[&]{return !arrivals.empty();});
                    if(arrivals.empty()){
                        if(!Armed()){
                            ordered.clear();pool.erase(std::remove_if(pool.begin(),pool.end(),[](const auto& p){return p.use_count()==1;}),pool.end());
                            running=false;return;
                        }
                        continue;
                    }
                    f=std::move(arrivals.front());arrivals.pop_front();space.notify_all();
                }
                if(f->session!=Armed())continue;
                if(currentSession!=f->session){ordered.clear();crops.clear();dest.clear();lastEventFrame=0;currentSession=f->session;}
                if(!f->Map()){Issue(f->session,L"Readback failed");continue;}
                if(std::any_of(ordered.begin(),ordered.end(),[&](const auto& p){return p->number==f->number;}))continue;
                ordered.push_back(f);std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a->number<b->number;});
                // Fences may retire multiple slots in array order. Admit triples by engine frame.
                while(ordered.size()>=3){
                    auto a=ordered[0],b=ordered[1],c=ordered[2];
                    if(!Consecutive(*a,*b)||!Consecutive(*b,*c)){
                        if(ordered.size()<5)break;
                        ordered.erase(ordered.begin());continue;
                    }
                    ordered.erase(ordered.begin());
                    if(crops.empty()){
                        event=Find(*a,*b,*c);if(!event.score)continue;
                        crops={Crop(*a,event.positions[0]),Crop(*b,event.positions[1]),Crop(*c,event.positions[2])};
                        std::lock_guard<std::mutex> guard(stateMutex);
                        if(active!=c->session)break;
                        if(!WriteEvent(c->session,event,crops,dest)){
                            status=L"Could not save capture";active=0;break;
                        }
                        capturingTail=true;tail=event.positions[2];lastEventFrame=c->number;
                        status=L"Flash saved; collecting follow-through";
                        continue;
                    }else if(c->number==lastEventFrame+1){
                        // Find the destination that reprojects to the previous confirmed receiver.
                        bool matched=false;XY next{};int nearest=0;
                        for(int dy=-48;dy<=48;++dy)for(int dx=-48;dx<=48;++dx){
                            XY q{tail.x+dx,tail.y+dy},old{};
                            if(MatchPrevious(*c,q,*b,old)&&old.x==tail.x&&old.y==tail.y){
                                int d=dx*dx+dy*dy;if(!matched||d<nearest){matched=true;nearest=d;next=q;}
                            }
                        }
                        if(matched){crops.push_back(Crop(*c,next));tail=next;}
                        else crops.push_back("{\"frame\":"+std::to_string(c->number)+",\"surface_match\":false}");
                        ++lastEventFrame;
                        std::lock_guard<std::mutex> guard(stateMutex);
                        if(active!=c->session)break;
                        if(!WriteEvent(c->session,event,crops,dest)){
                            status=L"Flash saved; follow-through incomplete";active=0;break;
                        }
                        if(matched&&crops.size()<6)continue;
                    }else if(c->number<=lastEventFrame)continue;
                    {std::lock_guard<std::mutex> lock(stateMutex);
                    if(active==c->session){
                        status=L"Saved in ControlFGProbe";active=0;
                    }}
                    ordered.clear();crops.clear();break;
                }
            }
        }catch(...){std::lock_guard<std::mutex> lock(mutex);running=false;arrivals.clear();space.notify_all();
            std::lock_guard<std::mutex> state(stateMutex);status=L"Capture reader stopped";active=0;}
    }
};
// Process-lifetime storage, matching the existing probe's renderer lifetime.
inline Reader& CaptureReader(){static Reader* r=new Reader;return *r;}
inline void Copy(ID3D12GraphicsCommandList* list,UINT64& commands,Frame& f,unsigned i,ID3D12Resource* r,D3D12_RESOURCE_STATES state){
    auto barrier=[&](D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        if(before==after)return;
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,0,before,after};
        ++commands;list->ResourceBarrier(1,&b);
    };
    barrier(state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=r;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.pResource=f.buffer.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=f.layouts[i];
    ++commands;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);barrier(D3D12_RESOURCE_STATE_COPY_SOURCE,state);
    f.sources[i]=r;
}
}
