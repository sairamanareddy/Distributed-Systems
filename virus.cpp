#include <vector>
#include <iterator>
#include <numeric>
#include <cassert>
#include <sstream>
#include <random>
#include <algorithm>
#include <mpi.h>
#include <cstdio>
#include <unistd.h>
using namespace std;
#define BUFSIZE 1000
void printgraph(int const &r, vector<vector<int>> const &g)
{
	fprintf(stderr, "Process %d:\n", r);
	for (auto &i : g)
	{
		for (auto &j : i)
		{
			fprintf(stderr, "%d ", j);
		}
		fprintf(stderr, "\n");
	}
}
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm intracomm;
	char *buf = new char[BUFSIZE]{0};
	int world_rank, ierr;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	fprintf(stderr, "Process %d PID: %d\n", world_rank, getpid());
	// Now some IO
	MPI_File infile;
	// Open the input file in read-only mode.
	MPI_File_open(MPI_COMM_WORLD, "inp-params.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &infile);
	// Read info from the file. first into a char buffer and then into a stringstream.
	MPI_File_read(infile, buf, BUFSIZE, MPI_CHAR, MPI_STATUS_IGNORE);
	int N, I_r, I_b;
	int W_r, W_b, L_b, L_r, L_snd, p, q;
	stringstream instream(buf);
	instream >> N >> W_r >> I_r >> W_b >> I_b >> L_b >> L_r >> L_snd >> p >> q;
	fprintf(stderr, "Process %d: %d %d %d %d %d %d %d %d %d %d\n", world_rank, N, W_r, I_r, W_b, I_b, L_b, L_r, L_snd, p, q);
	vector<vector<int>> graph(N);
	string line;
	for (int i = 0, src, dst; i < N; i++)
	{
		getline(instream, line);
		istringstream ss(line);
		ss >> src;
		while (ss >> dst)
		{
			graph[src - 1].push_back(dst - 1);
		}
		line.clear();
	}
	int *nodes = new int[N];
	int *degrees = new int[N];
	printgraph(world_rank, graph);
	vector<int> targets;
	for (int i = 0; i < N; i++)
	{
		nodes[i] = i;
		degrees[i] = graph[i].size();
		targets.insert(targets.end(), graph[i].begin(), graph[i].end());
	}
	MPI_File_close(&infile);
	// END OF IO
	// process 0 creates the graph topology.
	if (world_rank == 0)
	{
		MPI_Dist_graph_create(MPI_COMM_WORLD, N, nodes, degrees, targets.data(), MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);
		fprintf(stderr, "Graph topology created!\n");
	}
	else
		MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL, NULL, MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);
	// fprintf(stderr, "Process %d entered barrier at line 56\n", world_rank);
	MPI_Barrier(MPI_COMM_WORLD);
	// fprintf(stderr, "Process %d exited barrier at line 58\n", world_rank);
	vector<int> indices(N);
	iota(indices.begin(), indices.end(), 0);

	fprintf(stderr, "Process %d Done!\n", world_rank);
	MPI_Finalize();
}
