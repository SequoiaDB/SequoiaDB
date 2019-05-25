/*
 * Copyright 2017 SequoiaDB Inc.
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
 * Options for client.
 */
public class ClientOptions {
    private boolean enableCache;
    private long cacheInterval;

    public ClientOptions() {
        enableCache = true;
        cacheInterval = 300 * 1000;
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

}
