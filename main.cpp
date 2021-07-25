#include <mpi.h>
#include<omp.h>
#include <stdio.h>
#include <cstdlib>
#include<iostream>
#include<unistd.h>
using namespace std;

//the size of the array that is to be distributed amoung processes
#define num_size 55	//size of array that will be distributed among slave processes (5 elements per 1 slave process)

int main(int argc, char *argv[]){
	
	int search_num = 48;	//number that needs to be searched
	int* data_set1;		//array that will have the numbers data set from which we will search
	int myrank, nprocs;
	int dest = 1;
	int master_tag = 1;	//tag for messages sent from master process
	int worker_tag = 2;	//tag for messages sent from slave process
	int ind = 0;
	int abort = 0;		//variable to signal the slave to abort search
	int signal = 0;		//variable to get signal from slave if number is found
	int not_found = 1;	//variable to get signal if number not found by slave
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);	
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int div = num_size/(nprocs-1);		//getting the number of elements per process
	MPI_Status status;
	//*********************************master process*******************************8
	if(myrank == 0) {
	cout<<"Name: Areesha Tahir "<<"Roll no: 18I-1655"<<" Sec: A"<<endl;
		data_set1 = new int[num_size];		//dynamically giving array  size
		int num = 1;
		for(int i = 0; i < num_size; i++){	//populating the array with numbers
			data_set1[i] = num;
			num++;
		}
		cout<<"Process 0 has data input ";
		for(int i = 0; i < num_size; i++){	//populating the array with numbers
			cout<<data_set1[i]<<" ";
		}
		cout<<endl;
		//cout<<"IN THE MASTER PROCESS\n";
		cout<<"Master Process: Number to search "<<search_num<<endl;
		
	//********************************sending the distributed array to all the processes***************************************
		while (dest < nprocs){			
			#pragma omp critical 
			{
				MPI_Send(&search_num, 1, MPI_INT, dest, master_tag, MPI_COMM_WORLD);	//sending number to be searched
				MPI_Send(&data_set1[ind], div, MPI_INT, dest, master_tag, MPI_COMM_WORLD);	//sending distributed array
				dest += 1;
				ind = ind + div;						//getting next division index of array for next process
			}
		}
		
		dest = 1;
//******************************Using openMP and MPI to find the number******************************************
		while(signal == 0){
			while(dest < nprocs){
				#pragma omp critical 
				{
					MPI_Send(&abort, 1, MPI_INT, dest, master_tag, MPI_COMM_WORLD);	//if number not found tell all slaves to continue searching
					dest = dest + 1;
				}
			}
			dest = 1;
			//receiving signal from slaves to see if number found or not
			MPI_Recv(&signal, 1, MPI_INT, MPI_ANY_SOURCE, worker_tag, MPI_COMM_WORLD, &status);
			#pragma omp critical 
			{
			//if number is found
			if(signal == 1){
				cout<<"Master Process: Process "<<status.MPI_SOURCE<<" has found the number"<<endl;
				cout<<"Informing all processes to abort"<<endl;
				dest = 1;
				abort = 1;
				while(dest < nprocs){
					MPI_Send(&abort, 1, MPI_INT, dest, master_tag, MPI_COMM_WORLD);	//number found so sending abort siganl to all slaves
					dest = dest + 1;
					}
					sleep(2);
					exit(0);
				}
			}
			//if number not found
			if(signal == 2){
				not_found += 1; 
				if(not_found < nprocs){	//if number not found than keep incrementing not_found variable until it is equal to nprocs
					signal = 0;
				}
			//cout<<not_found<<endl;
				}
			}
			//if none of the processes have the number in their data set
			if (not_found == nprocs){
			cout<<"Master Process: Number not in data set"<<endl;
			exit(0);
			}
	
	}
	
	
	//**************************SLAVES***************************
	else {
		int recv_data[div];	//array to receive distributed array sent by master process
		int sig = 0;		//to signal if number has been found
		int id = 0;		
		int abort_sig = 0;	//abort signal to tell all slaves to abort search
		bool found = true;
		#pragma omp critical
		{
			MPI_Recv(&search_num, 1, MPI_INT, 0, master_tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	//getting number to search
			MPI_Recv(&recv_data[id], div, MPI_INT, 0, master_tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	//getting distributed array
			sleep(1);
			cout<<"Process "<<myrank<<" has data input ";	//printing the data set the slave process has
			//sleep(2);
			for(int i = 0; i < (div); i++){
				cout<<recv_data[i]<<" ";
			}
			cout<<endl;
			sleep(2);
				//cout<<"number = "<<search_num<<endl;
				#pragma omp barrier
		{
				int i = 0;
				for(; i<(div) ;i++){	
					MPI_Recv(&abort_sig, 1, MPI_INT, 0, master_tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//waiting to recv status of abort signal from master process
					//sleep(2);
					if (!abort_sig){	//if num not found then no need to abort as abort signal will be 0
						if(recv_data[i] == search_num){		//check if number exists in the given data set
							//cout<<"Process "<<myrank<<" : I have found the number"<<endl;
							sig = 1;		//if number is found set sig = 1
							MPI_Send(&sig, 1, MPI_INT, 0, worker_tag, MPI_COMM_WORLD);	//and send it to the master node to signal it that the number has been found 
							sleep(2);
				
							//break;
						}
					if(i == (div-1)){	//if all array searched set found to false
						found = false;
					}
					if(!found){	//if found is false than the number does not exist in this data set
						sig = 2;	//signal is 2 for nmber not found
						//exit(0);
						//cout<<"Not found"<<endl;
						MPI_Send(&sig, 1, MPI_INT, 0, worker_tag, MPI_COMM_WORLD);	//send number not found signal to master process
						break;
						//sleep(2);
				
					}
					if(i != (div-1) & recv_data[i] != search_num){	//if number not found yet than keep searching
						MPI_Send(&sig, 1, MPI_INT, 0, worker_tag, MPI_COMM_WORLD);
					}
						
				}
			}
			if(sig != 0) {	//if number found by another process and signal from master process recv than abort searching
			while(!abort_sig){
			MPI_Recv(&abort_sig, 1, MPI_INT, 0, master_tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if(abort_sig){
			sleep(1);
					cout<<"PROCESS "<<myrank<<" SEARCH ABORTED"<<endl;
					//sleep(1);
					//found = true;
					break;
				}
				}
				//sleep(1);
				}
		
	}
	}	
		//printf("IN THE SLAVE PROCESS %d\n", myrank);
	}
	
	MPI_Finalize();
	return 0;
}
