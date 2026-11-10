#include "../src/relax_quality.h"
#include <cassert>
int main(){using namespace control_relax;
 assert(requested==Profile::Balanced);unsigned manual=0;
 manual=Cycle(manual);assert(manual==0&&requested==Profile::Original);
 manual=Cycle(manual);assert(manual==4&&requested==Profile::Original);
 manual=Cycle(manual);assert(manual==0&&requested==Profile::Balanced);
 manual=Cycle(1);assert(manual==0&&requested==Profile::Balanced);
 assert(!Valid(static_cast<Profile>(99)));
}
