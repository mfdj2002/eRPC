#include "common.h"
#include <thread>
#include <iostream>
erpc::Rpc<erpc::CTransport> *rpc;
erpc::MsgBuffer req;
erpc::MsgBuffer resp;

void cont_func(void *, void *) { 
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "cont func Thread ID: " << tid << std::endl;
  printf("%s\n", resp.buf_); }

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

int main() {
  std::string client1_uri = kClient1Hostname + ":" + std::to_string(kUDPPort);
  erpc::Nexus nexus(client1_uri);
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "main Thread ID: " << tid << std::endl;

  rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, sm_handler);

  std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
  int session_num = rpc->create_session(server_uri, 0);

  while (!rpc->is_connected(session_num)) rpc->run_event_loop_once();

  req = rpc->alloc_msg_buffer_or_die(kMsgSize);
  resp = rpc->alloc_msg_buffer_or_die(kMsgSize);

  rpc->enqueue_request(session_num, kReq1Type, &req, &resp, cont_func, nullptr);
  rpc->run_event_loop(100);

  delete rpc;
}
