#include <stdio.h>
#include "rpc.h"

static const std::string kServerHostname = "node0.jf3516-231932.hetkv-pg0.utah.cloudlab.us";
static const std::string kClientHostname = "node1.jf3516-231932.hetkv-pg0.utah.cloudlab.us";

static constexpr uint16_t kUDPPort = 31850;
static constexpr uint8_t kReqType = 2;
static constexpr size_t kMsgSize = 16;
