#include "func.hpp"

void *thread_func(void *ptr) {
    Args *arg = (Args *) ptr;
    int k = arg->k, p = arg->p, n1 = arg->n1, n2 = arg->n2, n = n1 * n2;
    double *a = arg->a, *a_help = arg->a_help;
    double cpu_time = 0;
    cpu_time = get_cpu_time();

    for(int i = k; i < n1; i += p) {
        for(int j = 0; j < n2; j++) {
            if (i == 0 || j == 0 || j == n2 - 1 || i == n1 - 1) a_help[i * n2 + j] = a[i * n2 + j];
            else a_help[i * n2 + j] = more_eq(a, n2, i, j);
        }
    }
    reduce_sum(p);

    for (int i = k; i < n; i += p) {
        if (i < n2 && i % n2 != 0 && i >= (n1 - 1) * n2  && i % n2 != n2 - 1) {}
        else a[i] = a_help[i];
    }

    arg->cpu_time = get_cpu_time() - cpu_time;
    return 0;
}




int main(int argc, char *argv[]) {
    Args *a = nullptr;
    int p = 0, n1 = 0, n2 = 0, k, i, j;
    char *filename = nullptr;
    double full_time = 0;

    if (argc == 1) {
        printf("Usage:  %s  p  n1  n2  file\n", argv[0]);
        return 1;
    }

    if (!(argc == 5 && sscanf(argv[1], "%d", &p) == 1 && sscanf(argv[2], "%d", &n1) == 1 && sscanf(argv[3], "%d", &n2) == 1)) {
        printf("Некорректный ввод\n");
        return 1;
    }

    filename = argv[4];

    if (n1 <= 0 || n2 <= 0 || p <= 0) {
        printf("Неправильные аргументы\n");
        return 1;
    }

    a = new Args[p];
    if (a == nullptr) {
        printf("Ошибка конструктора\n");
        delete [] a;
        return 4;
    }

    int n = n1 * n2;
    double *data    = new double[n];
    double *data_2  = new double[n];
    if (read_array(filename, data, n) == -1) return 5;

    for(k = 0; k < p; k++) {
        a[k].p = p;
        a[k].k = k;
        a[k].n1 = n1;
        a[k].n2 = n2;
        a[k].a = data;
        a[k].a_help = data_2;
    }

    full_time = get_full_time();

    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)) {
            printf("Ошибка при создании потока %d\n", k);
            delete [] data;
            delete [] data_2;
            delete [] a;
            return -100;
        }
    }
    a[0].tid = pthread_self();
    thread_func(a + 0);

    for (k = 1; k < p; k++) pthread_join(a[k].tid, 0);

    full_time = get_full_time() - full_time;

    printf("RESULT: %d\n", p);

    for (i = 0; i < n1; i++) {
        for (j = 0; j < n2; j++) printf("%12.4e ", data[i * n2 + j]);
        printf("\n");;
    }

    for (k = 0; k < p; k++) printf("cpu time of %d: %.4e\n", a[k].k, a[k].cpu_time);
    printf("full time:     %.4e\n", full_time);

    delete [] data;
    delete [] data_2;
    delete [] a;
    return 0;
}

