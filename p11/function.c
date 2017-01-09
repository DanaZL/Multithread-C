#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include <time.h>
#include <openacc.h>

int main() {

    // printf("%s\n", "Функция: (3 * exp(x) - 9 * x + 1) / (x^2 + 1)");
    // printf("%s\n", "Введите два числа - начало и конец отрезка табулирования");

    double x1, x2;
    scanf("%lf%lf", &x1, &x2);

    // printf("%s\n", "Введите целое число - количество точек на отрезке табулирования");

    int cnt_dot;
    scanf("%d", &cnt_dot);

    // double* x_array = (double *)calloc(cnt_dot + 2, sizeof(int));
    // double* y_array = (double *)calloc(cnt_dot + 2, sizeof(int));

    // memset(x_array, 0, sizeof(x_array));
    // memset(y_array, 0, sizeof(y_array));
    int num = 0;

    double x;
    double y;

    double delta = (x2 - x1) / (cnt_dot + 1); 

    #pragma acc kernels
    for (double i = x1; i <= x2; i += delta) {
        // x_array[num] = i;
        // y_array[num] = (3 * exp(i) - 9 * i + 1) / (i * i + 1 + sin(i)*sin(i));
        y = (3 * exp(i) - 9 * i + 1) / (i * i + 1 + sin(i)*sin(i));
        if (num % ((int)(cnt_dot / 20)) == 0) {
            // printf("Iteration: %d, x = %lf, f(x) = %lf\n", num, i, y);
        }
        num += 1;
    }

    printf("%lf\n", clock()/1000.0);
    // free(x_array);
    // free(y_array);
    
}