#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "omp.h"

using namespace std;

#define SIZE 1000000
#define TESTS_CNT 10
#define THREADS_CNT 4

int sort(int* arr, long long left_idx, long long right_idx, int threads_cnt)
{   
    int less_elems[right_idx - left_idx];
    int greater_elems[right_idx - left_idx];
    long long i, j;
    int main_elem = arr[right_idx];
    int less_n = 0, greater_n = 0;

    for(i = left_idx; i < right_idx; i++) {
        if (arr[i] < main_elem) {
            less_elems[less_n] = arr[i];
            less_n++;
        } else {
            greater_elems[greater_n] = arr[i];
            greater_n++;
        }
    }

#pragma omp parallel num_threads(threads_cnt)
    {

#pragma omp  for
        for (i = 0; i < less_n; i++) {
            arr[left_idx + i] = less_elems[i];
        }

#pragma omp single
        arr[left_idx + less_n] = main_elem;
#pragma omp  for
        for (j = 0; j < greater_n; j++) {
            arr[left_idx + less_n + j + 1] = greater_elems[j];
        }
    }
    return left_idx + less_n;
}

void quick_sort(int *arr, long long left_idx, long long right_idx, int threads_cnt)
{
    int main_elem_idx;

    if (left_idx < right_idx) {

        main_elem_idx = sort(arr, left_idx, right_idx, threads_cnt);

#pragma omp parallel sections
        {
#pragma omp section
            {
                quick_sort(arr, left_idx, main_elem_idx - 1, threads_cnt);
            }
#pragma omp section
            {
                quick_sort(arr, main_elem_idx + 1, right_idx, threads_cnt);
            }

        }
    }
}

void fill_array(int* arr)
{
    srand(time(NULL));
    for (int i = 0; i < SIZE; ++i) {
        arr[i] = rand() % 1000 - 500;
    }
}

void print_arr(int *a)
{
    for (int i = 0; i < SIZE; i++){
        std::cout << a[i] << ' ';
    }
    std::cout << std::endl;
}

double run_test(int threads_cnt)
{
    omp_set_num_threads(threads_cnt);
    double time, sum_time = 0;

    cout << "TEST WITH " << threads_cnt << " THREADS" << endl;
    int *arr = (int *)malloc(SIZE * sizeof(int));

    for (int i = 0; i < TESTS_CNT; ++i) {
        fill_array(arr);
        // print_arr(arr);

        time = omp_get_wtime();
        quick_sort(arr, 0, SIZE - 1, threads_cnt);

        // print_arr(arr);
        time = omp_get_wtime() - time;
        cout << "TEST " << i << ": " << time << " sec" << endl;
        sum_time += time;
    }

    free(arr);
    cout << TESTS_CNT << " tests average time: " << sum_time/TESTS_CNT << endl << endl;
    return sum_time/TESTS_CNT;
}

int main()
{
    omp_set_num_threads(THREADS_CNT);

    cout << "ARRAY SIZE:" << SIZE << endl << endl;

    double openmp_time = run_test(4);
    double time = run_test(1);

    cout << "BOOST WITH 4 THREADS: " << 100 - (openmp_time / time) * 100 << "%"<< endl;

    return 0;
}