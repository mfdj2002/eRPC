#include <stdio.h>
#include "rpc.h"

static const std::string kServer1Hostname = "node0.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";
static const std::string kServer2Hostname = "node1.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";
// static const std::string kClient1Hostname = "node2.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";
static const std::string kClientHostname = "node3.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";

static constexpr uint16_t kUDPPort = 31850;
static constexpr uint8_t kReq1Type = 1;
static constexpr uint8_t kReq2Type = 2;
static constexpr size_t kMsgSize = 32;

struct BasicContext {
    erpc::Rpc<erpc::CTransport> *rpc;
    std::vector<erpc::MsgBuffer> reqs;
    std::vector<erpc::MsgBuffer> resps;
};