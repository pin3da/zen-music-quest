#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#include "utils.cc"

#define PIPELINE 10

using namespace std;
using namespace zmqpp;

const string music_path = "./music";


bool search_file(string name){
  DIR* dirp = opendir(music_path.c_str());
  dirent* dp;
  if (dirp != NULL) {
    while ((dp = readdir(dirp) )!= NULL) {
      if (strcmp(dp->d_name, name.c_str()) == 0)
        return true;
    }
  } else {
    return false;
  }
  return false;
}


void wake_up(socket &broker, string &address){
  string ans = "";
  while(ans != "OK"){
    message incmsg, outmsg;
    outmsg << address;
    broker.send(outmsg);
    broker.receive(incmsg);
    incmsg >> ans;
  }
  cout << "Accepted by broker!" << endl;
}

/**
 *  send_song : Must be called after a "fetch" request is performed.
 *     Read from third frame : song_name
 * */
void send_song(message &request, message &response) {
  ifstream song;
  // Third frame is song name
  string song_name;
  request >> song_name;

  if (search_file(song_name)){
    song.open(music_path +  "/" + song_name);
    // Fourth frame is chunk offset in file
    size_t offset;
    request >> offset;

    // Fifth frame is maximum chunk size
    size_t chunksz;
    request >> chunksz;

    // Read chunk of data from file
    song.seekg(offset);
    char *data = (char *)malloc (chunksz);
    assert (data);
    song.read(data, chunksz);

    size_t size = song.gcount();

    string chunk(data, size);
    response << chunk;
    song.close();
  } else {
    response << "NF";
  }
}

void search_song(message &request, message &response) {
  response << "Not working yet";
}


void dispatch(message &incmsg, message &output) {

  // First frame in each message is the sender identity
  string identity;
  incmsg >> identity;

  if (identity.size() == 0)
    return; // Shutting down, quit

  // Second frame is "fetch" command
  string command;
  incmsg >> command;
  if (command == "fetch") {
    output << identity;
    return send_song(incmsg, output);
  }

  if (command == "search") {
    output << identity;
    return search_song(incmsg, output);
  }

}

int main(int argc, char** argv) {
  string client_endpoint = "tcp://*:",
         address = "tcp://";

  if (argc == 3) {
    int client_port = zen::ports::server_client + stoi(argv[2]);
    client_endpoint += to_string(client_port);
    address  += argv[1] + string(":") + to_string(client_port);
  } else {
    cout << "Must provide an IP and id" << endl;
    cout << "Usage : " << argv[0] << " ip id" << endl;
    return 0;
  }

  const string broker_endpoint = "tcp://localhost:" + to_string(zen::ports::boker_server);
  context context;

  socket broker(context, socket_type::req);
  broker.connect(broker_endpoint);

  socket client(context, socket_type::router);
  // We have two parts per message so HWM is PIPELINE * 2
  client.set(socket_option::send_high_water_mark, PIPELINE * 2);
  client.bind(client_endpoint);

  wake_up(broker, address);

  poller poller;
  poller.add(client);

  while (true) {
    if (poller.poll()) {
      if (poller.has_input(client)) {
        message incmsg, output;
        client.receive(incmsg);
        dispatch(incmsg, output);
        client.send(output);
      }
    }
  }

  return 0;
}
