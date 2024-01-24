package com.sequoiadb.testdata;

import java.util.LinkedList;
import java.util.List;

public class ListClassA {
    private List<String> listEmpty = new LinkedList<String>();
    private List<String> listHaveEles = new LinkedList<String>();
    private List<String> listNull = null;
    private List<Integer> listInteger = new LinkedList<Integer>();
    private List<List<Integer>> listListFloat = new LinkedList<List<Integer>>();

    public List<String> getListEmpty() {
        return listEmpty;
    }

    public void setListEmpty(List<String> value) {
        listEmpty = value;
    }

    public List<String> getListHaveEles() {
        return listHaveEles;
    }

    public void setListHaveEles(List<String> value) {
        listHaveEles = value;
    }

    public List<String> getListNull() {
        return listNull;
    }

    public void setListNull(List<String> value) {
        listNull = value;
    }

    public List<Integer> getListInteger() {
        return listInteger;
    }

    public void setListInteger(List<Integer> value) {
        listInteger = value;
    }

    public List<List<Integer>> getListListFloat() {
        return listListFloat;
    }

    public void setListListFloat(List<List<Integer>> value) {
        listListFloat = value;
    }

    public ListClassA() {
        for (int i = 0; i < 10; i++) {
            listHaveEles.add("element" + i);
        }

        for (int i = 0; i < 10; i++) {
            listInteger.add(i);
        }

        for (int i = 0; i < 10; i++) {
            List<Integer> listFloat = new LinkedList<Integer>();
            for (int j = 0; j < 10; j++) {
                listFloat.add(i);
            }
            listListFloat.add(listFloat);
        }

    }

    @Override
    public boolean equals(Object otherObj) {
        if (otherObj == null || !(otherObj instanceof ListClassA)) {
            return false;
        }

        ListClassA other = (ListClassA) otherObj;
        if ((this.listEmpty == null && other.listEmpty != null)
            || (this.listEmpty != null && !this.listEmpty
            .equals(other.listEmpty))) {
            return false;
        }

        if ((this.listHaveEles == null && other.listHaveEles != null)
            || (this.listHaveEles != null && !this.listHaveEles
            .equals(other.listHaveEles))) {
            return false;
        }

        if ((this.listNull == null && other.listNull != null)
            || (this.listNull != null && !this.listNull
            .equals(other.listNull))) {
            return false;
        }

        if ((this.listInteger == null && other.listInteger != null)
            || (this.listInteger != null && !this.listInteger
            .equals(other.listInteger))) {
            return false;
        }

        if ((this.listListFloat == null && other.listListFloat != null)
            || (this.listListFloat != null && !this.listListFloat
            .equals(other.listListFloat))) {
            return false;
        }

//		if (this.listListFloat == null && other.listListFloat != null) {
//			return false;
//		} else if (this.listListFloat.size() != other.listListFloat.size()) {
//			return false;
//		} else {
//			for (int i = 0; i < this.listListFloat.size(); i++)
//			{
//				List<Float> thisListEle = this.listListFloat.get(i);
//				List<Float> otherListEle = other.listListFloat.get(i);
//				if (!thisListEle.equals(otherListEle))
//				{
//					return false;
//				}
//			}
//		}


        return true;
    }

}