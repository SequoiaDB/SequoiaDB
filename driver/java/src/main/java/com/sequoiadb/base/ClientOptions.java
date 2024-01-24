/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.base;

/**
 * Global configuration of SequoiaDB driver.
 * 
 * When using the SequoiaDB driver, you can specify its global configuration and initialize
 * it with {@link Sequoiadb#initClient(ClientOptions)}
 *
 * </p>
 * Usage example:
 * <pre>
 * ClientOptions options = new ClientOptions();
 * options.setEnableCache( true );
 * options.setCacheInterval( 300 * 1000 );  // 300s
 * options.setExactlyDate( false );
 *
 * Sequoiadb.initClient( options );
 * </pre>
 */
public class ClientOptions {
    private static final long CACHE_INTERVAL_DEFAULT = 300L * 1000; // 300s

    private boolean enableCache;
    private long cacheInterval;
    private boolean exactlyDate;

    private boolean infoEncryption;

    /**
     * The construction method of ClientOptions.
     */
    public ClientOptions() {
        enableCache = true;
        cacheInterval = CACHE_INTERVAL_DEFAULT;
        exactlyDate = false;
        infoEncryption = true;
    }

    /**
     * @return True if cache is enabled and false if not.
     */
    public boolean getEnableCache() {
        return enableCache;
    }

    /**
     * Set caching the name of collection space and collection in client or not.
     *
     * @param enable True or false.
     */
    public void setEnableCache(boolean enable) {
        enableCache = enable;
    }

    /**
     * @return The value of caching interval.
     */
    public long getCacheInterval() {
        return cacheInterval;
    }

    /**
     * Set the interval for caching the name of collection space
     * and collection in client in milliseconds.
     * This value should not be less than 0, or it will be set to the default value,
     * default value is 300*1000ms.
     *
     * @param interval The interval in milliseconds.
     */
    public void setCacheInterval(long interval) {
        cacheInterval = interval;
    }


    /**
     * @return A boolean value indicating whether to use the exact date or not.
     */
    public Boolean getExactlyDate() {
        return exactlyDate;
    }

    /**
     * Whether to use exactly date, default is false. False indicates that the hours, minutes,
     * seconds and milliseconds of java.util.Date are valid. True indicates that they are meaningless
     * and the SequoiaDB driver will discard the hours, minutes, seconds and milliseconds of
     * java.util.Date.
     *
     * <pre>
     * Example:
     * A instance of java.util.Date is "2022-01-01 01:01:01", insert it to SequoiaDB with the SequoiaDB driver.
     * 1. exactlyDate is false: the inserted data is "2022-01-01 01:01:01"
     * 2. exactlyDate is true: the inserted data is "2022-01-01 00:00:00"
     * </pre>
     *
     * @param exactlyDate True or false.
     */
    public void setExactlyDate( boolean exactlyDate ) {
        this.exactlyDate = exactlyDate;
    }

    /**
     * @return A boolean value indicating whether to use information encryption.
     */
    public boolean getInfoEncryption() {
        return infoEncryption;
    }

    /**
     * Whether to use data encryption. The default is 'true.' When set to 'false,' it indicates printing raw
     * record data in logs and exception stack traces. When set to 'true,' it indicates printing encrypted
     * record data in logs and exception stack traces. Encrypted data can be decrypted using sdbsecuretool.
     *
     * @param infoEncryption True or false.
     */
    public void setInfoEncryption(boolean infoEncryption) {
        this.infoEncryption = infoEncryption;
    }
}
