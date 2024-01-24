package com.sequoiadb.testdata;

import java.util.ArrayList;

public class ArrayListBean {
    ArrayList<Integer> list = new ArrayList<Integer>();

    public ArrayListBean() {
        for (int i = 0; i < 3; ++i) {
            list.add(i);
        }
    }

    /**
     * @return the list
     */
    public ArrayList<Integer> getList() {
        return list;
    }


    /**
     * @param list the list to set
     */
    public void setList(ArrayList<Integer> list) {
        this.list = list;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((list == null) ? 0 : list.hashCode());
        return result;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        ArrayListBean other = (ArrayListBean) obj;
        if (list == null) {
            if (other.list != null)
                return false;
        } else if (!list.equals(other.list))
            return false;
        return true;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "ArrayListBean [list=" + list + "]";
    }


}
