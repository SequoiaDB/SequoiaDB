package com.sequoiadb.index;

import java.util.Objects;

import org.bson.BasicBSONObject;

/**
 * Created by laojingtang on 18-1-12.
 */
class IndexEntity {
    private String indexName;
    private BasicBSONObject key;
    // default is false
    private boolean enforced;
    // default is false
    private boolean isUnique;
    // sortBufferSize defaule is 64.see
    // SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE
    private int sortBufferSize = 64;

    IndexEntity() {
    }

    public IndexEntity setIndexName( String indexName ) {
        this.indexName = indexName;
        return this;
    }

    public BasicBSONObject getKey() {
        return key;
    }

    public String getIndexName() {
        return indexName;
    }

    public boolean isEnforced() {
        return enforced;
    }

    public boolean isUnique() {
        return isUnique;
    }

    public IndexEntity setEnforced( boolean enforced ) {
        this.enforced = enforced;
        return this;
    }

    public IndexEntity setUnique( boolean unique ) {
        isUnique = unique;
        return this;
    }

    public IndexEntity setSortBufferSize( int sortBufferSize ) {
        this.sortBufferSize = sortBufferSize;
        return this;
    }

    public int getSortBufferSize() {
        return sortBufferSize;
    }

    public IndexEntity setKey( BasicBSONObject key ) {
        this.key = key;
        return this;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o )
            return true;
        if ( !( o instanceof IndexEntity ) )
            return false;
        IndexEntity that = ( IndexEntity ) o;
        return isEnforced() == that.isEnforced()
                && isUnique() == that.isUnique()
                && Objects.equals( getIndexName(), that.getIndexName() )
                && Objects.equals( getKey(), that.getKey() );
    }

    @Override
    public int hashCode() {
        return Objects.hash( getIndexName(), getKey(), isEnforced(),
                isUnique() );
    }

    @Override
    public String toString() {
        return "IndexEntity{" + "indexName='" + indexName + '\'' + ", key="
                + key + ", enforced=" + enforced + ", isUnique=" + isUnique
                + ", sortBufferSize=" + sortBufferSize + '}';
    }
}
