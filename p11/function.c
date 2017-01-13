#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include <time.h>
#include <openacc.h>

int main() {

    printf("%s\n", "Функция: (3 * exp(x) - 9 * x + 1) / (x^2 + 1 + sin(x) * sin(x))");
    printf("%s\n", "Введите два числа - начало и конец отрезка табулирования");

    double x1, x2;
    scanf("%lf%lf", &x1, &x2);

    printf("%s\n", "Введите целое число - количество точек на отрезке табулирования");

    int cnt_dot;
    scanf("%d", &cnt_dot);

    double* y_array = (double *)calloc(cnt_dot + 2, sizeof(double));

    int num;
    double delta = (x2 - x1) / (cnt_dot + 1); 

    double start_time = clock(); 

#pragma acc kernels
    for (int i = 0; i < cnt_dot + 2; ++i) {
        y_array[i] = (3 * exp(x1 + delta * i) - 9 * (x1 + delta * i) + 1) / 
                    ((x1 + delta * i) * (x1 + delta * i) + 1 + sin(x1 + delta * i) * sin(x1 + delta * i));     
    }

    num = 0;
    for (double x = x1; x <= x2; x += delta) {
        if (num % ((int)(cnt_dot / 6)) == 0) {
            printf("Iteration: %d, x = %lf, f(x) = %lf\n", num, x, y_array[num]);
        }
        num += 1;
    }

    printf("Time: %lf\n", (clock() - start_time) / 1000.0);

    free(y_array); 
}
