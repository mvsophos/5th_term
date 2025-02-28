#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
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
    char *filename = nullptr;
    int count = 0;
};

double get_full_time() {
    struct timeval buf;
    gettimeofday(&buf, 0);
    return buf.tv_sec + buf.tv_usec * 1.e-6;
}

// чтение массива из файла
int read_array(double *a, int n, char *filename) {
    int i;
    FILE *fp = fopen(filename, "r");
    if (fp == nullptr) return -1;
    for (i = 0; i < n; i++) {
        if (fscanf(fp, "%lf", &a[i]) != 1) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// вывод массива
void print_array(double *a, int n) {
    // printf("Массив:     ");
    for(int i = 0; i < n; i++) printf("%8.5le ", a[i]);
    printf("\n");
}

//////////////////////////////////////

void reduce_sum(int p, int *a = nullptr);
void reduce_sum(int p, int *a);
void solve(Args *ptr);

void *thread_func(void *ptr) {
    Args *arg = (Args *) ptr;
    FILE *file = nullptr;
    int p = arg->p;
    int k = arg->k;
    
    char *filename = arg->filename;
    double *a = arg->a;
    int begin = arg->begin;
    int end   = arg->end;
    int n = arg->n;

    reduce_sum(p);
    if (filename != nullptr) {
        int flag = 0;
        file = fopen(filename, "r");
        if (file == nullptr) {
            arg->error = io_status::error_open;
            return nullptr;
        }
        if (k == 0) flag = read_array(a, n, filename);
        reduce_sum(p, &flag);
        
        if (flag > 0) {
            arg->error = io_status::error_read;
            return nullptr;
        }
        else arg->error = io_status::success;
    }
    else return nullptr;

    if (arg->begin > 0) arg->start = a[begin - 1];
    if (arg->end < n - 1) arg->finish = a[end + 1];

    reduce_sum(p);
    arg->time = get_full_time();
    solve(arg);
    arg->time = get_full_time() - arg->time;
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
        if (end > begin) { // если у потока в обработке участок длины 1
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
    else prev = a[0];

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
    reduce_sum(ptr->p, &ptr->count);
}



void reduce_sum(int p, int *a) {
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

    if (r != a) *a = *r;
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



