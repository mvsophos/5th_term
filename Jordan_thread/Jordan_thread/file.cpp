//#include <sys/sysinfo.h>
#include <sched.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "file.h"


// ФОРМАТ ВВОДА ТАКОВ: ./a.out  n  m  p  r  s  <filename>
int main(int argc, char* argv[]) {
    /* int errcode = 0; */
    int k = 0;
    int n, m, p, r, s;
    // Solving AX = B
    // V1, V2, V3 вспомогательные матрицы
    double *A  = NULL, *B  = NULL, *X  = NULL; // double *V1 = NULL, *V2 = NULL, *V3 = NULL;
    /* double r1, r2; */
    double t1  /* , t2 */; // t1 время вычисления решения, t2 время вычисления невязки

    if (!((argc == 6 || argc == 7) &&
        (sscanf(argv[1], "%d", &n) == 1) &&
        (sscanf(argv[2], "%d", &m) == 1) &&
        (sscanf(argv[3], "%d", &p) == 1) &&
        (sscanf(argv[4], "%d", &r) == 1) &&
        (sscanf(argv[5], "%d", &s) == 1) &&
        (n >= 0) && (m > 0 && m <= n) && (r >= 0 && r <= n) && (p > 0)))
            return error(1);


    

    // выделяем память под матрицу и под правый вектор
    A  = new double[n * n], B  = new double[n * 1], X  = new double[n * 1];
    //V1 = new double[m * m], V2 = new double[m * m], V3 = new double[m * m];
    if (A  == NULL || B  == NULL || X  == NULL) {
        delete [] A, delete [] B, delete [] X;
        //delete [] V1, delete [] V2, delete [] V3;
        return error(2);
    }

    if (p > n / m) { // чтобы не создавать слишком много потоков
        p = n / m + (n % m == 0 ? 0 : 1);
    }



    // САМУ МАТРИЦУ НАДО ИНИЦИАЛИЗИРОВАТЬ В ПОТОЧНОЙ ФУНКЦИИ.

    /* if (s == 0 && argc == 7)
        fill(A, n, m, 0, argv[6], &errcode);
    else if ((s > 0 && s < 5) && argc == 6)
        fill(A, n, m, s, NULL, NULL);
    else
        errcode = 1;

    if (errcode > 0) {
        delete [] A, delete [] B, delete [] X;
        //delete [] V1, delete [] V2, delete [] V3;
        return error(errcode);
    }

    fill_right_part(A, B, n, m);

    if (r > 0) {
        print_matrix(A, n, n, m, r);
        print_matrix(B, 1, n, m, r);
    } */

    // ТО ЧТО ВЫШЕ И НИЖЕ КОММЕНТА ПОМЕСТИТЬ В ПОТОК НОМЕР НОЛЬ

    // ЕСЛИ ПОТОКОВ БОЛЬШЕ ЧЕМ БЛОЧНЫХ СТРОК, ТО НАДО УМЕНЬШИТЬ ИХ КОЛИЧЕСТВО ДО ЧИСЛА СТРОК

    /* if (n % m == 0) p = n / m;
    else p = n / m + 1; */

    args *a = new args[p];

    if (argc == 7) a[0].filename = argv[6];
    a[0].how_many_lines =   r;
    a[0].s =                s;
    a[0].__argc =           argc;
    // ФАЙЛ ОТКРЫВАЕТ ТОЛЬКО ПОТОК С НОМЕРОМ НОЛЬ. ОСТАЛЬНЫМ ПОТОКАМ ОН НЕ НУЖЕН
    // вообще с этими данными работает только потоко номер ноль


    for (k = 0; k < p; k++) {
        a[k].n = n;
        a[k].m = m;
        a[k].p = p;
        a[k].tk = k;
        a[k].A = A;
        a[k].B = B;
        a[k].X = X;
        // передать им один и тот же указатель на массивы решения и матрицы
    }


    t1 = clock();
    for (k = 1; k < p; k++) {
        if (pthread_create(&a[k].tid, nullptr, thread_solve, a + k)) {
            printf("Ошибка при создании потока %d\n", k);
        }
    }
    a[0].tid = pthread_self();
    thread_solve(a + 0);
    //thread_solve(a + 0, n, m, A, B, X);

    for (k = 1; k < p; k++) {
        pthread_join(a[k].tid, 0);
    }

    // ВЫВЕСТИ ВРЕМЯ, КОТОРОЕ БЫЛО ПОТРАЧЕНО НА КАЖДЫЙ ПОТОК

    t1 = (clock() - t1) / CLOCKS_PER_SEC;

    // printf("Вектор решения\n");
    // print_matrix(a->X, 1, n, m, r);

    // невязки надо считать в функции thread_func
    
    if ((int)a->error == 0) {
        //print_matrix(X, 1, n, m, n);
        printf ("%s : Task = %d Res1 = %e Res2 = %e T1 = %.2f T2 = %.2f S = %d N = %d M = %d P = %d\n", argv[0], 15, a[0].r1, a[0].r2, a[0].t1, a[0].t2, s, n, m, p);
    }
    else printf("Что-то пошло не так\n");

    //for (int i = 0; i < p; i++) if (a[i].uk) delete [] a[i].uk;
    delete [] a;
    delete [] A; delete [] B; delete [] X;
    return 0;
}
