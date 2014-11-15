#include <string>
#include <uuid/uuid.h>

namespace zen {
  namespace ports {
    int server_server = 4444; // Beginning of interval
    int server_client = 5555; // Beginning of interval
    int boker_server  = 6668;
    int broker_client = 6667;
  }
}

std::string gen_uuid() {
  uuid_t uuid;
  uuid_generate(uuid);
  char tmp[50];
  uuid_unparse(uuid, tmp);
  return std::string(tmp);
}
