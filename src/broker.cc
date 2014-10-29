#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;

unordered_map<int, string> servers;


int main() {
  const string srv_port = "tcp://*:5555";
  const string cl_port = "tcp://*:6666";
  context ctx;
  
  socket srv_end(ctx, socket_type::rep);
  srv_end.bind(srv_port);
  
  socket cl_end(ctx, socket_type::rep);
  cl_end.bind(cl_port);
  
  poller pol;
  pol.add(srv_end);
  pol.add(cl_end);
  
  unsigned int n = 0;
  
  while(true){
    if (pol.poll()){
      if(pol.has_input(srv_end)){
        message incmsg, outmsg;
        string srv_address;
        srv_end.receive(incmsg);
        incmsg >> srv_address;
        servers[servers.size()] = srv_address;
        outmsg << "OK";
        srv_end.send(outmsg);
        cout << "Added Server! " << servers[servers.size() -1] << endl;
      }
      if(pol.has_input(cl_end)){
        message incmsg, outmsg;
        string hello;
        cl_end.receive(incmsg);
        incmsg >> hello;
        if(servers.size() > 0){
          if (n > servers.size() - 1)
            n = 0;
          outmsg << "hello" << servers[n];
          n++;
        }
        else{
          outmsg << "bye"; 
        }
        cl_end.send(outmsg);
      }
    }
    
  } 
  
  return 0;
}
