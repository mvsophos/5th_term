#include <stdio.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include <string.h>
#include <math.h>

enum class io_status {
    undef, error_open, error_read, success
};

class Args {
public:
    pthread_t tid = -1;
    int p = 0;
    int n = 0;
    io_status error = io_status::undef;
    int k = 0;
    
    double *a = nullptr;
    int begin = 0;
    int end = 0;
    double start = 0;
    double finish = 0;
    double time = 0;
    char *name = nullptr;
    int count = 0;
};

double get_cpu_time() {
    struct rusage buf;
    getrusage(RUSAGE_THREAD, &buf);
    return buf.ru_utime.tv_sec + buf.ru_utime.tv_usec * 1.e-6;
}

double get_full_time() {
    struct timeval buf;
    gettimeofday(&buf, 0);
    return buf.tv_sec + buf.tv_usec * 1.e-6;
}

// чтение массива из файла
int read_array(double *a, int n, const char *name) {
    int i;
    FILE *fp = fopen(name, "r");
    if (fp == nullptr) return -1;
    for (i = 0; i < n; i++) {
        if (fscanf(fp, "%lf", a + i) != 1) break;
    }
    if (i < n) {
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

// вывод массива
void print_array(double *a, int n) {
    for(int i = 0; i < n; i++) printf("%8.5lf ", a[i]);
    printf("\n");
}

//////////////////////////////////////

void reduce_summa_int(int p, int *a = nullptr);
void reduce_summa_int(int p, int *a);
void reduce_sum(Args *ptr);
void solve(Args *ptr);

void *thread_func(void *ptr) {
    Args *arg = (Args*)ptr;
    int p = arg->p;
    int k = arg->k;
    
    char *name = arg->name;
    double *a = arg->a;
    int begin = arg->begin;
    int end = arg->end;
    int n = arg->n;

    reduce_summa_int(p);
    if (name != nullptr) {
        int ret = 0;
        if (k == 0) {
            ret = read_array(a, n, name);
        }
        reduce_summa_int(p, &ret);
        if (ret < 0) {
            arg->error = io_status::error_open;
            return nullptr;
        }
        else if (ret > 0) {
            arg->error = io_status::error_read;
            return nullptr;
        }
        else arg->error = io_status::success;
    }
    else return nullptr;

    if (arg->begin > 0) arg->start = a[begin - 1];
    if (arg->end < n - 1) arg->finish = a[end + 1];

    if (k == 0) {
        printf("Массив:     ");
        print_array(a, n);
    }
    reduce_summa_int(p);
    arg->time = get_cpu_time();
    solve(arg);
    arg->time = get_cpu_time() - arg->time;
    return nullptr;
}

double osborn(double elem1, double middle, double elem2) {
    if (((elem1 > middle && elem2 > middle) || (elem1 < middle && elem2 < middle)) && (elem1 * elem2 > 1e-15)) return sqrt(elem1 * elem2);
    else return middle;
}

void solve(Args *ptr) {
    int i;
    int n = ptr->n;
    double *a = ptr->a;
    int begin = ptr->begin;
    int end = ptr->end;
    double start = ptr->start;
    double finish = ptr->finish;
    double s, prev;
    if (begin > 0) {
        if (end > begin) {
            s = osborn(start, a[begin], a[begin + 1]);
            prev = a[begin];
            a[begin] = s;
            ptr->count += 1;
        }
        else if (end < n - 1) {
            s = osborn(start, a[begin], finish);
            prev = a[begin];
            a[begin] = s;
            ptr->count += 1;
        }
        else prev = a[begin];
    }
    else prev = a[begin];

    for (i = begin + 1; i < end; i++) {
        s = osborn(prev, a[i], a[i + 1]);
        prev = a[i];
        a[i] = s;
        ptr->count += 1;
    }
    if (i == end) {
        if (end < n - 1) {
            s = osborn(prev, a[i], finish);
            a[i] = s;
            ptr->count += 1;
        }
    }
    reduce_sum(ptr);
}

void reduce_sum(Args *ptr) {
    static pthread_mutex_t mutex   = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t cond_in  = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t cond_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0;
    static int t_out = 0;
    static int r = 0;
    int p = ptr->p;
    pthread_mutex_lock(&mutex);
    r += ptr->count;
    t_in++;
    if (t_in >= p) {
        t_out = 0;
        pthread_cond_broadcast(&cond_in);
    }
    else while (t_in < p) pthread_cond_wait(&cond_in, &mutex);
    ptr->count = r;
    t_out++;
    if (t_out >= p) {
        t_in = 0;
        pthread_cond_broadcast(&cond_out);
    }
    else while (t_out < p) pthread_cond_wait(&cond_out, &mutex);
    pthread_mutex_unlock(&mutex);
}

void reduce_summa_int(int p, int *a) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static int *r = nullptr;
    
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else *r += *a;
    t_in++;
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a) a = r;
    t_out++;
    if (t_out >= p){
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m);
    pthread_mutex_unlock(&m);
}

//////////////////////////////////////



