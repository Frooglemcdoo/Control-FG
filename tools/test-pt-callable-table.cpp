#include "../src/pt_callable_table.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    for(auto count:{1ull,2ull,37ull,131072ull}){
        const auto begin=PTCallableTable::Offset(count);
        assert(begin%64==0&&begin==128+count*128);
        assert(PTCallableTable::Bytes(count,false)==begin);
        assert(PTCallableTable::Bytes(count,true)==begin+64);
        assert(begin+PTCallableTable::RecordBytes<=PTCallableTable::Bytes(count,true));
        assert(PTCallableTable::RecordBytes%32==0);
    }
    puts("PASS: optional callable SBT is64-byte aligned, non-overlapping, bounded; legacy table size unchanged");
}
