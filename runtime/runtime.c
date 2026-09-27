#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
void nexus_print_i64(int64_t x){printf("%lld\n",(long long)x);}
void nexus_print_bool(bool x){puts(x?"true":"false");}
void nexus_print_str(const char* x){puts(x?x:"null");}
void* nexus_alloc(size_t n){return malloc(n);}
void nexus_free(void* p){free(p);}
