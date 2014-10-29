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

int main(int argc, char** argv) {
  string endpoint = "tcp://localhost:";
  const string br_port = "tcp://localhost:6666";
  context ctx;
  socket dealer(ctx, socket_type::dealer);
  socket br_end(ctx, socket_type::req);
  br_end.connect(br_port);
  
  string ans = "";
  
  while(ans != "hello"){
    message incmsg, outmsg;
    outmsg << "hello";
    br_end.send(outmsg);
    br_end.receive(incmsg);
    incmsg >> ans;
    incmsg >> endpoint;
  }
  
  cout << "I know where to connect to " << endpoint << endl;
  
  dealer.connect(endpoint);
  string name;
  
  if (argc > 1){
    name = argv[1];
  }
  else{
    cout << "No song provided!" << endl;
    return 0;
  }
  

  // Up to this many chunks in transit
  size_t credit = PIPELINE;

  size_t total  = 0; // Total bytes received
  size_t chunks = 0; // Total chunks received
  size_t offset = 0; // Offset of next chunk request

  ofstream song("output.mp3");

  while (true) {
    while (credit) {
      // Ask for next chunk
      message message;
      message << "fetch" << name << offset << CHUNK_SIZE;
      dealer.send(message);
      offset += CHUNK_SIZE;
      credit--;
    }
    message message;
    dealer.receive(message);
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
    if (size > CHUNK_SIZE)
      break; // Last chunk received; exit
  }
  printf ("%zd chunks received, %zd bytes\n", chunks, total);
  song.close();
  return 0;
}
