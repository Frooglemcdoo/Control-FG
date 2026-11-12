#include "../src/gi_reuse/frozen_attempt_policy.h"
#include <cassert>
int main(){
 gi_reuse::FrozenAttemptPolicy policy;
 for(unsigned i=0;i<7;++i)assert(!policy.Rejected());
 policy.Succeeded();
 for(unsigned i=0;i<7;++i)assert(!policy.Rejected());
 assert(policy.Rejected());assert(policy.Stopped());
 policy.Succeeded();assert(policy.Stopped());assert(policy.Rejected());
 // Recovered unguided completions count just like preparation rejection.
 gi_reuse::FrozenAttemptPolicy mixed;
 for(unsigned i=0;i<7;++i)assert(!mixed.Rejected());
 assert(mixed.Rejected());assert(mixed.Stopped());
 gi_reuse::FrozenAttemptPolicy fatal;
 fatal.Stop();fatal.Succeeded();assert(fatal.Stopped());
}
