#include "../src/native_material_diff.h"
#include <cassert>
#include <thread>
#include <cstdio>
int main(){
    using namespace native_material_diff;
    std::uint32_t before[Words]{},after[Words]{};
    for(unsigned word=0;word<Words;++word){
        after[word]=42;const auto e=Compare(1,7,2,3,4,5,6,before,after);
        assert(e.changes==1&&e.offset[0]==word*4&&e.before[0]==0&&e.after[0]==42);
        assert(e.mask[word/64]==(std::uint64_t(1)<<(word%64))&&e.mask[1-word/64]==0);
        after[word]=0;
    }
    for(auto& value:after)value=42;
    auto e=Compare(1,7,2,3,4,5,6,before,after);assert(e.changes==Words&&e.offset[7]==28&&e.committedDerived==42);
    static Queue q;
    for(unsigned i=0;i<Capacity;++i){e.entry=i;assert(q.Push(e));}
    assert(!q.Push(e)&&q.dropped==1);
    for(unsigned i=0;i<Capacity;++i){assert(q.Pop(e)&&e.entry==i);}
    assert(!q.Pop(e));
    std::atomic<unsigned> finished{0},accepted{0};std::thread writers[4];
    for(unsigned t=0;t<4;++t)writers[t]=std::thread([&,t]{for(unsigned i=0;i<10000;++i){Event item;item.entry=t*10000+i;item.oldHash=item.entry;item.newHash=~item.oldHash;if(q.Push(item))++accepted;}++finished;});
    unsigned read=0;
    while(finished!=4){if(q.Pop(e)){assert(e.newHash==~e.oldHash&&e.entry==e.oldHash);++read;}}
    for(auto& writer:writers)writer.join();
    while(q.Pop(e)){assert(e.newHash==~e.oldHash&&e.entry==e.oldHash);++read;}
    assert(read==accepted&&read+q.dropped.load()-1==40000);
    std::puts("PASS material diff: all 84 word offsets, masks, examples, full/wrapped queue, concurrent bounded publication and drop accounting");
}
