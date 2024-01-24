package com.sequoiadb.index;

import org.bson.BSONObject;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-8
 * @Version 1.00
 */
public class IndexBean {
    private boolean isCreated = false;
    private String name = null;
    private BSONObject indexDef = null;
    private boolean isUnique = false;
    private boolean isEnforced = false;
    private int sortBufferSize = 64;
    private boolean isDeleted = false;

    public void setDeleted( boolean deleted ) {
        isDeleted = deleted;
    }

    public boolean isDeleted() {
        return isDeleted;
    }

    public IndexBean setEnforced( boolean enforced ) {
        isEnforced = enforced;
        return this;
    }

    public boolean isEnforced() {
        return isEnforced;
    }

    public IndexBean() {
    }

    public boolean isCreated() {
        return isCreated;
    }

    public IndexBean setCreated( boolean created ) {
        isCreated = created;
        return this;
    }

    public String getName() {
        return name;
    }

    public IndexBean setName( String name ) {
        this.name = name;
        return this;
    }

    public BSONObject getIndexDef() {
        return indexDef;
    }

    public IndexBean setIndexDef( BSONObject indexDef ) {
        this.indexDef = indexDef;
        return this;
    }

    public boolean isUnique() {
        return isUnique;
    }

    public IndexBean setUnique( boolean unique ) {
        isUnique = unique;
        return this;
    }

    public int getSortBufferSize() {
        return sortBufferSize;
    }

    public IndexBean setSortBufferSize( int sortBufferSize ) {
        this.sortBufferSize = sortBufferSize;
        return this;
    }
}
