#include "sope.h"

tlv_reply_t newReply();
tlv_reply_t makeErrorReply(int retCode, tlv_request_t request);
tlv_reply_t makeShutdownReply(int accountId, int activeOffices);
