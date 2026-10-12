#pragma once
#include <array>
#include <cstdint>
#include <cstring>
// DXBC retail container checksum (not a security hash). MD5 compression with
// Microsoft's documented retail final-block layout:
// https://microsoft.github.io/hlsl-specs/proposals/infra/inf-0004-validator-hashing/
namespace PTDxbc {
inline std::uint32_t Word(const unsigned char* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
inline void Put(unsigned char* p,std::uint32_t n){for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(n>>(8*i));}
inline std::array<unsigned char,16> Checksum(const unsigned char* data,std::uint32_t bytes){
    static constexpr std::uint32_t constants[64]={
        0xd76aa478u,0xe8c7b756u,0x242070dbu,0xc1bdceeeu,0xf57c0fafu,0x4787c62au,0xa8304613u,0xfd469501u,
        0x698098d8u,0x8b44f7afu,0xffff5bb1u,0x895cd7beu,0x6b901122u,0xfd987193u,0xa679438eu,0x49b40821u,
        0xf61e2562u,0xc040b340u,0x265e5a51u,0xe9b6c7aau,0xd62f105du,0x2441453u,0xd8a1e681u,0xe7d3fbc8u,
        0x21e1cde6u,0xc33707d6u,0xf4d50d87u,0x455a14edu,0xa9e3e905u,0xfcefa3f8u,0x676f02d9u,0x8d2a4c8au,
        0xfffa3942u,0x8771f681u,0x6d9d6122u,0xfde5380cu,0xa4beea44u,0x4bdecfa9u,0xf6bb4b60u,0xbebfbc70u,
        0x289b7ec6u,0xeaa127fau,0xd4ef3085u,0x4881d05u,0xd9d4d039u,0xe6db99e5u,0x1fa27cf8u,0xc4ac5665u,
        0xf4292244u,0x432aff97u,0xab9423a7u,0xfc93a039u,0x655b59c3u,0x8f0ccc92u,0xffeff47du,0x85845dd1u,
        0x6fa87e4fu,0xfe2ce6e0u,0xa3014314u,0x4e0811a1u,0xf7537e82u,0xbd3af235u,0x2ad7d2bbu,0xeb86d391u,
    };
    static constexpr unsigned shifts[16]={7,12,17,22,5,9,14,20,4,11,16,23,6,10,15,21};
    std::array<std::uint32_t,4> state{{0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u}};
    const auto compress=[&](const unsigned char* block){
        std::uint32_t words[16];for(unsigned i=0;i<16;++i)words[i]=Word(block+4*i);
        auto a=state[0],b=state[1],c=state[2],d=state[3];
        for(unsigned i=0;i<64;++i){
            std::uint32_t f;unsigned at;
            if(i<16){f=(b&c)|(~b&d);at=i;}
            else if(i<32){f=(d&b)|(~d&c);at=(5*i+1)%16;}
            else if(i<48){f=b^c^d;at=(3*i+5)%16;}
            else {f=c^(b|~d);at=(7*i)%16;}
            const auto value=a+f+constants[i]+words[at];const auto shift=shifts[(i/16)*4+i%4];
            const auto next=b+((value<<shift)|(value>>(32-shift)));a=d;d=c;c=b;b=next;
        }
        state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;
    };
    const std::uint32_t whole=bytes&~63u,remainder=bytes%64;
    for(std::uint32_t at=0;at<whole;at+=64)compress(data+at);
    std::array<unsigned char,64> last{};
    if(remainder>=56){
        std::memcpy(last.data(),data+whole,remainder);last[remainder]=0x80;compress(last.data());last.fill(0);
    }else{
        if(remainder)std::memcpy(last.data()+4,data+whole,remainder);
        last[4+remainder]=0x80;
    }
    Put(last.data(),bytes<<3);Put(last.data()+60,(bytes<<1)|1u);compress(last.data());
    std::array<unsigned char,16> result{};for(unsigned i=0;i<4;++i)Put(result.data()+4*i,state[i]);return result;
}
}
