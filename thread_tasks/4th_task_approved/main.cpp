#include "func.hpp"

// 0 - все ок
// 1 - ошибка открытия файла
// 2 - ошибка чтения файла
// остальные ошибки маловероятны

int main (int argc, char *argv[]) {
    int p, n;
    char *filename = nullptr;
    if ((!(argc == 4 && sscanf(argv[1], "%d", &p) == 1 && sscanf(argv[2], "%d", &n) == 1))) {
        printf("Usage: %s  p  n  filename\n", argv[0]);
        return 900;
    }

    filename = argv[3];
    
    double *data;
    data = new double[n];
    if (data == nullptr) {
        printf("Ошибка выделения памяти\n");
        return 300;
    }

    int k, buf = 0;
    if (p > n) p = n;
    Args *a = new Args[p];
    if (a == nullptr) {
        delete [] data;
        printf("Ошибка выделения памяти\n");
        return 300;
    }
    for (k = 0; k < p; k++) {
        a[k].begin = buf;
        buf += n / p;
        if (k == p - 1) a[k].end = n - 1;
        else a[k].end = buf - 1;


        a[k].n = n;
        a[k].p = p;
        a[k].k = k;
        a[k].filename = filename;
        a[k].a = data;
    }

    

    double time = clock(); // начинаем засекать время
    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)) {
            printf("Ошибка при создании потока %d\n", k);
        }
    }
    thread_func(a + 0);
    
    for (k = 1; k < p; k++) pthread_join(a[k].tid, 0);
    time = (clock() - time) / CLOCKS_PER_SEC; // закончили засекать время

    if (a[0].error != io_status::success) {
        if (a[0].error == io_status::error_open) {
            printf("Не открылся файл\n");
            delete [] a;
            delete [] data;
            return 1;
        }
        if (a[0].error == io_status::error_read) {
            printf("Ошибка при чтении файла\n");
            delete [] a;
            delete [] data;
            return 2;
        }
    }

    for (k = 0; k < p; k++) printf("Поток %d: %.2lf\n", a[k].k, a[k].time);
    printf("Общее время:        %.2f\n", time);
    printf("Изменено элементов: %d\n", a[0].count);
    printf("RESULT %3d: ", p);
    print_array(data, n); // выводим массив
    delete [] a;
    delete [] data;
    return 0;
}
