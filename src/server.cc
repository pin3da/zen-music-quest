#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#define PIPELINE 10

using namespace std;

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

int main() {
  const string endpoint = "tcp://*:6666";
  zmqpp::context context;
  ifstream song;

  zmqpp::socket router(context, zmqpp::socket_type::router);
  // We have two parts per message so HWM is PIPELINE * 2
  router.set(zmqpp::socket_option::send_high_water_mark, PIPELINE * 2);
  router.bind(endpoint);

  while (true) {
    // First frame in each message is the sender identity
    zmqpp::message message;
    router.receive(message);

    string identity;
    //zmqpp::frame identity;
    message >> identity;
    
    if (identity.size() == 0)
      break; // Shutting down, quit
    
    // Second frame is "fetch" command
    string command;
    message >> command;
    assert (command == "fetch");
    
    //Third frame is song name
    string name;
    message >> name;
    
    zmqpp::message output;
    
    if (searchFile(name)){
      song.open("Music/" + name);
      // Third frame is chunk offset in file
      size_t offset;
      message >> offset;

      // Fourth frame is maximum chunk size
      size_t chunksz;
      message >> chunksz;

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

      // Send resulting chunk to client
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
