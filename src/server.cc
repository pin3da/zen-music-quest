#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#define PIPELINE 10

using namespace std;
using namespace zmqpp;

int searchFile(string name){ 
	DIR* dirp = opendir("Music");
	dirent* dp;
	if(dirp != NULL){
		while((dp = readdir(dirp) )!= NULL){
			if(strcmp(dp->d_name, name.c_str()) == 0)
				return 1; 			
		}
	}
	else{
		return -1;
	}
	return 0;
}  

int main(int argc, char** argv) {
  string endpoint = "tcp://*:";
  string address = "tcp://";
  if(argc == 3){
    endpoint += argv[2];
    address += argv[1]; 
    address += ":";
    address += argv[2];
  }
  else{
    cout << "Must provide an IP and port" << endl;
    return 0;
  }
  
  const string br_port = "tcp://localhost:5555";
  context ctx;
  
  socket br_end(ctx, socket_type::req);
  br_end.connect(br_port); 
  
  socket router(ctx, socket_type::router);
  // We have two parts per message so HWM is PIPELINE * 2
  router.set(socket_option::send_high_water_mark, PIPELINE * 2);
  router.bind(endpoint);
  
  string ans = "";
  while(ans != "OK"){
    message incmsg, outmsg;
    outmsg << address;
    br_end.send(outmsg);
    br_end.receive(incmsg);
    incmsg >> ans;    
  }
  
  cout << "Accepted by broker!" << endl;
  ifstream song;
  
  while (true) {
    // First frame in each message is the sender identity
    message incmsg;
    router.receive(incmsg);

    string identity;
    //zmqpp::frame identity;
    incmsg >> identity;
    
    if (identity.size() == 0)
      break; // Shutting down, quit
    
    // Second frame is "fetch" command
    string command;
    incmsg >> command;
    assert (command == "fetch");
    
    //Third frame is song name
    string name;
    incmsg >> name;
    
    message output;
    
    if (searchFile(name)){
      song.open("Music/" + name);
      // Fourth frame is chunk offset in file
      size_t offset;
      incmsg >> offset;

      // Fifth frame is maximum chunk size
      size_t chunksz;
      incmsg >> chunksz;

      // Read chunk of data from file
      song.seekg(offset);
      char *data = (char *)malloc (chunksz);
      assert (data);
      song.read(data, chunksz);
      
      size_t size;
      
      if(song.eof())
        size = chunksz + 1;
      else
        size = chunksz;

      string chunk(data, size);
      //cout << chunk << endl;
      output << identity << chunk;
      song.close();
    }
    else{
      output << identity << "NF";
    }
      

    
    router.send(output);
    
    
  }
  
  return 0;
}
