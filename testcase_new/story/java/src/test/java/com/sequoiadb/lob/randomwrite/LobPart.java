package com.sequoiadb.lob.randomwrite;

import java.util.Random;

class LobPart {
    private int offset = 0;
    private int length = 0;
    private byte[] data = null;

    public LobPart( int offset, int length ) {
        this.offset = offset;
        this.length = length;
        data = new byte[ length ];
        new Random().nextBytes( data );
    }

    public int getOffset() {
        return offset;
    }

    public int getLength() {
        return length;
    }

    public byte[] getData() {
        return data;
    }
}