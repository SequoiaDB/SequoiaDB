package com.sequoiadb.testdata;

import java.util.LinkedList;
import java.util.List;

public class ListNestedClassA {
    private List<BasicClass> listEmpty = new LinkedList<BasicClass>();
    private List<BasicClass> listNull = null;
    private List<BasicClass> listHaveEles = new LinkedList<BasicClass>();

    public ListNestedClassA() {
        for (int i = 0; i < 10; i++) {
            listHaveEles.add(new BasicClass());
        }
    }

    public List<BasicClass> getListHaveEles() {
        return listHaveEles;
    }

    public void setListHaveEles(List<BasicClass> value) {
        listHaveEles = value;
    }

    public List<BasicClass> getListEmpty() {
        return listEmpty;
    }

    public void setListEmpty(List<BasicClass> value) {
        listEmpty = value;
    }

    public List<BasicClass> getListNull() {
        return listNull;
    }

    public void setListNull(List<BasicClass> value) {
        listNull = value;
    }

    @Override
    public boolean equals(Object otherObj) {
        if (!(otherObj instanceof ListNestedClassA)) {
            return false;
        }

        ListNestedClassA other = (ListNestedClassA) otherObj;

        if ((this.listHaveEles == null && other.listHaveEles != null)
            || (this.listHaveEles != null && !this.listHaveEles
            .equals(other.listHaveEles))) {
            return false;
        }

        if ((this.listEmpty == null && other.listEmpty != null)
            || (this.listEmpty != null && !this.listEmpty
            .equals(other.listEmpty))) {
            return false;
        }

        if ((this.listNull == null && other.listNull != null)
            || (this.listNull != null && !this.listNull
            .equals(other.listNull))) {
            return false;
        }

        return true;
    }
}