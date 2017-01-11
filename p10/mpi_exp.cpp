#include "mpi.h"
#include <iostream>
#include "stdlib.h"
using namespace std;

double fact(int n) { 
    if (n == 0) {
        return 1;
    } else { 
        return n * fact(n - 1); 
    }
}

int main(int argc, char *argv[]) {

  int terms_cnt, id, process_cnt, i;

  long double exp = 0;
  long double result_exp, start_time, finish_time;
  
  terms_cnt = atoi(argv[1]);

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD,&process_cnt); 
  MPI_Comm_rank(MPI_COMM_WORLD,&id); 

  if (id == 0) { 
    start_time = MPI_Wtime();
  }

  MPI_Bcast(&terms_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);

  for (i = id; i <= terms_cnt; i += process_cnt) {
    exp += 1/fact(i);
  }

  MPI_Reduce(&exp, &result_exp, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  cout.precision(20);
  
  if (id == 0) {   
    cout << "Exp(1): " << result_exp << endl; 
    finish_time = MPI_Wtime();
    cout << "Time: "<<(finish_time - start_time) * 1000 << endl;      
  }

  MPI_Finalize();
  return 0;
}