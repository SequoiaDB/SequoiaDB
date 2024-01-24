package com.sequoiadb.message.request;

import java.nio.ByteBuffer;

import com.sequoiadb.message.MsgOpCode;

public class LobRuntimeDetailRequest extends LobRequest {

    public LobRuntimeDetailRequest(long contextID) {
        opCode = MsgOpCode.LOB_GETRTDETAIL_REQ;
        this.contextId = contextID;
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        // no lob body
    }
}