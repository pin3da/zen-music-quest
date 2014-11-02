#include <bits/stdc++.h>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;

vector<string> av_servers;

int main() {
  const string srv_endpoint = "tcp://*:6668",
               cl_endpoint  = "tcp://*:6667";

  context ctx;
  socket servers(ctx, socket_type::rep);
  servers.bind(srv_endpoint);

  socket clients(ctx, socket_type::rep);
  clients.bind(cl_endpoint);

  poller pol;
  pol.add(servers);
  pol.add(clients);

  unsigned int n = 0;

  message incmsg, outmsg;
  while(true){
    if (pol.poll()) {
      if (pol.has_input(servers)) {
        string srv_address;
        servers.receive(incmsg);
        incmsg >> srv_address;
        av_servers.push_back(srv_address);
        outmsg << "OK";
        servers.send(outmsg);
        cout << "Added Server! " << av_servers.back() << endl;
      }
      if (pol.has_input(clients)) {
        string hello;
        clients.receive(incmsg);
        incmsg >> hello;
        if (av_servers.size() > 0) {
          outmsg << "hello" << av_servers[n];
          n = (n + 1) % av_servers.size();
        } else {
          outmsg << "bye";
        }
        clients.send(outmsg);
      }
    }
  }

  return 0;
}
