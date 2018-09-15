#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
using namespace std;

class node{
public:
    int maxbackoff;
    int backoff;
    int maxpacket;
    int packetleft;
    int numOfCollision;
    node(int s,int L){maxbackoff = s; backoff = 0; maxpacket = L; packetleft = L; numOfCollision = 0;}
};

vector<int> hasSomeoneToSend(int start,vector<node*>&nodevec){
    vector<int>ans;
    for(int i=start+1;i<nodevec.size();++i){
        if(nodevec[i]->backoff==0)ans.push_back(i);
    }
    return ans;
}

void collisionOfNode(int i, int M, const vector<int>&Rvec, vector<node*>&nodevec,vector<int>&collisionNum){
    nodevec[i]->numOfCollision++;
    collisionNum[i]++;
    if(nodevec[i]->numOfCollision==M){ //drop the current packet if collision number = M
        nodevec[i]->maxbackoff = Rvec[0];
        nodevec[i]->backoff = rand()%(1+nodevec[i]->maxbackoff);
        nodevec[i]->numOfCollision = 0;
    }
    else{
        nodevec[i]->maxbackoff *= 2;
        if(nodevec[i]->maxbackoff > Rvec.back())
            nodevec[i]->maxbackoff = Rvec.back();
        nodevec[i]->backoff = rand()%(1+nodevec[i]->maxbackoff);
    }
}
/*
double variance(vector<int>resultSet){
    double sum = std::accumulate(std::begin(resultSet), std::end(resultSet), 0.0);
    double mean =  sum / resultSet.size(); //mean

    double accum  = 0.0;
    std::for_each (std::begin(resultSet), std::end(resultSet), [&](const double d) {
        accum  += (d-mean)*(d-mean);
    });

    double stdev = accum/(resultSet.size()-1); //variance
    return stdev;
}
*/

double variance(vector<int>resultSet){
    double sum = 0;
    for(int i=0;i<resultSet.size();++i)sum+=resultSet[i];
    double mean =  sum / resultSet.size(); //mean

    double accum  = 0.0;
    for(int i=0;i<resultSet.size();++i) {
        accum  += (resultSet[i]-mean)*(resultSet[i]-mean);
    }

    double stdev = accum/(resultSet.size()); //variance
    return stdev;
}


//////////////////////////////////////////////////////////////////////////
int main(int argc, char**argv)
{
    if(argc != 2)
	{
		fprintf(stderr, "usage: %s input.txt\n", argv[0]);
		exit(1);
	}

    ifstream fin(argv[1]);
    ofstream fout("output.txt");

    srand(time(NULL));
    int N,L,R,M,T;    //reading in all the needed variables
    vector<int>Rvec;
    char ch;
    string str;
    bool channelOccupied = false;
    int whoOccupied = 0;
    int packetsent = 0;
    int globalTime = 0;

    fin>>ch>>N>>ch>>L>>ch; //N,L

    while(fin.get()!='\r'){
        fin>>R;
        Rvec.push_back(R);
    }
    fin>>ch>>M>>ch>>T;

    vector<int>successPacketNum(N,0);
    vector<int>collisionNum(N,0);
    int totalCollision = 0;
    int idletime = 0;

    //organizing all the needed nodes for transmission

    vector<node*>nodevec(N);
    for(int i=0;i<N;++i){nodevec[i]=new node(Rvec[0], L);} //initialization of all nodes

    for(globalTime=0; globalTime<T; ++globalTime){
        //cout<<channelOccupied<<"   ";
        //for(int i=0;i<N;++i)cout<<nodevec[i]->backoff<<' ';
        //cout<<endl;

        vector<int>ans = hasSomeoneToSend(-1,nodevec);
        int nums = ans.size();

        if(nums == 0)idletime++;

        //cout<<nums<<endl;
        if(nums>1&&channelOccupied==false){   //more than one want to occupy the empty channel
            totalCollision++;
            for(int i=0;i<nums;++i){
                collisionOfNode(ans[i],M,Rvec,nodevec,collisionNum);
            }
            continue;
        }
        for(int i=0;i<N;++i){  //every time check the 25 nodes
            if(nodevec[i]->backoff==0){
                //cout<<"here"<<endl;
                if(channelOccupied){   //if occupied happens
                    if(whoOccupied == i){   //occupied by itself
                        if(nodevec[i]->packetleft == 1){ //in this slot it is all sent and re-initialize
                            packetsent++;
                            successPacketNum[i]++;
                            channelOccupied = false;
                            nodevec[i]->packetleft = L;
                            nodevec[i]->maxbackoff = Rvec[0];
                            nodevec[i]->backoff = rand()%(1+nodevec[i]->maxbackoff);
                            nodevec[i]->numOfCollision = 0;
                            break;
                        }
                        else{
                            nodevec[i]->packetleft--;
                            continue;
                        }
                    }
                    else{                     //occupied by others
                        collisionOfNode(i,M,Rvec,nodevec,collisionNum);
                        continue;
                    }
                }
                else{  //the channel is not yet occupied and only you want to transmit
                    channelOccupied = true;
                    whoOccupied = i;
                    nodevec[i]->packetleft--;
                }
            }
            else{  //the current node's backoff > 0
                if(channelOccupied){
                    //cout<<"occupied channel"<<endl;
                    continue;
                }
                else if(hasSomeoneToSend(i,nodevec).size()==0){
                    nodevec[i]->backoff--;
                    continue;
                }
                else{
                    continue;
                }
            }
        }
        //cout<<channelOccupied<<"   ";
        //for(int i=0;i<N;++i)cout<<nodevec[i]->backoff<<' ';
        //cout<<endl;
    }
    fout<<"channel utilization "<<packetsent*L*1.0/T<<endl;
    fout<<"channel idle fraction "<<idletime*1.0/T<<endl;
    fout<<"total collision "<<totalCollision<<endl;
    fout<<"variance of success transmission "<<variance(successPacketNum)<<endl;
    fout<<"variance of collision "<<variance(collisionNum)<<endl;
    return 0;
}
