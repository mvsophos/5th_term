#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <math.h>

#define eps 1e-15

enum class io_status{
    undef, error_open, error_read, success
};

class Param{
public:
    double length;
    double maxc;
};

class Args{
public:
    pthread_t tid = -1;
    int p = 0; // количество потоков
    int k = 0; // номер потока
    const char *name = nullptr; // имя файла
    double res = 0; // сюда считаем количество
    Param param;
    //int count = 0; // количество элементов
    io_status error_type = io_status::undef;
    double error = 0;
    double error_flag = 0;
};

void reduce_sum(int p, double *a = nullptr, int n = 0);

void reduce_sum(int p, double *a, int n){
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static double *r = nullptr;
    int i;
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else for(i = 0; i < n; i++) r[i] += a[i];
    t_in++;
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a) for (i = 0; i < n; i++) a[i] = r[i];
    t_out++;
    if (t_out >= p){
        t_in = 0;
        r = nullptr;
        pthread_cond_broadcast(&c_out);
    }
    else while (t_out < p) pthread_cond_wait(&c_out, &m);
    pthread_mutex_unlock(&m);
}

void reduce_max(int p, Args *ptr){ // эта функция синхронизирует максимумы (в данном случае длины постоянной последовательности максимальной длины и элемента постоянного в ней)
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t c_in = PTHREAD_COND_INITIALIZER;
    static pthread_cond_t c_out = PTHREAD_COND_INITIALIZER;
    static int t_in = 0; // кол-во вошедших
    static int t_out = 0; // кол-во вышедших
    static Args *r = nullptr;

    Args *a = (Args *)ptr;
    // int i;
    if (p <= 1) return;
    pthread_mutex_lock(&m);
    if (r == nullptr) r = a;
    else{
        if (int(a->error) != 0) r->error = a->error;
        else
            if (r->param.length < a->param.length || ((int(r->param.length) == int(a->param.length)) && (r->param.maxc < a->param.maxc))){
                r->param.length = a->param.length;
                r->param.maxc = a->param.maxc;
            }
    }
    t_in++;
    if (t_in >= p){
        t_out = 0;
        pthread_cond_broadcast(&c_in);
    }
    else while(t_in < p) pthread_cond_wait(&c_in, &m);

    if (r != a){
        a->param = r->param;
        a->error = r->error;
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

double find_max_len(FILE *fp, Param *param){
    double last, curr, max = 0;
    int c = 0, len = 0;
    if (fscanf(fp, "%lf", &last) != 1){
        // return 1;
        //return io_status::error_read;
        if (feof(fp)){
            param->maxc = 0;
            param->length = 0;
            return 0;
        }
        else return -1;
    }
    while (fscanf(fp, "%lf", &curr) == 1){
        if (fabs(last - curr) < eps) c++;
        else{
            if (c > len){
                len = c + 1;
                max = last;
            }
            c = 0;
        }
        last = curr;
    }
    if (!feof(fp)) return 1; //return io_status::error_read;
    
    if (c > len){
        len = c + 1;
        max = last;
    }

    param->maxc = max;
    param->length = len;
    //printf("%lf  %d    ", *maxc, len); DELEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEETE
    return 0; //io_status::success;
}

io_status find_values(FILE *fp, double maximum, double *amount){ // maximum это то значение, которое нашли при первом просмотре. amount это количество элементов, больших maximum
    double curr;
    double res = 0;
    while (fscanf(fp, "%lf", &curr) == 1){
        if (curr > maximum){
            res += 1;
            //printf("num %lf num", curr); DELETEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
        }
    }
    if (!feof(fp)) return io_status::error_read;
    *amount = res;
    return io_status::success;
}

// многопоточная функция
void *thread_func(void *arg){
    double result;
    Args *a = (Args *) arg;
    FILE *fp = fopen(a->name, "r");
    if (fp == nullptr){
        a->error_type = io_status::error_open;
        a->error_flag = 1;
    }
    else a->error_flag = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){
        if (fp != nullptr) fclose(fp);
        return 0;
    }

    result = find_max_len(fp, &a->param);
    a->res = result;

    if (int(a->res) != 0) a->error_flag = 1;
    else a->error_flag = 0;

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){
        if (fp != nullptr) fclose(fp);
        return 0;
    }
    //a->error_type = io_status::success;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /* if (a->error_flag != 0){
        if (fp != 0) fclose(fp);
        return nullptr;
    } */
    
    fclose(fp);
    a->error_type = io_status::success;
    reduce_max(a->p, a);
    //reduce_sum(a->p, &a->param.maxc, 1);
    //reduce_sum(a->p, &a->param.length, 1);
    //printf(" %lf %lf    ", a->param.maxc, a->param.length);


    fp = fopen(a->name, "r");
    if (fp == nullptr){
        a->error_type = io_status::error_open;
        a->error_flag = 1;
    }
    else a->error_flag = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){
        if (fp != nullptr) fclose(fp);
        return 0;
    }

    a->error_type = find_values(fp, a->param.maxc, &a->res);

    if (a->error_type != io_status::success){
        a->error_flag = 1;
    }
    else a->error_flag = 0;

    reduce_sum(a->p, &a->error_flag, 1);

    if (a->error_flag > 0){
        if (fp != nullptr) fclose(fp);
        return 0;
    }

    fclose(fp);
    a->error_type = io_status::success;
    reduce_sum(a->p, &a->res, 1);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    return 0;
}

int process_args(Args *a){
    for (int i = 0; i < a->p; i++)
        if (a[i].error_type != io_status::success){
            //printf("%lf %d  ", a[i].error_flag, i);
            printf("Error in file %s \n", a[i].name);
            return 1;
        }
    return 0;
}

int main(int argc, char *argv[]){
    Args *a;
    int k, p = argc - 1;
    if (argc == 1){
        printf("Usage: %s files\n", argv[0]);
        return 1;
    }
    a = new Args[p];
    if (a == nullptr){
        printf("Ошибка конструктора\n");
        delete [] a;
        return 2;
    }
    for (k = 0; k < p; k++){
        a[k].k = k;
        a[k].p = p;
        a[k].name = argv[k + 1];
    }
    for (k = 1; k < p; k++){
        if (pthread_create(&a[k].tid, nullptr, thread_func, a + k)){
            printf("Ошибка при создании потока %d\n", k);
        }
    }
    a[0].tid = pthread_self();
    thread_func(a + 0);

    for (k = 1; k < p; k++){
        pthread_join(a[k].tid, 0);
    }
    if (!process_args(a))
        printf("Result = %d\n", (int)a[0].res);
    delete [] a;
    return 0;
}


// if (a->error_type == io_status::error_read) printf("%s   ", a->name);
