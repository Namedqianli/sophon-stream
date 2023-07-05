#include "http_interact_mgr.h"

int main(int argc, const char** argv) {
  int port = 12345;

  if (argc >= 2) {
    port = atoi(argv[1]);
  }

  HTTP_Interact_Mgr* mgr = HTTP_Interact_Mgr::GetInstance();

  mgr->init(port);
  std::cout << "listen on: " << mgr->port_ << "\n";

  while (1) {
    sleep(10);
  }
  return 0;
}
