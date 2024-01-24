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

import java.util.List;


enum Operation {
    GET_CONN, DEL_CONN
}

enum PoolType {
    IDLE_POOL, USED_POOL
}

enum Action {
    CREATE_CONN, RELEASE_CONN
}

interface IConnectStrategy {
    public void init(List<Pair> _idleConnPairs, List<Pair> _usedConnPairs);

    public ConnItem pollConnItemForGetting();

    public ConnItem pollConnItemForDeleting();

    public ConnItem peekConnItemForDeleting();

    /*
       PoolType    incDecItemCount      meaning
       IDLE_POOL          +             one connection had been add to idle pool,
                                        strategy need to record the info of that idle connection.
       IDLE_POOL          -             one connection had been removed from idle pool,
                                        strategy need to remove the info of that idle connection
       USED_POOL          +             one connection was filled to used pool,
                                        strategy need to increase amount of used connection with specified address
       USED_POOL          -             one connection was got out from the used pool,
                                        strategy need to decrease amount of used connection with specified address
     */
//    public void update(PoolType poolType, ConnItem connItem, int change);

    public void addConnItemAfterCreating(ConnItem connItem);

    public void addConnItemAfterReleasing(ConnItem connItem);

    public void removeConnItemAfterCleaning(ConnItem connItem);

    public void updateUsedConnItemCount(ConnItem connItem, int change);

    public ServerAddress selectAddress(List<ServerAddress> addressList);

    public List<ConnItem> removeConnItemByAddress(String address);
}
