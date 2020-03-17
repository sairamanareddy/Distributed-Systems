#include <vector>
#include <iterator>
#include <numeric>
#include <cassert>
#include <sstream>
#include <mpi.h>
#include <cstdio>
#define getpid() 0
using namespace std;
#define printgraph(graph) for(auto& i : graph) for(auto& j : i) cout << j << " "; cout << endl;
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm intracomm, new_intracomm;
	MPI_Comm_get_parent(&intracomm);
	int world_rank, group_size, ierr;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &group_size);
	fprintf(stderr, "Process %d spawned in group with size %d, ABS PID: %d\n", world_rank, group_size, getpid());
	if (intracomm != MPI_COMM_NULL)
		goto section;
	else
		goto init_section;
init_section: //done only once by the master process which starts first.
{
	fprintf(stderr, "Process %d entered init_section\n", world_rank);
	int abc, I_r, I_b;
	int W_r, W_b, L_b, L_r, L_snd, p, q;
	fscanf(stdin, "%d %d %d %d %d %d %d %d %d %d", &abc, &W_r, &I_r, &W_b, &I_b, &L_b, &L_r, &L_snd, &p, &q);
	fprintf(stderr, "Process%d: %d %d %d %d %d %d %d %d %d %d\n", world_rank, abc, W_r, I_r, W_b, I_b, L_b, L_r, L_snd, p, q);
	getchar(); // to consume '\n' that remains.
	vector<vector<int>> graph(abc);
	string line;
	for (int i = 0, temp; i < abc; i++)
	{
		getline(cin, line);
		istringstream ss(line);
		ss >> temp;
		std::copy(istream_iterator<int>{ss}, istream_iterator<int>{}, std::back_inserter(graph[temp - 1]));
		line.clear();
	}
	if (intracomm == MPI_COMM_NULL)
	{
		assert(group_size == 1);
		fprintf(stderr, "Process %d entered spawning section\n", world_rank);
		MPI_Comm intercomm;
		fprintf(stderr, "Line 46: Process%d: %d %d %d %d %d %d %d %d %d %d\n", world_rank, abc, W_r, I_r, W_b, I_b, L_b, L_r, L_snd, p, q);
		printgraph(graph);
		MPI_Comm_spawn("a.out", MPI_ARGV_NULL, 4, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, &ierr);
		fprintf(stderr, "Line 48: Process%d: %d %d %d %d %d %d %d %d %d %d\n", world_rank, abc, W_r, I_r, W_b, I_b, L_b, L_r, L_snd, p, q);
		printgraph(graph);
		// fprintf(stderr, "Process %d: N is %d\n",world_rank, N);
		fprintf(stderr, "Line 49: Process %d: %d Processes created. &abc is %p\n", world_rank, abc, &abc);
		MPI_Intercomm_merge(intercomm, 0, &intracomm);
	}
	// N processes in the same group with intracommunicator as intracomm.
	// Now form the topology using MPI_Dist_Graph_Create
	int *nodes = new int[abc];
	int *degrees = new int[abc];
	vector<int> targets;
	std::iota(nodes, nodes + abc, 0);
	for (int i = 0; i < abc; i++)
	{
		degrees[i] = graph[i].size();
		targets.insert(targets.end(), graph[i].begin(), graph[i].end());
	}
	MPI_Dist_graph_create(intracomm, abc, nodes, degrees, targets.data(), MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, 0, &new_intracomm);
	fprintf(stderr, "Graph topology created by process %d", world_rank);
}

section:
	MPI_Comm_rank(intracomm, &world_rank);

	// cerr << "Now creating graph topology";
	// MPI_Dist_graph_create_adjacent(intracomm, graph[world_rank].size(), graph[world_rank].data(), MPI_WEIGHTS_EMPTY,
	// graph[world_rank].size(), graph[world_rank].data(), MPI_WEIGHTS_EMPTY, MPI_INFO_NULL, 0, &new_intracomm);
	// intracomm = new_intracomm;
	// cerr << "Graph topology now created.\n";
	fprintf(stderr, "Process %d Done!\n", world_rank);
	MPI_Finalize();

	// intracomm now contains the process topology.
}
