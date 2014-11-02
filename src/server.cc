#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>
#include <dirent.h>

#define PIPELINE 10

using namespace std;
using namespace zmqpp;

const string music_path = "./music";


bool searchFile(string name){
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


void dispatch(message &incmsg, message &output) {
  ifstream song;
  // First frame in each message is the sender identity
  string identity;
  incmsg >> identity;

  if (identity.size() == 0)
    return; // Shutting down, quit

  // Second frame is "fetch" command
  string command;
  incmsg >> command;
  assert (command == "fetch");

  // Third frame is song name
  string song_name;
  incmsg >> song_name;

  if (searchFile(song_name)){
    song.open(music_path +  "/" + song_name);
    // Fourth frame is chunk offset in file
    size_t offset;
    incmsg >> offset;

    // Fifth frame is maximum chunk size
    size_t chunksz;
    incmsg >> chunksz;

    // Read chunk of data from file
    song.seekg(offset);
    char *data = (char *)malloc (chunksz);
    assert (data);
    song.read(data, chunksz);

    size_t size = song.gcount();

    string chunk(data, size);
    output << identity << chunk;
    song.close();
  } else {
    output << identity << "NF";
  }

}

int main(int argc, char** argv) {
  string song_endpoint = "tcp://*:";
  string address = "tcp://";
  if (argc == 3) {
    song_endpoint += argv[2];
    address  += argv[1] + string(":") + argv[2];
  } else {
    cout << "Must provide an IP and port" << endl;
    cout << "Usage : " << argv[0] << " ip port" << endl;
    return 0;
  }

  const string broker_endpoint = "tcp://localhost:6668";
  context ctx;

  socket broker(ctx, socket_type::req);
  broker.connect(broker_endpoint);

  socket song_c(ctx, socket_type::router);
  // We have two parts per message so HWM is PIPELINE * 2
  song_c.set(socket_option::send_high_water_mark, PIPELINE * 2);
  song_c.bind(song_endpoint);

  wake_up(broker, address);

  while (true) {
    message incmsg, output;
    song_c.receive(incmsg);
    dispatch(incmsg, output);
    song_c.send(output);
  }

  return 0;
}
