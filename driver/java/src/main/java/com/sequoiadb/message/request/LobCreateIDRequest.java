package com.sequoiadb.message.request;

import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;

public class LobCreateIDRequest extends LobRequest {
    private byte[] bsonBytes;

    public LobCreateIDRequest(BSONObject obj) {
        opCode = MsgOpCode.LOB_CREATEID_REQ;

        if (obj == null) {
            obj = new BasicBSONObject();
        }

        bsonBytes = Helper.encodeBSONObj(obj);
        bsonLength = bsonBytes.length;
        length += Helper.alignedSize(bsonBytes.length);
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        encodeBSONBytes(bsonBytes, out);
    }
}
