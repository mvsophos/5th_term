#include "func.hpp"

// 0 - все ок
// 1 - ошибка открытия файла
// 2 - ошибка чтения файла
// остальные ошибки маловероятны

int main (int argc, char *argv[]) {
    int p, n;
    char *filename = nullptr;
    if ((!(argc == 4 && sscanf(argv[1], "%d", &p) == 1 && sscanf(argv[2], "%d", &n) == 1)) || (argv[3] == nullptr)) {
        printf("Usage: %s  p  n  filename\n", argv[0]);
        return 900;
    }
    else filename = argv[3];
    
    double *data;
    data = new double[n];
    if (data == nullptr) {
        printf("Ошибка выделения памяти\n");
        return 300;
    }

    int k;
    if (p > n) p = n;
    Args *a = new Args[p];
    if (a == nullptr) {
        delete [] data;
        printf("Ошибка выделения памяти\n");
        return 300;
    }
    for (k = 0; k < p; k++) {
        a[k].begin = k * n / p;
        a[k].end = (k + 1) * n / p - 1;

        a[k].n = n;
        a[k].p = p;
        a[k].k = k;
        a[k].filename = filename;
        a[k].a = data;
    }

    

    double time = get_full_time(); // начинаем засекать время
    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)) {
            printf("Ошибка при создании потока %d\n", k);
        }
    }
    thread_func(a + 0);
    
    for (k = 1; k < p; k++) pthread_join(a[k].tid, 0);
    time = get_full_time() - time; // закончили засекать время

    switch (a[0].error) {
    case io_status::undef:
        printf("Неизвестная ошибка\n");
        delete [] a;
        delete [] data;
        return -1000;
    case io_status::error_open:
        printf("Не открылся файл %s\n", filename);
        delete [] a;
        delete [] data;
        return 1;
    case io_status::error_read:
        printf("Не прочитался файл %s\n", filename);
        delete [] a;
        delete [] data;
        return 2;
    case io_status::success:
        break;
    }

    printf("RESULT %3d: ", p);
    print_array(data, n); // выводим массив
    for (k = 0; k < p; k++) printf("Поток %d: %.2f\n", a[k].k, a[k].time);
    printf("Общее время:        %.2f\n", time);
    printf("Изменено элементов: %d\n", a[0].count);
    delete [] a;
    delete [] data;
    return 0;
}
