#include "common.h"
#include <thread>
#include <iostream>
#include <cstdint>
erpc::Rpc<erpc::CTransport> *rpc;
// erpc::MsgBuffer req1;
// erpc::MsgBuffer resp1;
// erpc::MsgBuffer resp2;

void cont_func(void* context, void* tag) { 
  BasicContext *ctx = static_cast<BasicContext *>(context);
  std::thread::id tid = std::this_thread::get_id();
  std::cout << "cont func Thread ID: " << tid << std::endl;
  printf("tag: %ld\n", reinterpret_cast<size_t>(tag));
  printf("%s\n", ctx->resps[reinterpret_cast<size_t>(tag)].buf_);
  fflush(stdout);
}

void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}


// void req_handler(erpc::ReqHandle *req_handle, void *) {
//   auto &resp2 = req_handle->pre_resp_msgbuf_;
//   rpc->resize_msg_buffer(&resp2, kMsgSize);
//   sprintf(reinterpret_cast<char *>(resp2.buf_), "hello_from_server");
//   rpc->enqueue_response(req_handle, &resp2);
// }

int main() {
  std::string client_uri = kClientHostname + ":" + std::to_string(kUDPPort);
  erpc::Nexus nexus(client_uri);
  // nexus.register_req_func(kReq2Type, req_handler);
  // std::thread::id tid = std::this_thread::get_id();
  // std::cout << "main Thread ID: " << tid << std::endl;

  BasicContext *ctx = new BasicContext();

  rpc = new erpc::Rpc<erpc::CTransport>(&nexus, ctx, 0, sm_handler);
  ctx->rpc = rpc;

  std::string server_uri1 = kServer1Hostname + ":" + std::to_string(kUDPPort);
  int session_num1 = rpc->create_session(server_uri1, 0);
  erpc::rt_assert(session_num1 >= 0, "session_num is negative");

  std::string server_uri2 = kServer2Hostname + ":" + std::to_string(kUDPPort);
  int session_num2 = rpc->create_session(server_uri2, 0);
  erpc::rt_assert(session_num2 >= 0, "session_num is negative");

  while (!rpc->is_connected(session_num1)) rpc->run_event_loop_once();


  ctx->reqs.push_back(rpc->alloc_msg_buffer_or_die(kMsgSize));
  ctx->resps.push_back(rpc->alloc_msg_buffer_or_die(kMsgSize));
  ctx->reqs.push_back(rpc->alloc_msg_buffer_or_die(kMsgSize));
  ctx->resps.push_back(rpc->alloc_msg_buffer_or_die(kMsgSize));

  rpc->enqueue_request(session_num1, kReq1Type, &ctx->reqs[0], &ctx->resps[0], cont_func, reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
  rpc->enqueue_request(session_num2, kReq1Type, &ctx->reqs[1], &ctx->resps[1], cont_func, reinterpret_cast<void*>(static_cast<uintptr_t>(1)));

  rpc->run_event_loop(10000);

  delete rpc;
  delete ctx;
}
