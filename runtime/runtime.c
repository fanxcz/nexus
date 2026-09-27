#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
void nexus_print_i64(int64_t x){printf("%lld\n",(long long)x);}
void nexus_print_f64(double x){printf("%.15g\n",x);}
void nexus_print_bool(bool x){puts(x?"true":"false");}
void nexus_print_str(const char* x){puts(x?x:"null");}
void* nexus_alloc(size_t n){return malloc(n);}
void nexus_free(void* p){free(p);}
void nexus_bounds_check(int64_t index,int64_t size){if(index<0||index>=size){fprintf(stderr,"Nexus runtime error: array index %lld out of bounds for size %lld\n",(long long)index,(long long)size);exit(1);}}
char* nexus_str_concat(const char* a,const char* b){if(!a)a="";if(!b)b="";size_t la=strlen(a),lb=strlen(b);char* out=(char*)malloc(la+lb+1);if(!out)return NULL;memcpy(out,a,la);memcpy(out+la,b,lb);out[la+lb]=0;return out;}
bool nexus_str_equal(const char* a,const char* b){if(a==b)return true;if(!a||!b)return false;return strcmp(a,b)==0;}
