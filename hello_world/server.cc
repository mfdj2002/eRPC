#include "common.h"
#include <iostream>
erpc::Rpc<erpc::CTransport> *rpc;
erpc::MsgBuffer resp1;
erpc::MsgBuffer req2;
erpc::MsgBuffer resp2;


void cont_func(void *, void *) { 
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "cont func Thread ID: " << tid << std::endl;
  printf("%s\n", resp2.buf_);
  fflush(stdout);
}

void req_handler(erpc::ReqHandle *req_handle, void *) {
  int session_num = rpc->create_session(kClient2Hostname + ":" + std::to_string(kUDPPort), 0);

  while (!rpc->is_connected(session_num)) rpc->run_event_loop_once();

  req2 = rpc->alloc_msg_buffer_or_die(kMsgSize);
  resp2 = rpc->alloc_msg_buffer_or_die(kMsgSize);

  rpc->enqueue_request(session_num, kReq2Type, &req2, &resp2, cont_func, nullptr);
  rpc->run_event_loop(100);

  printf("now sending response to client 1\n");
  fflush(stdout);
  
  auto &resp1 = req_handle->pre_resp_msgbuf_;
  rpc->resize_msg_buffer(&resp1, kMsgSize);
  sprintf(reinterpret_cast<char *>(resp1.buf_), "hello_from_server");
  rpc->enqueue_response(req_handle, &resp1);
}


void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

int main() {
  std::string server_uri = kServerHostname + ":" + std::to_string(kUDPPort);
  erpc::Nexus nexus(server_uri);
  nexus.register_req_func(kReq1Type, req_handler);

  rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, sm_handler);
  rpc->run_event_loop(100000);
}
