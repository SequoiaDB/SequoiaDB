/**
 * Copyright (C) 2012 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.sequoiadb.net;

import java.net.InetSocketAddress;

/**
 * @author Jacky Zhang
 */
@Deprecated
public class ServerAddress {
    private InetSocketAddress hostAddress;

    /**
     * @return
     */
    public InetSocketAddress getHostAddress() {
        return hostAddress;
    }

    /**
     * @return
     */
    public String getHost() {
        return hostAddress.getHostName();
    }

    /**
     * @return
     */
    public int getPort() {
        return hostAddress.getPort();
    }

    /**
     * @param addr
     */
    public ServerAddress(InetSocketAddress addr) {
        hostAddress = addr;
    }

    public String toString() {
        return hostAddress.toString().split("/")[1];
    }
}
