#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#include "utils.cc"

#define PIPELINE 10

using namespace std;
using namespace zmqpp;

const string music_path = "./Music";
vector<string> av_children; // List of "below" servers.

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
  string ans;
  message incmsg, outmsg;
  while(ans != "OK"){
    outmsg << address;
    broker.send(outmsg);
    broker.receive(incmsg);
    incmsg >> ans;
  }
  cout << "Accepted by broker!" << endl;
}

void connect_parent(socket &parent) {
  string ans;
  message incmsg, outmsg;
  while(ans != "OK"){
    outmsg << "add";
    parent.send(outmsg);
    parent.receive(incmsg);
    incmsg >> ans;
  }
  // No orphan anymore :D
  cout << "Accepted by parent!" << endl;
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

void add_child(const string &identity, message &response) {
  av_children.push_back(identity);
  response << "OK";
}

void dispatch_client(message &incmsg, message &output) {
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

void dispatch_child(message &request, message &response) {
  string identity;
  request >> identity;
  if (identity.size() == 0)
    return;

  string command;
  request >> command;

  response << identity;

  if (command == "add") {
    add_child(identity, response);
    return;
  }

  if (command == "search") {
    search_song(response, response);
    return;
  }
}

int main(int argc, char** argv) {
  string client_endpoint = "tcp://*:",
         address = "tcp://",
         parent_endpoint   = "tcp://",
         children_endpoint = "tcp://*:";

  if (argc == 4) {
    int client_port   = zen::ports::server_client + stoi(argv[2]);
    int children_port = zen::ports::server_server + stoi(argv[2]);
    client_endpoint += to_string(client_port);
    address  += argv[1] + string(":") + to_string(client_port);
    parent_endpoint += argv[3];
    children_endpoint += to_string(children_port);
  } else {
    // cout << "Must provide an IP and id" << endl;
    cout << "Usage : " << argv[0] << " ip id parent_ip" << endl;
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


  socket children(context, socket_type::router);
  children.bind(children_endpoint);

  wake_up(broker, address);
  socket parent(context, socket_type::dealer);
  if (parent_endpoint != "tcp://localhost:4444") { // Root
    parent.connect(parent_endpoint);
    connect_parent(parent);
  }

  cout << "Listening for servers : " << children_endpoint << endl;
  cout << "Listening for clients : " << client_endpoint << endl;
  cout << "Connected to parent   : " << parent_endpoint << endl;
  cout << "Connected to broker   : " << broker_endpoint << endl;

  poller poller;
  poller.add(client);
  poller.add(children);

  message request, response;
  while (true) {
    if (poller.poll()) {
      if (poller.has_input(client)) {
        client.receive(request);
        dispatch_client(request, response);
        client.send(response);
      }
      if (poller.has_input(children)) {
        children.receive(request);
        dispatch_child(request, response);
        children.send(response);
      }
    }
  }

  return 0;
}
