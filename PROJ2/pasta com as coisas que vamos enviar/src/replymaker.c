#include "replymaker.h"

tlv_reply_t newReply() {
    tlv_reply_t reply;
    reply.type = -1;
    reply.value.header.account_id = -1;
    reply.value.header.ret_code = OTHER;
    reply.value.balance.balance = 0;
    reply.value.transfer.balance = 0;
    reply.value.shutdown.active_offices = 0;
    reply.length = -1;
    return reply;
}

tlv_reply_t makeErrorReply(int retCode, tlv_request_t request) {
    tlv_reply_t reply = newReply();
    reply.type = request.type;
    reply.value.header.account_id = request.value.header.account_id;
    reply.value.header.ret_code = retCode;
    reply.length = sizeof(reply.value.header);
    return reply;
}

tlv_reply_t makeShutdownReply(int accountId, int activeOffices) {
    tlv_reply_t reply;
    reply.type = OP_SHUTDOWN;
    reply.value.header.account_id = accountId;
    reply.value.header.ret_code = OK;
    reply.value.shutdown.active_offices = activeOffices;
    reply.length = sizeof(reply.value.header) + sizeof(rep_shutdown_t);
    return reply;
}

tlv_reply_t makeBalanceReply(int accountId, int balance) {
    tlv_reply_t reply;

    reply.type = OP_BALANCE;
    reply.value.header.ret_code = OK;
    reply.value.header.account_id = accountId;
    reply.value.balance.balance = balance;
    reply.length = sizeof(reply.value.header) + sizeof(rep_balance_t);

    return reply;
}

tlv_reply_t makeTransferReply(int accountId, int balance, int retCode) {
    tlv_reply_t reply;

    reply.type = OP_TRANSFER;
    reply.value.header.ret_code = retCode;
    reply.value.header.account_id = accountId;
    reply.value.transfer.balance = balance;
    reply.length = sizeof(reply.value.header) + sizeof(rep_transfer_t);

    return reply;
}

tlv_reply_t makeCreateReply(int accountId) {
    tlv_reply_t reply;

    reply.type = OP_CREATE_ACCOUNT;
    reply.value.header.ret_code = OK;
    reply.value.header.account_id = accountId;
    reply.length = sizeof(reply.value.header);

    return reply;
}
