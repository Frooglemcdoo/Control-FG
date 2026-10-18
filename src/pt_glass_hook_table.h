#pragma once
#include <array>
#include <mutex>
namespace PTGlass {
// Each native/interposer vtable owns its own predecessor. Never call a saved
// method from one implementation on an object with another implementation.
class HookTable {
 struct Entry {void** table=nullptr;void* original=nullptr;};
 std::array<Entry,32> entries{};std::mutex mutex;
public:
 void* Find(void** table) {
  std::lock_guard<std::mutex> guard(mutex);
  for(const auto& e:entries)if(e.table==table)return e.original;
  return nullptr;
 }
 template<class Install> bool Ensure(void** table,unsigned index,void* hook,Install install) {
  if(!table||!hook)return false;
  std::lock_guard<std::mutex> guard(mutex);
  // A later hook may chain through ours. Do not install ours a second time.
  for(const auto& e:entries)if(e.table==table)return e.original!=nullptr;
  if(!table[index]||table[index]==hook)return false;
  for(auto& e:entries)if(!e.table){
   void* previous=table[index];e={table,previous};
   // The installer must either publish exactly hook, or leave previous intact.
   if(install(&table[index],previous,hook))return true;
   e={};return false;
  }
  return false;
 }
};
}
