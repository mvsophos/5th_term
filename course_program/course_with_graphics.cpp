#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <cstring>

#define eps 1e-15



// void u_setting(double *u, double (*u_function()), int N) - передача функции через указатель

inline double u_function(/* double *u_ar,  */double x, double time) // зависит от времени и точки x
{
    return 1;
    //return 0; // тут формулой задается сама функция u
}
inline double p0_function(double x) // функция распределения плотности в начальный момент времени
{
    return (x > 0.2 && x < 0.4) ? 1 : 0;
    // return sin(2 * 3.1415926 * x);
    // return 0.25;
}
double f(double x) // также можно задать ее с помощью массива значений в полуцелых точках
{
    return 0;
    // return 0.1;
}

// помним, что в функциях ниже мы задаем значения функции в полуцелых точках
// в функциях u_function, p0, f, x - это аргумент (точнее полуцелая точка)
// u_ar - это массив для u
void setup_u(double *u_ar, double h, double t, int N){
    for (int i = 0; i < N; i++){
        u_ar[i] = u_function(h * (i + 0.5), t);
    }
}

void setup_p0(double *p0, double h, double t, int N){
    for (int i = 0; i < N; i++){
        p0[i] = p0_function((i + 0.5) * h);
    }
}

void setup_f(double *right_part, double h, double t, int N){
    for (int i = 0; i < N; i++){
        right_part[i] = f((i + 0.5) * h);
    }
    // printf("%lf  \n", (N) * h);
}

double sgn(double x)
{
    if (x < eps && -x < eps) return 0;
    else if (x > 0) return 1.0;
    return -1.0;
}

void print_matrix(double *p15, double *p5, double *p_5, double *right_part, int N){
    if (N <= 15) // если не слишком много элементов
        for (int i = 0; i < N; i++){
            for (int j = 0; j < N; j++){
                // можно переделать функцию вывода так, чтобы она правильно выводила знаки для ровного вывода
                if (j < i - 1 || j > i + 1)         printf("        ");
                else if (j == i - 1 && i != 0)      printf("%5.1lf   ", *(p_5 + i));
                else if (j == i)                    printf("%5.1lf   ", *(p5 + i));
                else if (j == i + 1 && i != N - 1)  printf("%5.1lf   ", *(p15 + i));
            }
            printf(" | %5.5lf\n", right_part[i]);
        }
}

void print_array(double *mas, int N){
    if (N <= 100) {
        printf("[");
        for (int i = 0; i < N; i++) printf(" %lf; ", mas[i]);
        printf("]\n");
    }
}

// i --- это номер уравнения, t_1 - это единица деленная на шаг по времени
void setup_eq(double *u, double *phalf1, double *phalf, double *p_half, double *right_part, double *p_prev, double t_1, /* double h, */ int i, int N)
{
    /* phalf[i] += 1.0 / t; // это левая часть
    right_part[i] += p_prev[i] / t; */

    phalf[i] += t_1;
    right_part[i] += t_1 * p_prev[i];


    // с условием ниже это уравнение не будет иметь выходы справа и слева (то есть частицы будут накапливаться у стены)
    //if (i == N-1); // это поток через правую границу
    /* else */ if (sgn(u[i]) > 0 && sgn(u[i+1]) > 0){
        phalf1[i] += (0.5 * u[i+1] - 0.5 * (u[i] + u[i+1])) * N;
        phalf[i]  += (0.5 * u[i] + 0.5 * (u[i] + u[i+1])) * N; //p_half[i] += 0;
    }
    else if (sgn(u[i]) < 0 && sgn(u[i+1]) < 0){
        phalf1[i] += (0.5 * u[i+1] + 0.5 * (u[i] + u[i+1])) * N;
        phalf[i]  += (0.5 * u[i] - 0.5 * (u[i] + u[i+1])) * N; //p_half[i] += 0;
    }
    else if ((int)sgn(u[i]) != (int)sgn(u[i+1]) && u[i] + u[i+1] > 0){
        phalf[i]  += 0.5 * (u[i] + u[i+1]) * N;
    }
    else{ // может ли это условие не работать если я сравниваю с нулем и как это измениться, если 0 лежит в окрестности 1e-15
        phalf1[i] += 0.5 * (u[i] + u[i+1]) * N;
    }



    // if (i == 0); // это поток через левую границу
    /* else */ if (sgn(u[i-1]) > 0 && sgn(u[i]) > 0){
        phalf[i]  -= (0.5 * u[i] - 0.5 * (u[i] + u[i-1])) * N;
        p_half[i] -= (0.5 * u[i-1] + 0.5 * (u[i] + u[i-1])) * N;
    }
    else if (sgn(u[i-1]) < 0 && sgn(u[i]) < 0){
        phalf[i]  -= (0.5 * u[i] + 0.5 * (u[i] + u[i-1])) * N;
        p_half[i] -= (0.5 * u[i-1] - 0.5 * (u[i] + u[i-1])) * N;
    }
    else if ((int)sgn(u[i-1]) != (int)sgn(u[i]) && u[i] + u[i-1] < 0){
        phalf[i]  -= 0.5 * (u[i-1] + u[i]) * N;
    }
    else{
        p_half[i] -= 0.5 * (u[i-1] + u[i]) * N;
    }
}

void solver(double *p_half, double *phalf, double *phalf1, double *pprev, int N){ // pprev - это правая часть, и вместе с тем туда пишется решение
    phalf1[0] /= phalf[0];
    pprev[0] /= phalf[0];
    phalf[0] = 1;
    for (int i = 1; i <= N - 1; i++){
        // p_half[i] -= p_half[i] * phalf[i - 1];
        pprev[i]  -= p_half[i] * pprev[i - 1];
        phalf[i]  -= p_half[i] * phalf1[i - 1];
        p_half[i] = 0;
        // сейчас элемент p_half[i] = 0
        phalf1[i] /= phalf[i];
        pprev[i]  /= phalf[i];
        phalf[i]  =  1;
    }
    for (int i = N - 1; i >= 1; i--){
        // pprev[i] = f[i]; // запоминаем плотность на прошлом шаге
        pprev[i - 1] -= pprev[i] * phalf1[i - 1];
        phalf1[i - 1] = 0;
    }
}

// задание массива уравнений (задание коэффициентов выше, на и ниже диагонали), t_1 - это единица деленная на шаг по времени
void setup_array(double *u, double *phalf1, double *phalf, double *p_half, double *right_part, double *p_prev, double t_1, /* double h, */ int N){
    for (int i = 0; i < N; i++){
        setup_eq(u, phalf1, phalf, p_half, right_part, p_prev, t_1, i, N);
    }
}

// функция должна принимать одну и ту же функцию, заданную в массиве right_part
// N - это точек в дискретизации пространства, M - шагов по времени
void solver_in_time(double *u, double *phalf1, double *phalf, double *p_half, double *right_part, double *p0, double *f, double t_1, double h, int N, int M){
    FILE *myfile = NULL;
    char name_of_file[20];
    for (int j = 0; j < M; j++){
        setup_array(u, phalf1, phalf, p_half, right_part, p0, t_1, N);
        solver(p_half, phalf, phalf1, p0, N);



        // пишем решение в файл
        int n = sprintf(name_of_file, "it_%04d.txt", j + 1);
        myfile = fopen(name_of_file, "w");
        for (int i = 0; i < N; i++) {
            fprintf(myfile, "%lf  ", right_part[i]);
        }
        fclose(myfile);
    }
}




// в этой программе нет выхода частиц справа и нет выхода частиц слева
int main(int argc, char *argv[])
{
    // чтение ведется из какого-то файла, который передаем как аргумент
    // вводим массив длины N. Три массива значения p[i] = p_{i+0.5}, u[i] = u_{i+0.5}, где i = 0,...,N-1
    int N, M; // читаем количество точек из строки
    double length_of_segment = 1.;
    double h; /* = length_of_segment / N; */ // шаг между полуцелыми точками (он равен шагу между целыми точками)
    double t = 0.01; // шаг по времени
    double t_1 = 1. / t;

    FILE *myfile = NULL;
    // char *filename_solution = "solution_array.txt";

    //N = (int)argv[1];
    if (!((argc == 3) && (sscanf(argv[1], "%d", &N) == 1) && (sscanf(argv[2], "%d", &M) == 1))){
        printf("Укажите N (число точек)\n");
        return -1;
    }
    h = length_of_segment / N;
    
    //else printf("%d\n", N);

    double *phalf1 = new double[N - 1]; // массив \rho_{i+1.5}, верхняя диагональ
    double *phalf = new double[N];  // массив \rho_{i+0.5}, главная диагональ
    double *p_half = new double[N - 1]; // массив \rho_{i-0.5}, нижняя  диагональ
    double *right_part = new double[N]; // вектор правой части (задается значениями f в полуцелых точках)
    double *p0 = new double[N]; // массив распределения плотности в начальный момент времени
    double *p_prev = new double[N];
    double *u = new double[N];
    double *f = new double [N]; // дополнительный указатель, передаваемый для того, чтобы сохранить правую часть

    printf("Шаг по пространству h = %e, шаг по времени t = %e, количество точек N = %d\nНачальное распределение p0:  ", h, t, N);

    setup_u(u, h, t, N);
    setup_p0(p0, h, t, N);
    setup_f(right_part, h, t, N);

    print_array(p0, N);

    // myfile = fopen(filename_solution, "w");
    char name_of_file[20];
    int n = sprintf(name_of_file, "it_%04d.txt", 0);
    //printf("%d\n", n);
    myfile = fopen(name_of_file, "w");
    for (int i = 0; i < N; i++) {
        fprintf(myfile, "%lf  ", p0[i]);
    }
    fclose(myfile);

    setup_array(u, phalf1, phalf, p_half, right_part, p0, t_1, N);
    // print_matrix(phalf1, phalf, p_half, right_part, N);
    solver(p_half, phalf, phalf1, right_part, N);
    // print_matrix(phalf1, phalf, p_half, right_part, N);

    solver_in_time(u, phalf1, phalf, p_half, right_part, right_part, f, t_1, h, N, M - 1);

    // print_matrix(phalf1, phalf, p_half, right_part, N);

    double o = 0;
    for (int i = 0; i < N; i++) o += right_part[i];
    printf("\n summa = %lf \n", o);

    print_array(right_part, N);


    // далее пишем решение в файл
    
    
    
    
    
    
    
    // solver(p_half, phalf, phalf1, right_part, p_prev, N);

    return 0;
}






















/* 



#define eps 1e-15

int main(int argc, char *argv[])
{
    // чтение ведется из какого-то файла, который передаем как аргумент
    // вводим массив длины N. Три массива значения p[i] = p_{i+0.5}, u[i] = u_{i+0.5}, где i = 0,...,N-1
    int N; // читаем количество точек из строки
    double length_of_segment = 1;
    double h = length_of_segment / N;
    double t;
    double *p0 = new double[N + 1]; // массив распределения плотности в начальный момент времени
    double *p_next = new double[N + 1];
    double *p = new double[N + 1];
    double *u = new double[2 * N + 1]; // в условии задачи u постоянная, поэтому можно было бы задавать ее массивом значений, который задается формулой функции
/*     double *pu = new double[2 * N + 1];
    double *pur = new double[N];
    double *pul = new double[N]; // коммент многострочный был до сюда
    ////
    return 0;
}

// void u_setting(double *u, double (*u_function()), int N) - передача функции через указатель
inline double p0(double x) // функция распределения плотности в начальный момент времени
{
    return 0;
}

inline double ui(double u_half, double uhalf) // u_half это u_{i-0.5}, uhalf это u_{i+0.5}
{
    return 0.5 * (u_half + uhalf);
}

inline double pui(double p_half, double phalf, double u_half, double uhalf)
{
    return 0.5 * (phalf * uhalf  +  p_half * u_half);
}

inline double pul_ihalf(double p_half, double phalf, double u_half, double uhalf, int i) // это \{pu\}^{left}_{i + 0.5}
{
    if (i == 0); return 0;
}

inline double pur_i_half(double p_half, double phalf, double u_half, double uhalf, int i, double N /* быть может лучше сделать эту переменную глобальной (или например через #define)) // это \{pu\}^{right}_{i - 0.5}
{
    if (i == N) return 0;
}

double u_function(double x, double time) // зависит от времени и точки x
{
    return 0; // тут формулой задается сама функция u
}

inline double sgn(double x)
{
    if (x < eps && -x < eps) return 0;
    else if (x > 0) return 1.0;
    return -1.0;
}

// массив pu_i состоит из 2N+1 элемента */