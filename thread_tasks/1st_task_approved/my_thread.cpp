#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <math.h>

// этот принцип нужен для большинства параллельных программ (тут выставляются точки синхронизации (там нельзя делиться данными))

#define epsilon 1e-15 // чтобы понять нестрогое неравенство

// класс ошибок
enum class io_status{
    undef, error_open, error_read, success
};

// можно сделать res и error_flag целочисленными (но одновременно)
class Args{
    public:
    pthread_t tid = -1;
    int p = 0; // количество потоков
    int k = 0; // номер потока
    const char *name = nullptr; // имя файла
    double res = 0;
    //int count = 0; // количество элементов
    io_status error_type = io_status::undef;
    double error_flag = 0;
};

void reduce_sum(int p, double *a = nullptr, int n = 0); // если аргументы в функцию не переданы, то она принимает аргументы идущие в конце по умолчанию

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

io_status minecraft(FILE *fp, double *res /* , int *count */){ // количество участков строгого возрастания
    double buf, prev1, prev2;
    if (feof(fp)) return io_status::success;
    if (fscanf(fp, "%lf", &prev2) != 1 && !feof(fp)) return io_status::error_read;
    //if (feof(fp)) return io_status::success;
    if (fscanf(fp, "%lf", &prev1) != 1) return io_status::success;
    while (fscanf(fp, "%lf", &buf) == 1){
        if ((buf < prev1 || fabs(buf - prev1) < epsilon) && (prev2 < prev1)) *res += 1;  // замени обратно res на count
        prev2 = prev1;
        prev1 = buf;
    }
    if (prev1 > prev2) *res += 1; // замени обратно res на count
    if (feof(fp)) return io_status::success; // это если дочитали файл до конца
    else return io_status::error_read; // если же в конце файла есть нечитаемые символы, то это считается ошибкой чтения
}

// сама многопоточная функция
void *thread_func(void *arg){
    Args *a = (Args *) arg; // [объявили указатель на класс. это массив аргументов потоков]
    FILE *fp = fopen(a->name, "r");
    if (fp == nullptr){ // если не открыли, то кидаем ошибку
        a->error_type = io_status::error_open;
        a->error_flag = 1;
    }
    else a->error_flag = 0;

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){
        if (fp != nullptr) fclose(fp);
        return 0; // на самом деле тут nullptr
    }

    a->error_type = minecraft(fp, &a->res /* , &a->count */);

    if (a->error_type != io_status::success) a->error_flag = 1; // если функция выполнилась неудачно, то кидаем ошибку
    else a->error_flag = 0;

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){ // копипастнули тот же if-блок, что выше
        if (fp != nullptr) fclose(fp);
        return 0; // на самом деле тут nullptr
    }
    fclose(fp);
    a->error_type = io_status::success;
    reduce_sum(a->p, &a->res, 1);
    return nullptr;
}

int process_args(Args *a){
    //if (a->error_type != io_status::success){
        // коммент ниже можно убрать, но он работает не так как задумано
    for (int i = 0; i < a->p; i++)
        if (a[i].error_type != io_status::success){
            printf("Error in file %s \n", a[i].name); // если хотя бы один из файлов не существует, то выводится, что ошибка даже в нормальных файлах
            return 1;
        }
    //}
    return 0;
}

int main(int argc, char *argv[]){
    Args *a;
    int k, p = argc - 1; // количество потоков (количество файлов которые мы ввели)
    if (argc == 1){
        printf("Usage: %s files\n", argv[0]);
        return 1;
    }
    a = new Args[p];
    if (a == nullptr){
        printf("Ошибка конструктора\n");
        return 2;
    }
    for (k = 0; k < p; k++){
        a[k].k = k; // одно есть название переменной в классе, а другое это переменная в функции main
        a[k].p = p;
        a[k].name = argv[k + 1];
    }
    for (k = 1; k < p; k++){
        // a + k - это адрес объекта
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)){ // отдельно напомню, что if выполняется, если значение в скобках отлично от нуля (нуль и прочие интерпретируемые как 0 являются ложными). а pthread_create возвращает 0 в случае успеха
            printf("Ошибка при создании потока %d\n", k);
            return 5;
        }
    }
    a[0].tid = pthread_self(); // эта функция возвращает идентификатор вызывающего потока
    thread_func(a + 0);

    // строка ниже эквивалентна барьеру (но если она не закомментирована, то программа не выполнится до конца)
    // reduce_sum(p); ////////////////////////////////////////////////////////// ЭТА ФУНКЦИЯ НЕ ВЫПОЛНЯЕТСЯ, точнее выполняется бесконечно, если потоков больше одного //////////////////////////////////////////////// если потоков два то выполняется бесконечно
    for (k = 1; k < p; k++){
        pthread_join(a[k].tid, 0); // 0 эквивалентен nullptr
    }
    if (process_args(a) == 0){
        printf("Result = %d\n", (int)a[0].res); // обработка ответа идет, когда потоки закончились
        delete [] a;
        return 0;
    }
    else{ // эта ветка выполнится, если process_args(a) != 0, то есть если есть ошибка при открытии или чтении. При этом эта функция выведет файлы, в которых была ошибка
        delete [] a;
        return -1;
    }
}
