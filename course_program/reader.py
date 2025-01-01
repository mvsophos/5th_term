import numpy as np
import matplotlib.pyplot as plt

# cis argv
# json, c++
# делать конфигурационную программу






# a = np.array([1, 4, 5, 8], float)

# plt.subplot
# set_ylim

# придумать как выводить ровно столько файлов сколько шагов по времени (M-1 файлов), иначе в других может содержаться информация с прошлых запусков
for k in range(0, 1000):
    #plt.close()
    plt.clf()
    a = np.loadtxt('it_%04d.txt' % (k))
    #a = np.loadtxt('solution_array.txt')
    a = np.array(a, float)

    # как написать баш скрипт, чтобы при запуске запускалась сначала программа,
    # вводила данные в файл, а потом из этого файла, вот этот файл читал числа и
    # строил много графиков градиентом

    # вывод массива из файла, если не слишком много точек
    if len(a) <= 10:
        print('[', end = '  ')
        for i in range(0, len(a)):
            print(a[i], end = '  ')
        print(']')


    x = np.array(a, float)
    for i in range(0, len(a)):
        x[i] = ((i + 0.5) / (len(a)))
        
    plt.plot(x, a)
    #plt.show()

# было бы здорово сохранять в отдельную папку
    plt.savefig('output_%04d.png' % (k), dpi=150, bbox_inches='tight')









# import matplotlib.animation as anime
# anim = anime.FuncAnimation(...)
# plt.show()
