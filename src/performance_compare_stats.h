#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
namespace control_compare {
inline bool RepeatStable(double before,double after) noexcept {return std::isfinite(before)&&std::isfinite(after)&&before>0&&after>0&&std::abs(after/before-1)<=0.05;}
struct Statistics {double mean=0,p50=0,p95=0,p99=0,low1=0;unsigned count=0;};
inline Statistics Summarize(std::vector<double> values){
 values.erase(std::remove_if(values.begin(),values.end(),[](double v){return !std::isfinite(v)||v<=0;}),values.end());
 Statistics s;s.count=unsigned(values.size());if(values.empty())return s;
 std::sort(values.begin(),values.end());for(double v:values)s.mean+=v;s.mean/=values.size();
 auto q=[&](double p){return values[(std::min)(values.size()-1,size_t(std::ceil(p*values.size())-1))];};
 s.p50=q(.5);s.p95=q(.95);s.p99=q(.99);
 const auto n=(std::max)(size_t(1),size_t(std::ceil(values.size()*.01)));double tail=0;
 for(size_t i=values.size()-n;i<values.size();++i){tail+=values[i];}
 s.low1=1000./(tail/n);return s;
}
}
