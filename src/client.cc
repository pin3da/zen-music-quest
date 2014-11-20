#include <queue>
#include <mutex>
#include <thread>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <zmqpp/zmqpp.hpp>
#include <SFML/Audio.hpp>
#include "utils.cc"

#define PIPELINE 10
const size_t CHUNK_SIZE = 250000;

using namespace std;
using namespace zmqpp;

mutex cool_mutex;

deque<string> playlist;
unordered_map<int, pair<string, string>> playqueue;
//unordered_map<string, string> corres;
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

void ask_for_song(socket &song_s, const string &song_name, string output = "output.mp3", bool is_adver = false) {
  // Up to this many chunks in transit
  size_t credit = PIPELINE;

  size_t total  = 0; // Total bytes received
  size_t chunks = 0; // Total chunks received
  size_t offset = 0; // Offset of next chunk request

  system("exec mkdir -p tmp");

  ofstream song("tmp/" + output);

  while (true) {
    while (credit) {
      // Ask for next chunk
      message message;
      if(is_adver){
        message << "fetch" << "adver" << song_name << offset << CHUNK_SIZE;
      }else{
        message << "fetch" << song_name << offset << CHUNK_SIZE;
      }
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

  //printf("%zd chunks received, %zd bytes\n", chunks, total);
  song.close();
}

void search_for_song(socket &server, string song_name, string &dload_endpoint){
  message outmsg, incmsg;
  string uuid = gen_uuid();
  outmsg << "search" << uuid << song_name;
  server.send(outmsg);
  server.receive(incmsg);
  incmsg >> dload_endpoint;
  while(dload_endpoint == ""){
    server.receive(incmsg);
    incmsg >> dload_endpoint;
  }
}

void ask_for_adver(socket &server, string &adver_name){
  message outmsg, incmsg;
  outmsg << "adver";
  server.send(outmsg);
  server.receive(incmsg);
  incmsg >> adver_name;
}


void download_queue(string server_endpoint){
  context ctx;
  string dload_endpoint = "";
  socket server(ctx, socket_type::dealer);
  server.connect(server_endpoint);
  socket dload(ctx, socket_type::dealer);
  string song_name;
  int song_num = 0;
  while (true) {
    int pl_size = 0;
    cool_mutex.lock();
    pl_size = playlist.size();
    cool_mutex.unlock();
    if (pl_size > 0) {
      cool_mutex.lock();
      song_name = playlist.front();
      playlist.pop_front();
      cool_mutex.unlock();
      string outname = "song" + to_string(song_num) + ".ogg";
      if(song_name == "adver"){
        ask_for_adver(server, song_name);
        if(song_name!= "NF"){
          ask_for_song(server, song_name, outname, true);
          cool_mutex.lock();
          playqueue[song_num] = {"adver", outname};
          cool_mutex.unlock();
          song_num++;
          if (song_num > 500)
            song_num = 0;
        }

        //cout << "after ask_for_adver: " << song_name << endl;
      } else{
        search_for_song(server, song_name, dload_endpoint);
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        if (dload_endpoint == "NF" or dload_endpoint == "") {
          cout << "Song not found, sorry!" << endl;
        } else {
          cout << "Your song will be here in no time!" << endl;
          dload.connect(dload_endpoint);

          ask_for_song(dload, song_name, outname);
          cool_mutex.lock();
          playqueue[song_num] = {song_name, outname};
          cool_mutex.unlock();
          dload.disconnect(dload_endpoint);
          song_num++;
          if (song_num > 500) {
            song_num = 0;
          }
        }
         cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
      }
    }
  }
}

void play(){
  sf::Music music;
  int s_counter = 0;
  int player_status = 0;
  int queue_size = 0;
  string song_name = "";
  string outname = "";
  char cmd = 'c';
  while (true) {
    cool_mutex.lock();
    queue_size = playqueue.size();
    cool_mutex.unlock();

    if (queue_size > 0){
      cool_mutex.lock();
      cmd = player_cmd;
      cool_mutex.unlock();
      if (cmd == 's') {
        music.stop();
        if(s_counter > 0)
          s_counter--;
        cool_mutex.lock();
        player_cmd = 'z';
        cool_mutex.unlock();
      } else if (cmd == 'n') {
        cool_mutex.lock();
        if(playqueue[s_counter - 1].first != "adver"){
          music.stop();
          if ((s_counter > queue_size - 1 and queue_size > 0))
            s_counter = queue_size - 1;

          if (playqueue[s_counter].second == "*DEL*" and s_counter > queue_size - 1)
            s_counter--;
        }
        player_cmd = 'c';
        cool_mutex.unlock();
      } else if (cmd == 'p') {
        music.stop();
        s_counter = s_counter - 2;
        if (s_counter < 0)
          s_counter = 0;
        cool_mutex.lock();
        while (playqueue[s_counter].second == "*DEL*" and s_counter > 0){
          s_counter--;
        }
        player_cmd = 'c';
        cool_mutex.unlock();
      }

      if (s_counter < queue_size and cmd == 'c') {
        cool_mutex.lock();
        outname = playqueue[s_counter].second;
        song_name = playqueue[s_counter].first;
        player_status = music.getStatus();
        cool_mutex.unlock();

        if (player_status == 0 ) {
          if (outname == "*DEL*" ) {
            s_counter ++;
          } else if (music.openFromFile("tmp/" + outname)) {
            cout << "------------------------" << endl;
            cout << "Now playing: " << song_name << endl;
            cout << "------------------------" << endl;
            music.play();
            s_counter++;
          }
        }
      }
    }
  }
}

void delete_song(string song_name){
  if (song_name == "adver"){
    cout << "Very clever my dear Watson."<< endl;
    return;
  }

  for (unsigned int i = 0; i < playlist.size(); i++) {
    if (playlist[i] == song_name) {
      playlist.erase(playlist.begin() + i);
      return;
    }
  }

  for (unsigned int i = 0; i < playqueue.size(); i++) {
    if (playqueue[i].first == song_name) {
      string command = "exec rm -f tmp/" + playqueue[i].second;
      system(command.c_str());
      playqueue[i].second = "*DEL*";
    }
  }
  return;

}

int main(int argc, char **argv) {

  string broker_endpoint = "tcp://localhost:6667";
  string server_endpoint = "tcp://localhost:6666";
  string dload_endpoint = "tcp://localhost:6666";

  for (int i = 1; i < argc; ++i) {
    string cur(argv[i]);
    if (cur == "-br") {
      broker_endpoint = argv[i + 1];
      i++;
    }
  }


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
  //sf::Music music;
  int adv_counter = 0;
  while (command != "exit") {
    cout << "<<<<<<<<<<<<<<<<    >>>>>>>>>>>>>>>>" << endl;
    cout << "Welcome to Zen Music Quest!" << endl << endl;
    cout << "Type add and the name of a song to add it to your playlist or:" << endl;
    cout << "Type del and the name of a song to delete it from your playlist" << endl;
    cout << "Type next to skip a song and prev to hear the previous one" << endl;
    cout << "Type stop to stop playing music, or play to start playing" << endl;
    cin >> command;

    if (command == "add") {
      adv_counter++;
      cin >> song_name;
      cool_mutex.lock();
      playlist.push_back(song_name);
      //cout << playlist.front()<< endl;
      if(adv_counter > 3){
        playlist.push_back("adver");
        adv_counter = 0;
      }
      cool_mutex.unlock();
    }

    if (command == "next") {
      cool_mutex.lock();
      player_cmd = 'n';
      cool_mutex.unlock();
    }

    if (command == "stop") {
      cool_mutex.lock();
      player_cmd = 's';
      cool_mutex.unlock();
    }

    if (command == "prev") {
      cool_mutex.lock();
      player_cmd = 'p';
      cool_mutex.unlock();
    }

    if (command == "play") {
      cool_mutex.lock();
      player_cmd = 'c';
      cool_mutex.unlock();
    }

    if (command == "del") {
      cin >> song_name;
      cool_mutex.lock();
      delete_song(song_name);
      cool_mutex.unlock();
    }
    getchar();
  }

  system("exec rm -rf tmp");
  downloads.~thread();
  playing.~thread();
  return 0;
}
