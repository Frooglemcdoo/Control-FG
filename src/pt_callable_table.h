#pragma once
#include <cstdint>
namespace PTCallableTable {
// Hit records remain at128 with128-byte stride. Optional callable record starts
// after the hit table, already aligned to the DXR64-byte table requirement.
constexpr std::uint64_t Offset(std::uint64_t hitRecords){return 128+hitRecords*128;}
constexpr std::uint64_t Bytes(std::uint64_t hitRecords,bool enabled){return Offset(hitRecords)+(enabled?64:0);}
constexpr std::uint64_t RecordBytes=32;
}
