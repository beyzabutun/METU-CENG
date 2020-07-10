#include <iostream>
#include <fstream>
#include <limits.h>
#include <string>
#include <vector>
#include <utility>
#include <queue>
#include <deque>

int is_Forbidden(int k,std::vector<int> evenPer, std::vector<int> oddPer, int v){
	if(k%2==0){
		for(int i=0; i<evenPer.size(); i++){
			if(evenPer[i]==v) return 1;
		}
	}
	else{
		for(int i=0; i<oddPer.size(); i++){
			if(oddPer[i]==v) return 1;
		}
	}
	return 0;
}

int findMin(int dist[], bool visited[], int nRooms,int k,std::vector<int> evenPer, std::vector<int> oddPer) 
{ 

	int min = INT_MAX, min_index; 
    
	for (int v = 0; v < nRooms; v++) {
		if (visited[v] == false && dist[v] <= min && !is_Forbidden(k,evenPer,oddPer,v+1)) {
			min = dist[v];
			min_index = v; 
		}
	}

	return min_index; 
} 

std::vector<int> newPath(std::vector<int> path,int parent[],int key,int per){
	
    std::vector<int> temp;
	while(parent[key]!=-1) {
		temp.push_back(key+1);
   		key=parent[key];
   	}
   	temp.push_back(key+1);
   	int size=temp.size();
   	for(int i=size-1;i>=0;i--){
   		path.push_back(temp[i]);
   	}

    return path;

}

std::vector<std::pair<int,int> > newRoomYammo(std::vector<int> path,int per, std::vector<std::pair<int,int> > RoomYammo){
	int size=RoomYammo.size();
	for(int i=0; i<per+2 ; i++){
		for(int j=0; j<size;j++){
			if(RoomYammo[j].first == path[i]){
				RoomYammo[j].second=0;
			}

		}
	}
	return RoomYammo;
}

int ammoAmount(std::vector<std::pair<int,int> > RoomYammo, int v){
	int size=RoomYammo.size();
	for(int i=0; i<size ; i++){
		if(RoomYammo[i].first == v){
			return RoomYammo[i].second;
		}
	}
	return 0;
}



int main(int argc, char **argv){

	std::ifstream file;
	file.open(argv[1]);
	int i, ammo, nRooms, chamberRoom, keyRoom, sciRoom, nOdd, nEven, val, nCorr, r1, r2, a1,j, nRoomsAmmo;
	std::vector<int> oddPer;
	std::vector<int> evenPer;
	std::vector<std::pair<int,int> > RoomYammo;
	file >> ammo >> nRooms >> chamberRoom >> keyRoom >> sciRoom ;


	std::vector<std::vector<int> > adj(nRooms,std::vector<int>(nRooms,0));
	for(i=0;i<nRooms;i++){
		for(int j=0; j<nRooms;j++) adj[i][j]=0;
	}

	file >> nOdd;
	for(int i=0; i<nOdd ; i++){
		file >> val;
		oddPer.push_back(val);
	}

	file >> nEven;
	for(int i=0; i<nEven; i++){
		file >> val;
		evenPer.push_back(val);
	}

	file >> nCorr;
	for(i=0;i<nCorr;i++){
		file >> r1 >> r2 >> a1;

		adj[r1-1][r2-1]=a1;
		adj[r2-1][r1-1]=a1;
	}
	file >> nRoomsAmmo;

	for(int i=0; i<nRoomsAmmo ; i++){
		file >> r1 >> a1;

		RoomYammo.push_back(std::make_pair(r1,a1));

	}

	int dist[nRooms];	/*belli bir noktadan olan uzaklıklarını tutacak */
	bool visited[nRooms];  /* gidilip gidilmediğini tutacak */
	int parent[nRooms];
	int periods[nRooms];
	std::vector<int> path;
	std::vector<int> allKeys; /* bizim 3 ana duracağımızı tutacak. Başlangıç,key,sciroom,chamber */
	allKeys.push_back(1);
	allKeys.push_back(keyRoom);
	allKeys.push_back(sciRoom);
	allKeys.push_back(chamberRoom);
	int init=0;
	int per=1;
	int oldPer=1;
	for(int j=0; j<3;j++){
		for (int i = 0; i < nRooms; i++){
			periods[i]=per;
			dist[i] = INT_MAX; 
			visited[i] = false;
		}
		parent[allKeys[j]-1]=-1;
		dist[allKeys[j]-1]=init;
		if(j==0){
			visited[allKeys[2]-1]=true; visited[allKeys[3]-1]=true;
		}
		if(j==1){
			visited[allKeys[3]-1]=true;
		}

		for(int k=0; k<nRooms ; k++){
			
			int u= findMin(dist,visited, nRooms, per, evenPer, oddPer); /*min distance indexini verir,.*/
			per=periods[u];
			visited[u]=true;

			if((u+1)==allKeys[j+1]) {init=dist[u]; break;}

			for(int v=0; v<nRooms ; v++){
				int amountAmmo = ammoAmount(RoomYammo,v+1); /* adj roomdaki ammo miktarını verir.*/
				int isForbidden = is_Forbidden(per+1,evenPer, oddPer, v+1); /* even veya odd periyotlarda gidilip gidilemeyeceğini kontrol eder.*/
								
				if(isForbidden) continue;
				if (!visited[v] && adj[u][v] && dist[u] != INT_MAX && (dist[u]+adj[u][v] - amountAmmo) < dist[v]){ 
					parent[v]=u;
					periods[v]=per+1;
					dist[v] = dist[u] + adj[u][v] - amountAmmo; 
					}
			}						
		}
		
		path= newPath(path,parent,allKeys[j+1]-1,per);
		RoomYammo=newRoomYammo(path,per,RoomYammo);
		oldPer=per;
	}
	std::ofstream myfile;
  	myfile.open ("the3.out");
  	myfile << ammo-init;
  	myfile << "\n";
  	myfile << per;
  	myfile << "\n";
	for(int i=0;i<per+2;i++){
		if(path[i]==path[i+1]) i=i+1;
		myfile << path[i] ;
		if(i!=per+1) myfile << " " ;
	}

  	myfile.close();
  
	file.close();
}