package com.sequoias3.commlibs3;

public interface StorageInterface {
    void envPrePare( String url );

    void envRestore( String url );

    String getUrls( String url );

    String getClusterInfo( String url );

}
