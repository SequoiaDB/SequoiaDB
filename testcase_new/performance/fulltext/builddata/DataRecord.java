package com.sequoiadb.builddata;

import java.util.ArrayList;
import java.util.List;

public class DataRecord {

    private List<Field> fields = new ArrayList<>();

    public void add(DataField field) {
        Field f = field;
        fields.add(f);
    }

    public List<Field> getFields() {
        return fields;
    }
}
