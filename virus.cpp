#include <vector>
#include <iterator>
#include <numeric>
#include <sstream>
#include <random>
#include <algorithm>
#include <cassert>
#include <climits>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <mpi.h>
using namespace std;
#define BUFSIZE 200
FILE* logfile;
int world_rank, flag, parent = INT_MAX, root = INT_MIN, N;
double p, q;
std::random_device __rd;
std::mt19937 gen(__rd());
std::exponential_distribution<> exp_send, exp_red, exp_blue;
// A spanning tree algo intial color is white.
char algo_token = 'W';
// App initial color is white for all.
char app_color = 'W';
// receive buffer for app messages
char recv;
const char r = 'R', b = 'B', w = 'W', t = 'T';
bool *terminated, token_sent = false;
vector<vector<int>> graph, tree;
MPI_Status stat;
MPI_Request request_handler;
void logwrite(const string &__s)
{
	fprintf(logfile, __s.c_str());
}
void printgraph(vector<vector<int>> const &g)
{
	fprintf(stderr, "Process %d:\n", world_rank);
	for (auto &i : g)
	{
		for (auto &j : i)
		{
			fprintf(stderr, "%d ", j);
		}
		fprintf(stderr, "\n");
	}
}

void cleanup()
{
	logwrite(string("Process ") + to_string(world_rank) + " Done!\n");
	MPI_Finalize();
	exit(0);
}

void start()
{
	flag = -1;
	MPI_Test(&request_handler, &flag, &stat);
	// return if we sent a token to our parent, and be ready for receiving the next message.
	if(token_sent == true && flag==0){
		sleep(1);
		return;
	}
	if (flag == 1 && stat.MPI_TAG == 0)
	{
		// app message
		assert(recv == 'R' || recv == 'B');
		logwrite(string("Process ") + to_string(world_rank) + " has received a " + (recv == 'R' ? "Red " : "Blue ") + "message from Process " + to_string(stat.MPI_SOURCE) + '\n');
		app_color = recv;
		// If we receive a blue app message, we have terminated.
		if (recv == 'B')
		{
			// check if we have received terminated message from all our children.
			bool __flag = true;
			for (int &i : tree[world_rank])
			{
				if (!terminated[i])
					__flag = false;
			}
			if (__flag && world_rank == root)
			{
				// we are root and all our children have terminated.
				if (algo_token == 'W')
				{
					// termination has occurred.
					for (int &i : tree[root])
					{

						MPI_Send(&t, 1, MPI_CHAR, i, 2, MPI_COMM_WORLD);
						logwrite(string("Process ") + to_string(root) + " (root) has detected termination\n");
						cleanup();
					}
				}
				else if (algo_token == 'B')
				{
					// restart required.
					for (int &i : tree[root])
					{
						usleep(10000 * exp_send(gen));
						MPI_Send(&r, 1, MPI_CHAR, i, 2, MPI_COMM_WORLD);
						logwrite(string("Process ") + to_string(root) + " (root) has restarted algorithm\n");
						algo_token = 'W';
						fill(terminated, terminated + N, false);
						token_sent = false;
					}
				}
			}
			else if (__flag)
			{
				// send token to our parent.
				usleep(10000 * exp_send(gen));
				MPI_Send(&algo_token, 1, MPI_CHAR, parent, 1, MPI_COMM_WORLD);
				logwrite(string("Process ") + to_string(world_rank) + " has sent " + (algo_token == 'W' ? "white" : "black") + " token to process " + to_string(parent) + '\n');
				token_sent = true;
			}
		}
	}
	else if (flag == 1 && stat.MPI_TAG == 1)
	{
		// algo message
		assert(recv == 'W' || recv == 'B');
		logwrite(string("Process ") + to_string(world_rank) + " has received a " + (recv == 'W' ? "White " : "Black ") + "token from Process " + to_string(stat.MPI_SOURCE) + '\n');
		if (algo_token == 'W' && recv == 'B')
		{
			algo_token = 'B';
		}
		terminated[stat.MPI_SOURCE] = true;
	}
	else if (flag == 1 && stat.MPI_TAG == 2)
	{
		// terminated / restart message
		assert(recv == 'T' || recv == 'R');
		if (recv == 'T')
		{
			for (int &i : tree[world_rank])
			{
				usleep(10000 * exp_send(gen));
				MPI_Send(&t, 1, MPI_CHAR, i, 2, MPI_COMM_WORLD);
				logwrite(string("Process ") + to_string(world_rank) + " has sent terminate msg to Process " + to_string(i));
			}
			cleanup();
		}
		else
		{
			for (int &i : tree[world_rank])
			{
				usleep(10000 * exp_send(gen));
				MPI_Send(&r, 1, MPI_CHAR, i, 2, MPI_COMM_WORLD);
				logwrite(string("Process ") + to_string(world_rank) + " has sent restart msg to Process " + to_string(i));
			}
			algo_token = 'W';
			fill(terminated, terminated + N, false);
			token_sent = false;
		}
	}
	if (!token_sent && app_color != 'W')
	{
		// send red/blue messages to our neighbors
		usleep(10000 * (app_color == 'R' ? exp_red(gen) : exp_blue(gen)));
		// randomly send red messages to p% of our neighbors.
		shuffle(graph[world_rank].begin(), graph[world_rank].end(), gen);
		for (int i = 0; i <= (app_color == 'R' ? p : q) * graph[world_rank].size(); i++)
		{
			if (i == world_rank)
				continue;
			usleep(10000 * exp_send(gen));
			MPI_Send(&app_color, 1, MPI_CHAR, graph[world_rank][i], 0, MPI_COMM_WORLD);
			logwrite(string("Process ") + to_string(world_rank) + " sent " + (app_color == 'R' ? "red" : "blue") + " message to Process " + to_string(i) + '\n');
		}
	}
	if(flag==1) MPI_Irecv(&recv, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_handler);
	
}
int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm intracomm;
	char *buf = new char[BUFSIZE]{0};
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	fprintf(stderr, "Process %d PID: %d\n", world_rank, getpid());
	// Now some IO
	MPI_File infile;
	// Open the input file in read-only mode.
	MPI_File_open(MPI_COMM_WORLD, "inp-params.txt", MPI_MODE_RDONLY, MPI_INFO_NULL, &infile);
	// Read info from the file. first into a char buffer and then into a stringstream.
	MPI_File_read(infile, buf, BUFSIZE, MPI_CHAR, MPI_STATUS_IGNORE);
	int I_r, I_b;
	double W_r, W_b, L_b, L_r, L_snd;
	stringstream instream(buf);
	instream >> N >> W_r >> I_r >> W_b >> I_b >> L_b >> L_r >> L_snd >> p >> q;
	fprintf(stderr, "Process %d: %d %lf %d %lf %d %lf %lf %lf %lf %lf\n", world_rank, N, W_r, I_r, W_b, I_b, L_b, L_r, L_snd, p, q);
	exp_send = exponential_distribution<>(L_snd);
	exp_blue = exponential_distribution<>(L_b);
	exp_red = exponential_distribution<>(L_r);
	string line;
	graph.resize(N);
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
	terminated = new bool[N]{false};

	// Now read the spanning tree.
	tree.resize(N);
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
			if (dst - 1 == world_rank)
				parent = src - 1;
			tree[src - 1].push_back(dst - 1);
		}
	}
	// End of IO. Close the input file.
	// process root creates the graph topology.
	if (world_rank == root)
	{
		int *nodes = new int[N], *degrees = new int[N];
		vector<int> targets;
		for (int i = 0; i < N; i++)
		{
			nodes[i] = i;
			degrees[i] = graph[i].size();
			targets.insert(targets.end(), graph[i].begin(), graph[i].end());
		}
		MPI_Dist_graph_create(MPI_COMM_WORLD, N, nodes, degrees, targets.data(), MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);
		fprintf(stderr, "Graph topology created by Process %d!\n", world_rank);
	}
	// Other Processes just don't pass anything.
	else
		MPI_Dist_graph_create(MPI_COMM_WORLD, 0, NULL, NULL, NULL, MPI_UNWEIGHTED, MPI_INFO_NULL, 0, &intracomm);

	// Actual Application starts now.
	logfile = fopen("out-log.txt", "a");

	// A Non-Blocing receive instead of using another thread.
	MPI_Irecv(&recv, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_handler);
	// Introduce a barrier to prevent send raising error.
	MPI_Barrier(MPI_COMM_WORLD);
	if (world_rank == root)
	{
		vector<int> indices(N);
		iota(indices.begin(), indices.end(), 0);
		shuffle(indices.begin(), indices.end(), gen);
		// sleep with delay described by W_r.
		usleep(10000 * exponential_distribution<>(1 / W_r)(gen));
		for (int i = 0; i < I_r; i++)
		{
			fprintf(stderr, "Process %d: line 260: Dest: %d\n", world_rank, indices[i]);
			MPI_Send(&r, 1, MPI_CHAR, indices[i], 0, MPI_COMM_WORLD);
		}

		// sleep with delay described by W_r.
		usleep(10000 * exponential_distribution<>(1 / W_b)(gen));
		shuffle(indices.begin(), indices.end(), gen);
		for (int i = 0; i < I_b; i++)
		{
			fprintf(stderr, "Process %d: line 269: Dest: %d\n", world_rank, indices[i]);
			MPI_Send(&b, 1, MPI_CHAR, indices[i], 0, MPI_COMM_WORLD);
		}
	}
	// root has now done its part. It now joins the other processes.
	// every process waits for root to complete sending messages.
	MPI_Barrier(MPI_COMM_WORLD);
	while (true)
	{
		start();
	}
}
