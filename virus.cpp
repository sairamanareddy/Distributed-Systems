#include <bits/stdc++.h>
#include <mpi.h>
#include <unistd.h>
using namespace std;
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm intracomm, new_intracomm;
	MPI_Comm_get_parent(&intracomm);
	if (intracomm != MPI_COMM_NULL)
		goto section;
	int world_rank, group_size, ierr;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &group_size);
	fprintf(stderr, "Process %d spawned in group with size %d, ABS PID: %d\n", world_rank, group_size, getpid());
init_section: //done only once by the master process which starts first.
{
	int N, I_r, I_b;
	double W_r, W_b, L_b, L_r, L_snd, p, q;
	cin >> N >> W_r >> I_r >> W_b >> I_b >> L_b >> L_r >> L_snd >> p >> q;
	getchar(); // to consume '\n' that remains.
	vector<vector<int>> arr(N);
	string line;
	for (int i = 0, temp; i < N; i++)
	{
		getline(cin, line);
		istringstream ss(line);
		ss >> temp;
		std::copy(istream_iterator<int>{ss}, istream_iterator<int>{}, std::back_inserter(arr[temp - 1]));
		line.clear();
	}

	if (intracomm == MPI_COMM_NULL)
	{
		assert(group_size == 1);
		MPI_Comm intercomm;
		MPI_Comm_spawn(argv[0], MPI_ARGV_NULL, N - 1, MPI_INFO_NULL, world_rank, MPI_COMM_WORLD, &intercomm, &ierr);
		MPI_Intercomm_merge(intercomm, 0, &intracomm);
		cerr << "N Processes created\n";
	}
	// N processes in the same group with intracommunicator as intracomm.
	// Now form the topology using MPI_Dist_Graph_Create
	MPI_Dist_graph_create(intracomm)
}

	
section:
	MPI_Comm_rank(intracomm, &world_rank);

	// cerr << "Now creating graph topology";
	// MPI_Dist_graph_create_adjacent(intracomm, arr[world_rank].size(), arr[world_rank].data(), MPI_WEIGHTS_EMPTY,
	// arr[world_rank].size(), arr[world_rank].data(), MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, 0, &new_intracomm);
	// intracomm = new_intracomm;
	// cerr << "Graph topology now created.\n";
	cerr << "Process " << world_rank << " Done.\n";
	MPI_Finalize();

	// intracomm now contains the process topology.
}