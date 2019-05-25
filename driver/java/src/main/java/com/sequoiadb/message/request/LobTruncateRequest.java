package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;

public class LobTruncateRequest extends LobRequest {
    private byte[] bsonBytes;

    public LobTruncateRequest(BSONObject obj) {
        opCode = MsgOpCode.LOB_TRUNCATE_REQ;

        if (obj == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "obj is null");
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
