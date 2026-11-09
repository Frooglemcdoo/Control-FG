#pragma once
#include "pt_owned_scene.h"
#include "frozen_geometry.h"
#include <tuple>
namespace gi_reuse {
// CPU conversion records only: never caches a scene cut, ownership lease or
// GPU bytes. The host serializes access with its live-owner lock. Every hit
// in a new transaction rechecks unique current owners before fresh leases.
// Begin must be called at each owner-locked admission; ownership cannot mutate
// within that transaction (the same constraint as the original local cache).
class GIValidatedMappingCache {
 using U64=std::uint64_t;
public:
 struct Key {
  std::array<U64,21> identity{};
  std::array<unsigned char,PTShaderTableAdapter::NativeStride> source{};
  bool operator==(const Key& b)const{return identity==b.identity&&source==b.source;}
 };
 static Key MakeKey(U64 address,U64 token,U64 revision,unsigned geometry,
  const PTOwnedScene::Triangle& t,const GIFrozenGeometry* frozen,const unsigned char* source){
  Key key;key.identity={address,token,revision,geometry,t.indices,t.positions,t.positionStride,
   t.indexToken,t.positionToken,t.indexStride,t.indexCount,t.vertexCount,U64(frozen!=nullptr)};
  if(frozen){const auto& i=frozen->indices;const auto& p=frozen->positions;
   key.identity[13]=i.original;key.identity[14]=i.bytes;key.identity[15]=i.address;key.identity[16]=i.capacity;
   key.identity[17]=p.original;key.identity[18]=p.bytes;key.identity[19]=p.address;key.identity[20]=p.capacity;}
  std::memcpy(key.source.data(),source,key.source.size());return key;
 }
private:
 struct Entry {
  bool valid=false;U64 validatedTransaction=0;Key key;std::weak_ptr<void> frozenOwner;
  PTOwnedScene::Index::Resolved original{},mapped{};PTShaderTableAdapter::Bytes record{};
 };
 static constexpr std::size_t Slots=16384;
 std::vector<Entry> entries_;
 U64 native_=0,producer_=0,transaction_=0;PTShaderTableAdapter::Exports exports_{};
 static std::size_t Slot(const Key& key){
  // Hash the stable BLAS identity and local-root words. Remaining key bytes
  // are compared exactly; omitting them from the hash can only cause a miss.
  U64 hash=key.identity[1]^(key.identity[2]*0x9e3779b97f4a7c15ull)^(key.identity[3]*0xbf58476d1ce4e5b9ull);
  for(unsigned offset=32;offset<88;offset+=8){
   U64 word=0;std::memcpy(&word,key.source.data()+offset,8);
   hash^=word+0x9e3779b97f4a7c15ull+(hash<<6)+(hash>>2);
  }
  hash=(hash^(hash>>30))*0xbf58476d1ce4e5b9ull;
  hash=(hash^(hash>>27))*0x94d049bb133111ebull;
  hash^=hash>>31;
  return static_cast<std::size_t>(hash)&(Slots-1);
 }
 static void Epoch(PTSceneAdmission::Geometry& g,U64 epoch){
  g.epoch=g.indices.allocation.epoch=g.positions.allocation.epoch=g.attributes.allocation.epoch=epoch;
 }
public:
 void Begin(U64 native,U64 producer,const PTShaderTableAdapter::Exports& exports){
  if(native_!=native||producer_!=producer||exports_.nativeOpaque!=exports.nativeOpaque||
   exports_.nativeAlpha!=exports.nativeAlpha||exports_.privateOpaque!=exports.privateOpaque||exports_.privateAlpha!=exports.privateAlpha){
   entries_.clear();native_=native;producer_=producer;exports_=exports;
  }
  if(++transaction_==0){entries_.clear();transaction_=1;}
  if(entries_.empty())entries_.resize(Slots);
 }
 bool Find(const Key& key,const GIFrozenGeometry* frozen,const PTOwnedScene::Index& owners,
  U64 epoch,PTOwnedScene::Index::Resolved& mapped,PTShaderTableAdapter::Bytes& record,bool& acrossTransaction) {
  if(entries_.empty()||!epoch)return false;
  auto& e=entries_[Slot(key)];if(!e.valid||!(e.key==key))return false;
  acrossTransaction=e.validatedTransaction!=transaction_;
  if(acrossTransaction){
   if(frozen){const auto owner=e.frozenOwner.lock();
    if(!owner||!frozen->owner||owner.get()!=frozen->owner.get()||
       owner.owner_before(frozen->owner)||frozen->owner.owner_before(owner))return false;}
   if(!owners.StillOwns(e.original))return false;
  }
  e.validatedTransaction=transaction_;
  mapped=e.mapped;Epoch(mapped.geometry,epoch);record=e.record;return true;
 }
 void Store(const Key& key,const GIFrozenGeometry* frozen,
  const PTOwnedScene::Index::Resolved& original,const PTOwnedScene::Index::Resolved& mapped,
  const PTShaderTableAdapter::Bytes& record){
  auto& e=entries_[Slot(key)];e.valid=false;e.key=key;
  e.frozenOwner=frozen?frozen->owner:std::shared_ptr<void>{};
  e.original=original;e.mapped=mapped;e.record=record;e.validatedTransaction=transaction_;e.valid=true;
 }
};
}
