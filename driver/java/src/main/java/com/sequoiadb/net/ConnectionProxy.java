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

package com.sequoiadb.net;

/**
 * Connection proxy object
 *
 * @since 3.4.3
 */
public class ConnectionProxy {
    private IConnection connection;

    public ConnectionProxy(IConnection connection){
        this.connection = connection;
    }

    /**
     * Set the connect timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     *
     * @param timeout The socket timeout in milliseconds. Default is 0ms and means no timeout.
     */
    public void setSoTimeout(int timeout){
        connection.setSoTimeout(timeout);
    }
}
