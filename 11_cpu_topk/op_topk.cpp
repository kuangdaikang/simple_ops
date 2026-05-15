#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <cstdint>   // 提供 uint32_t 等固定宽度整数类型
static double NowUs() { struct timeval tv; gettimeofday(&tv,0); return tv.tv_sec*1e6+tv.tv_usec; }
int main() {
    constexpr uint32_t N=10240, K=10;
    std::vector<float> d(N);
    srand(42); for(uint32_t i=0;i<N;i++) d[i]=(float)rand()/RAND_MAX*10.f-5.f;
    printf("========== [CPU] TopK N=%u K=%u ==========\n",N,K);
    double t0=NowUs();
    std::vector<std::pair<float,uint32_t>> idx(N);
    for(uint32_t i=0;i<N;i++) idx[i]={d[i],i};
    std::partial_sort(idx.begin(),idx.begin()+K,idx.end(),[](auto&a,auto&b){return a.first>b.first;});
    double t1=NowUs();
    printf("CPU time: %.2f us\n",t1-t0);
    for(uint32_t i=0;i<K;i++) printf("  Top[%u]: val=%.4f idx=%u\n",i,idx[i].first,idx[i].second);
    return 0;
}
