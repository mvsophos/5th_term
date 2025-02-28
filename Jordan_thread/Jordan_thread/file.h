#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/time.h>



class args{
public:
    pthread_t tid = -1;
    int n = 0;
    int m = 0;
    int how_many_lines = 0; // сколько вывести строк


    int p = 0; // количество потоков
    int tk = 0; // thread_k - номер потока
    //double errorflag = 0; // если есть ошибка
    //double res = 0.1; // 
    double norm = 0; // норма обратной к главному элементу (должна быть минимальной)
    int i = 0;

    double error = 0;

    //double *uk = nullptr; // указатель на блок с минимальной нормой обратной

    double *A, *B, *X;

    double t1 = 0; // время на выполнение одного потока
    double t2 = 0;
    double r1 = 0; // невязки, нужны только в конце
    double r2 = 0;

    char *filename = nullptr;
    int s = 0;
    int __argc = 0;

    void print_data(){
        printf("\n p = %d tk = %d \n norm = %lf i = %d error = %lf \n", p, tk, norm, i, error);
    }
};

double get_time() {
    struct timeval time;
    gettimeofday (&time, 0);
    return time.tv_sec + time.tv_usec * 1.e-6;
}

static inline void copy(const double *source, double *dest, const int v, const int h){
    memcpy(dest, source, v * h * sizeof(double));
}

#define max(x,y) (x > y ? x : y)
#define min(a, b) (a < b ? a : b)
#define A(i,j) (A + i*n*m + j*av*m)
#define a(p,q) (A(i,j) + p*ah + q)
#define eps (1e-15) // нормы для невязки правой части и решения

void f0(double *const A, const int n, const int m, const char* const filename, int *errno) {
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;
    int counter = 0;
    FILE* file_input = fopen(filename, "r");
    if (file_input == NULL) {
        *errno = 3;
        return;
    }

    for (i = 0; i * m < n; i++) {
        av = i < k ? m : l;
        for (p = 0; p < av; p++) {
            for (j = 0; j * m < n; j++) {
            ah = j < k ? m : l;
                for (q = 0; q < ah; q++) {
                    if (fscanf(file_input, "%lf", a(p, q)) == 1)
                        counter++;
                    else{
                        break;
                    }
                }
            }
        }
    }
    if (counter != n * n) *errno = 4;
    fclose(file_input);
}
void f1(double *const A, const int n, const int m) {
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;

    for (i = 0; i * m < n; i++) {
        av = i < k ? m : l;
        for (j = 0; j * m < n; j++) {
            ah = j < k ? m : l;
            for (p = 0; p < av; p++) {
                for (q = 0; q < ah; q++) {
                    *a(p, q) = n - max(i * m + p + 1, j * m + q + 1) + 1; // i*m + p + 1 это номер строки этого элемента во всей исходной матрице (+1 потому что нумерация с 1)
                }
            }
        }
    }
}
void f2(double *const A, const int n, const int m) {
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;

    for (i = 0; i * m < n; i++) {
        av = i < k ? m : l;
        for (j = 0; j * m < n; j++) {
            ah = j < k ? m : l;
            for (p = 0; p < av; p++) {
                for (q = 0; q < ah; q++) {
                    *a(p, q) = max(i * m + p + 1, j * m + q + 1);
                }
            }
        }
    }
}
void f3(double *const A, const int n, const int m) {
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;

    for (i = 0; i * m < n; i++) {
        av = i < k ? m : l;
        for (j = 0; j * m < n; j++) {
            ah = j < k ? m : l;
            for (p = 0; p < av; p++) {
                for (q = 0; q < ah; q++) {
                    *a(p, q) = abs(i * m + p - (j * m + q));
                }
            }
        }
    }
}
void f4(double* const A, const int n, const int m) {
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;

    for (i = 0; i * m < n; i++)
    {
        av = i < k ? m : l;
        for (j = 0; j * m < n; j++)
        {
            ah = j < k ? m : l;
            for (p = 0; p < av; p++)
            {
                for (q = 0; q < ah; q++) *a(p, q) = fabs(1. / (i * m + p + j * m + q + 1));
            }
        }
    }
}

void fill(double *const A, const int n, const int m, const int s, const char *const filename, int *const errno)
{
    switch (s) {
    case 0: f0(A, n, m, filename, errno); break;
    case 1: f1(A, n, m); *errno = 0; break;
    case 2: f2(A, n, m); *errno = 0; break;
    case 3: f3(A, n, m); *errno = 0; break;
    case 4: f4(A, n, m); *errno = 0; break;
    default: break;
    }
}

void print_matrix(const double *A, const int h, const int v, const int m, const int r);

void fill_right_part(const double *const A, double *const B, const int n, const int m) // вероятно это не нужно
{
    int i, j, p, q, av, ah;
    const int k = n / m;
    const int l = n - k * m;
    int c = 0;

    for (i = 0; i < n; i++) B[i] = 0; // вероятно это необязательно
    
    for (i = 0; i * m < n; i++) { // цикл по строкам
        av = i < k ? m : l;
        for (p = 0; p < av; p++) {
            B[i * m + p] = 0;
            c = 0;
            for (j = 0; j * m < n; j++){
                // здесь было c = 0;
                ah = j < k ? m : l;
                for (q = 0; q < ah; q++, c++){
                    //printf("%d  ", c);
                    /* if (c % 2 == 0) */ B[i * m + p] += *a(p, q) * ((c + 1) % 2); // напоминание: четность другая, потому что нумерация в матрице и в массиве идет с 1 и 0 соответственно
                }
            }
        }
    }
}

double full_norm(const double *const A, const int n, const int m){
    const int k = n / m;
    const int l = n - k * m;
    int i, j, p, q, av, ah;
    double max = 0., current = 0.;

    for (i = 0; i * m < n; i++) {
        av = i < k ? m : l;
        for (p = 0; p < av; p++) {
            current = 0.;
            for (j = 0; j * m < n; j++){
                ah = j < k ? m : l;
                for (q = 0; q < ah; q++){
                    current += fabs(*a(p, q));
                }
            }
            if (current > max) max = current;
        }
    }
    return max;
}

double st_norm(const double *const A, const int n){
    int i, j;
    double max = 0., current = 0.;
    for (i = 0; i < n; i++) {
        current = 0.;
        for (j = 0; j < n; j++) {
            current += fabs(A[j * n + i]);
        }
        if (current > max) max = current;
    }
    return max;
}

// error
int error(const int error) {
    switch (error) {
    case 1: {
        printf("Некорректный ввод команды\n");
        break;
    }
    case 2: {
        printf("Ошибка выделения памяти\n");
        break;
    }
    case 3: {
        printf("Ошибка чтения файла\n");
        break;
    }
    case 4: {
        printf("Некорректные данные или недостаточно данных\n");
        break;
    }
    case 5: {
        printf("Алгоритм неприменим\n");
        break;
    }
    default:
        return 0;
    }
    return error;
}

double res1(double *A, double *B, double *X, const int n, const int m) {
    int i, j, q, r;
    int av, ah;
    double *pa, *pi, *pj;
    const int k = n / m;
    const int l = n - k * m;
    double norm = 0.;
    double sum = 0.;

    for (i = 0; i < n; i++) {
        norm += fabs(B[i]);
    }
    for (j = 0; j * m < n; j++) {
        ah = (j < k) ? m : l;
        pj = X + j * m;
        for (i = 0; i * m < n; i++) {
            pi = B + i * m;
            av = (i < k) ? m : l;
            pa = A + i * m * n + j * av * m;
            for (q = 0; q < av; q++) {
                sum = 0.;
                for (r = 0; r < ah; r++) {
                    sum += pa[q * ah + r] * pj[r];
                }
                pi[q] -= sum;
            }
        }
    }
    sum = 0.;
    if (norm > eps) {
        for (i = 0; i < n; i++) {
            sum += fabs(B[i]);
        }
        //printf("norm for res 1 = %e\n", norm);
        sum /= norm;
    }

    return sum;
}

double res2(double *B, double *X, const int n) { // B это вектор в знаменателе
    int i;
    double norm1 = 0., norm2 = 0.;

    for (i = 0; i < n; i++) {
        //norm1 += ((i + 1) % 2);
        norm1 += fabs(B[i]);
    }
    //if (norm1 > eps) {
    for (i = 0; i < n; i++) {
        //norm2 += fabs(B[i] - ((i + 1) % 2));
        norm2 += fabs(B[i] - X[i]);
    }
    //printf("norm for res 2 = %e\n", norm1);
    //print_matrix(B, 1, n, n, n);
    // printf("\n %e \n", norm1);
    norm2 /= norm1;
    //}
    return norm2;
}

void print_matrix(const double *A, const int h, const int v, const int m, const int r) { // вывод матрицы
    int nv = min(v, r);
    int nh = min(h, r); // если матрица будет меньше, чем размер вывода, то это условие будет существенно

    int mv, mh;

    int prn_val_h = r;
    int prn_val_v = r;

    int i, j, p, q, av, ah;
    int kh = h / m, kv = v / m;
    int lh = nh - kh * m, lv = nv - kv * m;

    for (i = 0; i * m < nh; i++) {
        av = i < kh ? m : lh;
        mv = min(av, prn_val_v);
        for (p = 0; p < mv; p++) {
            prn_val_h = r;
            for (j = 0; j * m < nv; j++) {
                ah = j < kv ? m : lv;
                mh = min(ah, prn_val_h);
                for (q = 0; q < mh; q++) printf(" %10.3e", *(A + i*h*m + j*av*m  + p*ah + q));
                prn_val_h -= ah;
            }
            printf("\n");
        }
        prn_val_v -= av;
    }
    printf("\n");
}

// c = a * b
int multiply(const double* const a, const int av, const int ah, const double* const b, const int bv, const int bh, double* const c) {
    int r = 0, t = 0, q, temp_r, temp_t;
    // для вышеописанной логики
    double c00, c01, c02, c10, c11, c12, c20, c21, c22;
    if (ah != bv)
        return -1;
    // Зануляем c
    for (r = 0; r < av; r++)
        for (t = 0; t < bh; t++)
            c[r * bh + t] = 0.;

    for (r = 0; r + 2 < av; r += 3)
        for (t = 0; t + 2 < bh; t += 3) {
            c00 = 0., c01 = 0., c02 = 0.;
            c10 = 0., c11 = 0., c12 = 0.;
            c20 = 0., c21 = 0., c22 = 0.;
            for (q = 0; q < ah; q++) {
                c00 += a[(r + 0) * ah + q] * b[q * bh + (t + 0)];
                c01 += a[(r + 0) * ah + q] * b[q * bh + (t + 1)];
                c02 += a[(r + 0) * ah + q] * b[q * bh + (t + 2)];
                c10 += a[(r + 1) * ah + q] * b[q * bh + (t + 0)];
                c11 += a[(r + 1) * ah + q] * b[q * bh + (t + 1)];
                c12 += a[(r + 1) * ah + q] * b[q * bh + (t + 2)];
                c20 += a[(r + 2) * ah + q] * b[q * bh + (t + 0)];
                c21 += a[(r + 2) * ah + q] * b[q * bh + (t + 1)];
                c22 += a[(r + 2) * ah + q] * b[q * bh + (t + 2)];
            }
            c[(r + 0) * bh + (t + 0)] += c00;
            c[(r + 0) * bh + (t + 1)] += c01;
            c[(r + 0) * bh + (t + 2)] += c02;
            c[(r + 1) * bh + (t + 0)] += c10;
            c[(r + 1) * bh + (t + 1)] += c11;
            c[(r + 1) * bh + (t + 2)] += c12;
            c[(r + 2) * bh + (t + 0)] += c20;
            c[(r + 2) * bh + (t + 1)] += c21;
            c[(r + 2) * bh + (t + 2)] += c22;
        }
    temp_t = t;
    temp_r = r;
    // если не получилось разделить четно, то
    // повторяем процесс для последнего столбца и строчки
    // так, как делали раньше
    if ((av - temp_r) == 2) {
        for (t = 0; t + 1 < bh; t += 2) {
            c00 = 0., c01 = 0.;
            c10 = 0., c11 = 0.;
            for (q = 0; q < ah; q++) {
                c00 += a[(temp_r + 0) * ah + q] * b[q * bh + (t + 0)];
                c01 += a[(temp_r + 0) * ah + q] * b[q * bh + (t + 1)];
                c10 += a[(temp_r + 1) * ah + q] * b[q * bh + (t + 0)];
                c11 += a[(temp_r + 1) * ah + q] * b[q * bh + (t + 1)];
            }
            c[(temp_r + 0) * bh + (t + 0)] = c00;
            c[(temp_r + 0) * bh + (t + 1)] = c01;
            c[(temp_r + 1) * bh + (t + 0)] = c10;
            c[(temp_r + 1) * bh + (t + 1)] = c11;
        }
        if (t < bh) {
            for (r = temp_r; r < av; r++)
                for (t = bh & ~1; t < bh; t++) {
                    c00 = 0.;
                    for (q = 0; q < ah; q++) {
                        c00 += a[(r + 0) * ah + q] * b[q * bh + (t + 0)];
                    }
                    c[(r + 0) * bh + (t + 0)] = c00;
                }
        }
    }

    if ((bh - temp_t) == 2) {
        for (r = 0; r + 1 < av; r += 2) {
            c00 = 0., c01 = 0.;
            c10 = 0., c11 = 0.;
            for (q = 0; q < ah; q++) {
                c00 += a[(r + 0) * ah + q] * b[q * bh + (temp_t + 0)];
                c01 += a[(r + 0) * ah + q] * b[q * bh + (temp_t + 1)];
                c10 += a[(r + 1) * ah + q] * b[q * bh + (temp_t + 0)];
                c11 += a[(r + 1) * ah + q] * b[q * bh + (temp_t + 1)];
            }
            c[(r + 0) * bh + (temp_t + 0)] = c00;
            c[(r + 0) * bh + (temp_t + 1)] = c01;
            c[(r + 1) * bh + (temp_t + 0)] = c10;
            c[(r + 1) * bh + (temp_t + 1)] = c11;
        }
        for (r = av & ~1; r < av; r++) {
            for (t = temp_t; t < bh; t++) {
                c00 = 0.;
                for (q = 0; q < ah; q++) {
                    c00 += a[(r + 0) * ah + q] * b[q * bh + (t + 0)];
                }
                c[(r + 0) * bh + (t + 0)] = c00;
            }
        }
    }

    if (av - temp_r == 1) {
        for (t = 0; t < bh; t++) {
            c00 = 0.;
            for (q = 0; q < ah; q++)
                c00 += a[temp_r * ah + q] * b[q * bh + t];
            c[temp_r * bh + t] = c00;
        }
    }
    if (bh - temp_t == 1) {
        for (r = 0; r < av; r++) {
            c00 = 0.;
            for (q = 0; q < ah; q++)
                c00 += a[r * ah + q] * b[q * bh + temp_t];
            c[r * bh + temp_t] = c00;
        }
    }
    return 0;
}

// c -= a * b
// эта функция неверна, что-то в ней необходимо переписать

// c -= b
int extract(double *const c, const int cv, const int ch, const double *const b, const int bv, const int bh){ // вторая версия параллельная, но это не нужно, так как это для блоков, которые целиком умещаются в оперативную память
    if (cv != bv || ch != bh) return -1;
    int i, j;
    for(i = 0; i < cv; i++)
        for(j = 0; j < ch; j++) c[i * ch + j] -= b[i * ch + j];
    return 0;
}

#undef A

// p - общее количество потоков, k - номер потока, i и j - это номер строки.
void swap_line(int p, int thread_k, double *a, double *buf, int n, int m, int i, int j) {
    int k = n / m;
    int l = n - k * m;
    int r;
    double *a1, *a2;
    if (!(i >= 0 && j >= 0)) {
        return;
    }
    if (i == j) return;

    for (int g = thread_k; g < k + 1; g += p) {
        r = (g < k ? m : l); // величина блока (ширина)
        a1 = a + i * n * m + g * m * r;
        a2 = a + j * n * m + g * m * r;
        copy(a1,  buf, r, m);
        copy(a2,  a1,  r, m);
        copy(buf, a2,  r, m);
    }
}

void swap(double* const lhs, double* const rhs) {
    double temp = *lhs;
    *lhs = *rhs;
    *rhs = temp;
}



#define A(i, j) A[i * n + j]
#define E(i, j) A_inversed[i * n + j]

int jord_inverse(double *A, double *A_inversed, const int n, double ERROR) { // ERROR тут фиктивен, надо его убрать
    int i, j; // итераторы
    double max = 0.; // максимальный элемент по столбцу
    int m = 0; // строчка и столбец с максимальным элементом
    double c = 0.; // коэффициент
    
    for (j = 0; j < n; j++) { // идем по столбам
        max = fabs(A(j, j));
        m = j;

        for (i = j; i < n; i++) {
            if (fabs(A(i, j)) > max) {
                m = i;
                max   = fabs(A(i, j));
            }
        } // нашли максимальный элемент и строку в которой он находится

        if (max <= ERROR) { // ошибка нужна чтобы нормально найти максимальный элемент
            return -1;
        }

        if (m != j) {
            for (i = 0; i < j; i++) { // меняем строки местами
                swap(&(E(j, i)), &(E(m, i))); // элемент на дигонали мы не меняем так как там все равно будет 1 в (j,j), а все остальные 0
            }
            for (i = j; i < n; i++) { // менять их в матрице А не надо так как там нули
                swap(&(A(j, i)), &(A(m, i)));
                swap(&(E(j, i)), &(E(m, i)));
            }
        }
        //c = 1. / A(j, j);
        c = A(j,j); // этот элемент на диагонали не равен 0
        //printf("%lf    ", c);
        //if (fabs(c - 1) > ERROR) {
            for (m = 0; m < j + 1; m++) E(j, m) /= c; // m больше не нужен так как строки уже переставили
            A(j, j) = 1.;
            for (m = j + 1; m < n; m++) {
                A(j, m) /= c;
                E(j, m) /= c;
            }
        //}

        // цикл ниже вычитает обнуляет почти весь столб
        for (i = 0; i < n; i++) { // все элементы в столбе j кроме одного равны 0
            if (i != j){
                c = A(i, j);
                //if (fabs(c) > ERROR) {
                    for (m = 0; m < j + 1; m++) {
                        E(i, m) -= c * E(j, m);
                    }
                    // A(i, j) = 0.;
                    for (m = j + 1; m < n; m++) {
                        A(i, m) -= c * A(j, m);
                        E(i, m) -= c * E(j, m);
                    }
                //}
            }
        }
    } // теперь в Rev находится обратная матрица решенная Жорданом
    return 0;
}


void reduce_sum(int p, double *a = nullptr, int n = 0);
/* void reduce_sum (int p, double *a, int n) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0;
    static int t_out = 0;
    //static int con = 0;
    if (p <= 1)
        return;
    pthread_mutex_lock(&m);
    //con++;
    //printf("con = %d\n", con);
    t_in++;
    if (t_in >= p)
        {
        t_out = 0;
        pthread_cond_broadcast (&c_in);
        }
    else
        {
        while (t_in < p)
            pthread_cond_wait (&c_in, &m);
        }
    t_out++;
    if (t_out >= p)
        {
        t_in = 0;
        pthread_cond_broadcast (&c_out);
        }
    else
        {
        while (t_out < p)
            pthread_cond_wait (&c_out, &m);
        }
    

    pthread_mutex_unlock(&m);
} */

// эту функцию будем использовать для синхронизации необратимых блоков (если блок обратим, то 1, иначе 0 (так что если сумма ошибок нулевая, то ни один блок не обратим (или можно искать максимум)))
void reduce_sum(int p, double *a, int n){ // эта функция была int (так в конспекте)
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static double *r = nullptr;
    int i;
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a; // первый пришедший запоминает в r указатель себя
    else for(i = 0; i < n; i++) r[i] += a[i];
    t_in++;
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in); // так как broadcast владелец блокировки, то для него 
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m); // первый поток освободил мьютекс

    // так пока последний вошедший не скажет broadcast, они не начнут работать
    // они вышли из ожидания (владелец мьютекса - это тот, кто сказал broadcast)
    if (r != a) for (i = 0; i < n; i++) a[i] = r[i];
    t_out++;
    if (t_out >= p){
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m); // все встают в ожидание
    pthread_mutex_unlock(&m);
}

// эта функция будет синхронизировать минимальную норму и номер строки с этой минимальной нормой
// насчет ошибок (если все блоки необратимы)
// функция синхронизирует и ошибки, и строку с минимальным элементом
void reduce_min(int p, args *ptr){ // эта функция синхронизирует максимумы (в данном случае длины постоянной последовательности максимальной длины и элемента постоянного в ней)
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static args *r = nullptr;

    args *a = (args *)ptr;
    // int i;
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else{
                // первый элс иф можно убрать
                // if (int(a->error) == 0); // r->error += a->error;
                // else
                    /* if (r->param.length < a->param.length || ((int(r->param.length) == int(a->param.length)) && (r->param.maxc < a->param.maxc))){
                        r->param.length = a->param.length;
                        r->param.maxc = a->param.maxc;
                    } */
        if (r->norm < a->norm){
            r->norm = a->norm;
            r->i = a->i;
            //r->uk = a->uk;
            // r->error += a->error; // мб вот это убрать
        }
        r->error += a->error;
    }
    t_in++;
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a){
        a->norm = r->norm;
        a->i = r->i;
        //a->uk = r->uk;
        a->error = r->error;
        /* a->param = r->param;
        a->error = r->error; */
    }
    t_out++;
    if (t_out >= p){
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m);
    pthread_mutex_unlock(&m);
}

/* void zero_matrix_p(double *matrix, int n, int m, int p, int pi) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = pi; j < n; j += p * m) {
            memset(matrix + i * n + j, 0, m);
        }
    }
} */



int solve(args *a, int p, int tnum, int n, int m, double *A, double *B, double *X, double *V1, double *V2, double *V3);

void *thread_solve(void *argument) { /////////////////////////////////////////////////////////////////////////////////////////

    args *a = (args *) argument;
    int n = a->n, m = a->m, r = a->how_many_lines;
    int i = 0  /* , j = 0, q = 0 */;                     // итераторы
    int p = a->p;
    int tnum = a->tk;           // номер потока


    cpu_set_t cpu;
    CPU_ZERO (&cpu);
    
    int n_cpus = get_nprocs();
    int cpu_id = n_cpus - 1 - (a->tk % n_cpus);
    CPU_SET(cpu_id, &cpu);
    
    //mas_k = mas_k; v1=v1; не нужная строка
    pthread_t tid = a->tid;
    pthread_setaffinity_np(tid, sizeof(cpu), &cpu);

    //zero_matrix_p(a->A, n, m, p, tnum);
    
    for (i = tnum * m; i < n; i += p * m){          // привязываем определенную память к потоку, чтобы он работал только со своими блоками
        int h = (i + m < n ? m : n - i); // i + m - n
        memset (a->A + i * n,   0, h * n * sizeof(double));
        memset (a->B + i,       0, h * sizeof(double));
        memset (a->X + i,       0, h * sizeof(double));
    }

    // int p = (int)a->p;      // потоков
    // int tnum = (int)a->tk;     // номер потока

    double *V1, *V2, *V3;

    if ((V1 = new double[m * m]) == nullptr || (V2 = new double[m * m]) == nullptr || (V3 = new double[m * m]) == nullptr) {
        if (V1 != nullptr) delete[] V1;
        if (V2 != nullptr) delete[] V2;
        if (V3 != nullptr) delete[] V3;
        a->error = 1;
        return nullptr;
    }
    /* memset(V1, 0, m * m * sizeof(double));
    memset(V2, 0, m * m * sizeof(double));
    memset(V3, 0, m * m * sizeof(double)); */

    reduce_sum(a->p, &a->error, 1);
    if (a->error > 0) {
        delete[] V1;
        delete[] V2;
        delete[] V3;
        return nullptr;
    }

    //a->uk = new double[m * m];
    
    //for (i = 0; i < m * m; i++) a->uk[i] = 0;
    
    //memset(a->uk, 0, m * m * sizeof(double));


    // ИНИЦИАЛИЗАЦИЯ МАТРИЦЫ ПРОИСХОДИТ ЗДЕСЬ. ЭТИМ ЗАНИМАЕТСЯ ТОЛЬКО ПЕРВЫЙ ПОТОК С НОМЕРОМ НОЛЬ. МОЖНО ДЕЛАТЬ ЭТО, ТАК КАК ПАМЯТЬ УЖЕ ПРИВЯЗАНА
    int errcode; // эта переменная применяется только здесь и больше нигде
    if (tnum == 0) {
        switch (a->s) {
            case 0: f0(a->A, n, m, a->filename, &errcode); break;
            case 1: f1(a->A, n, m); errcode = 0; break;
            case 2: f2(a->A, n, m); errcode = 0; break;
            case 3: f3(a->A, n, m); errcode = 0; break;
            case 4: f4(a->A, n, m); errcode = 0; break;
            default: break;
        }

        // ошибка может быть вызвана только проблемами с файлом

        a->error = (double)errcode;
        
    }
    
    reduce_sum(p, &a->error, 1);
    
    if (a->error > 0) {
        // delete [] a->A, delete [] a->B, delete [] a->X;
        delete [] V1; delete [] V3; delete [] V2;
        // надо придумать как вывести результат, если матрицу прочесть или инициализировать не удалось
        return 0;
    }

    if (tnum == 0) {
        fill_right_part(a->A, a->B, n, m);
        if (r > 0) {
            printf("Исходная матрица\n");
            print_matrix(a->A, n, n, m, r);
            printf("Правая часть\n");
            print_matrix(a->B, 1, n, m, r);
        }
    }


    reduce_sum(p, 0, 0); // это чтобы другие потоки не пошли дальше

    double t = get_time(); // начинаем отсчитывать время выполнения потока

    int resultat = solve(a, a->p, a->tk, a->n, a->m, a->A, a->B, a->X, V1, V2, V3);
    
    a->t1 = get_time() - t; // закончили считать время выполнения потока
    delete [] V1;
    delete [] V2;
    delete [] V3;

    reduce_sum(p);

    if (tnum == 0) printf("result : %d\n", resultat);

    reduce_sum(p);

    if (a->tk == 0 && resultat == 0) { // вычисление невязки

        switch (a->s) {
                case 0: f0(a->A, n, m, a->filename, &errcode); break;
                case 1: f1(a->A, n, m); errcode = 0; break;
                case 2: f2(a->A, n, m); errcode = 0; break;
                case 3: f3(a->A, n, m); errcode = 0; break;
                case 4: f4(a->A, n, m); errcode = 0; break;
                default: break;
        }
        fill_right_part(a->A, a->B, n, m);

        t = get_time();
        a->r1 = res1(a->A, a->B, a->X, n, m); // теперь правая часть в X (была скопирована перед решением), а в B решение
        for (int i = 0; i < n; i++){ // может переписать этот цикл
            a->B[i] = (i + 1) % 2;
        }
        a->r2 = res2(a->X, a->B, n);
        a->t2 = (get_time() - t);

        a->error = 0; // это код того, что нет ошибок, все верно
    }
    else a->error = 1;
    //reduce_sum(p, &a->error, 1); // наверное это не нужно, так как работу выполняет только один поток
    
    
    //delete [] a->uk;

    return 0;
}



// на размере блока 1, не идет дальше первой итерации (на любом числе потоков)
// не работает даже на 1м потоке и размере блока 1, но работает на больших блоках
int solve(args *a, int p, int tnum, int n, int m, double *A, double *B, double *X, double *V1, double *V2, double *V3) { /////////////////////////////////////////////////////////////////////////////////////////
    int i = 0, j = 0, r = 0, q = 0;                     // итераторы
    double *pa = NULL, *pi = NULL, *pj = NULL;          // вспомогательные указатели
    int av = 0, ah = 0;                                 // размер текущего блока av * ah
    const int k = n / m;                                // количество блоков
    const int l = n - k * m;                            // крайний блок имеет сторону ...
    double ERROR = (full_norm(A, n, m) * eps);          // примерная допустимая погрешность
    double min = 0.;                                    // 1 делить на минимальную норму обратной матрицы (по столбцам)
    int  min_i = 0;                                     // номер строки с минимальной нормой обратной матрицы в столбце
    int c = 0;                                          // счетчик необратимых матриц в столбце
    double current = 0.;                                // текущая норма

    for (j = 0; j * m < n; j++) { // в этом цикле смотрим квадратные блоки порядка m. 
        
        
//////////////////////////// НЕОБХОДИМО ОТРЕДАКТИРОВАТЬ ВЫБОР ГЛАВНОГО ЭЛЕМЕНТА ПО СТОЛБЦУ
        av = ah = j < k ? m : l;

        c = 0;
        a->error = 0;
        min = 0;
        a->norm = 0;
        
       ///////////////// НЕОБХОДИМО РАСПАРАЛЛЕЛИТЬ ВОТ ЭТО ГОВНО
        
        for (i = j; i < k; i += 1) {
            pi = A + i * n * m + j * av * m; // это блок (i, j)

            for (r = 0; r < av; r++) { // вносим pi в указатель V1
                for (q = 0; q < ah; q++) {
                    V1[r * ah + q] = pi[r * ah + q];
                    V2[r * ah + q] = (r == q);
                }
            }

            if (jord_inverse(V1, V2, av, ERROR) == 0) { // если существует обратная у V1
                current = 1. / st_norm(V2, av);
                
                if (current > min) {
                    pj = V2;
                    V2 = V3;
                    V3 = pj;
                    min   = current;
                    min_i = i;
                    //c = 1;

                    
                    
                    //copy(V3, a->uk, m, m);
                    
                    //a->uk    = V3;
                    a->norm  = min;
                    a->i     = min_i;
                    //a->error = c;

                }
            }
            else {
                // added lines
                c++; // удалить если че-то в говне
                a->error = c;
            }
        }
        
        // ТУТ НАДО ПРОСУММИРОВАТЬ ОШИБКИ И ЕСЛИ ПЛОХО, ТО ВСЕ ПОТОКИ ДОЛЖНЫ ВЫЙТИ. ТО ЕСТЬ ЕСЛИ НИ У КОГО НЕТ ОБРАТНОГО
        // в reduce_sum суммируем ошибки

        
        


///////////////////////// ТО, ЧТО НИЖЕ ВЫПОЛНЯЕТСЯ ВЕРНО
        if (l != 0 && j == k /* && tnum == j % p */) {
            //if (tnum == j % p) {
            //if (tnum == 0) {
                pa = A + j * n * m + j * av * m;
                for (r = 0; r < av; r++) { // тут работаем только с первым блоком (делаем первый шаг, чтобы записать указатели V1, V2, V3)
                    for (q = 0; q < ah; q++) {
                        V1[r * ah + q] = pa[r * ah + q];
                        V3[r * ah + q] = (r == q);
                    }
                }

                if (jord_inverse(V1, V3, av, ERROR) == 0) { // ищем матрицу с наименьшей нормой обратного
                    min   = 1. / st_norm(V3, av);
                    min_i = j;

                    
                    
                    a->i     = min_i;
                    a->norm  = min;
                    //a->uk    = V3;
                    
                    
                    
                    
                    //copy(V3, a->uk, m, m);

                }
                else {
                    min = 0;
                    a->norm  = min;

                    //// added lines
                    c++;
                    a->error = c;
                    printf("error = %d\n", (int)a->error);
                }
            //}

            // выбор обратного в V3 если остаток ненулевой
        }


        // требуется ли суммировать ошибки? можно использовать в функции reduce_min и уже там определять, нашли ли потоки главный элемент или нет
        // лучше делать c++ если мы не нашли обратимый блок и суммировать и если они вообще все необратимые, то кидать ошибку
        // после этого делается синхронизация с помощью reduce_min
        
        //printf("%d        %d\n", tnum, (int)a->error);

/////////////////////////////////////// ОБРАБОТКА БЛОКОВ ВЫПОЛНЕНА ПРАВИЛЬНО //////////////
        //reduce_sum(p, &a->error, 1);
////////////////////////////////////// НАДО СДЕЛАТЬ ЭТУ ШТУКУ, КОГДА РАСПАРАЛЛЕЛИТСЯ ВЫБОР ЭЛЕМЕНТА

        /* printf("поток %d, шаг %d      ошибка %d\n", tnum, j, (int)a->error);
        printf("какая не должна быть сумма %d    последний ли шаг%d\n", (int)a->error == k - j + int(av == l), int(av == l)); */
        
        
        //if ((int)a->error == 1 && av == l) {
        //    return -(j + 1);
        //}
        
        
        
        //printf("thread # = %d, %d\n", tnum, j+1);
        if ((int)a->error == k - j + int(av == l)) { // если все блоки необратимы, то возвращаем фигню
            //print_matrix(A,n,n,m,n);
            //printf("thread # = %d, %d", tnum, j+1);
            return -(j + 1);
            //printf("%d   \n", tnum);
        }
        
///////////////////////////////////////////////////////////////////////////////////////////
        //if (j == k) printf("%d   %f ___ %lf\n", tnum, a->error, a->norm); // на шаге k это не выполняется если нацело поделилось

        //reduce_min(p, a); // эта синхронизация выполняется верно. проблема начинается в умножении

        //V3 = a->uk;
        
        
        
        
        //copy(a->uk, V3, av, av);

        // V3 ищется верно, но где-то дальше она ведет себя неправильно и обнуляется, и меняется



    







        ////////////////////////////////////// ВСЕ ЧТО НИЖЕ - РАБОТАЕТ

        reduce_sum(p);

       if (min_i != j && j < k) { // меняем местами строки блоков
            for (i = j + tnum; i * m < n; i += p) {
                q  = (i < k) ? m : l; // мы помним что нумерация блоков идет с 0, поэтому столб k содержит неквадратные блоки
                pi = A + min_i * n * m + i * q * m;
                pj = A + j * n * m + i * q * m;

                for (r = 0; r < m; r++) {
                    for (c = 0; c < q; c++) {
                        current       = pi[r * q + c];
                        pi[r * q + c] = pj[r * q + c];
                        pj[r * q + c] = current;
                    }
                }
            }
            if (tnum == j % p) {
                pi = B + min_i * m;
                pj = B + j * m;

                for (r = 0; r < m; r++) { // меняем местами блоки свободного вектора
                    current = pi[r];
                    pi[r]   = pj[r];
                    pj[r]   = current;
                }
            }
        }
        reduce_sum(p);


        // 
////////////////////////////////////////////////////////////////////////////////////////////
//
        for (i = j + tnum; i * m < n; i += p) { // for (i = j + 1; i * m < n; i++) { 
            r = (i < k) ? m : l;
            pi = A + j * n * m + i * av * m;
            copy(pi, V1, av, r);
            multiply(V3, av, av, V1, av, r, V2);
            copy(V2, pi, av, r);
        }

        if (tnum == j % p) {
            pi = B + j * m;
            copy(pi, V1, av, 1);
            multiply(V3, av, av, V1, av, 1, V2);
            copy(V2, pi, av, 1);
        }
        reduce_sum(p);

        

////////////////////////////////////////////////////////////////////////////////////////////
        for (i = tnum; i * m < n; i += p) { // идем по строчкам вниз
            if (i != j){
                q = (i < k) ? m : l;
                pi = A + i * n * m + j * q * m;
                copy(pi, V1, q, av);

                for (c = j; c * m < n; c++) {
                    ah = (c < k) ? m : l;
                    pa = A + i * n * m + c * q * m;
                    pj = A + j * n * m + c * av * m;

                    copy(pj, V2, av, ah);
                    multiply(V1, q, av, V2, av, ah, V3);
                    extract(pa, q, ah, V3, q, ah);
                }
                pa = B + i * m;
                pj = B + j * m;
                copy(pj, V2, av, 1);
                multiply(V1, q, av, V2, av, 1, V3);
                extract(pa, q, 1, V3, q, 1);
            }
        }

        
////////////////////////////////////////////////////////////////////////////////////////////
        reduce_sum(p);
    }
    reduce_sum(p);

    if (tnum == 0) copy(B, X, n, 1);

    return 0;
}









