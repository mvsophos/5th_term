#include <cstdio>
#include <cstdlib>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <cmath>
#include <iostream>

#define eps 1e-15

enum class io_status {
    undef, error_open, error_read, success
};

class Args {
public:
    pthread_t tid = -1;
    int k = 0;
    int p = 0;
    double *a = nullptr;
    double *a_help = nullptr;
    int n1 = 0;
    int n2 = 0;
    
    double cpu_time = 0;

    io_status status = io_status::undef;
};



double get_full_time() {
    struct timeval buf;
    gettimeofday(&buf, NULL);
    return buf.tv_sec + buf.tv_usec * 1.e-6;
}

double get_cpu_time() {
    struct rusage buf;
    getrusage(RUSAGE_THREAD, &buf);
    return buf.ru_utime.tv_sec + buf.ru_utime.tv_usec * 1.e-6;
}



void reduce_sum(int p, double *a = nullptr, int n = 0);

void reduce_sum(int p, double *a, int n) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static double *r = nullptr;
    int i;
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else for(i = 0; i < n; i++) r[i] += a[i];
    t_in++;
    if (t_in >= p) {
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while (t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a) for (i = 0; i < n; i++) a[i] = r[i];
    t_out++;
    if (t_out >= p) {
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m);
    pthread_mutex_unlock(&m);
}

int read_array(char *filename, double *a, int n) {
    FILE *file = nullptr;

    file = fopen(filename, "r");
    if (file == nullptr) {
        printf("Не открылся файл %s\n", filename);
        return -1;
    }

    for(int i = 0; i < n; i++) {
        if(fscanf(file, "%lf", &a[i]) != 1) {
            printf("Не прочитался %dй элемент\n", i);
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

double more_eq(double *a, int n2, int i, int j) {
    if (    a[i * n2 + j] + eps > a[(i + 1) * n2 + j] && 
            a[i * n2 + j] + eps > a[(i - 1) * n2 + j] &&
            a[i * n2 + j] + eps > a[i * n2 + j + 1]   && 
            a[i * n2 + j] + eps > a[i * n2 + j - 1]   )
        return 1;
    else if (   a[i * n2 + j] < a[(i + 1) * n2 + j] + eps && 
                a[i * n2 + j] < a[(i - 1) * n2 + j] + eps &&
                a[i * n2 + j] < a[i * n2 + j + 1] + eps   && 
                a[i * n2 + j] < a[i * n2 + j - 1] + eps   )
        return -1;
    else return a[i * n2 + j];
}

