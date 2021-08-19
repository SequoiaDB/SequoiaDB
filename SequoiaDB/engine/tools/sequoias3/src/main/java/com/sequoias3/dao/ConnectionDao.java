package com.sequoias3.dao;

public interface ConnectionDao {
    void setTransTimeOut(int timeSecond);

    int getTransTimeOut();
}
