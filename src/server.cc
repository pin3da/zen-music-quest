#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#include "utils.cc"

#define PIPELINE 10

using namespace std;
using namespace zmqpp;

const int MAX_DOWNLOADS = numeric_limits<int>::max() - 1;

struct query{
  string parent_id, address;
  int parts, times;
  bool is_client;
  query() : parts(0), is_client(false){}
  query(string _parent_id) : parent_id(_parent_id), address("NF"), parts(0), times(MAX_DOWNLOADS + 1) , is_client(false){}
};

// begin server's state
const string music_path   = "./Music";
const string no_parent_id = "-1";
string address;
unordered_set<string> av_children;
unordered_map<string, query> queries;
bool is_root;
// end ss.


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
  while (ans != "OK") {
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
  string song_name;
  request >> song_name;

  if (search_file(song_name)){
    song.open(music_path +  "/" + song_name);
    size_t offset;
    request >> offset;

    size_t chunksz;
    request >> chunksz;

    song.seekg(offset);
    char *data = (char *)malloc (chunksz);
    assert (data);
    song.read(data, chunksz);

    string chunk(data, song.gcount());
    response << chunk;
    song.close();
  } else {
    response << "NF";
  }
}

void search_song(socket &children, socket &parent, message &request, message &response,
    const string &parent_id) {
  string uuid, song_name;
  request >> uuid >> song_name;

  cout << "looking for " << song_name << " at " << uuid << endl;
  cout << "av_children : " << av_children.size() << endl;

  if (queries.count(uuid) == 0) {
    cout << "Regis " << uuid << endl;
    queries[uuid] = query(parent_id);
    if (av_children.count(parent_id) == 0 and parent_id != no_parent_id) {
      cout << "Getting request from client" << endl;
      queries[uuid].is_client = true;
    }
  }

  if (parent_id != no_parent_id and !is_root) {
    message message;
    message << "search" << uuid << song_name;
    cout << "Sending message " << uuid << " to parent" << endl;
    parent.send(message);
    cout << "Sent message to parent" << endl;
  }

  for (auto child : av_children) {
    if (child != parent_id) {
      message message;
      message << child << "search" << uuid << song_name;
      cout << "Sending message " << uuid <<" to " << child << endl;
      children.send(message);
      cout << "Sent message to child" << endl;
    }
  }

  if (av_children.size() == 0 and parent_id == no_parent_id) { // leaf
    cout << "Resolve locally bc i'm cool" << endl;
    if (search_file(song_name)) {
      response << "found" << uuid << address << 1; // The 1 will be changed with the number of responses.
    } else {
      response << "found" << uuid << "NF" << MAX_DOWNLOADS;
    }
    queries.erase(uuid);
  }
}

void add_child(const string &identity, message &response) {
  if (av_children.count(identity) == 0) {
    av_children.insert(identity);
    response << "OK";
    cout << "Added new child with id " << identity << endl;
  } else {
    cout << "The child is already added" << endl;
  }
}

void dispatch_client(socket &children, socket &parent, message &incmsg, message &output) {
  string identity;
  incmsg >> identity;

  if (identity.size() == 0)
    return; // Shutting down, quit

  string command;
  incmsg >> command;
  if (command == "fetch") {
    output << identity;
    return send_song(incmsg, output);
  }
  if (command == "search") {
    output << identity;
    return search_song(children, parent, incmsg, output, identity);
  }
}

void notify_answer(socket &children, socket &parent, socket &client, string &uuid, const string &parent_id ) {
  query &current = queries[uuid];

  cout << "Is client : " << current.is_client << endl;
  if (current.is_client) {
    cout << "Notify client with best answer" << endl;
    message message;
    message << current.parent_id << current.address;
    client.send(message);
    return;
  }

  if (av_children.count(current.parent_id) == 0 and parent_id != no_parent_id) {
      cout << "Notify parent with best answer" << endl;
      message message;
      message << "found" << uuid << current.address << current.times;
      parent.send(message);
  } else {
    cout << "Notify child " << current.parent_id << " with the best answer" << endl;
    message message;
    message << current.parent_id << "found" << uuid << current.address << current.times;
    children.send(message);
  }
}


void process_answer(socket &children, socket &parent, socket &client, message &request, message & response,const string &identity) {
    string uuid, answer;
    int times = 0;
    request >> uuid >> answer >> times;
    query &current = queries[uuid];
    current.parts++;
    if (answer != "NF") {
      cout << identity << " says : " << answer << " " << times << endl;

      if (current.times > times) {
        current.address = answer;
        current.times   = times;
      }
    } else {
      current.times = MAX_DOWNLOADS;
      current.address = "NF";
      cout << "child did not found anything" << endl;
    }

    if (current.is_client) {
      if (is_root) {
        if ((size_t)current.parts == av_children.size())
          notify_answer(children, parent, client, uuid, identity);
      } else {
        if ((size_t)current.parts == av_children.size() + 1)
          notify_answer(children, parent, client, uuid, identity);
      }
    } else {
      if (is_root) {
        if ((size_t)current.parts == av_children.size() - 1)
          notify_answer(children, parent, client, uuid, identity);
      } else {
        if ((size_t)current.parts == av_children.size())
          notify_answer(children, parent, client, uuid, identity);
      }
    }
}

void dispatch_child(socket &children, socket &parent, socket &client, message &request, message &response) {
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
    search_song(children, parent, request, response, identity);
    return;
  }

  if (command == "found") {
    process_answer(children, parent, client, request, response, identity);
    return;
  }
  cout << "I don't know what to say" << endl;
  // response with an empty message otherwise
}



void dispatch_parent(socket &children, socket &parent, socket &client, message &request, message &response) {
  string command;
  request >> command;

  if (command == "search") {
    search_song(children, parent, request, response, no_parent_id);
    return;
  }
  if (command == "found") {
    process_answer(children, parent, client, request, response, no_parent_id);
    return;
  }
  cout << "I don't know what to say" << endl;
  // response with an empty message otherwise
}

int main(int argc, char** argv) {
  string client_endpoint = "tcp://*:",
         parent_endpoint   = "tcp://",
         children_endpoint = "tcp://*:";
         address = "tcp://";

  is_root = true;

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
    is_root = false;
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
  poller.add(parent);

  message request, response;
  while (true) {
    if (poller.poll()) {
      if (poller.has_input(client)) {
        client.receive(request);
        dispatch_client(children, parent, request, response);
        if (response.parts())
          client.send(response);
      }
      if (poller.has_input(children)) {
        children.receive(request);
        dispatch_child(children, parent, client, request, response);
        children.send(response);
      }
      if (poller.has_input(parent)) {
        parent.receive(request);
        dispatch_parent(children, parent, client, request, response);
        if (response.parts()) {
          cout << "answer found and set to the parent" << endl;
          parent.send(response);
          cout << "Really, I do" << endl;
        }
      }
    }
  }

  return 0;
}
