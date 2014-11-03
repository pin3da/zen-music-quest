#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>

#define PIPELINE 10
const size_t CHUNK_SIZE = 250000;

using namespace std;
using namespace zmqpp;

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
  outmsg << "search" << song_name;
  server.send(outmsg);
  server.receive(incmsg);
    
}


int main(int argc, char** argv) {
  const string broker_endpoint = "tcp://localhost:6667";
  string server_endpoint = "tcp://localhost:6666";
  string dload_endpoint = "tcp://localhost:6666";
  string song_name;
  
  context context;
  socket broker(context, socket_type::req);
  broker.connect(broker_endpoint);

  ask_for_server(broker, server_endpoint);
  socket server(context, socket_type::dealer);
  server.connect(server_endpoint);
  
  socket dload(context, socket_type::dealer);
  

  while(true){
    cout << "Welcome to Zen Music Quest!" << endl << endl;
    cout << "Type listen and the name of a song and then press enter:" << endl;
   
    
    cin >> song_name;
    getchar();
    
    search_for_song(server, song_name, dload_endpoint);
    
    if(dload_endpoint == "NF"){
      cout << "Song not found, sorry!" << endl << endl;
    }
    else{
      cout << "Your song will be here in no time!" << endl << endl;
      dload.connect(dload_endpoint);
      ask_for_song(dload, song_name);
      dload.disconnect(dload_endpoint);
    }
    
    
  }

  return 0;
}
