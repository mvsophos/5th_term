#include "func.hpp"

// 0 - все ок
// 1 - ошибка открытия файла
// 2 - ошибка чтения файла
// остальные ошибки маловероятны

int main (int argc, char *argv[]) {
    int p, n;
    char *filename = nullptr;
    if (!(argc == 4 && sscanf(argv[1], "%d", &p) == 1 && sscanf(argv[2], "%d", &n) == 1)) {
        printf("Usage: %s  p  n  filename\n", argv[0]);
        return 900;
    }

    if (argv[3] != nullptr) filename = argv[3];
    else {
        printf("Usage: %s  p  n  filename\n", argv[0]);
        return 900;
    }
    if (p > n) p = n;
    
    // массив чисел
    double *a;
    a = new double [n];
    if (a == nullptr) {
        printf("Ошибка выделения памяти\n");
        return 300;
    }

    // массив аргументов потоков
    int k;
    args *arg = new args[p];
    if (arg == nullptr) {
        delete [] a;
        printf("Ошибка выделения памяти\n");
        return 300;
    }
    for (k = 0; k < p; k++) {
        arg[k].begin = k * n / p;
        arg[k].end = (k + 1) * n / p - 1;

        arg[k].p = p;
        arg[k].k = k;
        arg[k].n = n;
        arg[k].name = filename;
        arg[k].a = a;
    }

    

    pthread_t *threads = new pthread_t [p];
    if (threads == nullptr) {
        printf("Ошибка выделения памяти для потоков\n");
        delete [] a;
        delete [] arg;
        return 300;
    }
    double elapsed = get_full_time(); // начинаем засекать время
    for (k = 1; k < p; k++) {
        pthread_create(threads + k, 0, thread_func, arg + k);
    }
    thread_func(arg + 0);
    elapsed = get_full_time() - elapsed; // закончили засекать время

    for (k = 1; k < p; k++) {
        pthread_join(threads[k], 0);
    }

    switch (arg[0].error) {
    case io_status::undef:
        printf("Неизвестная ошибка\n");
        delete [] arg;
        delete [] threads;
        delete [] a;
        return -1000;
    case io_status::error_open:
        printf("Не открылся файл %s\n", filename);
        delete [] arg;
        delete [] threads;
        delete [] a;
        return 1;
    case io_status::error_read:
        printf("Не прочитался файл %s\n", filename);
        delete [] arg;
        delete [] threads;
        delete [] a;
        return 2;
    case io_status::success:
        break;
    }

    printf("RESULT %3d: ", p);
    print_array(a, n); // выводим массив
    for (k = 0; k < p; k++) {
        printf("Поток %d: %.2f\n", arg[k].k, arg[k].t);
    }
    printf("Общее время:        %.2f\n", elapsed);
    printf("Изменено элементов: %d\n", arg[0].res);
    delete [] arg;
    delete [] threads;
    delete [] a;
    return 0;
}
