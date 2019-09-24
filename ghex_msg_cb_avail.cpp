#include <iostream>
#include <vector>
#include "tictoc.h"

#ifdef USE_MPI
#include "communicator_mpi.hpp"
using CommType = gridtools::ghex::mpi::communicator;
#else
#ifdef USE_UCX_NBR
#include "communicator_ucx_nbr.hpp"
#else
#include "communicator_ucx.hpp"
#endif
using CommType = gridtools::ghex::ucx::communicator;
#endif

CommType comm;


#include "message.hpp"
using MsgType = gridtools::ghex::mpi::raw_shared_message<>;

/* available comm slots */
int *available = NULL;
int ongoing_comm = 0;

void send_callback(int rank, int tag, MsgType &mesg)
{
    // std::cout << "send callback called " << rank << " thread " << omp_get_thread_num() << " tag " << tag << "\n";
    available[tag] = 1;
    ongoing_comm--;
}

void recv_callback(int rank, int tag, MsgType &mesg)
{
    // std::cout << "recv callback called " << rank << " thread " << omp_get_thread_num() << " tag " << tag << "\n";
    available[tag] = 1;
    ongoing_comm--;
}

int main(int argc, char *argv[])
{
    int rank, size, threads, peer_rank;
    int niter, buff_size;
    int inflight;

    niter = atoi(argv[1]);
    buff_size = atoi(argv[2]);
    inflight = atoi(argv[3]);   
    
    rank = comm.m_rank;
    size = comm.m_size;
    peer_rank = (rank+1)%2;
    
    if(rank==0)	std::cout << "\n\nrunning test " << __FILE__ << " with communicator " << comm.name << "\n\n";

    {
	std::vector<MsgType> msgs;
	available = new int[inflight];

	for(int j=0; j<inflight; j++){
	    available[j] = 1;
	    msgs.emplace_back(buff_size, buff_size);
	}
	
	if(rank == 1) {
	    tic();
	    bytes = (double)niter*size*buff_size/2;
	}

	if(rank == 0){

	    /* send niter messages - as soon as a slot becomes free */
	    int sent = 0;
	    while(sent != niter){

		for(int j=0; j<inflight; j++){
		    if(available[j]){
			if(rank==0 && (sent)%(niter/10)==0) fprintf(stderr, "%d iters\n", sent);
			available[j] = 0;
			sent++;
			ongoing_comm++;
			comm.send(msgs[j], 1, j, send_callback);
			if(sent==niter) break;
		    }
		}
		if(sent==niter) break;
	    
		/* progress a bit: for large inflight values this yields better performance */
		/* over simply calling the progress once */
		// int p = 0.1*inflight-1;
		// do {
		//     p-=comm.progress();
		// } while(ongoing_comm && p>0);
		comm.progress();
	    }

	} else {

	    /* recv requests are resubmitted as soon as a request is completed */
	    /* so the number of submitted recv requests is always constant (inflight) */
	    /* expect niter messages (i.e., niter recv callbacks) on receiver  */
	    ongoing_comm = niter;
	    while(ongoing_comm){

		for(int j=0; j<inflight; j++){
		    if(available[j]){
			available[j] = 0;
			comm.recv(msgs[j], 0, j, recv_callback);
		    }
		}
	    
		/* progress a bit: for large inflight values this yields better performance */
		/* over simply calling the progress once */
		// int p = 0.1*inflight-1;
		// do {
		//     p-=comm.progress();
		// } while(ongoing_comm && p>0);
		comm.progress();
	    }	    
	}

	/* complete all comm */
	while(ongoing_comm){
	    comm.progress();
	}

	if(rank == 1) toc();
	comm.fence();
    }
}
