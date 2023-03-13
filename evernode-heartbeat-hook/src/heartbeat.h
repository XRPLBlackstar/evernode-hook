#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"
#include "../../headers/transactions.h"

#define OP_HEARTBEAT 1

// IOU Payment with single memo (Reward).
uint8_t REWARD_PAYMENT[333] = {
    0x12, 0x00, 0x00,                   // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,       // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,       // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,       // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,       // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00, // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00, // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)><currency_code(20)><issuer(20)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 84
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 95)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 130
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 152
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x0D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoType (13 bytes) offset 176
    0x7D, 0x00,
    0x7E, 0x00,
    0xE1, 0xF1, // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 191
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define REWARD_PAYMENT_TX_SIZE \
    sizeof(REWARD_PAYMENT)
#define PREPARE_REWARD_PAYMENT_TX(evr_amount, to_address)                    \
    {                                                                        \
        uint8_t *buf_out = REWARD_PAYMENT;                                   \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                   \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                   \
        SET_AMOUNT_OUT((buf_out + 35), EVR_TOKEN, issuer_accid, evr_amount); \
        COPY_20BYTES((buf_out + 130), hook_accid);                           \
        COPY_20BYTES((buf_out + 152), to_address);                           \
        COPY_8BYTES((buf_out + 176), HOST_REWARD);                           \
        COPY_4BYTES((buf_out + 176 + 8), (HOST_REWARD + 8));                 \
        COPY_BYTE((buf_out + 176 + 12), (HOST_REWARD + 12));                 \
        etxn_details((buf_out + 195), REWARD_PAYMENT_TX_SIZE);               \
        int64_t fee = etxn_fee_base(buf_out, REWARD_PAYMENT_TX_SIZE);        \
        uint8_t *fee_ptr = buf_out + 84;                                     \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                        \
    }

// Simple XRP Payment with single memo.
uint8_t CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX[340] = {
    0x12, 0x00, 0x00,                                     // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,                         // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,                         // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,                         // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 44
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 55)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 90
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 112
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, // MemoType (24 bytes) offset 136
    0x7D, 0x21,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (33 bytes) offset 162
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 197
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 202
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX_SIZE \
    sizeof(CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX)
#define PREPARE_CANDIDATE_STATUS_CHANGE_MIN_PAYMENT(drops_amount, to_address, memo_data)   \
    {                                                                                      \
        uint8_t *buf_out = CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX;                         \
        uint32_t cls = (uint32_t)ledger_seq();                                             \
        UINT32_TO_BUF((buf_out + 25), cls + 1);                                            \
        UINT32_TO_BUF((buf_out + 31), cls + 5);                                            \
        uint8_t *fee_ptr = (buf_out + 35);                                                 \
        _06_01_ENCODE_DROPS_AMOUNT(fee_ptr, drops_amount);                                 \
        COPY_20BYTES((buf_out + 90), hook_accid);                                          \
        COPY_20BYTES((buf_out + 112), to_address);                                         \
        COPY_16BYTES((buf_out + 136), CANDIDATE_STATUS_CHANGE);                            \
        COPY_8BYTES((buf_out + 136 + 16), (CANDIDATE_STATUS_CHANGE + 16));                 \
        COPY_32BYTES((buf_out + 162), memo_data);                                          \
        COPY_BYTE((buf_out + 162 + 32), (memo_data + 32));                                 \
        COPY_2BYTES((buf_out + 197), FORMAT_HEX);                                          \
        COPY_BYTE((buf_out + 197 + 2), (FORMAT_HEX + 2));                                  \
        etxn_details((buf_out + 202), CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX_SIZE);        \
        int64_t fee = etxn_fee_base(buf_out, CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX_SIZE); \
        fee_ptr = buf_out + 44;                                                            \
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                             \
    }

#define EQUAL_HEARTBEAT(buf, len)      \
    (sizeof(HEARTBEAT) == (len + 1) && \
     BUFFER_EQUAL_8(buf, HEARTBEAT) && \
     BUFFER_EQUAL_4((buf + 8), (HEARTBEAT + 8)))

const uint8_t PARAM_STATE_HOOK[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

#define FOREIGN_REF SBUF(NAMESPACE), state_hook_accid, ACCOUNT_ID_SIZE