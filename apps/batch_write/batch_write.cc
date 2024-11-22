#include "batch_write.h"
#include <signal.h>
#include <cstring>
#include "util/autorun_helpers.h"

static constexpr bool kAppVerbose = false;
static constexpr uint8_t kAppDataByte = 3;
static constexpr bool kAppClientCheckResp = false;   // Check entire response


void batch_write_req_handler(erpc::ReqHandle *req_handle, void *_context) {
  auto *c = static_cast<AppContext *>(_context);
  const erpc::MsgBuffer *req_msgbuf = req_handle->get_req_msgbuf();
  //demarshall..
  //do something given the info...
  //total_bytes_written = ...
  uint8_t resp_byte = req_msgbuf->buf_[0];

  // Use dynamic response
  erpc::MsgBuffer &resp_msgbuf = req_handle->dyn_resp_msgbuf_;
  resp_msgbuf = c->rpc_->alloc_msg_buffer_or_die(FLAGS_resp_size);

  // Touch the response
  // placeholder for the actual response, which depends on total_bytes_written
  resp_msgbuf.buf_[0] = resp_byte;

  c->stat_rx_bytes_tot += FLAGS_req_size;
  c->stat_tx_bytes_tot += FLAGS_resp_size;

  c->rpc_->enqueue_response(req_handle, &resp_msgbuf);
}

// Send one request using this MsgBuffer
void send_req(AppContext *c, size_t msgbuf_idx) {
  erpc::MsgBuffer &req_msgbuf = c->req_msgbuf[msgbuf_idx];
  assert(req_msgbuf.get_data_size() == FLAGS_req_size);

  if (kAppVerbose) {
    printf("large_rpc_tput: Thread %zu sending request using msgbuf_idx %zu.\n",
           c->thread_id_, msgbuf_idx);
  }

  c->req_ts[msgbuf_idx] = erpc::rdtsc();
  c->rpc_->enqueue_request(c->session_num_vec_[0], kAppBatchWriteReqType, &req_msgbuf,
                           &c->resp_msgbuf[msgbuf_idx], app_cont_func,
                           reinterpret_cast<void *>(msgbuf_idx));

  c->stat_tx_bytes_tot += FLAGS_req_size;
}


void app_cont_func(void *_context, void *_tag) {
  auto *c = static_cast<AppContext *>(_context);
  auto msgbuf_idx = reinterpret_cast<size_t>(_tag);

  const erpc::MsgBuffer &resp_msgbuf = c->resp_msgbuf[msgbuf_idx];
  if (kAppVerbose) {
    printf("large_rpc_tput: Received response for msgbuf %zu.\n", msgbuf_idx);
  }

  // Measure latency. 1 us granularity is sufficient for large RPC latency.
  double usec = erpc::to_usec(erpc::rdtsc() - c->req_ts[msgbuf_idx],
                              c->rpc_->get_freq_ghz());
  c->lat_vec.push_back(usec);

  // Check the response
  erpc::rt_assert(resp_msgbuf.get_data_size() == FLAGS_resp_size,
                  "Invalid response size");

  if (kAppClientCheckResp) {
    bool match = true;
    // Check all response cachelines (checking every byte is slow)
    for (size_t i = 0; i < FLAGS_resp_size; i += 64) {
      if (resp_msgbuf.buf_[i] != kAppDataByte) match = false;
    }
    erpc::rt_assert(match, "Invalid resp data");
  } else {
    erpc::rt_assert(resp_msgbuf.buf_[0] == kAppDataByte, "Invalid resp data");
  }

  c->stat_rx_bytes_tot += FLAGS_resp_size;

  // Create a new request clocking this response, and put in request queue

  //placeholder for response?
  c->req_msgbuf[msgbuf_idx].buf_[0] = kAppDataByte;

  send_req(c, msgbuf_idx);
}

void client_print_stats(AppContext &c) {
  const double ns = c.tput_t0.get_ns();
  erpc::Timely *timely_0 = c.rpc_->get_timely(0);

  // Publish stats
  auto &stats = c.app_stats[c.thread_id_];
  stats.rx_gbps = c.stat_rx_bytes_tot * 8 / ns;
  stats.tx_gbps = c.stat_tx_bytes_tot * 8 / ns;
  stats.re_tx = c.rpc_->get_num_re_tx(c.session_num_vec_[0]);
  stats.rtt_50_us = timely_0->get_rtt_perc(0.50);
  stats.rtt_99_us = timely_0->get_rtt_perc(0.99);

  if (c.lat_vec.size() > 0) {
    std::sort(c.lat_vec.begin(), c.lat_vec.end());
    stats.rpc_50_us = c.lat_vec[c.lat_vec.size() * 0.50];
    stats.rpc_99_us = c.lat_vec[c.lat_vec.size() * 0.99];
    stats.rpc_999_us = c.lat_vec[c.lat_vec.size() * 0.999];
  } else {
    // Even if no RPCs completed, we need retransmission counter
    stats.rpc_50_us = kAppEvLoopMs * 1000;
    stats.rpc_99_us = kAppEvLoopMs * 1000;
    stats.rpc_999_us = kAppEvLoopMs * 1000;
  }

  printf(
      "large_rpc_tput: Thread %zu: Tput {RX %.2f (%zu), TX %.2f (%zu)} "
      "Gbps (IOPS). Retransmissions %zu. Packet RTTs: {%.1f, %.1f} us. "
      "RPC latency {%.1f 50th, %.1f 99th, %.1f 99.9th}. Timely rate %.1f "
      "Gbps. Credits %zu (best = 32).\n",
      c.thread_id_, stats.rx_gbps, c.stat_rx_bytes_tot / FLAGS_resp_size,
      stats.tx_gbps, c.stat_tx_bytes_tot / FLAGS_req_size, stats.re_tx,
      stats.rtt_50_us, stats.rtt_99_us, stats.rpc_50_us, stats.rpc_99_us,
      stats.rpc_999_us, timely_0->get_rate_gbps(), erpc::kSessionCredits);

  // Reset stats for next iteration
  c.stat_rx_bytes_tot = 0;
  c.stat_tx_bytes_tot = 0;
  c.rpc_->reset_num_re_tx(c.session_num_vec_[0]);
  c.lat_vec.clear();
  timely_0->reset_rtt_stats();

  if (c.thread_id_ == 0) {
    app_stats_t accum_stats;
    for (size_t i = 0; i < FLAGS_num_client_threads; i++) {
      accum_stats += c.app_stats[i];
    }

    // Compute averages for non-additive stats
    accum_stats.rtt_50_us /= FLAGS_num_client_threads;
    accum_stats.rtt_99_us /= FLAGS_num_client_threads;
    accum_stats.rpc_50_us /= FLAGS_num_client_threads;
    accum_stats.rpc_99_us /= FLAGS_num_client_threads;
    accum_stats.rpc_999_us /= FLAGS_num_client_threads;
    c.tmp_stat_->write(accum_stats.to_string());
  }

  c.tput_t0.reset();
}

void client_thread_func(size_t thread_id, app_stats_t *app_stats,
                        erpc::Nexus *nexus) {
  AppContext c;
  c.thread_id_ = thread_id;
  c.app_stats = app_stats;

  if (thread_id == 0) {
    c.tmp_stat_ = new TmpStat(app_stats_t::get_template_str());
  }

  std::vector<size_t> port_vec = flags_get_numa_ports(FLAGS_numa_node);
  erpc::rt_assert(port_vec.size() > 0);
  const uint8_t phy_port = port_vec.at(thread_id % port_vec.size());

  erpc::Rpc<erpc::CTransport> rpc(nexus, static_cast<void *>(&c),
                                  static_cast<uint8_t>(thread_id),
                                  basic_sm_handler, phy_port);
  rpc.retry_connect_on_invalid_rpc_id_ = true;
  c.rpc_ = &rpc;

  // Each client creates a session to only one server thread
  const size_t client_gid =
      (FLAGS_process_id * FLAGS_num_client_threads) + thread_id;
  const size_t server_tid =
      client_gid % FLAGS_num_server_fg_threads;  // eRPC TID

  c.session_num_vec_.resize(1);
  c.session_num_vec_[0] =
      rpc.create_session(erpc::get_uri_for_process(0), server_tid);
  assert(c.session_num_vec_[0] >= 0);

  while (c.num_sm_resps_ != 1) {
    rpc.run_event_loop(200);  // 200 milliseconds
    if (ctrl_c_pressed == 1) return;
  }
  assert(c.rpc_->is_connected(c.session_num_vec_[0]));
  fprintf(stderr, "main: Thread %zu: Connected. Sending requests.\n",
          thread_id);

  alloc_req_resp_msg_buffers(&c);
  c.tput_t0.reset();
    // Any thread that creates a session sends requests
  if (c.session_num_vec_.size() > 0) {
    for (size_t msgbuf_idx = 0; msgbuf_idx < FLAGS_concurrency; msgbuf_idx++) {
      send_req(&c, msgbuf_idx);
    }
  }

  for (size_t i = 0; i < FLAGS_test_ms; i += kAppEvLoopMs) {
    c.rpc_->run_event_loop(kAppEvLoopMs);
    if (ctrl_c_pressed == 1) break;
    client_print_stats(c);
  }
}

void server_thread_func(size_t thread_id, erpc::Nexus *nexus) {
  AppContext c;
  c.thread_id_ = thread_id;

  std::vector<size_t> port_vec = flags_get_numa_ports(FLAGS_numa_node);
  erpc::rt_assert(port_vec.size() > 0);
  uint8_t phy_port = port_vec.at(thread_id % port_vec.size());

  erpc::Rpc<erpc::CTransport> rpc(nexus, static_cast<void *>(&c),
                                  static_cast<uint8_t>(thread_id),
                                  basic_sm_handler, phy_port);
  c.rpc_ = &rpc;
  while (ctrl_c_pressed == 0) rpc.run_event_loop(200);
}

int main(int argc, char **argv) {
  signal(SIGINT, ctrl_c_handler);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (is_server()) {
    erpc::rt_assert(FLAGS_process_id == 0, "Invalid server process ID");

    // Create a thread_info for every core, we'll use only some of them for
    // the actual benchmark

    // eRPC stuff
    erpc::Nexus nexus(erpc::get_uri_for_process(FLAGS_process_id),
                      FLAGS_numa_node, FLAGS_num_server_bg_threads);

    nexus.register_req_func(kAppBatchWriteReqType, batch_write_req_handler,
                            erpc::ReqFuncType::kForeground);

    std::vector<std::thread> thread_arr(FLAGS_num_server_fg_threads);
    for (size_t i = 0; i < FLAGS_num_server_fg_threads; i++) {
      thread_arr[i] = std::thread(server_thread_func, i, &nexus);
      erpc::bind_to_core(thread_arr[i], FLAGS_numa_node, i);
    }

    for (auto &thread : thread_arr) thread.join();
  } else {
    erpc::rt_assert(FLAGS_process_id > 0, "Invalid process ID");
    erpc::Nexus nexus(erpc::get_uri_for_process(FLAGS_process_id),
                      FLAGS_numa_node, FLAGS_num_server_bg_threads);

    std::vector<std::thread> thread_arr(FLAGS_num_client_threads);
    auto *app_stats = new app_stats_t[FLAGS_num_client_threads];
    for (size_t i = 0; i < FLAGS_num_client_threads; i++) {
      thread_arr[i] = std::thread(client_thread_func, i, app_stats, &nexus);
      erpc::bind_to_core(thread_arr[i], FLAGS_numa_node, i);
    }

    for (auto &thread : thread_arr) thread.join();
    delete[] app_stats;
  }
}
