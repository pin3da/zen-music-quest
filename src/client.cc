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


int main(int argc, char** argv) {
  const string broker_endpoint = "tcp://localhost:6667";
  string server_endpoint = "tcp://localhost:6666";

  string song_name;
  if (argc > 1) {
    song_name = argv[1];
  } else {
    cout << "No song provided!" << endl;
    cout << "Usage : " << argv[0] << " song_name" << endl;
    return 0;
  }


  context context;
  socket broker(context, socket_type::req);
  broker.connect(broker_endpoint);

  ask_for_server(broker, server_endpoint);
  socket server(context, socket_type::dealer);
  server.connect(server_endpoint);

  ask_for_song(server, song_name);

  return 0;
}
