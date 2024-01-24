package com.sequoias3.core;

public class IDGenerator {
    public static final String ID_TYPE     = "Type";
    public static final String ID_ID       = "ID";

    public static final int TYPE_USER      = 1;
    public static final int TYPE_BUCKET    = 2;
    public static final int TYPE_PARENTID  = 3;
    public static final int TYPE_TASK      = 4;
    public static final int TYPE_UPLOAD    = 5;
    public static final int TYPE_ACLID     = 6;

    private int type;
    private long id;

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setType(int type) {
        this.type = type;
    }

    public int getType() {
        return type;
    }
}
