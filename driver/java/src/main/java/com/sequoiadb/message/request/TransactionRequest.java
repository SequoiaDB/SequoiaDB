/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;

import java.nio.ByteBuffer;

public class TransactionRequest extends SdbRequest {
    public enum TransactionType {
        Begin,
        Commit,
        Rollback
    }

    public TransactionRequest(TransactionType transactionType) {
        switch (transactionType) {
            case Begin:
                opCode = MsgOpCode.TRANSACTION_BEGIN_REQ;
                break;
            case Commit:
                opCode = MsgOpCode.TRANSACTION_COMMIT_REQ;
                break;
            case Rollback:
                opCode = MsgOpCode.TRANSACTION_ROLLBACK_REQ;
                break;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid transaction type");
        }
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        // no body
    }
}
