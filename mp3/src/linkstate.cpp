#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
using namespace std;

ofstream fout("output.txt");

struct three{
    int a,b,c;
    three(){}
    three(int aa, int bb, int cc){a=aa;b=bb;c=cc;}
};

class graph{
    private:
    int vers,edges;  //num of ver and edge
    struct edgenode
    {
        int end;
        int weight;
        edgenode*next;
        edgenode(int e,int w,edgenode*n=NULL){end=e;weight=w;next=n;}
    };
    struct vernode
    {
        int ver;
        edgenode*head;
        vernode(edgenode*h=NULL){head=h;}
    };
    vernode*verlist;

    public:
    graph(int vsize,int d[])
    {
        vers=vsize;
        edges=0;
        verlist=new vernode[vsize];
        for(int i=0;i<vers;++i)
        {
            verlist[i].ver=d[i];
        }
    }
    ~graph()
    {
        edgenode*p;
        for(int i=0;i<vers;++i)
        {
            while(verlist[i].head!=NULL)
            {
                p=verlist[i].head;
                verlist[i].head=p->next;
                delete p;
            }
        }
        delete []verlist;
    }
    bool insert(int u,int v,int w)
    {
        verlist[u].head=new edgenode(v,w,verlist[u].head);
        ++edges;
        return true;
    }
    bool remove(int u,int v)
    {
        edgenode*p=verlist[u].head,*q;  //要remove这条边。就要找它的前一个边，对这两种特殊情况是没有前一条边的
        if(p==NULL)return false;
        if(p->end==v)
        {
            verlist[u].head=p->next;
            delete p;
            --edges;
            return true;
        }
        if(p->next!=NULL&&p->next->end!=v)p=p->next;
        if(p->next==NULL)return false;
        if(p->next->end==v)
        {
            q=p->next;
            p->next=q->next;
            delete q;
            --edges;
            return true;
        }

    }
    bool exist(int u,int v)
    {
        edgenode*p=verlist[u].head;
        while(p!=NULL&&p->end!=v)p=p->next;
        if(p==NULL)return false;
        else return true;

    }

    void printpath(int start, int end, int prev[], vector<int>&saveres){

        if(end==start){
            saveres.push_back(verlist[start].ver);
            return;
        }
        printpath(start, prev[end], prev, saveres);
        saveres.push_back(verlist[end].ver);
    }

    static bool compare2(const three &aa, const three &bb){
        return aa.a<bb.a;
    }

    void dijkstra(int start)
    {
        bool*known=new bool[vers];
        int*distance=new int[vers];
        int *prev=new int[vers];

        edgenode*p;
        int no;
        int i,j;
        for(i=0;i<vers;++i)
        {
            known[i]=false;
            distance[i]=10000;
        }
        int sno;
        for(sno=0;sno<vers;++sno)
        {
            if(verlist[sno].ver==start)break;
        }
        distance[sno]=0;
        prev[sno]=sno;

        for(i=1;i<vers;++i)        //update for n-1 times is enough
        {
            int min=10000;
            for(j=0;j<vers;++j){
                if(!known[j]&&distance[j]<min){
                    min=distance[j];
                    no=j;
                }
            }
            known[no]=true;
            for(p=verlist[no].head;p!=NULL;p=p->next){
                if(!known[p->end]&&distance[p->end]>=min+p->weight){
                    if(distance[p->end]>min+p->weight)prev[p->end]=no;
                    if(distance[p->end]!=10000&&distance[p->end]==min+p->weight&&prev[p->end]>no)prev[p->end]=no;
                    distance[p->end]=min+p->weight;
                }
            }
        }
        vector<int>saveres;
        vector<three>ans;
        three tmpans;
        for(i=0;i<vers;++i){
            tmpans.a = verlist[i].ver;
            saveres.clear();
            if(distance[i]!=10000)printpath(sno, i, prev, saveres);
            else continue;                   //////////////////////////////important!!!
            if(saveres.size()==0)continue;
            else if(saveres.size()==1)tmpans.b = saveres[0];
            else tmpans.b = saveres[1];
            if(distance[i]!=10000)tmpans.c = distance[i];
            else tmpans.c = -999;
            ans.push_back(tmpans);
        }
        sort(ans.begin(),ans.end(),compare2);   //according to start num (inner loop)
        for(int i=0;i<ans.size();++i){
            if(ans[i].c != -999)fout<<ans[i].a<<' '<<ans[i].b<<' '<<ans[i].c<<endl;
        }
    }

    void dijkstra_all(int start, int to)// both are sequence
    {
        bool*known=new bool[vers];
        int*distance=new int[vers];
        int *prev=new int[vers];

        edgenode*p;
        int no;
        int i,j;
        for(i=0;i<vers;++i)
        {
            known[i]=false;
            distance[i]=10000;
        }
        int sno=start;

        distance[sno]=0;
        prev[sno]=sno;

        for(i=1;i<vers;++i)        //update for n-1 times is enough
        {
            int min=10000;
            for(j=0;j<vers;++j)
            {
                if(!known[j]&&distance[j]<min)
                {
                    min=distance[j];
                    no=j;
                }
            }
            known[no]=true;
            for(p=verlist[no].head;p!=NULL;p=p->next)
            {
                if(!known[p->end]&&distance[p->end]>=min+p->weight)
                {
                    if(distance[p->end]>min+p->weight)prev[p->end]=no;
                    if(distance[p->end]!=10000&&distance[p->end]==min+p->weight&&prev[p->end]>no)prev[p->end]=no;
                    distance[p->end]=min+p->weight;
                }
            }
        }
        if(distance[to]!=10000)
            fout<<distance[to]<<" hops ";
        else{
            fout<<"infinite hops unreachable ";
            return;
        }
        vector<int>saveres;
        printpath(sno, to, prev, saveres);
        for(int i=0;i<saveres.size()-1;++i)fout<<saveres[i]<<' ';
    }
};

static bool compare(const pair<int,int>&a, const pair<int,int>&b){
    return a.first<b.first;
}

struct message{
    int from;
    int to;
    string sentence;
};
////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{

    ifstream fin(argv[1]);
    ifstream fin2(argv[3]);
    ifstream fin3(argv[2]);

    int aa,bb,cc;
    vector<three>vec;
    unordered_map<int,int>mp;
    int seq=0;
    while(fin>>aa>>bb>>cc){
        //if(fin.peek()==EOF)break;
        three tmp(aa,bb,cc);
        vec.push_back(tmp);
        if(mp.find(aa)==mp.end()){mp[aa]=seq;seq++;}
        if(mp.find(bb)==mp.end()){mp[bb]=seq;seq++;}

        //fin.get();
    }
    int vernum=seq;
    int*array=new int[vernum];
    for(int i=0;i<vernum;++i){
        int tmp=0;
        for(auto iter=mp.begin();iter!=mp.end();++iter){
            if(iter->second==i){tmp=iter->first;break;}
        }
        array[i]=tmp;
    }
    graph gr(vernum,array);
    for(int i=0;i<vec.size();++i){
        if(vec[i].c == -999)continue;
        if(!gr.exist(mp[vec[i].a],mp[vec[i].b]))gr.insert(mp[vec[i].a],mp[vec[i].b],vec[i].c);
        if(!gr.exist(mp[vec[i].b],mp[vec[i].a]))gr.insert(mp[vec[i].b],mp[vec[i].a],vec[i].c);
    }

    vector<pair<int,int>>pp;
    pair<int,int> tmppair;
    for(auto iter=mp.begin();iter!=mp.end();++iter){
        tmppair.first=iter->first;
        tmppair.second=iter->second;
        pp.push_back(tmppair);
    }
    sort(pp.begin(),pp.end(),compare);
    for(int i=0;i<vernum;++i){
        gr.dijkstra(pp[i].first);
    }

////////////////////////////////////////////////////////////////
    vector<message>message_vec;
    message mess;
    int ff,tt;
    char ww;
    string s;
    while(fin3>>ff>>tt){
        //if(fin3.peek() == EOF||fin3.peek() == '\n') break;

        fin3.get(ww);
        getline(fin3,s);
        mess.from=ff;
        mess.to=tt;
        mess.sentence=s;
        message_vec.push_back(mess);

        //fin3.get();
    }
    for(int i=0;i<message_vec.size();++i){
        fout<<"from "<<message_vec[i].from<<" to "<<message_vec[i].to<<" cost ";
        gr.dijkstra_all(mp[message_vec[i].from],mp[message_vec[i].to]);
        fout<<"message "<<message_vec[i].sentence<<endl;
    }
///////////////////////////////////////////////////////////////////
    int c1,c2,wei;
    while(fin2>>c1>>c2>>wei){
        //if(fin2.peek() == EOF||fin2.peek() == '\n') break;

        gr.remove(mp[c1],mp[c2]);
        gr.remove(mp[c2],mp[c1]);
        if(wei != -999){
            if(!gr.exist(mp[c1],mp[c2]))gr.insert(mp[c1],mp[c2],wei);
            if(!gr.exist(mp[c2],mp[c1]))gr.insert(mp[c2],mp[c1],wei);
        }
        //
        for(int i=0;i<vernum;++i){
            gr.dijkstra(pp[i].first);
        }

        for(int i=0;i<message_vec.size();++i){
            fout<<"from "<<message_vec[i].from<<" to "<<message_vec[i].to<<" cost ";
            gr.dijkstra_all(mp[message_vec[i].from],mp[message_vec[i].to]); //concrete information about hops
            fout<<"message "<<message_vec[i].sentence<<endl;
        }
        //
        //fin2.get();

    }

    return 0;
}
