package com.sequoias3.model;

import com.sequoias3.core.ObjectMeta;
import com.sequoias3.dao.DataLob;

public class GetResult {
    ObjectMeta meta;
    DataLob    data;

    public GetResult(ObjectMeta meta, DataLob    data){
        this.meta = meta;
        this.data = data;
    }

    public void setMeta(ObjectMeta meta) {
        this.meta = meta;
    }

    public ObjectMeta getMeta() {
        return meta;
    }

    public void setData(DataLob data) {
        this.data = data;
    }

    public DataLob getData() {
        return data;
    }
}
