#include <stdio.h>
#include "rpc.h"

static const std::string kServerHostname = "node0.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";
static const std::string kClient1Hostname = "node1.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";
static const std::string kClient2Hostname = "node3.dfuse-erpc.hetkv-pg0.utah.cloudlab.us";

static constexpr uint16_t kUDPPort = 31850;
static constexpr uint8_t kReq1Type = 1;
static constexpr uint8_t kReq2Type = 2;
static constexpr size_t kMsgSize = 32;
