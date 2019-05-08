#include "sope.h"

tlv_reply_t newReply();
tlv_reply_t makeErrorReply(int retCode, tlv_request_t request);
tlv_reply_t makeShutdownReply(int accountId, int activeOffices);
tlv_reply_t makeBalanceReply(int accountId, int balance);
tlv_reply_t makeTransferReply(int accountId, int balance);
tlv_reply_t makeCreateReply(int accountId);
