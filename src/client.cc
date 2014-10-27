#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>

#define PIPELINE 10
const size_t CHUNK_SIZE = 250000;

using namespace std;

/**
 * Example taken of : http://zguide.zeromq.org/c:fileio3
 * NOTE : I'm using std::string as zmqpp::frame in this moment.
 *   reason : Library doesn't have cool ("<<"/">>") operators for zmqpp::frame
 *            and we can handle every char like a byte.
 * */

int main() {
  const string endpoint = "tcp://localhost:6666";
  zmqpp::context context;
  zmqpp::socket dealer(context, zmqpp::socket_type::dealer);
  dealer.connect(endpoint);

  // Up to this many chunks in transit
  size_t credit = PIPELINE;

  size_t total  = 0; // Total bytes received
  size_t chunks = 0; // Total chunks received
  size_t offset = 0; // Offset of next chunk request

  FILE *file = fopen ("output.mp3","wb");

  while (true) {
    while (credit) {
      // Ask for next chunk
      zmqpp::message message;
      message << "fetch" << offset << CHUNK_SIZE;
      dealer.send(message);
      offset += CHUNK_SIZE;
      credit--;
    }
    zmqpp::message message;
    dealer.receive(message);
    string chunk;
    message >> chunk;
    if (chunk.size() == 0)
      break; // Shutting down, quit
    chunks++;
    credit++;
    size_t size = chunk.size();
    fwrite(chunk.data(), sizeof(char), chunk.size(), file);
    total += size;
    if (size < CHUNK_SIZE)
      break; // Last chunk received; exit
  }
  printf ("%zd chunks received, %zd bytes\n", chunks, total);
  fclose(file);
  return 0;
}
