#include "functions.h"

int main(int argc, char *argv[]) {
    Args* a;
    unsigned long long int n, p, chunk = 500, k, i;
    pthread_mutex_t mutex;

    if(argc != 3) {
        printf("Usage %s p n\n", argv[0]);
        return -1;
    }

    if (!(sscanf(argv[1], "%llu", &p) == 1 && sscanf(argv[2], "%llu", &n) == 1)) {
        printf("Неправильный ввод\n");
        return -2;
    }
    
    

    if(n <= 0 || p <= 0) {
        printf("Неправильные аргументы\n");
        return -3;
    }


    a = new Args[p];
    if(a == nullptr) {
        printf("Не выделилась память под массив потоков\n");
        return -4;
    }

    pthread_mutex_init(&mutex, nullptr);
    
    for(k = 0; k < p; k++) {
        a[k].n = n;
        a[k].k = k;
        a[k].p = p;
        a[k].chunk = chunk;
        a[k].mutex = &mutex;
    }
    
    double t = get_full_time();

    for(i = 1; i < p; i++) {
        if(pthread_create(&a[i].tid, nullptr, thread_func, a + i)) {
            printf("Не создался поток с номером %llu\n", i);
            delete[] a;
            return -5;
        }
    }

    a[0].tid = pthread_self();

    thread_func(a + 0);
    

    for (i = 1; i < p; i++) pthread_join(a[i].tid, nullptr);
        
    t = get_full_time() - t;

    printf("Result = %llu\n", a[0].res);


    for (i = 0; i < p; i++) printf("\nThread %3llu time = %lf", i, a[i].cpu_time);
    printf("\n\n     Total time = %lf\n", t);
        

    delete[] a;


    return 0;
}
