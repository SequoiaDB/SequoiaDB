package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

public class SequenceFetchRequest extends SdbRequest {
    private static final byte[] EMPTY_BSON_BYTES = Helper.encodeBSONObj(new BasicBSONObject());
    private static final int ALIGNED_EMPTY_BSON_LENGTH = Helper.alignedSize(EMPTY_BSON_BYTES.length);
    private static final int FIXED_LENGTH = 88;
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private int flag;
    private long skipNum;
    private long returnedNum;
    private byte[] clNameBytes;
    private byte[] matcherBytes;
    private byte[] selectorBytes;
    private byte[] orderBytes;
    private byte[] hintBytes;

    public SequenceFetchRequest(BSONObject matcher) {
        opCode = MsgOpCode.MSG_BS_SEQUENCE_FETCH_REQ;
        length = FIXED_LENGTH;

        this.flag = 0;
        this.skipNum = 0;
        this.returnedNum = -1;

        try {
            this.clNameBytes = "".getBytes(Helper.ENCODING_TYPE);
            length += Helper.alignedSize(this.clNameBytes.length + 1);
        }catch (UnsupportedEncodingException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }

        if (matcher == null) {
            matcherBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            matcherBytes = Helper.encodeBSONObj(matcher);
            length += Helper.alignedSize(matcherBytes.length);
        }

        selectorBytes = EMPTY_BSON_BYTES.clone();
        length += ALIGNED_EMPTY_BSON_LENGTH;

        orderBytes = EMPTY_BSON_BYTES.clone();
        length += ALIGNED_EMPTY_BSON_LENGTH;

        hintBytes = EMPTY_BSON_BYTES.clone();
        length += ALIGNED_EMPTY_BSON_LENGTH;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putInt(clNameBytes.length);
        out.putLong(skipNum);
        out.putLong(returnedNum);
        out.put(clNameBytes);
        out.put((byte) 0);
        int length = clNameBytes.length + 1;
        int paddingLen = Helper.alignedSize(length) - length;
        Helper.fillZero(out, paddingLen);
        encodeBSONBytes(matcherBytes, out);
        encodeBSONBytes(selectorBytes, out);
        encodeBSONBytes(orderBytes, out);
        encodeBSONBytes(hintBytes, out);
    }
}
