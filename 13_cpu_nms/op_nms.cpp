#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sys/time.h>
struct Box{float x1,y1,x2,y2,score;uint32_t idx;};
static float IoU(const Box&a,const Box&b){float ix1=std::max(a.x1,b.x1),iy1=std::max(a.y1,b.y1),ix2=std::min(a.x2,b.x2),iy2=std::min(a.y2,b.y2);float iw=std::max(0.f,ix2-ix1),ih=std::max(0.f,iy2-iy1);float inter=iw*ih,areaA=(a.x2-a.x1)*(a.y2-a.y1),areaB=(b.x2-b.x1)*(b.y2-b.y1);return inter/(areaA+areaB-inter+1e-8f);}
static double NowUs(){struct timeval tv;gettimeofday(&tv,0);return tv.tv_sec*1e6+tv.tv_usec;}
int main(){
    constexpr uint32_t N=512; constexpr float TH=0.5f;
    std::vector<Box> boxes(N);
    srand(42); for(uint32_t i=0;i<N;i++){float cx=(float)rand()/RAND_MAX,cy=(float)rand()/RAND_MAX,w=(float)rand()/RAND_MAX*.5f,h=(float)rand()/RAND_MAX*.5f;boxes[i]={cx-w,cy-h,cx+w,cy+h,(float)rand()/RAND_MAX,i};}
    printf("========== [CPU] NMS N=%u IoU=%.1f ==========\n",N,TH);
    double t0=NowUs();
    std::sort(boxes.begin(),boxes.end(),[](const Box&a,const Box&b){return a.score>b.score;});
    std::vector<uint32_t> keep; std::vector<bool> supp(N,false);
    for(uint32_t i=0;i<N;i++){if(supp[i])continue;keep.push_back(boxes[i].idx);for(uint32_t j=i+1;j<N;j++){if(supp[j])continue;if(IoU(boxes[i],boxes[j])>TH)supp[j]=true;}}
    double t1=NowUs();
    printf("CPU time: %.2f us\nKept: %zu / %u\n",t1-t0,keep.size(),N);
    printf("Verification: %s\n",(keep.size()>0&&keep.size()<N)?"PASS":"FAIL");
    return 0;
}
