#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <queue>

using namespace std;

int total_num_nodes;
int total_num_node_arrays;
int node_count;

vector <vector<int> > edgeCost;


//map<int, bool> cur_nodes;
//basic element of our topological map
struct DVmessage
{
	int source;
	int destination;
	int via;
	int cost;
	vector<int> distance_vector;
};

struct Node
{
	int id;
	unordered_map <int,int> neighbors;
	vector <vector <int> > costTable;
	vector <int> path;
	vector <int> smallestCost;
	bool flag;
	vector <DVmessage> msgs;
};

//forwarding table format is destination, nexthop, pathcost ---> to be printed out
struct forward_entry{
	int destination;
	int nexthop;
	int pathcost;
};


//message to be parsed in and used
struct message_entry{
	int source;
	int destination;
	string message;
};



ofstream outfile;
string topo_filename, changes_filename, msg_filename;
map<int, Node*> topo_map; //changed from unordered to make it easier to get correct output
queue<message_entry*> allmessages;
map <int, Node* > cur_nodes;
vector< vector<forward_entry> >  forward_table;


vector <Node> all_nodes_on_graph;
vector <vector <DVmessage> >  NodeMessages; //2d vector array holds messages from nodes in DV alrogrithm

int check_valid_cost(int src, int dest, int via)
{
	int cost;
	cost = all_nodes_on_graph[src].costTable[dest][via];
	if(cost > 0 && cost <100)
	{
		return 1;
	}
	else return 0;
}

void Print_Forward_Table(int cur_node){
	Node cur=all_nodes_on_graph[cur_node];
	forward_table[cur_node].resize(total_num_node_arrays);
	for(int i = 1; i < total_num_node_arrays; i++){
		outfile << i;
		forward_table[cur_node][i].destination=i;
		outfile << "\t";
		if(cur_node==i){
			outfile<<i;
			forward_table[cur_node][i].nexthop=i;
		}
		else{
			outfile << cur.path[i];
			forward_table[cur_node][i].nexthop=cur.path[i];
		}
		outfile << "\t";
		outfile << cur.costTable[cur_node][i];
		forward_table[cur_node][i].pathcost=cur.costTable[cur_node][i];
		outfile << "\n";
		cout << forward_table[cur_node][i].destination;
		cout << "\t";
		cout << forward_table[cur_node][i].nexthop;
		cout << "\t";
		cout << forward_table[cur_node][i].pathcost;
		cout << "\n";
	}
}



void send_messages(){
	// int i;

	// for(i = 0; i < allmessages.size(); i++){
	while(allmessages.size() > 0){
		int source, dest;
		source = allmessages.front()->source;
		dest = allmessages.front()->destination;
		//initial output
		//outfile << "from " << source << " to " << dest << " cost "<<all_nodes_on_graph[source].costTable[source][dest]<<" hops ";
		outfile << "from " << source << " to " << dest << " cost ";
		//calculate next hops

		//need to look at hash map of forward tables, recursively choose nexthop
		queue<int> pathhops; //store current path of hops
		pathhops.push(source);
		//int temp = source;
		int vectorindex = 0;
		vector<forward_entry> cur_table = forward_table[source];
		bool isreachable = false;
		while(cur_table[vectorindex].destination != dest && vectorindex < cur_table.size()){
			vectorindex++;
		}
		if(cur_table[vectorindex].nexthop != -1){
				isreachable = true;
		}
		if(isreachable){
			outfile << all_nodes_on_graph[source].costTable[source][dest]<<" hops ";
			int nexthop = cur_table[vectorindex].nexthop;
			if(nexthop != dest){
				pathhops.push(nexthop);
			}
			while(nexthop != dest){
				cur_table = forward_table[nexthop];
				vectorindex = 0;
				while(cur_table[vectorindex].destination != dest && vectorindex < cur_table.size()){
				vectorindex++;
				}
				nexthop = cur_table[vectorindex].nexthop;
				if(nexthop != dest){
					pathhops.push(nexthop);
				}
			}
			//now have hop path so need to print it out
			while(pathhops.size() > 0){
				outfile << pathhops.front() << " ";
				pathhops.pop();
			}
			outfile << "message" << allmessages.front()->message << "\n";
			allmessages.pop();
		}
		//case where message cant be sent to destination
		else{
			outfile << "infinite hops ";
			outfile << "unreachable message" << allmessages.front()->message << "\n";
			allmessages.pop();
		}
	}
	outfile << "\n";

}
void get_messages(string fname){
	ifstream infile(fname);
	string line, message;

	if (infile.is_open()){
		while(getline(infile,line)){
			int source;
			int dest;
			sscanf(line.c_str(), "%d %d %*s", &source, &dest);
			message = line.substr(line.find(" ")); //want message to start after second space so do twice
			message = message.substr(line.find(" ") + 1); //copy from position 4 to end of line to only get the message
			message_entry *msg = new message_entry;
			msg->source = source;
			msg->destination = dest;
			msg->message = message;
			allmessages.push(msg);
		}
		infile.close();
	}
}
int get_topology(string fname){
	ifstream topfile(fname);
	int total_nodes=0;
	int a, b, c;
	cout << "---Getting topology of the network---" <<endl;

	while(topfile >> a >> b >> c){
		//if a not in keys of map
		if(topo_map.find(a) == topo_map.end()){
			Node *source_node = new Node;
			//topo_map.insert(make_pair(a, source_node));
			topo_map[a] = source_node;
			topo_map[a]->id = a; //node ide, probably redundant but might need later
			total_nodes++;
		}
		//if b not in keys of map
		if(topo_map.find(b) == topo_map.end()){
			Node *dest_node = new Node;
			//topo_map.insert(make_pair(b, dest_node));
			topo_map[b]= dest_node;
			topo_map[b]->id = b;
			total_nodes++;
		}
		//node have been mapped for current source/dest, add in neighbor/costs
		if(topo_map[a]->neighbors.find(b) == topo_map[a]->neighbors.end())
			topo_map[a]->neighbors[b] = c; //cost of the link
		//have to do the same for the other node
		if(topo_map[b]->neighbors.find(a) == topo_map[b]->neighbors.end())
			topo_map[b]->neighbors[a] = c;
	}
	//cout<<topo_map[1]->neighbors.size()<<endl;
	topfile.close();

	cout << "The total number of nodes is "<<total_nodes<<endl;
	cout << "---Processing ends---" << endl;

	//initialize the edgecost matrixes
	edgeCost.resize(total_nodes+1);
	for(int i = 1; i <total_nodes+1; i++)
	{
		edgeCost[i].resize(total_nodes+1);
		for(int j = 1; j< total_nodes+1; j++)
		{
			if(i == j) //cost to node itself is zero
			{
				edgeCost[i][j] = 0;
			}
			else //initialize to infinity
			{
				edgeCost[i][j] = 9999; // we use 9999 as infinity number
			}
			//cout << edgeCost[i][j] << endl;
		}
		//cout << endl;
	}

	ifstream topfile2(fname);
		while(topfile2 >> a >> b >> c)
		{
			//cout <<"hello" <<endl;
			if(c > 0  && c < 100)
			{
				edgeCost[a][b] = c;
				edgeCost[b][a] = c;
				//cout<<edgeCost[a][b]<<endl;
			}

		}
		/*
		for(int m = 1; m< total_nodes+1 ; m++)
		{
			for(int n = 1; n < total_nodes+1 ; n++)
			{
				cout << edgeCost[m][n]<<'\t';
			}
			cout<<endl;
		}*/
	topfile2.close();

	return total_nodes;
}


/*
		INITIALIZATION FUNCTION NECESSARY??? and clear function?? for when we get message updates

*/
void initialize_nodes()
{
	cout<<"---Initialization starts---"<<endl;
	all_nodes_on_graph.resize(total_num_node_arrays);
	for(int i = 1 ; i < total_num_node_arrays; i++)
	{
		all_nodes_on_graph[i].id = i;
		all_nodes_on_graph[i].flag = false;
		all_nodes_on_graph[i].smallestCost.resize(total_num_node_arrays);
		all_nodes_on_graph[i].path.resize(total_num_node_arrays);
		//all_nodes_on_graph[i].neighbors.resize(total_num_node_arrays);
		all_nodes_on_graph[i].costTable.resize(total_num_node_arrays);
		for(int j = 1; j<total_num_node_arrays ; j++)
		{
			all_nodes_on_graph[i].costTable[j].resize(total_num_node_arrays);
		}
	}

	for(int i = 1 ; i < total_num_node_arrays; i++)//for each node in network
	{
		for(int j = 1; j < total_num_node_arrays; j++)
		{
			all_nodes_on_graph[i].smallestCost[j] = edgeCost[i][j];
			if(edgeCost[i][j]<100 && edgeCost[i][j]>0)
				all_nodes_on_graph[i].path[j]=j;
			else
				if(i==j)
					all_nodes_on_graph[i].path[j]=0;
				else
					all_nodes_on_graph[i].path[j]=-1;//not reachable

			//all_nodes_on_graph[i].costTable[i][j] = edgeCost[i][j];
			//cout<<all_nodes_on_graph[i].path[j]<<'\t';
		}
		//cout<<endl;
		for(int m=1;m<total_num_node_arrays;m++){
			for(int n=1;n<total_num_node_arrays;n++){
				if(i==m)
					all_nodes_on_graph[i].costTable[m][n] = edgeCost[m][n];
				else
					all_nodes_on_graph[i].costTable[m][n] = 9999;
				if(m==n)
					all_nodes_on_graph[i].costTable[m][n] = 0;
				//cout<<all_nodes_on_graph[i].costTable[m][n]<<'\t';
			}
			//cout<<endl;
		}
		//cout<<endl;

		//sending DV messages to its neighbors
		for(unordered_map<int, int>::iterator it=topo_map[i]->neighbors.begin();it!=topo_map[i]->neighbors.end();it++){
			int dest=it->first;
			//int cost=all_nodes_on_graph[i].costTable[i];
			DVmessage message;
			message.source=i;
			message.destination=dest;
			//message.cost=cost;
			message.via=i;
			message.distance_vector=all_nodes_on_graph[i].costTable[i];
			all_nodes_on_graph[dest].msgs.push_back(message);
			//cout<<'1';
		}

	}

	/*
	for(int i=1;i<total_num_node_arrays;i++){
		for(vector<DVmessage>::iterator it=all_nodes_on_graph[i].msgs.begin();it!=all_nodes_on_graph[i].msgs.end();it++){
			cout<<it->source<<'\t'<<it->destination<<'\t'<<it->via<<endl;
		}
		cout<<endl;
	}*/

	//NodeMessages.resize(total_num_node_arrays);
	cout<<"---Initialization ends---"<<endl;

}

void update_costTable(int cur_node){
	int flag=0;
	for(vector<DVmessage>::iterator it=all_nodes_on_graph[cur_node].msgs.begin();it!=all_nodes_on_graph[cur_node].msgs.end();it++){
		int via=it->via;
		all_nodes_on_graph[cur_node].costTable[via]=it->distance_vector;
		//for(int i=1;i<total_num_node_arrays;i++){
			for(int j=1;j<total_num_node_arrays;j++){
				if(cur_node!=j&&via!=j){
					int ori_cost=all_nodes_on_graph[cur_node].costTable[cur_node][j];
					//int new_cost=all_nodes_on_graph[cur_node].costTable[cur_node][via]+it->distance_vector[j];
					int new_cost=edgeCost[cur_node][via]+it->distance_vector[j];
					//cout<<ori_cost<<'\t'<<new_cost<<endl;
					if(new_cost<ori_cost){
						all_nodes_on_graph[cur_node].costTable[cur_node][j]=new_cost;
						all_nodes_on_graph[cur_node].path[j]=via;
						if(all_nodes_on_graph[cur_node].path[via]!=via)
							all_nodes_on_graph[cur_node].path[j]=all_nodes_on_graph[cur_node].path[via];
						all_nodes_on_graph[cur_node].smallestCost[j]=new_cost;
						flag=1;
						//cout<<"node "<<cur_node<<":"<<it->via<<endl;
					}
					if(new_cost==ori_cost){
						if(via<all_nodes_on_graph[cur_node].path[j])
							all_nodes_on_graph[cur_node].path[j]=via;
							int a=0;
					}
				all_nodes_on_graph[cur_node].costTable[j][cur_node]=all_nodes_on_graph[cur_node].costTable[cur_node][j];
				}
			}
		//}
	}

	all_nodes_on_graph[cur_node].msgs.clear();

	for(int m=1;m<total_num_node_arrays;m++)
		for(int n=1;n<total_num_node_arrays;n++)
			if(all_nodes_on_graph[cur_node].costTable[m][n]>all_nodes_on_graph[cur_node].costTable[n][m])
				all_nodes_on_graph[cur_node].costTable[m][n]=all_nodes_on_graph[cur_node].costTable[n][m];

	if(flag==1){
		//flag=0;
		for(unordered_map<int, int>::iterator it2=topo_map[cur_node]->neighbors.begin();it2!=topo_map[cur_node]->neighbors.end();it2++){
			int dest=it2->first;
			//int cost=all_nodes_on_graph[i].costTable[i];
			DVmessage message;
			message.source=cur_node;
			message.destination=dest;
			//message.cost=cost;
			message.via=cur_node;
			message.distance_vector=all_nodes_on_graph[cur_node].costTable[cur_node];
			all_nodes_on_graph[dest].msgs.push_back(message);
			//cout<<'1';
		}
	}

	/*
	if(cur_node==1){
	for(int m=1;m<total_num_node_arrays;m++){
		//cout<<
		for(int n=1;n<total_num_node_arrays;n++){
			cout<<all_nodes_on_graph[cur_node].costTable[m][n]<<'\t';
		}
		cout<<endl;
	}
	cout<<endl;
	}*/
}

void do_changes(string changefname, string msgfname){
	cout<<"---Changing the topology---"<<endl;
	ifstream changefile(changefname);
	if(!changefile.is_open())
		cout<<"File opened failed!"<<endl;
	int source, dest, cost;
	int change_time=0;
	while(changefile >> source >> dest >> cost){
		change_time++;
		cout<<"---Starts changing "<<change_time<<" time(s)---"<<endl;
		//check for existing link b/w the nodes in file
		if(topo_map[source]->neighbors.find(dest) != topo_map[source]->neighbors.end()){
			//link costs can only be positive, never 0. -999 means remove link
			if(cost > 0){
				//update path cost
				topo_map[source]->neighbors[dest] = cost;
				//have to do for destinations neighbor too
				topo_map[dest]->neighbors[source] = cost;
				edgeCost[source][dest]=cost;
				edgeCost[dest][source]=cost;
				//all_nodes_on_graph[source].path[dest]=dest;
				//all_nodes_on_graph[dest].path[source]=source;
			}
			//remove link if cost is -999
			else if(cost == -999){
				topo_map[source]->neighbors.erase(dest); //no longer neighbors
				topo_map[dest]->neighbors.erase(source);
				edgeCost[source][dest]=9999;
				edgeCost[dest][source]=9999;
				//all_nodes_on_graph[source].path[dest]=-1;
				//all_nodes_on_graph[dest].path[source]=-1;
			}
		}
		//no existing link exists, create one
		else{
			//if no link exists and cost value is 0 or -999 then ignore it
			if(cost > 0){
				topo_map[source]->neighbors[dest] = cost;
				topo_map[dest]->neighbors[source] = cost;
				edgeCost[source][dest]=cost;
				edgeCost[dest][source]=cost;
				//all_nodes_on_graph[source].path[dest]=dest;
				//all_nodes_on_graph[dest].path[source]=source;
				cout<<"Adding link from "<<source<<" to "<<dest<<endl;
			}
		}
		/*
		//now need to get new routing tables
		for(unordered_map<int, int>::iterator it=topo_map[source]->neighbors.begin();it!=topo_map[source]->neighbors.end();it++){
		int dest2=it->first;
		DVmessage message;
		message.source=source;
		message.destination=dest2;
		//message.cost=cost;
		message.via=source;
		if(cost>0)
			all_nodes_on_graph[source].costTable[source][dest]=cost;
		else if(cost==-999)
			all_nodes_on_graph[source].costTable[source][dest]=9999;
		message.distance_vector=all_nodes_on_graph[source].costTable[source];
		all_nodes_on_graph[dest2].msgs.push_back(message);
		//cout<<'1';
		}

		for(unordered_map<int, int>::iterator it=topo_map[dest]->neighbors.begin();it!=topo_map[dest]->neighbors.end();it++){
		int dest2=it->first;
		DVmessage message;
		message.source=dest;
		message.destination=dest2;
		//message.cost=cost;
		message.via=dest;
		if(cost>0)
			all_nodes_on_graph[dest].costTable[dest][source]=cost;
		else if(cost==-999)
			all_nodes_on_graph[dest].costTable[dest][source]=9999;
		message.distance_vector=all_nodes_on_graph[dest].costTable[dest];
		all_nodes_on_graph[dest2].msgs.push_back(message);
		//cout<<'1';
		}*/

		for(int i = 1 ; i < total_num_node_arrays; i++)//for each node in network
		{
			for(int j = 1; j < total_num_node_arrays; j++)
			{
				all_nodes_on_graph[i].smallestCost[j] = edgeCost[i][j];
				if(edgeCost[i][j]<100 && edgeCost[i][j]>0)
					all_nodes_on_graph[i].path[j]=j;
				else
					if(i==j)
						all_nodes_on_graph[i].path[j]=0;
					else
						all_nodes_on_graph[i].path[j]=-1;//not reachable

				//all_nodes_on_graph[i].costTable[i][j] = edgeCost[i][j];
				//cout<<all_nodes_on_graph[i].path[j]<<'\t';
			}

			for(int m=1;m<total_num_node_arrays;m++){
				for(int n=1;n<total_num_node_arrays;n++){
					if(i==m)
						all_nodes_on_graph[i].costTable[m][n] = edgeCost[m][n];
					else
						all_nodes_on_graph[i].costTable[m][n] = 9999;
					if(m==n)
						all_nodes_on_graph[i].costTable[m][n] = 0;
					//cout<<all_nodes_on_graph[i].costTable[m][n]<<'\t';
				}
				//cout<<endl;
			}

			//sending DV messages to its neighbors
			for(unordered_map<int, int>::iterator it=topo_map[i]->neighbors.begin();it!=topo_map[i]->neighbors.end();it++){
				int dest=it->first;
				//int cost=all_nodes_on_graph[i].costTable[i];
				DVmessage message;
				message.source=i;
				message.destination=dest;
				//message.cost=cost;
				message.via=i;
				message.distance_vector=all_nodes_on_graph[i].costTable[i];
				all_nodes_on_graph[dest].msgs.push_back(message);
				//cout<<'1';
			}

		}

		for(int node=1;node<total_num_node_arrays;node++){
			update_costTable(node);
			//if(iter>=100) break;
			if(node==total_num_node_arrays-1)
				for(int m=1;m<total_num_node_arrays;m++)
					if(!all_nodes_on_graph[m].msgs.empty())
						node=0;
		}

		for(int node=1;node<total_num_node_arrays;node++){
			/*
			cout<<"node "<<node<<" cost table: "<<endl;
			for(int m=1;m<total_num_node_arrays;m++){
				for(int n=1;n<total_num_node_arrays;n++){
					cout<<all_nodes_on_graph[node].costTable[m][n]<<'\t';
				}
				cout<<endl;
			}*/
			cout<<"node "<<node<<" entry:"<<endl;
			Print_Forward_Table(node);
			cout<<endl;
		}

		get_messages(msgfname);
		send_messages();
	}
	changefile.close();
}

int main(int argc, char** argv)
{
	outfile.open("output.txt"); //output file

	if(argc != 4)
	{
		fprintf(stderr, "usage: %s topofile messagefile changesfile\n", argv[0]);
		exit(1);
	}
	topo_filename = argv[1];
	msg_filename = argv[2];
	changes_filename = argv[3];

	total_num_nodes = get_topology(topo_filename); //get initial mapping
	total_num_node_arrays = total_num_nodes + 1;
	//get_topology(topo_filename); //get initial mapping
	//unordered_map<int, vector<forward_entry> > forward_table;
	forward_table.resize(total_num_node_arrays);

	initialize_nodes();
	int iter=0;
	for(int node=1;node<total_num_node_arrays;node++){
		update_costTable(node);
		iter++;
		//if(iter>=100) break;
		if(node==total_num_node_arrays-1)
			for(int m=1;m<total_num_node_arrays;m++)
				if(!all_nodes_on_graph[m].msgs.empty())
					node=0;
	}

	for(int node=1;node<total_num_node_arrays;node++){
		/*
		cout<<"node "<<node<<" cost table: "<<endl;
		for(int m=1;m<total_num_node_arrays;m++){
			for(int n=1;n<total_num_node_arrays;n++){
				cout<<all_nodes_on_graph[node].costTable[m][n]<<'\t';
			}
			cout<<endl;
		}*/
		cout<<"node "<<node<<" entry:"<<endl;
		Print_Forward_Table(node);
		cout<<endl;
	}

	/*
	for(map<int, Node*>::iterator it = topo_map.begin(); it != topo_map.end(); ++it){
		vector<forward_entry> table;
		send_new_min_cost();
		//distance_vector_algorithm(all_nodes_on_graph[it->first]);
		send_new_min_cost();
		Print_Forward_Table(table);
		outfile << "\n";
		forward_table[it->first] = table;
	}*/
	get_messages(msg_filename);
	send_messages();
	do_changes(changes_filename, msg_filename);

	outfile.close();
}


