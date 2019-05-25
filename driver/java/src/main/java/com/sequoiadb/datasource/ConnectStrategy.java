package com.sequoiadb.datasource;

/**
 * Data source connection strategy.
 */
public enum ConnectStrategy {
    /**
     * Get connection serially from given addresses.
     */
    SERIAL,

    /**
     * Get connection random from given addresses.
     */
    RANDOM,

    /**
     * Local connection preferred.
     */
    LOCAL,

    /**
     * Load balance preferred.
     */
    BALANCE
}