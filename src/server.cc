#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>

#define PIPELINE 10

using namespace std;

int main() {
  const string endpoint = "tcp://*:6666";
  zmqpp::context context;
  FILE *file = fopen ("testdata.mp3", "r");
  assert (file);

  zmqpp::socket router(context, zmqpp::socket_type::router);
  // We have two parts per message so HWM is PIPELINE * 2
  router.set(zmqpp::socket_option::send_high_water_mark, PIPELINE * 2);
  router.bind(endpoint);

  while (true) {
    // First frame in each message is the sender identity
    zmqpp::message message;
    router.receive(message);

    // string identity;
    zmqpp::frame identity;
    message >> identity;

    if (identity.size() == 0)
      break; // Shutting down, quit

    // Second frame is "fetch" command
    string command;
    message >> command;
    assert (command == "fetch");

    // Third frame is chunk offset in file
    size_t offset;
    message >> offset;

    // Fourth frame is maximum chunk size
    size_t chunksz;
    message >> chunksz;

    // Read chunk of data from file
    fseek (file, offset, SEEK_SET);
    char *data = (char *)malloc (chunksz);
    assert (data);

    // Send resulting chunk to client
    size_t size = fread (data, 1, chunksz, file);
    string chunk(data, size);
    zmqpp::message output;
    output << identity << chunk;
    router.send(output);
  }
  fclose (file);

  return 0;
}
