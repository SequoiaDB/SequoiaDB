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

package com.sequoiadb.datasource;

import java.util.*;

class ConcreteLocalStrategy extends AbstractStrategy {
    private final Random _rand = new Random(47);

    @Override
    public ServerAddress selectAddress(List<ServerAddress> addressList) {
        List<ServerAddress> serAddrLst = new ArrayList<>();
        for (ServerAddress serAddr : addressList) {
            if (serAddr.isLocal()) {
                serAddrLst.add(serAddr);
            }
        }
        ServerAddress retAddr;
        // Use the local address first
        if (serAddrLst.size() > 0) {
            retAddr = serAddrLst.get(_rand.nextInt(serAddrLst.size()));
        } else {
            retAddr = addressList.get(_rand.nextInt(addressList.size()));
        }
        return retAddr;
    }
}
