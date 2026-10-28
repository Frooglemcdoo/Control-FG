#include "../src/rr_compact_distance_policy.h"
#include <cassert>
#include <initializer_list>
#include <cstdio>
int main(){using namespace rr_compact_distance;
 assert(!Parse(nullptr)&&!Parse("")&&Parse("on"));
 for(auto s:{"off","ON","invalid","on "})assert(!Parse(s));
 assert(!enabled&&!installed&&!lastBound&&!evaluations&&!boundEvaluations);
 puts("PASS compact distance default/on/off and fail-closed invalid policy");
}
