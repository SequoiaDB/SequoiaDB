package com.sequoiadb.builddata;

public class Keyword {

    private String keyword;
    private int keywordNum;

    public Keyword(String keyword, int keywordNum) {
        this.keyword = keyword;
        this.keywordNum = keywordNum;
    }

    public String getKeyword() {
        return keyword;
    }

    public int getAndDecrement() {
        return keywordNum--;
    }

    public int get() {
        return keywordNum;
    }

    @Override
    public String toString() {
        return keyword + " " + keywordNum;
    }
}
