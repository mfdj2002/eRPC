#ifndef BATCH_WRITE_H
#define BATCH_WRITE_H

#include <gflags/gflags.h>
#include <signal.h>
#include "../apps_common.h"
#include "rpc.h"
#include "util/latency.h"
#include "util/numautils.h"
#include "util/timer.h"
#include <condition_variable>

static constexpr size_t kAppBatchWriteReqType = 1;
static constexpr uint8_t kAppDataByte = 3;  // Data transferred in req & resp
static constexpr size_t kAppEvLoopMs = 500;
static constexpr size_t kAppMaxConcurrency = 2;  // Outstanding reqs per thread

// Globals
volatile sig_atomic_t ctrl_c_pressed = 0;
void ctrl_c_handler(int) { ctrl_c_pressed = 1; }

// Flags
DEFINE_uint64(num_server_fg_threads, 0, "Number of server foreground threads");
DEFINE_uint64(num_server_bg_threads, 0, "Number of server background threads");
DEFINE_uint64(num_client_threads, 0, "Number of client threads");
DEFINE_uint64(req_size, 0, "Request data size");
DEFINE_uint64(resp_size, 0, "Response data size");
DEFINE_uint64(concurrency, 0, "Concurrent requests per thread");
DEFINE_double(drop_prob, 0, "Packet drop probability");
DEFINE_string(profile, "", "Experiment profile to use");
DEFINE_double(throttle, 0, "Throttle flows to incast receiver?");
DEFINE_double(throttle_fraction, 1, "Fraction of fair share to throttle to.");

// Return true iff this machine is the one server
bool is_server() { return FLAGS_process_id == 0; }

struct app_stats_t {
  double rx_gbps;
  double tx_gbps;
  size_t re_tx;
  double rtt_50_us;  // Median packet RTT
  double rtt_99_us;  // 99th percentile packet RTT
  double rpc_50_us;
  double rpc_99_us;
  double rpc_999_us;

  app_stats_t() { memset(this, 0, sizeof(app_stats_t)); }

  static std::string get_template_str() {
    return "rx_gbps tx_gbps re_tx rtt_50_us rtt_99_us rpc_50_us rpc_99_us "
           "rpc_999_us";
  }

  /// Return a space-separated string of all stats
  std::string to_string() {
    return std::to_string(rx_gbps) + " " + std::to_string(tx_gbps) + " " +
           std::to_string(re_tx) + " " + std::to_string(rtt_50_us) + " " +
           std::to_string(rtt_99_us) + " " + std::to_string(rpc_50_us) + " " +
           std::to_string(rpc_99_us) + " " + std::to_string(rpc_999_us);
  }

  /// Accumulate stats
  app_stats_t& operator+=(const app_stats_t& rhs) {
    this->rx_gbps += rhs.rx_gbps;
    this->tx_gbps += rhs.tx_gbps;
    this->re_tx += rhs.re_tx;
    this->rtt_50_us += rhs.rtt_50_us;
    this->rtt_99_us += rhs.rtt_99_us;
    this->rpc_50_us += rhs.rpc_50_us;
    this->rpc_99_us += rhs.rpc_99_us;
    this->rpc_999_us += rhs.rpc_999_us;
    return *this;
  }
};
static_assert(sizeof(app_stats_t) == 64, "");

// Per-thread application context
class AppContext : public BasicAppContext {
 public:
  // We need a wide range of latency measurements: ~4 us for 4KB RPCs, to
  // >10 ms for 8MB RPCs under congestion. So erpc::Latency doesn't work here.
  std::vector<double> lat_vec;

  erpc::ChronoTimer tput_t0;  // Start time for throughput measurement
  app_stats_t* app_stats;     // Common stats array for all threads

  size_t stat_rx_bytes_tot = 0;  // Total bytes received
  size_t stat_tx_bytes_tot = 0;  // Total bytes transmitted

  uint64_t req_ts[kAppMaxConcurrency];  // Per-request timestamps
  erpc::MsgBuffer req_msgbuf[kAppMaxConcurrency];
  erpc::MsgBuffer resp_msgbuf[kAppMaxConcurrency];

  std::mutex conn_mutex_;
  std::condition_variable conn_cv_;
  bool connection_ready_ = false;

  // Helper method to wait for connection
  void wait_for_connection() {
      std::unique_lock<std::mutex> lock(conn_mutex_);
      conn_cv_.wait(lock, [this] { return connection_ready_; });
  }
};

// Allocate request and response MsgBuffers
void alloc_req_resp_msg_buffers(AppContext* c) {
  for (size_t i = 0; i < FLAGS_concurrency; i++) {
    c->req_msgbuf[i] = c->rpc_->alloc_msg_buffer_or_die(FLAGS_req_size);
    c->resp_msgbuf[i] = c->rpc_->alloc_msg_buffer_or_die(FLAGS_resp_size);

    // Fill the request regardless of kAppMemset. This is a one-time thing.
    memset(c->req_msgbuf[i].buf_, kAppDataByte, FLAGS_req_size);
  }
}

#endif
