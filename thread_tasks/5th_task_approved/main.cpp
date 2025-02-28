#include "added.hpp"

// заменяются только элементы внутри этого интервала, те, с которых начинается и заканчивается замене не подлежат

int main(int argc, char *argv[]) {
    int p, pp, n;
    char *filename = 0;

    if (!(argc == 4 && sscanf(argv[1], "%d", &p) == 1 && sscanf(argv[2], "%d", &n) == 1 && (n > 0 && p > 0))) {
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

    Args *a = new Args[p];
    if (a == nullptr) {
        delete [] data;
        printf("Ошибка выделения памяти\n");
        return 300;
    }

    pp = p;
    if (pp > n) {
        pp = n;
        p = n;
    }

    p = (p >= 2 ? 2 : 1);

    int k, parameter_for_length = 0;
    if (n <= 2) { // просто нечего делать, выводим массив и все
        printf("Делать нечего, всего два элемента\n");
        delete [] a;
        delete [] data;
        return -1;
    }
    if (n == 3) { // тут отдельный случай, который надо обработать
        if (filename != nullptr) {
            int flag = read_array(data, n, filename);
            
            if (flag < 0) {
                printf("Ошибка при открытии файла %s\n", filename);
                delete [] a;
                delete [] data;
                return 1;
            }
            else if (flag > 0) {
                printf("Ошибка чтения в файле %s\n", filename);
                delete [] a;
                delete [] data;
                return 2;
            }
            else { // вывод и все прочее
                if (func(data[0], data[1], data[2]) == 1) {
                    data[1] = (data[0] + data[2]) / 2;
                    for (k = 0; k < pp; k++)  printf("Поток %d:     %.2lf\n", k + 1, 0.0);
                    printf("Общее время: %.2lf. Изменено элементов: %d\n", 0.0, 1);
                    printf("RESULT %2d: ", pp);
                    print_array(data, n);
                    delete [] a;
                    delete [] data;
                    return 0;
                }
                else {
                    for (k = 0; k < pp; k++) printf("Поток %d:     %.2lf\n", k + 1, 0.0);
                    printf("Общее время: %.2lf. Изменено элементов: %d\n", 0.0, 0);
                    printf("RESULT %2d: ", pp);
                    print_array(data, n);
                    delete [] a;
                    delete [] data;
                    return 0;
                }
            }
        }
    }
    while (n / p < 4)   p -= 1;



    for (k = 0; k < p; k++) {        
        a[k].cursor = parameter_for_length;
        if (k == p - 1) a[k].length = n - parameter_for_length;
        else a[k].length = (n / p);
        parameter_for_length += a[k].length;

        a[k].n = n;
        a[k].k = k; // потоки нумеруются с нуля
        a[k].p = p; // натуральное число потоков
        a[k].name = filename;
        a[k].a = data;
    }
    
    for (k = p; k < pp; k++) {
        a[k].n = n;
        a[k].k = k;
        a[k].p = p;
        a[k].name = filename;
    }

    
    double time = clock();
    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)) {
            printf("Ошибка при создании потока %d\n", k);
        }
    }
    thread_func(a + 0);

    for (k = 1; k < p; k++) pthread_join(a[k].tid, 0);
    time = (clock() - time) / CLOCKS_PER_SEC;

    
    if (process_args(a) == 0) {
        for (k = 0; k < pp; k++) printf("Поток %d:     %.2lf\n", a[k].k + 1, a[k].time);
        printf("Общее время: %.2lf. Изменено элементов: %d\n", time, (int)a[0].count);

        printf("RESULT %2d: ", pp);
        print_array(data, n);

        delete[] a;
        delete[] data;
        return 0;
    }
    else {
        delete[] a;
        delete[] data;
        return 1;
    }
}
