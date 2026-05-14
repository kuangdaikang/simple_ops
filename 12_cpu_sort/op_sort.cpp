#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
static double NowUs() { struct timeval tv; gettimeofday(&tv,0); return tv.tv_sec*1e6+tv.tv_usec; }
int main() {
    constexpr uint32_t N=4096*9;
    std::vector<float> d(N);
    srand(42); for(uint32_t i=0;i<N;i++) d[i]=(float)rand()/RAND_MAX*10.f-5.f;
    printf("========== [CPU] Sort N=%u ==========\n",N);
    double t0=NowUs(); std::sort(d.begin(),d.end()); double t1=NowUs();
    printf("CPU time: %.2f us\n",t1-t0);
    bool ok=true; for(uint32_t i=1;i<N;i++) if(d[i]<d[i-1]){ok=false;break;}
    printf("Monotonic: %s\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
