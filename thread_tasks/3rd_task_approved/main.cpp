#include "func.hpp"

int process_args(Args *a) {
    for (int i = 0; i < a->p; i++) {
        if (a[i].error_type == io_status::error_open || a[i].error_type == io_status::error_read) {
            printf("Ошибка в файле %s\n", a[i].name);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    Args *a;
    int k, p = argc - 1;
    if (argc == 1) {
        printf("Usage:  %s  files\n", argv[0]);
        return 1;
    }
    a = new Args[p];

    if (a == nullptr) {
        printf("Ошибка конструктора\n");
        delete [] a;
        return 2;
    }
    
    for (int k = 0; k < p; k++) {
        a[k].k = k;
        a[k].p = p;
        a[k].name = argv[k + 1];
    }
    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)) printf("Ошибка при создании потока %d\n", k);
    }
    a[0].tid = pthread_self();
    thread_func(a + 0);

    for (k = 1; k < p; k++) {
        pthread_join(a[k].tid, 0);
    }

    if (!process_args(a)) printf("RESULT: %d\n", (int)a[0].count);
    delete [] a;
    return 0;
}
