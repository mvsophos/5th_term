#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>

#define eps 1.e-15

enum class io_status {
    undef, error_open, error_read, success
};

class Args {
public:
    pthread_t tid;
    int p = 0;
    int k = 0;
    int n = 0;
    double time = 0;
    double flag = 0;
    double count = 0;

    double l_bufer = 0, r_bufer = 0; // сумма справа и слева
    int len_l_bufer = 0, len_r_bufer = 0;
    int right_finish = 0, right_start = 0, left_finish = 0, left_start = 0;
    io_status error_type = io_status::undef;

    double *a  = nullptr;
    char *name = nullptr;

    int cursor = 0; // курсор для потока в его куске массива
    int length = 0;

    int left_flag = 0, right_flag = 0;
    int ptr_l = -1,  ptr_r = -1;
    double num_l_begin = 0, num_r_begin = 0;
    double num_l_end   = 0, num_r_end = 0;
};

double get_full_time() {
    struct timeval buf;
    gettimeofday(&buf, 0);
    return buf.tv_sec + buf.tv_usec * 1.e-6;
}

void print_array(double *a, int n) {
    for(int i = 0; i < n; i++) printf("%8.5lf ", a[i]);
    printf("\n");
}

int read_array(double *a, int n, char *filename) {
    int i;
    FILE *fp = fopen(filename, "r");
    if (fp == nullptr) return -1;
    for (i = 0; i < n; i++) if (fscanf(fp, "%lf", a + i) != 1) break;
    if (i < n) {
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
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

int are_equal(double x, double y) {
    return (fabs(x - y) <= eps);
}

int func(double a, double b, double c) { // истина = 1, ложь = 0
    if (a * c > 0) return (fabs((a * c) - b * b) <= eps ? 1 : 0);
    return 0;
}

// это только нахождение склеек
void solve_1_step(Args *a) {
    double* data = a->a;
    int cursor = a->cursor;
    int length = a->length;
    int n = a->n;
    int i;

    if (cursor > 0) { // склейка на левой границе (считаем сумму на отрезке)
        i = cursor;
        while ((i < n - 1) && (func(data[i - 1], data[i], data[i + 1]) == 1)) {
            a->l_bufer += data[i];
            i++;
        }
        a->len_l_bufer = i - cursor;
        if (i != cursor) {
            a->left_flag = 1; // значит, что последовательность слева начинается и надо сшивать
            a->ptr_l = i; // длина начала
            a->num_l_end = data[i];
            a->left_finish = i;
        }
        else {
            a->left_flag = 0;
            a->ptr_l = i;
        }




        i = cursor;
        while ((i > 0) && (func(data[i - 1], data[i], data[i + 1]) == 1)) {
            i--;
            a->l_bufer += data[i];
        }

        
        if (i != cursor) {
            a->l_bufer -= data[i];
            a->num_l_begin = data[i];
            a->left_start = i;
        }
    }
    else {
        a->left_flag = 0;
        a->ptr_l = cursor;
    }




    if (cursor + length - 1 < n - 1) { // склейка на правой границе

        i = cursor + length - 1;
        while ((i > 0) && (func(data[i - 1], data[i], data[i + 1]) == 1)) {
            i--;
            a->r_bufer += data[i];
        }



        if (i != cursor + length - 1) {
            a->r_bufer -= data[i];
            a->right_flag = 1;
            a->ptr_r = i;
            a->num_r_begin = data[i];
            a->right_start = i;
        }
        else {
            a->right_flag = 0;
            a->ptr_r = i;
        }
        


        i = cursor + length - 1;
        while ((i < n - 1) && (func(data[i - 1], data[i], data[i + 1]) == 1)) {
            a->r_bufer += data[i];
            i++;
        }
        if (i != cursor + length - 1) {
            a->num_r_end = data[i];
            a->right_finish = i;
        }
    }
    else {
        a->right_flag = 0;
        a->ptr_r = cursor + length - 1;
    }
}

void solve_2_step(Args *a) {
    int i, i_to, i_from;
    int cursor = a->cursor;
    int length = a->length;
    double *data = a->a;

    // length > 1
    if (a->left_flag) { // замена склеек
        i_to = (a->ptr_l - 1 <= cursor + length - 1 ? a->ptr_l - 1 : cursor + length - 1);
        for (i = cursor; i <= i_to; i++) { // в цикле меняются числа, которые на склейке
            a->count += 1;
            data[i] = (a->l_bufer + a->r_bufer) / (a->left_finish - a->left_start - 1);
        }
    }
    if (a->right_flag && !(a->left_flag && a->ptr_l - 1 >= cursor + length - 1)) {
        i_from = (a->ptr_r + 1 >= cursor ? a->ptr_r + 1 : cursor);
        for (i = i_from; i <= cursor + length - 1; i++) {
            a->count += 1;
            data[i] = (a->l_bufer + a->r_bufer) / (a->right_finish - a->right_start - 1);
        }
    }



    if (a->ptr_l + 2 <= a->ptr_r) { // это меняет элементы внутри (там где нет склейки)
        int prev = 0, now, ptr_l = -1, ptr_r = -1, j;

        double summa = 0, length = 0;
        for (i = a->ptr_l + 1; i <= a->ptr_r - 1; i++) {
            now = func(data[i - 1], data[i], data[i + 1]);
            if (!prev && now) {
                summa = 0; length = 0;
                ptr_l = i;
            }
            else if (prev && !now) {
                ptr_r = i - 1;
                for (j = ptr_l; j <= ptr_r; j++) {
                    a->count += 1;
                    data[j] = summa / length;
                }
                summa = 0; length = 0;
            }
            prev = now;
            summa += data[i];
            length += 1;
        }
        if (prev) {
            ptr_r = i - 1;
            for (j = ptr_l; j <= ptr_r; j++) {
                a->count += 1;
                data[j] = summa / length;
            }
        }
    }
}



int process_args(Args *a) {
    if ((int)a[0].flag != 0) {
        if (a[0].error_type == io_status::error_open) {
            printf("Ошибка при открытии файла %s\n", a[0].name);
            return 1;
        }
        if (a[0].error_type == io_status::error_read) {
            printf("Ошибка чтения в файле %s\n", a[0].name);
            return 2;
        }
    }
    return 0;
}

void *thread_func(void *ptr) {
    Args *a = (Args *) ptr;
    int p = a->p;
    int k = a->k;
    int n = a->n;

    char *filename = a->name;
    double *data = a->a;



    reduce_sum (p);
    if (filename != nullptr) {
        if (k == 0) a->flag = read_array(data, n, filename);
        reduce_sum(p, &a->flag, 1);
        if (a->flag < 0) {
            a->error_type = io_status::error_open;
            return nullptr;
        }
        else if (a->flag > 0) {
            a->error_type = io_status::error_read;
            return nullptr;
        }
        else a->error_type = io_status::success;
    }
    else return nullptr;

    reduce_sum(a->p, &a->flag, 1);
    
    a->time = get_full_time();
    solve_1_step(a);
    reduce_sum(a->p);
    solve_2_step(a);
    reduce_sum(a->p, &a->count, 1);
    a->error_type = io_status::success;
    a->time = (get_full_time() - a->time);
    return nullptr;
}
