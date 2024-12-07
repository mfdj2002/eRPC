#include "common.h"
erpc::Rpc<erpc::CTransport> *rpc;

void req_handler(erpc::ReqHandle *req_handle, void *) {
  auto &resp = req_handle->pre_resp_msgbuf_;
  rpc->resize_msg_buffer(&resp, kMsgSize);
  sprintf(reinterpret_cast<char *>(resp.buf_), "hello_from_client2");
  rpc->enqueue_response(req_handle, &resp);
}

int main() {
  std::string client2_uri = kClient2Hostname + ":" + std::to_string(kUDPPort);
  erpc::Nexus nexus(client2_uri);
  nexus.register_req_func(kReq2Type, req_handler);

  rpc = new erpc::Rpc<erpc::CTransport>(&nexus, nullptr, 0, nullptr);
  rpc->run_event_loop(100000);
}
