#include <stdio.h>
#include "deploy.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<math.h>
#define MAX_LINK_NUM 20000
#define MAX_VER_NUM	1000
#define MAX_ARC_NUM 20000
#define MAX_CON_NUM 500


/******部署图链路集的维护队列******/
typedef struct{
	int p;
}QNode,*QList;

typedef struct{
	QList qlist;
	int front;
	int rear;
}LinkQueue;

void Put(LinkQueue *lq,int p){
	if(lq->rear==MAX_ARC_NUM+1) lq->rear=0;
	lq->qlist[lq->rear].p=p;
	lq->rear++;
}

int Get(LinkQueue *lq){
	int p;
	if(lq->front==MAX_ARC_NUM+1) lq->front=0;
	p=lq->qlist[lq->front].p;
	lq->front++;
	return p;
}

/******服务器部署图的十字链表结构******/
typedef struct{
	int firstin,firstout;
	int lastin,lastout;
	int obw;
	int ibw;
	int sif;//是否有服务器部署在该节点	
}DvNode,*DadjList;

typedef struct{
	int tailvex,headvex;
	int hlink,tlink;
	int subw;
	int bw;
	int unitcost;
	int u;
	int re;
}*DarcList,DarcNode;

typedef struct{
	int next;
}SLNode,*SLinkList;

typedef struct{
	SLinkList slist;
	int first;
	int last;
	int length;
}ServerList;

typedef struct{
	int adjvex;
	int demand;
}ConNode,*ConList;

typedef struct{
	DadjList vertices;
	DarcList arcs;
	ServerList servers;//部署了服务器的节点链表
	ConList clist;//消费节点表
	LinkQueue lq;//链路集维护队列
	int connum,vernum,arcnum;
	int serverprice;
	int cost;//部署方案成本
}Deploygraph;

/******消费者链表查找*****/
int Search(Deploygraph *dg,int adjvex){
	for(int i=0;i<dg->connum;i++){
		if(dg->clist[i].adjvex==adjvex) return i;
	}
	return -1;
}

/******服务器链表操作函数******/
void DelServer(Deploygraph *dg,int p){
	int pr,pn;

	pr=-1;
	pn=dg->servers.first;
	while(pn!=p){
		pr=pn;
		pn=dg->servers.slist[pn].next;
	}
	if(pr==-1){
		dg->servers.first=dg->servers.slist[pn].next;
	}else{
		dg->servers.slist[pr].next=dg->servers.slist[pn].next;
	}

	if(p==dg->servers.last){
		dg->servers.last=pr;	
	}
	
	dg->vertices[p].sif=0;
	dg->servers.length--;
}

void AddServer(Deploygraph *dg,int p){
	if(dg->servers.first==-1){
		dg->servers.first=p;
		dg->servers.last=p;
		dg->servers.slist[p].next=-1;
	}else{
		dg->servers.slist[dg->servers.last].next=p;
		dg->servers.last=p;
		dg->servers.slist[p].next=-1;
	}
	dg->vertices[p].sif=1;
	dg->servers.length++;
}

/******部署图初始化******/
void DgInit(Deploygraph *Dgraph){
	Dgraph->vertices=(DadjList)malloc(MAX_VER_NUM*sizeof(DvNode));
	Dgraph->arcs=(DarcList)malloc((MAX_ARC_NUM+1)*sizeof(DarcNode));
	Dgraph->servers.slist=(SLinkList)malloc((MAX_VER_NUM)*sizeof(SLNode));
	Dgraph->clist=(ConList)malloc(MAX_CON_NUM*sizeof(ConNode));
	Dgraph->lq.qlist=(QList)malloc((MAX_ARC_NUM+1)*sizeof(QNode));

	Dgraph->connum=0;
	Dgraph->vernum=0;
	Dgraph->arcnum=0;
	Dgraph->cost=0;

	Dgraph->servers.first=-1;
	Dgraph->servers.last=-1;
	Dgraph->servers.length=0;

	for(int i=0;i<MAX_VER_NUM;i++){
		Dgraph->servers.slist[i].next=-1;
	}
	
	Dgraph->lq.front=1;
	Dgraph->lq.rear=0;

	for(int i=0;i<MAX_ARC_NUM+1;i++){
		Dgraph->lq.qlist[i].p=i;
	}
}

/*******部署图释放******/
void FreeDg(Deploygraph *dg){
	free(dg->vertices);
	free(dg->arcs);
	free(dg->servers.slist);
	free(dg->lq.qlist);
	free(dg->clist);
}
/******部署图拷贝******/
void DgraphCopy(Deploygraph *d1,Deploygraph *d2){

	memcpy(d1->vertices,d2->vertices,MAX_VER_NUM*sizeof(DvNode));

	memcpy(d1->arcs,d2->arcs,(MAX_ARC_NUM+1)*sizeof(DarcNode));

	d1->servers.first=d2->servers.first;
	d1->servers.last=d2->servers.last;
	d1->servers.length=d2->servers.length;
	memcpy(d1->servers.slist,d2->servers.slist,MAX_VER_NUM*sizeof(SLNode));

	memcpy(d1->clist,d2->clist,MAX_CON_NUM*sizeof(ConNode));

	d1->lq.front=d2->lq.front;
	d1->lq.rear=d2->lq.rear;
	memcpy(d1->lq.qlist,d2->lq.qlist,(MAX_ARC_NUM+1)*sizeof(QNode));

	d1->connum=d2->connum;
	d1->cost=d2->cost;	
}

/******部署图加链路******/
void Addarc(Deploygraph *dg,DarcNode arc){
	int p;
	int hv;
	int tv;

	p=Get(&(dg->lq));
	 
	hv=arc.headvex;
	tv=arc.tailvex;
	
	dg->arcs[p]=arc;
	
	if(dg->vertices[hv].firstout==0){
		dg->vertices[hv].firstout=p;
		dg->vertices[hv].lastout=p;
		dg->arcs[p].hlink=0;
	}else{
		dg->arcs[dg->vertices[hv].lastout].hlink=p;
		dg->vertices[hv].lastout=p;
		dg->arcs[p].hlink=0;
	}
	
	if(dg->vertices[tv].firstin==0){
		dg->vertices[tv].firstin=p;
		dg->vertices[tv].lastin=p;
		dg->arcs[p].tlink=0;
	}else{
		dg->arcs[dg->vertices[tv].lastin].tlink=p;
		dg->vertices[tv].lastin=p;
		dg->arcs[p].tlink=0;
	}
}

/******部署图节点栈******/
typedef struct{
	int dbw;
	int arc;
	char str[MAX_VER_NUM];
}SNode,*SqList;

typedef struct{
	SqList sl;
	int base;
	int top;
}DStack;

void Push(DStack *ds,int dn,int dbw){
//	printf("top=%d\n",ds->top);
	ds->sl[ds->top].arc=dn;
	ds->sl[ds->top].dbw=dbw;
	ds->top++;
}

void Pop(DStack *ds,int *dn,int *dbw){
	ds->top--;
	*dn=ds->sl[ds->top].arc;
	*dbw=ds->sl[ds->top].dbw;
}

//以下两个操作用于部署图输出
void Pushstr(DStack *ds,int dn,int dbw,char* str){
	ds->sl[ds->top].arc=dn;
	ds->sl[ds->top].dbw=dbw;
	strcpy(ds->sl[ds->top].str,str);
	ds->top++;
}

void Popstr(DStack *ds,int *dn,int *dbw,char* str){
	ds->top--;
	*dn=ds->sl[ds->top].arc;
	*dbw=ds->sl[ds->top].dbw;
	strcpy(str,ds->sl[ds->top].str);
}
void Clear(DStack *ds){
	ds->top=0;
	ds->base=0;
}
int StackEmpty(DStack *ds){
	if(ds->top==ds->base) return 1;
	else return 0;
}

DStack ds; //全局用节点栈

/******初始化邻接表及相关数据*******/
void InitGraph(char **const buff,Deploygraph *dg){

	int vernum,arcnum,connum;
	int serverprice;
	DarcNode an,ran;
	ConNode cn;
	int cid;
	char *taken,*c;
	int crow=0;//当前行

	DgInit(dg);
	ds.top=0;
	ds.base=0;
	ds.sl=(SqList)malloc(MAX_VER_NUM*sizeof(SNode));

	c=buff[crow];
	taken=strtok(c," ");
	vernum=atoi(taken);
    dg->vernum=vernum;
	taken=strtok(NULL," ");
	arcnum=atoi(taken);
	dg->arcnum=2*arcnum;
	taken=strtok(NULL," ");
	connum=atoi(taken);
	dg->connum=connum;
	crow+=2;//跳过空行
	

	c=buff[crow];
	taken=strtok(c," ");
	serverprice=atoi(taken);
	dg->serverprice= serverprice;

	crow+=2;

	//网络拓扑图的初始化
	for(int j=1;j<=2*arcnum;j++){
		c=buff[crow];

		taken=strtok(c," ");
		an.headvex=atoi(taken);
		
		taken=strtok(NULL," ");
		an.tailvex=atoi(taken);

		taken=strtok(NULL," ");
		an.subw=atoi(taken);

		taken=strtok(NULL," ");
		an.unitcost=atoi(taken);

		an.bw=0;
		an.u=0;
		an.re=j+1;
		Addarc(dg,an);
		
		j++;
		//反向链路
		ran.headvex=an.tailvex;
		ran.tailvex=an.headvex;
		ran.subw=an.subw;
		ran.unitcost=an.unitcost;
		ran.bw=0;
		ran.u=0;
		ran.re=j-1;
		Addarc(dg,ran);
		crow++;
		
	}	

	crow++;

	for(int i=0;i<dg->connum;i++){
		c=buff[crow];

		taken=strtok(c," ");
		cid=atoi(taken);

		taken=strtok(NULL," ");
		cn.adjvex=atoi(taken);

		taken=strtok(NULL," ");
		cn.demand=atoi(taken);

		crow++;

		dg->clist[cid]=cn;
		
		AddServer(dg,cn.adjvex);
		dg->cost+=dg->serverprice;
		dg->vertices[cn.adjvex].obw=cn.demand;
	}	
}

/******打印部署图路径******/

void PrintRoute(Deploygraph *pg,char *topo){

	int tv,dm,bt;
	int ps,p;
	int cnt=0;
	char num[30];
	char* route;
	char str[MAX_VER_NUM];

	route=(char*)malloc(MAX_ARC_NUM*MAX_VER_NUM);
	route[0]='\0';
	str[0]='\0';
	
	ps=pg->servers.first;
	while(ps!=-1){		
		p=pg->vertices[ps].firstout;
		if(p==0){
			sprintf(num,"%d",ps);
			strcat(route,num);
			strcat(route," ");

			sprintf(num,"%d",Search(pg,ps));
			strcat(route,num);
			strcat(route," ");

			sprintf(num,"%d",pg->vertices[ps].obw);
			strcat(route,num);
			strcat(route," ");
			strcat(route,"\n");
			cnt++;	

		}else{
			while(p!=0){
				sprintf(num,"%d",ps);
				strcat(num," ");
				Pushstr(&ds,p,pg->arcs[p].bw,num);
				pg->vertices[ps].obw-=pg->arcs[p].bw;
				p=pg->arcs[p].hlink;	
			}
			if(pg->vertices[ps].obw!=0){

				sprintf(num,"%d",ps);
				strcat(route,num);
				strcat(route," ");

				sprintf(num,"%d",Search(pg,ps));
				strcat(route,num);
				strcat(route," ");

				sprintf(num,"%d",pg->vertices[ps].obw);
				strcat(route,num);
				strcat(route," ");
				strcat(route,"\n");
				cnt++;
				
			}
		}
		while(StackEmpty(&ds)!=1){
			Popstr(&ds,&p,&dm,str);

			tv=pg->arcs[p].tailvex;
				
			pg->arcs[p].bw-=dm;

			if(pg->vertices[tv].firstout==0){
				strcat(route,str);
				sprintf(num,"%d",tv);
				strcat(route,num);
				strcat(route," ");

				sprintf(num,"%d",Search(pg,tv));
				strcat(route,num);
				strcat(route," ");

				sprintf(num,"%d",dm);
				strcat(route,num);
				strcat(route," ");
				strcat(route,"\n");
				cnt++;
			

			}else{
				p=pg->vertices[tv].firstout;
				bt=0;
				while(p!=0){
					bt+=pg->arcs[p].bw;
					p=pg->arcs[p].hlink;
				}
				if(dm>bt){
					strcat(route,str);
					sprintf(num,"%d",tv);
					strcat(route,num);
					strcat(route," ");

					sprintf(num,"%d",Search(pg,tv));
					strcat(route,num);
					strcat(route," ");

					sprintf(num,"%d",dm-bt);
					strcat(route,num);
					strcat(route," ");
					strcat(route,"\n");
					cnt++;
				

					dm=bt;
				}
				p=pg->vertices[tv].firstout;
				sprintf(num,"%d",tv);
				strcat(num," ");
				strcat(str,num);
				while(dm>0){
					if(pg->arcs[p].bw>=dm){
						Pushstr(&ds,p,dm,str);
						dm=0;
					}else{
						if(pg->arcs[p].bw!=0){
							Pushstr(&ds,p,pg->arcs[p].bw,str);
						}
						dm-=pg->arcs[p].bw;
						p=pg->arcs[p].hlink;
					}
				}
			}
		}
		ps=pg->servers.slist[ps].next;
	}
	
	sprintf(num,"%d",cnt);
	strcat(topo,num);
	strcat(topo,"\n");
	strcat(topo,"\n");
	strcat(topo,route);
	Clear(&ds);
	free(route);
}
/******部署图删路径******/
void DeRoute(Deploygraph *dg){

	int hv,bw,uc,tv,p,server;
	
	Clear(&ds);
	server=dg->servers.first;
	while(server!=-1){	
		p=dg->vertices[server].firstin;
		while(p!=0){
			Push(&ds,p,dg->arcs[p].bw);

			while(StackEmpty(&ds)!=1){
				Pop(&ds,&p,&bw);	
				hv=dg->arcs[p].headvex;
				tv=dg->arcs[p].tailvex;
				uc=dg->arcs[p].unitcost;
				dg->vertices[hv].obw-=bw;
				dg->vertices[tv].ibw-=bw;
			
				dg->arcs[p].bw-=bw;
				dg->arcs[p].subw+=bw;
				dg->cost-=uc*bw;
				
				if(dg->arcs[p].bw==0){
					dg->arcs[dg->arcs[p].re].u=0;
				}
				if(dg->vertices[hv].sif==1){
					if(dg->vertices[hv].obw==0){
						DelServer(dg,hv);
						dg->cost-=dg->serverprice;
					
					}else{
						dg->vertices[hv].obw-=bw;
					}
				}else{
					p=dg->vertices[hv].firstin;
					while(bw>0){
						if(dg->arcs[p].bw>=bw){
							Push(&ds,p,bw);
							bw=0;
						}else{
							if(dg->arcs[p].bw!=0){
								Push(&ds,p,dg->arcs[p].bw);
							}
							bw-=dg->arcs[p].bw;
							p=dg->arcs[p].tlink;
						}
					}
				}
			}
			p=dg->arcs[p].tlink;
		}
		server=dg->servers.slist[server].next;
	}
	Clear(&ds);
}

/******部署服务器节点随机移动函数******/
void GetNext(Deploygraph *ng){

	int sn;//服务器随机移动的台数
	int ra;
	int sid;//随机删去的服务器编号
	int p;//链表操作用指针
	int ns;//新部署的服务器节点
	int bwd;
	int btd;//节点链路的可用总带宽;
	sn=rand()%(ng->servers.length);
	
	for(int i=0;i<sn;i++){

	    ra=rand()%(ng->servers.length);	
		p=ng->servers.first;
		for(int j=0;j<ra;j++){	
			p=ng->servers.slist[p].next;
		}
		sid=p;
		
		bwd=ng->vertices[sid].obw-ng->vertices[sid].ibw;
		p=ng->vertices[sid].firstin;
		btd=0;
		while(p!=0){
			if(ng->arcs[ng->arcs[p].re].u!=1){
				btd+=ng->arcs[p].subw;
			}
			p=ng->arcs[p].tlink;	
		}
		if(bwd>btd) continue;
		else{
			ng->cost-=ng->serverprice;
			DelServer(ng,sid);
			p=ng->vertices[sid].firstin;
			while(bwd>0){
				ns=ng->arcs[p].tailvex;
				if(ng->arcs[ng->arcs[p].re].u==1){
					p=ng->arcs[p].tlink;
					continue;
				}
				if(ng->arcs[p].subw>=bwd){
					ng->arcs[p].subw-=bwd;
					ng->arcs[p].bw+=bwd;
					ng->cost+=(bwd*ng->arcs[p].unitcost);
					ng->vertices[sid].ibw+=bwd;
					ng->vertices[ns].obw+=bwd;
					bwd=0;
					ng->arcs[p].u=1;
				}else{
					bwd-=ng->arcs[p].subw;
					ng->arcs[p].bw+=ng->arcs[p].subw;
					ng->cost+=(bwd*ng->arcs[p].unitcost);
					ng->vertices[sid].ibw+=ng->arcs[p].subw;
					ng->vertices[ns].obw+=ng->arcs[p].subw;
					ng->arcs[p].u=1;
					ng->arcs[p].subw=0;
				}						
			
				if(ng->vertices[ns].sif==1){
					p=ng->arcs[p].tlink;
					continue;
				}
				else{
					AddServer(ng,ns);
					ng->cost+=ng->serverprice;		
				}
				p=ng->arcs[p].tlink;
			}
		}	
	}
}


/******模拟退火算法求近似最优解******/
void SA(Deploygraph *dg){

	float t=3000,t_min=1e-6;
	float const delta=0.98;
	float diff;
	time_t s,e;
	int j=0;
	int i=400;//外循环次数

	Deploygraph og;//当前的的最优部署方案
	Deploygraph ng;//下一部署方案
	DgInit(&og);
	DgInit(&ng);

	DgraphCopy(&og,dg);
	DgraphCopy(&ng,&og);
	time(&s);

	while(i>0){
		while(t>t_min){
			GetNext(&ng);
			j++;
		//	printf("cost=%d\n",ng.cost);
			if(ng.cost<dg->cost) DgraphCopy(dg,&ng);
			diff=og.cost-ng.cost;
			if(diff>0) DgraphCopy(&og,&ng);
			else{
				if(exp(diff/t)>(rand()/(RAND_MAX+1.0))){
					DgraphCopy(&og,&ng);
				}
			}	
			t*=delta;	
		}
		DgraphCopy(&ng,dg);
		i--;
		t=3000;
		time(&e);
		if((e-s)>=5) break;
	}
	printf("time=%ld\n",e-s);
	printf("j=%d\n",j);
	FreeDg(&og);
	FreeDg(&ng);
}

void deploy_server(char * topo[MAX_ARC_NUM], int line_num,char * filename)
{
	char *topo_file;
	Deploygraph dg;
	InitGraph(topo,&dg);

	printf("*****arcs******\n");
	for(int i=0;i<dg.arcnum+3;i++){
		printf("%d %d %d %d %d %d %d %d %d\n",i,dg.arcs[i].headvex,dg.arcs[i].tailvex,dg.arcs[i].bw,dg.arcs[i].subw,dg.arcs[i].unitcost,dg.arcs[i].hlink,dg.arcs[i].tlink,dg.arcs[i].re);
	}
	printf("*****vers******\n");
	for(int i=0;i<dg.vernum;i++){
		printf("%d %d %d %d %d %d\n",i,dg.vertices[i].firstin,dg.vertices[i].lastin,dg.vertices[i].obw,dg.vertices[i].ibw,dg.vertices[i].sif);
	}
 	SA(&dg);
	topo_file=(char*)malloc(MAX_ARC_NUM*MAX_VER_NUM);
	topo_file[0]='\0';
	PrintRoute(&dg,topo_file);
	printf("cost=%d,servernum=%d\n",dg.cost,dg.servers.length);
	write_result(topo_file, filename);
	FreeDg(&dg);
}


