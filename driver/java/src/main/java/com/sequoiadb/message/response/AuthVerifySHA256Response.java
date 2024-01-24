/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.message.response;

import java.nio.ByteBuffer;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgConstants;
import com.sequoiadb.message.ResultSet;
import org.bson.BSONObject;

public class AuthVerifySHA256Response extends CommonResponse {

    BSONObject data = null;

    @Override
    protected void decodeData(ByteBuffer in) {
        if (flag == 0 && in.hasRemaining()) {
            ResultSet resultSet = new ResultSet(in, returnedNum);
            data = resultSet.getNext();
        }
    }

    private Object getValue(String field) {
        if (data.containsField(field)) {
            return data.get(field);
        }
        throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT, "The response data is missing field: " + field);
    }

    public Step1Data getStep1Data() {
        if (data == null || data.isEmpty()) {
            return null;
        }

        // check data
        int stepNum = (int) getValue(MsgConstants.AUTH_STEP);
        if (stepNum != MsgConstants.AUTH_SHA265_STEP_1) {
            throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT, "Incorrect step information");
        }

        String saltBase64 = (String) getValue(MsgConstants.AUTH_SALT);
        String combineNonceBase64 = (String) getValue(MsgConstants.AUTH_NONCE);
        int iterCount = (int) getValue(MsgConstants.AUTH_ITERATION_COUNT);
        return new Step1Data(saltBase64, combineNonceBase64, iterCount);
    }

    public Step2Data getStep2Data() {
        if (data == null) {
            throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT, "Response data is null");
        }

        // check data
        int stepNum = (int) getValue(MsgConstants.AUTH_STEP);
        if (stepNum != MsgConstants.AUTH_SHA265_STEP_2) {
            throw new BaseException(SDBError.SDB_UNEXPECTED_RESULT, "Incorrect step information");
        }

        String serverProofBase64 = (String) getValue(MsgConstants.AUTH_PROOF);
        return new Step2Data(serverProofBase64);
    }

    public static class Step1Data {
        private final String saltBase64;
        private final String combineNonceBase64;
        private final int iterCount;

        public Step1Data(String saltBase64, String combineNonceBase64, int iterCount) {
            this.saltBase64 = saltBase64;
            this.combineNonceBase64 = combineNonceBase64;
            this.iterCount = iterCount;
        }

        public String getSaltBase64() {
            return saltBase64;
        }

        public String getCombineNonceBase64() {
            return combineNonceBase64;
        }

        public int getIterCount() {
            return iterCount;
        }
    }

    public static class Step2Data {
        private final String serverProofBase64;

        public Step2Data(String serverProofBase64) {
            this.serverProofBase64 = serverProofBase64;
        }

        public String getServerProofBase64() {
            return serverProofBase64;
        }
    }
}
