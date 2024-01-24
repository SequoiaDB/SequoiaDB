/*
 * Copyright 2023 SequoiaDB Inc.
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


import org.bson.BSONObject;

import java.util.List;

interface IAddressMgr {

    String getLocation();

    List<ServerAddress> getAddress();

    List<ServerAddress> getNormalAddress();

    List<ServerAddress> getAbnormalAddress();

    List<ServerAddress> getLocalAddress();

    int getNormalAddressSize();

    int getAbnormalAddressSize();

    int getLocalAddressSize();

    boolean checkAddress(String address);

    void addAddress(String address);

    void removeAddress(String address);

    void enableAddress(String address);

    void disableAddress(String address);

    List<String> updateAddressInfo(BSONObject addrInfoObj, UpdateType type);

    String getAddressSnapshot();
}

enum UpdateType {
    ADDRESS,
    LOCATION
}
