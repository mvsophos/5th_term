#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <cmath>

#define eps 1e-15

enum class io_status {
    undef, error_open, error_read, success
};

class Args {
public:
    pthread_t tid = -1;
    int p = 0;
    int k = 0;
    char *name = nullptr;
    double result = 0;
    double count = 0;

    double *a = nullptr;
    double length = 0;

    double error_flag = 0;
    io_status error_type = io_status::undef;
};

void reduce_sum(int p, double *a = nullptr, int n = 0);

void reduce_sum(int p, double *a, int n){
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
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a) for (i = 0; i < n; i++) a[i] = r[i];
    t_out++;
    if (t_out >= p){
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m);
    pthread_mutex_unlock(&m);
}

int fib(double a, double b, double c) { // 1 истина, 0 ложь
    return (fabs(a - b - c) < eps) ? 1 : 0;
}

int second_step(double *A, int n) {
    int result = 0;
    int i, ml = 1, buf_len = 1;
    int exist = 0;
    double m = -1e95, buf_m = -1e95;

    int flag = 0;
    for (i = 0; i < n - 2; i++) {
        if (fib(A[i], A[i + 1], A[i + 2]) == 1) {
            exist = 1;
            if (flag == 1) {
                buf_len += 1;
                buf_m = fmax(buf_m, A[i + 2]);
            }
            else {
                flag = 1;
                buf_len = 3;
                buf_m = fmax(fmax(A[i], A[i + 1]), A[i + 2]);
            }
        }
        else if (flag == 1) {
            flag = 0;
            if (buf_len > ml) {
                ml = buf_len;
                m = buf_m;
            }
            buf_len = 1; buf_m = -1e95;
        }
    }

    if (flag == 1) {
        if (buf_len > ml) {
            ml = buf_len;
            m = buf_m;
        }
    }

    //if (exist == 1) printf("%lf %lf %d\n", m, buf_m, exist);

    if (exist == 1) {
        for (i = 0; i < n; i++) if (m < A[i]) result += 1;
    }

    return result;
}

double reading(FILE *f, io_status *error) {
    double res = 0, curr;
    while (fscanf(f, "%lf", &curr) == 1) {
        res += 1;
    }
    if (!feof(f)) {
        *error = io_status::error_read;
        return -1;
    }
    else {
        *error = io_status::success;
        return res;
    }
}

void print_array(double *a, int n) {
    for(int i = 0; i < n; i++) printf("%8.5lf ", a[i]);
    printf("\n");
}

void *thread_func(void *arg) {
    Args *a = (Args *)arg;
    //double res;
    int otst = 0, i;
    int k = a->k, p = a->p;
    FILE *file = fopen(a->name, "r");
    if (file == nullptr) {
        a->error_flag = 1;
        a->error_type = io_status::error_open;
    }
    else a->error_flag = 0;

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0) {
        if (file != nullptr) fclose(file);
        return 0;
    }

    a->result = reading(file, &a->error_type);
    a->length = a->result;

    if (a->error_type == io_status::error_read) {
        a->error_flag = 1;
    }

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0) {
        if (file != nullptr) fclose(file);
        return 0;
    }

    reduce_sum(a->p, &a->result, 1);


    if (a->k == 0) {
        a->a = new double[(int)a->result];
        for (int i = 1; i < p; i++) (a - k + i)->a = a->a;
    }

    rewind(file);

    for(int i = 0; i < k; i++) {
        otst += (a - k + i)->length;
    }

    reduce_sum(p);

    i = 0;
    while (fscanf(file, "%lf", &a->a[otst + i]) == 1) {
        i += 1;
    }

    reduce_sum(p);

    if (k == 0) {
        a->count = (double)second_step(a->a, (int)a->result);
    }

    reduce_sum(p, &a->count, 1);
    if (a->k == 0) delete [] a->a;
    return 0;
}

