package org.springframework.data.mongodb.assist;

abstract class WriteRequest {
    public abstract Type getType();

    enum Type {
        INSERT,
        UPDATE,
        REPLACE,
        REMOVE
    }
}
