#include <vector>
#include <iterator>
#include <numeric>
#include <cassert>
#include <climits>
#include <sstream>
#include <random>
#include <algorithm>
#include <mpi.h>
#include <cstdio>
#include <unistd.h>
using namespace std;
#define BUFSIZE 1000
std::random_device __rd;
std::mt19937 gen(__rd());
// A spanning tree algo intial color is white.
char algo_token = 'W';
// App initial color is white for all.
char app_color = 'W';
void logwrite(MPI_File __log, const string &__s)
{
	MPI_File_write_shared(__log, __s.c_str(), __s.length(), MPI_CHAR, MPI_STATUS_IGNORE);
}
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
// void send(int dest_id)
// {
// 	MPI_Send()
// }
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm intracomm;
	MPI_Status stat;
	char *buf = new char[BUFSIZE]{0};
	int world_rank, flag;
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
	vector<vector<int>> graph(N);
	string line;
	getline(instream, line);
	for (int i = 0, src, dst; i < N; i++)
	{
		getline(instream, line);
		//fprintf(stderr, "In Graph, %s\n", line.c_str());
		istringstream ss(line);
		ss >> src;
		while (ss >> dst)
		{
			graph[src - 1].push_back(dst - 1);
		}
	}
	int *nodes = new int[N];
	int *degrees = new int[N];

	vector<int> targets;
	for (int i = 0; i < N; i++)
	{
		nodes[i] = i;
		degrees[i] = graph[i].size();
		targets.insert(targets.end(), graph[i].begin(), graph[i].end());
	}
	// Now read the spanning tree.
	vector<vector<int>> tree(N);
	int root = INT_MIN;
	for (int src, dst; !instream.eof();)
	{
		getline(instream, line);
		if (line.length() <= 1)
			continue;
		istringstream ss(line);
		ss >> src;
		if (root == INT_MIN)
			root = src - 1;
		while (ss >> dst)
		{
			tree[src - 1].push_back(dst - 1);
		}
	}
	// End of IO. Close the input file.
	MPI_File_close(&infile);

	// process 0 creates the graph topology.
	if (world_rank == root)
	{
		MPI_Dist_graph_create(MPI_COMM_WORLD, N, nodes, degrees, targets.data(), MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);
		fprintf(stderr, "Graph topology created by Process %d!\n", world_rank);
	}
	// Other Processes just don't pass anything.
	else
		MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL, NULL, MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);

	// Actual Application starts now.
	MPI_File logfile;
	MPI_File_open(MPI_COMM_WORLD, "out-log.txt", MPI_MODE_WRONLY, MPI_INFO_NULL, &logfile);

	// A Non-Blocing receive instead of using another thread.
	char recv;
	MPI_Request request_handler;
	MPI_Irecv(&recv, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_handler);
	// Introduce a barrier to prevent send raising error.
	MPI_Barrier(MPI_COMM_WORLD);

	if (world_rank == root)
	{
		vector<int> indices(N);
		iota(indices.begin(), indices.end(), 0);
		shuffle(indices.begin(), indices.end(), gen);
		// sleep with delay described by W_r.
		sleep(exponential_distribution<>(1 / W_r)(gen));
		char r = 'R', b = 'B';
		for (int i = 0; i < I_r; i++)
		{
			MPI_Send(&r, 1, MPI_CHAR, indices[i], 0, MPI_COMM_WORLD);
		}
		algo_token = 'B';
		// sleep with delay described by W_r.
		sleep(exponential_distribution<>(1 / W_b)(gen));
		flag = 0;
		MPI_Test(&request_handler, &flag, &stat);
		if (flag == true && stat.MPI_TAG == 0)
		{
			// we have received a red mesage in the meantime.
			string msg("Process ");
			assert(recv == 'R' || recv == 'B');
			logwrite(logfile, msg + to_string(world_rank) + " has received a " + (recv == 'R' ? "red" : "blue") + " message from " + to_string(stat.MPI_SOURCE) + '\n');
			app_color = recv;
		}
		shuffle(indices.begin(), indices.end(), gen);
		for (int i = 0; i < I_b; i++)
		{
			MPI_Send(&b, 1, MPI_CHAR, indices[i], 0, MPI_COMM_WORLD);
		}
	}
	// root has now done its part. It now joins the other processes.
	


	fprintf(stderr, "Process %d Done!\n", world_rank);
	MPI_File_close(&logfile);
	MPI_Finalize();
}
