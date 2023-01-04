#include "../lib/hookapi.h"
// #include "../lib/emulatorapi.h"
#include "evernode.h"
#include "statekeys.h"
#include "transactions.h"

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(uint32_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(uint32_t reserved)
{
    uint8_t meta_params[META_PARAMS_SIZE];
    if (hook_param(SBUF(meta_params), SBUF(META_PARAMS)) < 0 || meta_params[CHAIN_IDX_PARAM_OFFSET] != 1)
        rollback(SBUF("Evernode: Invalid meta params for chain one."), 1);

    uint8_t op_type = meta_params[OP_TYPE_PARAM_OFFSET];
    uint8_t *seq_param_ptr = &meta_params[CUR_LEDGER_SEQ_PARAM_OFFSET];
    int64_t cur_ledger_seq = INT64_FROM_BUF(seq_param_ptr);
    uint8_t *ts_param_ptr = &meta_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET];
    int64_t cur_ledger_timestamp = INT64_FROM_BUF(ts_param_ptr);
    unsigned char hook_accid[ACCOUNT_ID_SIZE];
    COPY_20BYTES(hook_accid, (meta_params + HOOK_ACCID_PARAM_OFFSET));
    uint8_t account_field[ACCOUNT_ID_SIZE];
    COPY_20BYTES(account_field, (meta_params + ACCOUNT_FIELD_PARAM_OFFSET));
    uint8_t *issuer_accid = &meta_params[ISSUER_PARAM_OFFSET];
    uint8_t *foundation_accid = &meta_params[FOUNDATION_PARAM_OFFSET];

    // Memos can be empty for some transactions.
    uint8_t memo_params[MEMO_PARAM_SIZE];
    const res = hook_param(SBUF(memo_params), SBUF(MEMO_PARAMS));
    if (res != DOESNT_EXIST && res < 0)
        rollback(SBUF("Evernode: Could not get memo params for chain one."), 1);

    uint8_t chain_one_params[CHAIN_ONE_PARAMS_SIZE];
    if (hook_param(SBUF(chain_one_params), SBUF(CHAIN_ONE_PARAMS)) < 0)
        rollback(SBUF("Evernode: Could not get params for chain one."), 1);
    uint8_t *amount_buffer = &chain_one_params[AMOUNT_BUF_PARAM_OFFSET];
    uint8_t *float_amt_ptr = &chain_one_params[FLOAT_AMT_PARAM_OFFSET];
    int64_t float_amt = INT64_FROM_BUF(float_amt_ptr);
    uint8_t *txid = &chain_one_params[TXID_PARAM_OFFSET];

    if (op_type == OP_HOST_REG)
    {
        int64_t amt_drops = float_int(float_amt, 6, 0);
        if (amt_drops < 0)
            rollback(SBUF("Evernode: Could not parse amount."), 1);
        int64_t amt_int = amt_drops / 1000000;

        // Currency should be EVR.
        if (!IS_EVR(amount_buffer, issuer_accid))
            rollback(SBUF("Evernode: Currency should be EVR for host registration."), 1);

        // Checking whether this host has an initiated transfer to continue.
        TRANSFEREE_ADDR_KEY(account_field);
        uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];
        int has_initiated_transfer = (state(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR)) > 0);
        // Check whether host was a transferer of the transfer. (same account continuation).
        int parties_are_similar = (has_initiated_transfer && BUFFER_EQUAL_20((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field));


        // Take the host reg fee from config.
        int64_t host_reg_fee;
        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

        int64_t comparison_status = (has_initiated_transfer == 0) ? float_compare(float_amt, float_set(0, host_reg_fee), COMPARE_LESS) : float_compare(float_amt, float_set(0, NOW_IN_EVRS), COMPARE_LESS);

        if (comparison_status == 1)
            rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

        // Checking whether this host is already registered.
        HOST_ADDR_KEY(account_field);

        // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
        // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_index(8)><version(3)><registration_timestamp(8)>
        uint8_t host_addr[HOST_ADDR_VAL_SIZE];

        if ((has_initiated_transfer == 0 || (has_initiated_transfer == 1 && !parties_are_similar)) && state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
            rollback(SBUF("Evernode: Host already registered."), 1);

        // <country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><no_of_total_instances(4)><cpu_model(40)><cpu_count(2)><cpu_speed(2)><description(26)><email_address(40)>
        // Populate values to the state address buffer and set state.
        // Clear reserve and description sections first.
        COPY_2BYTES((host_addr + HOST_COUNTRY_CODE_OFFSET), (memo_params + HOST_COUNTRY_CODE_MEMO_OFFSET));
        CLEAR_8BYTES((host_addr + HOST_RESERVED_OFFSET));
        COPY_DESCRIPTION((host_addr + HOST_DESCRIPTION_OFFSET), (memo_params + HOST_DESCRIPTION_MEMO_OFFSET));
        INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
        UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
        COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (memo_params + HOST_TOT_INS_COUNT_MEMO_OFFSET));
        UINT64_TO_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET], cur_ledger_timestamp);

        if (has_initiated_transfer == 0)
        {
            // Continuation of normal registration flow...

            // Generate the NFT token id.

            // Take the account token sequence from keylet.
            uint8_t keylet[34];
            if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) != 34)
                rollback(SBUF("Evernode: Could not generate the keylet for KEYLET_ACCOUNT."), 10);

            int64_t slot_no = slot_set(SBUF(keylet), 0);
            if (slot_no < 0)
                rollback(SBUF("Evernode: Could not set keylet in slot"), 10);

            int64_t token_seq_slot = slot_subfield(slot_no, sfMintedNFTokens, 0);
            uint32_t token_seq = 0;
            if (token_seq_slot >= 0)
            {
                uint8_t token_seq_buf[4];
                token_seq_slot = slot(SBUF(token_seq_buf), token_seq_slot);
                token_seq = UINT32_FROM_BUF(token_seq_buf);
            }
            else if (token_seq_slot != DOESNT_EXIST)
                rollback(SBUF("Evernode: Could not find sfMintedTokens on hook account"), 20);

            uint8_t nft_token_id[NFT_TOKEN_ID_SIZE];
            GENERATE_NFT_TOKEN_ID(nft_token_id, tfBurnable, 0, hook_accid, 0, token_seq);
            trace("NFT token id:", 13, SBUF(nft_token_id), 1);

            COPY_32BYTES(host_addr, nft_token_id);

            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

            // Populate the values to the token id buffer and set state.
            // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><email_address(40)>
            uint8_t token_id[TOKEN_ID_VAL_SIZE];
            COPY_20BYTES((token_id + HOST_ADDRESS_OFFSET), account_field);
            COPY_40BYTES((token_id + HOST_CPU_MODEL_NAME_OFFSET), (memo_params + HOST_CPU_MODEL_NAME_MEMO_OFFSET));
            COPY_2BYTES((token_id + HOST_CPU_COUNT_OFFSET), (memo_params + HOST_CPU_COUNT_MEMO_OFFSET));
            COPY_2BYTES((token_id + HOST_CPU_SPEED_OFFSET), (memo_params + HOST_CPU_SPEED_MEMO_OFFSET));
            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (memo_params + HOST_CPU_MICROSEC_MEMO_OFFSET));
            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (memo_params + HOST_RAM_MB_MEMO_OFFSET));
            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (memo_params + HOST_DISK_MB_MEMO_OFFSET));
            COPY_40BYTES((token_id + HOST_EMAIL_ADDRESS_OFFSET), (memo_params + HOST_EMAIL_ADDRESS_MEMO_OFFSET));
            TOKEN_ID_KEY(nft_token_id);

            if (state_set(SBUF(token_id), SBUF(STP_TOKEN_ID)) < 0)
                rollback(SBUF("Evernode: Could not set state for token_id."), 1);

            uint32_t host_count;
            GET_HOST_COUNT(host_count);
            host_count += 1;
            SET_HOST_COUNT(host_count);

            // Take the fixed reg fee from config.
            int64_t conf_fixed_reg_fee;
            GET_CONF_VALUE(conf_fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee state.");

            // Take the fixed theoretical maximum registrants value from config.
            uint64_t conf_max_reg;
            GET_CONF_VALUE(conf_max_reg, STK_MAX_REG, "Evernode: Could not get max reg fee state.");

            etxn_reserve(3);

            // Froward 5 EVRs to foundation.
            // Create the outgoing hosting token txn.
            PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, float_set(0, conf_fixed_reg_fee), foundation_accid);

            uint8_t emithash[32];
            if (emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) < 0)
                rollback(SBUF("Evernode: Emitting EVR forward txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // Mint the nft token.
            PREPARE_REG_NFT_MINT_TX(txid);

            if (emit(SBUF(emithash), SBUF(REG_NFT_MINT_TX)) < 0)
                rollback(SBUF("Evernode: Emitting NFT mint txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // Amount will be 0.
            PREPARE_NFT_SELL_OFFER_TX(0, account_field, nft_token_id);

            if (emit(SBUF(emithash), SBUF(NFT_OFFER)) < 0)
                rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // If maximum theoretical host count reached, halve the registration fee.
            if (host_reg_fee > conf_fixed_reg_fee && host_count >= (conf_max_reg / 2))
            {
                uint8_t state_buf[8] = {0};

                host_reg_fee /= 2;
                UINT64_TO_BUF(state_buf, host_reg_fee);
                if (state_set(SBUF(state_buf), SBUF(STK_HOST_REG_FEE)) < 0)
                    rollback(SBUF("Evernode: Could not update the state for host reg fee."), 1);

                conf_max_reg *= 2;
                UINT64_TO_BUF(state_buf, conf_max_reg);
                if (state_set(SBUF(state_buf), SBUF(STK_MAX_REG)) < 0)
                    rollback(SBUF("Evernode: Could not update state for max theoretical registrants."), 1);
            }

            accept(SBUF("Host registration successful."), 0);
        }
        else
        {
            // Continuation of re-registration flow (completion of an existing transfer)...

            uint8_t prev_host_addr[HOST_ADDR_VAL_SIZE];
            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));
            if (state(SBUF(prev_host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Previous host address state not found."), 1);

            uint8_t prev_token_id[TOKEN_ID_VAL_SIZE];
            TOKEN_ID_KEY((uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));
            if (state(SBUF(prev_token_id), SBUF(STP_TOKEN_ID)) < 0)
                rollback(SBUF("Evernode: Previous host token id state not found."), 1);

            // Use the previous NFToken id for this re-reg flow.
            COPY_32BYTES(host_addr, (prev_host_addr + HOST_TOKEN_ID_OFFSET));

            // Copy some of the previous host state figures to the new HOST_ADDR state.
            const uint8_t *heartbeat_ptr = &prev_host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
            INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], INT64_FROM_BUF(heartbeat_ptr));
            UINT64_TO_BUF(&host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET], UINT64_FROM_BUF(prev_host_addr + HOST_HEARTBEAT_LEDGER_IDX_OFFSET));
            UINT64_TO_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET], UINT64_FROM_BUF(prev_host_addr + HOST_REG_TIMESTAMP_OFFSET));

            // Set the STP_HOST_ADDR with corresponding new state's key.
            HOST_ADDR_KEY(account_field);

            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

            // Update previous TOKEN_ID state entry with the new attributes.
            COPY_20BYTES((prev_token_id + HOST_ADDRESS_OFFSET), account_field);
            COPY_40BYTES((prev_token_id + HOST_CPU_MODEL_NAME_OFFSET), (memo_params + HOST_CPU_MODEL_NAME_MEMO_OFFSET));
            COPY_2BYTES((prev_token_id + HOST_CPU_COUNT_OFFSET), (memo_params + HOST_CPU_COUNT_MEMO_OFFSET));
            COPY_2BYTES((prev_token_id + HOST_CPU_SPEED_OFFSET), (memo_params + HOST_CPU_SPEED_MEMO_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_CPU_MICROSEC_OFFSET), (memo_params + HOST_CPU_MICROSEC_MEMO_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_RAM_MB_OFFSET), (memo_params + HOST_RAM_MB_MEMO_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_DISK_MB_OFFSET), (memo_params + HOST_DISK_MB_MEMO_OFFSET));
            COPY_40BYTES((prev_token_id + HOST_EMAIL_ADDRESS_OFFSET), (memo_params + HOST_EMAIL_ADDRESS_MEMO_OFFSET));

            if (state_set(SBUF(prev_token_id), SBUF(STP_TOKEN_ID)) < 0)
                rollback(SBUF("Evernode: Could not set state for token_id."), 1);

            etxn_reserve(1);
            // Amount will be 0.
            // Create a sell offer for the transferring NFT.
            PREPARE_NFT_SELL_OFFER_TX(0, account_field, (uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));

            uint8_t emithash[32];
            if (emit(SBUF(emithash), SBUF(NFT_OFFER)) < 0)
                rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // Set the STP_HOST_ADDR correctly of the deleting state.
            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));

            // Delete previous HOST_ADDR state and the relevant TRANSFEREE_ADDR state entries accordingly.

            if (!parties_are_similar && (state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0))
                rollback(SBUF("Evernode: Could not delete the previous host state entry."), 1);

            if (state_set(0, 0, SBUF(STP_TRANSFEREE_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not delete state related to transfer."), 1);

            accept(SBUF("Host re-registration successful."), 0);
        }
    }
    else if (op_type == OP_SET_HOOK)
    {
        uint8_t initializer_accid[ACCOUNT_ID_SIZE];
        const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
        if (initializer_accid_len < ACCOUNT_ID_SIZE)
            rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

        if (!BUFFER_EQUAL_20(initializer_accid, account_field))
            rollback(SBUF("Evernode: Only initializer is allowed to trigger a hook set."), 1);

        etxn_reserve(1);
        uint32_t txn_size;
        PREPARE_SET_HOOK_TX(memo_params, NAMESPACE, txn_size);

        uint8_t emithash[HASH_SIZE];
        if (emit(SBUF(emithash), SET_HOOK, txn_size) < 0)
            rollback(SBUF("Evernode: Hook set transaction failed."), 1);
        trace(SBUF("emit hash: "), SBUF(emithash), 1);

        accept(SBUF("Evernode: Successfully emitted SetHook transaction."), 0);
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 1."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
