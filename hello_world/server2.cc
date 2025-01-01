#include "common.h"
#include <thread>
#include <iostream>
erpc::Rpc<erpc::CTransport> *rpc;
erpc::MsgBuffer resp1;
// erpc::MsgBuffer req2;
// erpc::MsgBuffer resp2;


// void cont_func(void *, void *) { 
//   std::thread::id tid = std::this_thread::get_id();
//   std::cout << "cont func Thread ID: " << tid << std::endl;
//   printf("%s\n", resp2.buf_);
//   fflush(stdout);
// }

void req_handler(erpc::ReqHandle *req_handle, void *) {
  printf("entering req_handler\n");
  fflush(stdout);
  auto &resp1 = req_handle->pre_resp_msgbuf_;
  rpc->resize_msg_buffer(&resp1, kMsgSize);
  sprintf(reinterpret_cast<char *>(resp1.buf_), "hello_from_server");
  rpc->enqueue_response(req_handle, &resp1);
  printf("exiting req_handler\n");
  fflush(stdout);
}


void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

int main() {
  std::string server_uri = kServer2Hostname + ":" + std::to_string(kUDPPort);
  erpc::Nexus nexus(server_uri);
  nexus.register_req_func(kReq1Type, req_handler);

  rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, sm_handler);

  // std::string client1_uri = kClient1Hostname + ":" + std::to_string(kUDPPort);
  // int session_num = rpc->create_session(client1_uri, 0);
  // printf("session_num: %d\n", session_num);
  // fflush(stdout);

  // while (!rpc->is_connected(session_num)) rpc->run_event_loop_once();

  // req2 = rpc->alloc_msg_buffer_or_die(kMsgSize);
  // resp2 = rpc->alloc_msg_buffer_or_die(kMsgSize);

  // rpc->enqueue_request(session_num, kReq2Type, &req2, &resp2, cont_func, nullptr);
  
  rpc->run_event_loop(10000);
}
