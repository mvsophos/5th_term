#include <cstdio>
#include <cmath>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#define min(a, b) a < b ? a : b;
#define max(a, b) a > b ? a : b;

class Args {
public:
    unsigned long long int p = 0;
    unsigned long long int k = 0;
    pthread_t tid = -1;
    pthread_mutex_t *mutex = nullptr;

    unsigned long long int n = 0;
    unsigned long long int loc_cnt = 0;
    unsigned long long int chunk = 0;

    unsigned long long int step = 0;

    unsigned long long int res = 0;

    unsigned long long int start = 0;
    unsigned long long int end = 0;

    int errorflag = 0;

    double cpu_time = 0;
    double full_time = 0;
    double full_time1 = 0;
};



void *thread_func(void *arg);
double get_full_time(void);
double get_cpu_time(void);
unsigned long long int count_primes(unsigned long long int start, unsigned long long int end);


int reduce_min(int p, unsigned long long int *a = nullptr, int n = 0) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0;
    static int t_out = 0;
    static unsigned long long int *r = nullptr;
    int i;

    if (p <= 1)  return 0;

    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else for(i = 0; i < n; i++) r[i] = min(r[i], a[i]);  
    
    t_in++;

    if (t_in >= p) {
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);
    
    if (r != a) for(i = 0; i < n; i++) a[i] = r[i];
    
    t_out++;

    if (t_out >= p) {
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while(t_out < p) pthread_cond_wait(&c_out, &m);
    
    pthread_mutex_unlock(&m);
    return 0;
}

int reduce_max(int p, unsigned long long int *a = nullptr, int n = 0) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0;
    static int t_out = 0;
    static unsigned long long int *r = nullptr;
    int i;

    if(p <= 1)  return 0;

    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else for(i = 0; i < n; i++) r[i] = max(r[i], a[i]);  
    
    t_in++;

    if (t_in >= p) {
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);
    
    if(r != a) for(i = 0; i < n; i++) a[i] = r[i];
    
    t_out++;

    if (t_out >= p) {
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while(t_out < p) pthread_cond_wait(&c_out, &m);

    pthread_mutex_unlock(&m);
    return 0;
}

int reduce_sum(int p, unsigned long long int *a = nullptr, int n = 0) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0;
    static int t_out = 0;
    static unsigned long long int *r = nullptr;
    int i;

    if (p <= 1)  return 0;

    pthread_mutex_lock(&m);

    if (r == nullptr) r = a;
    else for (i = 0; i < n; i++) r[i] += a[i];
    
    t_in++;

    if (t_in >= p) {
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);
    
    if(r != a) for(i = 0; i < n; i++) a[i] = r[i];
    
    t_out++;

    if(t_out >= p) {
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while(t_out < p) pthread_cond_wait(&c_out, &m);
    
    pthread_mutex_unlock(&m);
    return 0;
}














static unsigned long long int global_cnt = 0;
static unsigned long long global_start = 1;


double get_full_time() {
    struct timeval time;
    gettimeofday(&time, 0);
    return time.tv_sec + time.tv_usec / 1.e6;
}

double get_cpu_time() {
    struct rusage buf;
    getrusage(RUSAGE_THREAD, &buf);
    return buf.ru_utime.tv_sec + buf.ru_utime.tv_usec / 1.e6;
}


int prime(unsigned long long int k) {
    unsigned long long int i;

    if (k < 2)                           return 0;
    if (k == 2 || k == 3)                return 1;
    if (k % 2 == 0 || k % 3 == 0)        return 0;
    for (i = 5; i * i <= k; i += 6)      if(k % i == 0 || k % (i + 2) == 0) return 0;
    
    return 1;
}

unsigned long long int count_primes(unsigned long long int start, unsigned long long int end) {
    unsigned long long int i;

    unsigned long long int cnt = 0;

    if(2 > start) {
        cnt++;
        start = 3;
    }

    if(start % 2 == 0) start++;

    for(i = start; i < end; i += 2) if (prime(i) == 1) cnt++;

    return cnt;
}


void *thread_func(void* arg) {
    Args* a = (Args*)arg;

    unsigned long long int n = a->n;
    unsigned long long int p = a->p;
    unsigned long long int k = a->k;
    unsigned long long int chunk = a->chunk;

    unsigned long long int cnt = 0;
    unsigned long long int i;
    unsigned long long int j;

    a->full_time = get_full_time();
    a->full_time1 = get_full_time();
    a->cpu_time = get_cpu_time();
    
    while (global_cnt < n) {
        a->step += 1;
        pthread_mutex_lock(a->mutex);

        a->start = global_start % 2 == 0 ? global_start + 1 : global_start;

        a->end = global_start + chunk;

        global_start += chunk;

        pthread_mutex_unlock(a->mutex);

        if (2 > a->start) cnt++;

        for (i = a->start; i < a->end; i += 2) {
            if (prime(i) == 1)
                cnt++;
        }

        a->loc_cnt = cnt;

        pthread_mutex_lock(a->mutex);

        if (a->errorflag == 1) {
            global_cnt += cnt;
            a->errorflag = 0;
            pthread_mutex_unlock(a->mutex);
            break;
        }

        global_cnt += cnt;

        if (global_cnt >= n) {
            for (j = 1; j <= k; j++) (a - j)->errorflag = 1;
            
            for (j = 1; j < p - k; j++) (a + j)->errorflag = 1;
            
            pthread_mutex_unlock(a->mutex);
            break;
        }   

        pthread_mutex_unlock(a->mutex);
        cnt = 0;
    }

    a->full_time1 = get_full_time() - a->full_time1;

    reduce_min(p, &a->start, 1);
    
    reduce_max(p, &a->end, 1);

    
    
    if (k == 0) {
        global_cnt -= count_primes(a->start, a->end);


        for(i = a->start; i < a->end; i++) {
            if(prime(i) == 1) global_cnt++;

            if(global_cnt == n) {
                a->res = i;
                break;
            }
        }
    }

    a->full_time = get_full_time() - a->full_time;
    a->cpu_time = get_cpu_time() - a->cpu_time;

    return nullptr;
}
