#include "../src/rt_spot_capture_layout.h"
#include "../src/rt_spot_capture_table.h"
#include "../src/rt_spot_capture_bounds.h"
#include "../src/rt_spot_table_coverage.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace rt_spot_capture;
    assert(ContainsGpuRange(4096, 512, 4096, 512));
    assert(ContainsGpuRange(4096, 512, 4160, 96));
    assert(!ContainsGpuRange(4096, 512, 4095, 1));
    assert(!ContainsGpuRange(4096, 512, 4608, 1));
    assert(!ContainsGpuRange(UINT64_MAX - 31, 64, UINT64_MAX - 31, 1));
    assert(!ContainsGpuRange(4096, 512, 4096, 0));
    assert(NativeHitTableLayout(4096, 192, 96));
    assert(!NativeHitTableLayout(4097, 192, 96));
    assert(!NativeHitTableLayout(4096, 193, 96));
    assert(!NativeHitTableLayout(4096, 192, 64));
    TableCoverage coverage;
    assert(coverage.Add(0,128,512));assert(coverage.Add(256,256,512));
    assert(!coverage.Covers(0,512));assert(coverage.Add(128,128,512));assert(coverage.Covers(0,512));
    assert(!coverage.Add(UINT64_MAX-1,4,512));assert(!coverage.Covers(UINT64_MAX-1,4));
    coverage.Clear();assert(!coverage.Covers(0,512));
    assert(coverage.Add(256,256,512));assert(!coverage.Covers(0,512));
    for(unsigned i=0;i<128;++i)coverage.ranges[i]={i*2,i*2+1};
    coverage.count=128;
    assert(!coverage.Add(300,1,512));assert(!coverage.count);
    unsigned char ids[2][32]{};ids[0][0]=1;ids[1][0]=2;
    unsigned char table[96*4]{};std::memcpy(table,ids[0],32);std::memcpy(table+96,ids[1],32);table[96*3]=3;
    const auto classified=CountTableIdentifiers(table,sizeof(table),ids[0],ids[1]);
    assert(classified.group0==1 && classified.group1==1 && classified.empty==1 && classified.unknown==1);
    static_assert(offsetof(CaptureResult, origin) == 32);
    static_assert(offsetof(CaptureResult, direction) == 48);
    static_assert(offsetof(CaptureResult, pixelX) == 64);
    std::vector<IdentifierReplacement> replacements(2);
    for (unsigned i = 0; i < 2; ++i) {
        replacements[i].native.fill(static_cast<unsigned char>(10 + i));
        replacements[i].capture.fill(static_cast<unsigned char>(20 + i));
    }
    std::vector<unsigned char> source(96 * 4);
    for (unsigned i = 0; i < source.size(); ++i) source[i] = static_cast<unsigned char>(i);
    for (unsigned i = 0; i < 4; ++i)
        std::memcpy(source.data() + i * 96, replacements[i % 2].native.data(), 32);
    const auto original = source;
    std::vector<unsigned char> destination;
    assert(TranslateHitTable(source, replacements, destination));
    assert(source == original && destination.size() == source.size());
    for (unsigned i = 0; i < 4; ++i) {
        assert(!std::memcmp(destination.data() + i * 96, replacements[i % 2].capture.data(), 32));
        assert(!std::memcmp(destination.data() + i * 96 + 32, source.data() + i * 96 + 32, 64));
    }
    const auto accepted = destination;
    source[96 * 3] = 99;
    assert(!TranslateHitTable(source, replacements, destination));
    assert(destination == accepted); // A late unknown ID cannot publish a partial table.
    source = original;
    source.pop_back();
    assert(!TranslateHitTable(source, replacements, destination));
    source = original;
    replacements.push_back(replacements[0]);
    assert(!TranslateHitTable(source, replacements, destination));
    replacements.pop_back();
    replacements[0].capture.fill(0);
    assert(!TranslateHitTable(source, replacements, destination));
    assert(!TranslateHitTable({}, {}, destination));
    assert(destination == accepted);
    std::cout << "PASS shared capture ABI, exact local-root retention and atomic table rejection\n";
}
