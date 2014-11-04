#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <SFML/Audio.hpp>
#include "utils.cc"

#define PIPELINE 10
const size_t CHUNK_SIZE = 250000;

using namespace std;
using namespace zmqpp;

mutex cool_mutex;

queue<string> playlist;
vector<string> playqueue;
char player_cmd = 'c';

/**
 * Example taken of : http://zguide.zeromq.org/c:fileio3
 * NOTE : I'm using std::string as zmqpp::frame in this moment.
 *   reason : Library doesn't have cool ("<<"/">>") operators for zmqpp::frame
 *            and we can handle every char like a byte.
 * */

void ask_for_server(socket &broker, string &song_endpoint) {
  string ans;
  while (ans != "hello") {
    message incmsg, outmsg;
    outmsg << "hello";
    broker.send(outmsg);
    broker.receive(incmsg);
    incmsg >> ans;
    if (ans != "bye")
      incmsg >> song_endpoint;
  }
  cout << "I know where to connect to " << song_endpoint << endl;
}

void ask_for_song(socket &song_s, const string &song_name, string output = "output.mp3") {
  // Up to this many chunks in transit
  size_t credit = PIPELINE;

  size_t total  = 0; // Total bytes received
  size_t chunks = 0; // Total chunks received
  size_t offset = 0; // Offset of next chunk request

  ofstream song(output);

  while (true) {
    while (credit) {
      // Ask for next chunk
      message message;
      message << "fetch" << song_name << offset << CHUNK_SIZE;
      song_s.send(message);
      offset += CHUNK_SIZE;
      credit--;
    }
    message message;
    song_s.receive(message);
    string chunk;
    message >> chunk;
    if(chunk == "NF"){
      cout << "Not Found" << endl;
      break;
    }
    if (chunk.size() == 0)
      break; // Shutting down, quit
    chunks++;
    credit++;
    size_t size = chunk.size();
    song.write(chunk.c_str(), size);
    total += size;
    if (size < CHUNK_SIZE)
      break; // Last chunk received; exit
  }

  printf("%zd chunks received, %zd bytes\n", chunks, total);
  song.close();
}

void search_for_song(socket &server, string song_name, string &dload_endpoint){
  message outmsg, incmsg;
  string uuid = gen_uuid();
  outmsg << "search" << uuid << song_name;
  server.send(outmsg);
  server.receive(incmsg);
  incmsg >> dload_endpoint;
}


void download_queue(string server_endpoint){
  context ctx;
  string dload_endpoint = "";
  
  socket server(ctx, socket_type::dealer);
  server.connect(server_endpoint);
  
  socket dload(ctx, socket_type::dealer);
  
  string song_name;
    
  int song_num = 0; 
  while(true){
    int jesus = 0;
    cool_mutex.lock();
    jesus = playlist.size();
    cool_mutex.unlock();
    if(jesus > 0){
      
      cool_mutex.lock();
      song_name = playlist.front();
      playlist.pop();
      cool_mutex.unlock();    
      search_for_song(server, song_name, dload_endpoint);

      if(dload_endpoint == "NF"){
        cout << "Song not found, sorry!" << endl << endl;
      }
      else{
        cout << "Your song will be here in no time!" << endl << endl;
        dload.connect(dload_endpoint);
        string outname = "song" + to_string(song_num) + ".ogg"; 
        ask_for_song(dload, song_name, outname);
        cool_mutex.lock();
        playqueue.push_back(outname);
        cool_mutex.unlock();
        dload.disconnect(dload_endpoint);
        song_num++;
        if (song_num > 1000){
          song_num = 0;
        }
      }
    }
    
  }
  
}

void play(){
  
  sf::Music music;
  int s_counter = 0;
  int jesus;
  string god;
  char cmd = 'c';
  while (true){
  
    cool_mutex.lock();
    jesus = playqueue.size();
    cmd = player_cmd;
    cool_mutex.unlock();
    
    if(cmd == 's'){
      music.stop();
      s_counter--;
      cool_mutex.lock();
      player_cmd = 'z';
      cool_mutex.unlock();
      
    }
    
    else if(cmd == 'n'){
      music.stop();
      if(s_counter > jesus - 1 and jesus > 0)
        s_counter = jesus - 1;
      cool_mutex.lock();
      player_cmd = 'c';
      cool_mutex.unlock();
      
      
    }
    else if(cmd == 'p'){
      music.stop();
      s_counter = s_counter - 2;
      if(s_counter < 0)
        s_counter = 0;
      cool_mutex.lock();
      player_cmd = 'c';
      cool_mutex.unlock();
    }
  
    if(s_counter < jesus){
      int holy = 0;
      
      cool_mutex.lock();
      god = playqueue[s_counter];
      holy = music.getStatus();
      
      cool_mutex.unlock();
      //cout << cmd << endl;
      if(holy == 0 and cmd == 'c'){
        if(music.openFromFile(god)){
          music.play();
          s_counter++;
        }
      }
         
            
    }
     
  }
}

int main(int argc, char** argv) {
  const string broker_endpoint = "tcp://localhost:6667";
  string server_endpoint = "tcp://localhost:6666";
  string dload_endpoint = "tcp://localhost:6666";
  string song_name;

  context ctx;
  socket broker(ctx, socket_type::req);
  broker.connect(broker_endpoint);

  ask_for_server(broker, server_endpoint);
  
  
  
  thread downloads(download_queue, server_endpoint);
  thread playing(play); 
  downloads.detach();
  playing.detach();
  
  string command = "";
  sf::Music music;
  while (command != "exit") {
    cout << "Welcome to Zen Music Quest!" << endl << endl;
    cout << "Type add and the name of a song to add it to your playlist or:" << endl;
    cout << "Type next to skip a song and prev to hear the previous one" << endl;
    cout << "Type stop to stop playing music, or play to start playing" << endl;
    cin >> command;
    
    if(command == "add"){
      cin >> song_name;
      cool_mutex.lock();
      playlist.push(song_name);
      cool_mutex.unlock();
      
      
    }
    
    if(command == "next"){      
      cool_mutex.lock();
      player_cmd = 'n';
      cool_mutex.unlock();      
    }
    
    if(command == "stop"){
      cool_mutex.lock();
      player_cmd = 's';
      cool_mutex.unlock();
    }
    
    if(command == "prev"){      
      cool_mutex.lock();
      player_cmd = 'p';
      cool_mutex.unlock();      
    } 
    
    if(command == "play"){
      cool_mutex.lock();
      player_cmd = 'c';
      cool_mutex.unlock();
    }   
    
    getchar();
    
  }
  
  downloads.~thread();
  playing.~thread();
  return 0;
}
