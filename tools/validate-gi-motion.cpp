// Streaming, bounded-memory verifier for the diagnostic GPU ABI.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
static float Float(std::uint32_t u){float f;std::memcpy(&f,&u,4);return f;}
static float Half(std::uint16_t h){
 const unsigned e=(h>>10)&31u,m=h&1023u;
 float v=e==0?std::ldexp(float(m),-24):e==31?(m?NAN:INFINITY):std::ldexp(float(1024+m),int(e)-25);
 return h&32768u?-v:v;
}
struct Ray{std::uint32_t u[56];};static_assert(sizeof(Ray)==224,"motion GPU layout");
static int Preview(const std::vector<std::filesystem::path>& a){
 if(a.size()!=8)return 2;
 const auto format=std::stoul(a[3].string()),w=std::stoul(a[4].string()),h=std::stoul(a[5].string());
 if((format!=10&&format!=26)||!w||!h||w>4096||h>2160)return 2;
 std::ifstream input(a[2],std::ios::binary);std::ofstream normal(a[6],std::ios::binary),bright(a[7],std::ios::binary);
 if(!input||!normal||!bright)return 2;
 normal<<"P6\n"<<w<<' '<<h<<"\n255\n";bright<<"P6\n"<<w<<' '<<h<<"\n255\n";
 std::array<unsigned char,65536> lut{},lut8{};
 for(unsigned i=0;i<65536;++i){float v=Half(static_cast<std::uint16_t>(i));v=std::isfinite(v)?std::max(0.f,v):0;
  lut[i]=static_cast<unsigned char>(std::min(255.,std::round(255*std::pow(v/(1.+v),1/2.2))));
  lut8[i]=static_cast<unsigned char>(std::min(255.,std::round(255*std::pow(8*v/(1.+8*v),1/2.2))));
 }
 std::vector<std::uint16_t> source(w*(format==10?4:2));std::vector<unsigned char> rgb(w*3),rgb8(w*3);
 for(unsigned y=0;y<h;++y){input.read(reinterpret_cast<char*>(source.data()),std::streamsize(source.size()*2));if(!input)return 2;
  for(unsigned x=0;x<w;++x){std::uint32_t packed=0;if(format==26)std::memcpy(&packed,source.data()+x*2,4);
   for(unsigned c=0;c<3;++c){const unsigned u=format==10?source[x*4+c]:c==0?((packed&2047u)<<4):c==1?(((packed>>11)&2047u)<<4):(((packed>>22)&1023u)<<5);
    rgb[x*3+c]=lut[u];rgb8[x*3+c]=lut8[u];}
  }
  normal.write(reinterpret_cast<char*>(rgb.data()),std::streamsize(rgb.size()));bright.write(reinterpret_cast<char*>(rgb8.data()),std::streamsize(rgb8.size()));
 }
 return normal&&bright?0:2;
}
static int Run(const std::vector<std::filesystem::path>& args){
 if(args.size()>1&&args[1]=="--preview")return Preview(args);
 if(args.size()!=9){std::cerr<<"usage: verifier trace hits width height rays frame raw_offset raw_pitch\n";return 2;}
 try{
  const auto w=std::stoul(args[3].string()),h=std::stoul(args[4].string()),rays=std::stoul(args[5].string());
  const auto frame=std::stoul(args[6].string());const auto offset=std::stoull(args[7].string()),pitch=std::stoull(args[8].string());
  if(!w||!h||w>4096||h>2160||(rays!=4&&rays!=10)||pitch<w*8)throw std::runtime_error("invalid dimensions");
  std::ifstream trace(args[1],std::ios::binary),raw(args[2],std::ios::binary);if(!trace||!raw)throw std::runtime_error("cannot open input");
  std::array<std::uint32_t,512> header{};trace.read(reinterpret_cast<char*>(header.data()),2048);
  if(!trace||header[284]!=0x4d343834u||header[285]!=frame||header[286]!=w||header[287]!=h||header[288]!=rays||header[289]!=224||header[290]!=2048||header[291]!=1)throw std::runtime_error("header mismatch");
  std::vector<Ray> row(w*rays);std::vector<std::uint16_t> image(w*4);
  std::uint64_t evaluated=0,invalid=0,stampErrors=0,weightErrors=0,indexErrors=0,numerical=0,mismatch=0;
  double maxError=0;
  for(unsigned y=0;y<h;++y){
   trace.read(reinterpret_cast<char*>(row.data()),std::streamsize(row.size()*sizeof(Ray)));
   raw.seekg(std::streamoff(offset+y*pitch));raw.read(reinterpret_cast<char*>(image.data()),std::streamsize(image.size()*2));
   if(!trace||!raw)throw std::runtime_error("truncated row");
   for(unsigned x=0;x<w;++x){float sum[3]{};float scale=0;bool valid=true,any=false;
    for(unsigned r=0;r<rays;++r){const auto& a=row[x*rays+r];
     if(a.u[3]!=0&&a.u[3]!=frame)++stampErrors;
     if(!(a.u[19]&0x80000000u)){++invalid;continue;}
     ++evaluated;any=true;
     if(a.u[3]!=frame)++stampErrors;
     if((a.u[19]&0x7fffffffu)!=r)++indexErrors;
     const float pw=Float(a.u[7]),weight=Float(a.u[11]);scale=Float(a.u[23]);
     if(std::isfinite(pw)&&std::isfinite(weight)&&std::abs(pw-weight)>1e-5f)++weightErrors;
     valid=valid&&std::isfinite(weight)&&weight>=0&&weight<=(rays==4?2.00001f:1.66667f);
     for(unsigned c=0;c<3;++c){const float value=Float(a.u[8+c]);valid=valid&&std::isfinite(value)&&value>=0;sum[c]+=value*weight;}
    }
    if(!valid)++numerical;
    for(unsigned c=0;c<3;++c){const float expected=valid&&any?scale*(sum[c]/float(rays)):0,actual=Half(image[x*4+c]);
     if(!std::isfinite(expected)||!std::isfinite(actual)){++numerical;continue;}
     const double error=std::abs(double(expected)-actual);maxError=std::max(maxError,error);
     // Half-precision output quantization + float accumulation tolerance.
     if(error>std::max(1e-7, std::abs(double(expected))*.0011))++mismatch;
    }
   }
  }
  char extra=0;if(trace.read(&extra,1))throw std::runtime_error("trailing trace data");
  const bool complete=!stampErrors&&!weightErrors&&!indexErrors&&!mismatch;
  std::cout<<"{\"complete\":"<<(complete?"true":"false")<<",\"frame\":"<<frame<<",\"evaluated_rays\":"<<evaluated<<",\"invalid_receiver_rays\":"<<invalid<<",\"stamp_errors\":"<<stampErrors<<",\"weight_errors\":"<<weightErrors<<",\"ray_index_errors\":"<<indexErrors<<",\"numerical_anomalies\":"<<numerical<<",\"gi_mismatched_channels\":"<<mismatch<<",\"max_gi_absolute_error\":"<<maxError<<"}\n";
  return complete?0:1;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv){std::vector<std::filesystem::path> a;for(int i=0;i<argc;++i)a.emplace_back(argv[i]);return Run(a);}
#else
int main(int argc,char** argv){std::vector<std::filesystem::path> a;for(int i=0;i<argc;++i)a.emplace_back(argv[i]);return Run(a);}
#endif
