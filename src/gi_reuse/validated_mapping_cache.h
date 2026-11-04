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
  U64 hash=1469598103934665603ull;
  for(auto value:key.identity){
   value=(value^(value>>30))*0xbf58476d1ce4e5b9ull;
   value=(value^(value>>27))*0x94d049bb133111ebull;
   hash^=value^(value>>31);hash*=1099511628211ull;
  }
  // Hash every source byte; exact comparison below also covers hash collisions.
  for(auto value:key.source){hash^=value;hash*=1099511628211ull;}
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
  if(frozen){const auto owner=e.frozenOwner.lock();
   if(!owner||!frozen->owner||owner.get()!=frozen->owner.get()||
      owner.owner_before(frozen->owner)||frozen->owner.owner_before(owner))return false;}
  acrossTransaction=e.validatedTransaction!=transaction_;
  if(acrossTransaction&&!owners.StillOwns(e.original))return false;
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
