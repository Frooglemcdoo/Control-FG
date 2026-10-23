#include "../src/rt_spot_capture_layout.h"
#include "../src/rt_spot_capture_table.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace rt_spot_capture;
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
