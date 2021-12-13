#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <libpmem.h>
#include <chrono>
#include <pthread.h>
#include <time.h>
#include <string>

#define PARA_NUM 1
#define DATA_SIZE (1ul<<30)*100 // 100GB
char* data[32];
uint64_t total_data_size = (1ul<<30)*30;
uint64_t num_thread ;
uint64_t block_size;
uint64_t num_record ;

void InitPM(){
    uint64_t len_size;
    int is_mem;
    uint64_t per_log_size = (total_data_size+num_thread)/num_thread;
    std::string path = "/mnt/pmem/log";
    for(uint64_t i = 0 ; i < num_thread ; i++){
        data[i]=(char*)pmem_map_file((path+std::to_string(i)).c_str(),per_log_size,PMEM_FILE_CREATE,0666,&len_size,&is_mem);
    }
}

void ExitPM(){
    uint64_t per_log_size = total_data_size/num_thread;
    for(uint64_t i = 0 ; i < num_thread ; i++){
        pmem_unmap(data[i],per_log_size);
    }
}

void PrintCost(std::chrono::steady_clock::time_point start,const char* desc){
    auto end = std::chrono::steady_clock::now();
    auto time_diff = end - start; 
    // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(time_diff);
    // printf("%s:duration:%-20ld ns\n",desc,duration.count());
    // double iops = (double)num_record;
    // iops = iops/(duration.count());
    // iops *= 1000;
    // printf("%s iops:%-10lf\n",desc,iops);

    uint64_t data_size = total_data_size;
    data_size /= (1ul<<20);
    double throughputs = (double)data_size;
    auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
    throughputs = (1000.0*throughputs)/(seconds.count());
    printf("%s:data_size:%-20lu MB\n",desc,data_size);
    printf("%s:cost:%-20ld ms\n",desc,seconds.count());
    printf("%s throughputs:%-10lf\n",desc,throughputs);
    fflush(stdout); 
}   

void* SeqWrite(void* id){
    uint64_t thread_id =(uint64_t)id;
    char * block_data = (char*)malloc(block_size);
    memset(block_data,1,block_size);
    uint64_t num_op = num_record/num_thread;
    for(uint64_t i = 0 ; i  <  num_op ; i++){
        pmem_memcpy_persist(data[thread_id]+i*block_size,block_data,block_size);
    }
    free(block_data);
    return NULL;
}

void* RndWrite(void* id){
    uint64_t thread_id =(uint64_t)id;
    char * block_data = (char*)malloc(block_size);
    memset(block_data,1,block_size);
    uint64_t num_op = num_record/num_thread;
    uint64_t offset;
    for(uint64_t i = 0 ; i  <  num_op ; i++){
        offset = rand()%num_op;
        // printf("Offset:%lu\n",offset);
        pmem_memcpy_persist(data[thread_id]+offset*block_size,block_data,block_size);
    }
    free(block_data);
    return NULL;
}


void* SeqRead(void* id){
    uint64_t thread_id =(uint64_t)id;
    char * block_data = (char*)malloc(block_size);
    uint64_t num_op = num_record/num_thread;
    for(uint64_t i = 0 ; i  <  num_op ; i++){
        memcpy(block_data,data[thread_id]+i*block_size,block_size);
    }
    free(block_data);
    return NULL;
}

void* RndRead(void* id){
    uint64_t thread_id =(uint64_t)id;
    char * block_data = (char*)malloc(block_size);
    memset(block_data,1,block_size);
    uint64_t num_op = num_record/num_thread;
    uint64_t offset;
    for(uint64_t i = 0 ; i  <  num_op ; i++){
        offset = rand()%num_op;
        // printf("Offset:%lu\n",offset);
        memcpy(block_data,data[thread_id]+offset*block_size,block_size);
    }
    free(block_data);
    return NULL;
}


void Test(std::string desc,void*(*func)(void*)){
    srand(time(NULL));
    for(num_thread = 1 ; num_thread <= 32 ; num_thread *= 2){
        printf("NUM_THREAD:%lu\n",num_thread);
        InitPM();
        for(block_size = 64; block_size <= 8*1024 ; block_size*=2){
            num_record = total_data_size/block_size;
            printf("num_record:%lu\n",num_record);
            printf("Block_size:%lu\n",block_size);
            pthread_t threads[num_thread];
            auto start = std::chrono::steady_clock::now();
            for(uint64_t i = 0 ; i < num_thread ; i++){
                pthread_create(&threads[i],NULL,func,(void*)i);
            }
            
            for(uint64_t i = 0 ; i < num_thread ; i++){
                pthread_join(threads[i],NULL);
            }
            PrintCost(start,(desc+"+block_size"+std::to_string(block_size)+"+num_thread"+std::to_string(num_thread)).c_str());
        }
        ExitPM();
    }
}


int main(int argc,char* argv[]){
    // Test("SeqWrite",SeqWrite);
    // Test("RndWrite",RndWrite);
    Test("SeqRead",SeqRead);
    Test("RndRead",RndRead);
}