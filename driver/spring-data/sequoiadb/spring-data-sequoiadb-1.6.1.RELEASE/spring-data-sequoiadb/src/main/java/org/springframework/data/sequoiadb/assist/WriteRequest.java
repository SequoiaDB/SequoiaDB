package org.springframework.data.sequoiadb.assist;

abstract class WriteRequest {
    public abstract Type getType();

    enum Type {
        INSERT,
        UPDATE,
        REPLACE,
        REMOVE
    }
}
