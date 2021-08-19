package com.sequoias3.core;

import org.bson.types.ObjectId;

public class DataAttr {
    private ObjectId lobId;
    private String   eTag;
    private long     size;

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public long getSize() {
        return size;
    }

    public void setLobId(ObjectId lobId) {
        this.lobId = lobId;
    }

    public ObjectId getLobId() {
        return lobId;
    }
}
