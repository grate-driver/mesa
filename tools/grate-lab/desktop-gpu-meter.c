#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static uint32_t u32(const unsigned char*p){uint32_t x;memcpy(&x,p,4);return x;}
static uint64_t u64(const unsigned char*p){uint64_t x;memcpy(&x,p,8);return x;}
static void strfield(char*out,size_t cap,const unsigned char*raw,unsigned size,unsigned pos){
 if(pos+4>size){snprintf(out,cap,"invalid");return;}
 uint32_t loc=u32(raw+pos);unsigned offset=loc&65535,len=loc>>16;
 if(offset>=size||len>size-offset){snprintf(out,cap,"invalid");return;}
 if(len>=cap)len=cap-1;
 memcpy(out,raw+offset,len);out[len]=0;
}
int main(int argc,char**argv){
 if(argc!=4)return 2;
 int event=atoi(argv[1]),pid=atoi(argv[2]);double duration=atof(argv[3]);if(event<=0||pid<=0||duration<=0||duration>10)return 2;
 struct perf_event_attr attr={.type=PERF_TYPE_TRACEPOINT,.size=sizeof(attr),.config=event,.sample_period=1,.sample_type=PERF_SAMPLE_TID|PERF_SAMPLE_TIME|PERF_SAMPLE_RAW,.disabled=1,.inherit=0,.wakeup_events=1};
 int fd=syscall(__NR_perf_event_open,&attr,pid,-1,-1,PERF_FLAG_FD_CLOEXEC);if(fd<0){perror("perf_event_open");return 3;}
 long page=sysconf(_SC_PAGESIZE);size_t bytes=page*65;void*map=mmap(NULL,bytes,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(map==MAP_FAILED){perror("mmap");close(fd);return 4;}
 struct perf_event_mmap_page*meta=map;size_t ring_size=meta->data_size;unsigned char*ring=(unsigned char*)map+meta->data_offset;
 if(!ring_size||ring_size>bytes-page){fprintf(stderr,"Invalid ring\n");return 5;}
 if(ioctl(fd,PERF_EVENT_IOC_RESET,0)||ioctl(fd,PERF_EVENT_IOC_ENABLE,0)){perror("enable");return 6;}
 printf("TRACE READY pid=%d event=%d seconds=%.1f\n",pid,event,duration);fflush(stdout);
 double end=now()+duration;uint64_t tail=0,total=0,lost=0,gr3d=0,gr2d=0;int disabled=0;
 for(;;){
  if(!disabled&&now()>=end){ioctl(fd,PERF_EVENT_IOC_DISABLE,0);disabled=1;}
  uint64_t head=meta->data_head;__sync_synchronize();
  while(tail<head){
   struct perf_event_header h;unsigned char header[sizeof(h)];for(unsigned i=0;i<sizeof(h);i++)header[i]=ring[(tail+i)%ring_size];memcpy(&h,header,sizeof(h));
   if(h.size<sizeof(h)||h.size>8192||tail+h.size>head){fprintf(stderr,"Invalid record size=%u\n",h.size);return 7;}
   unsigned char record[8192];for(unsigned i=0;i<h.size;i++)record[i]=ring[(tail+i)%ring_size];
   if(h.type==PERF_RECORD_SAMPLE&&h.size>=28){
    uint32_t sample_pid=u32(record+8),tid=u32(record+12),size=u32(record+24);const unsigned char*raw=record+28;
    if(size<=(unsigned)h.size-28&&size>=48){
     char name[128],dev[128];strfield(name,sizeof(name),raw,size,8);strfield(dev,sizeof(dev),raw,size,20);uint64_t client=u64(raw+40);
     total++;if(strstr(name,"3d")||strstr(dev,"3d"))gr3d++;if(strstr(name,"2d")||strstr(dev,"2d"))gr2d++;
     if(total<=500)printf("JOB pid=%u tid=%u client=%llu ring=%s dev=%s fence=%llu:%llu\n",sample_pid,tid,(unsigned long long)client,name,dev,(unsigned long long)u64(raw+24),(unsigned long long)u64(raw+32));
    }
   }else if(h.type==PERF_RECORD_LOST&&h.size>=24){lost+=u64(record+16);}
   tail+=h.size;
  }
  __sync_synchronize();meta->data_tail=tail;
  if(disabled)break;
  struct pollfd p={.fd=fd,.events=POLLIN};poll(&p,1,100);
 }
 printf("TRACE DONE jobs=%llu gr3d=%llu gr2d=%llu lost=%llu\n",(unsigned long long)total,(unsigned long long)gr3d,(unsigned long long)gr2d,(unsigned long long)lost);fflush(stdout);
 munmap(map,bytes);close(fd);return lost?8:0;
}
