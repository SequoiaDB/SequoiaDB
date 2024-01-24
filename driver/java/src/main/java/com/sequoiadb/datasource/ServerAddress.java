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

class ServerAddress {
    private final String address;
    private boolean isLocal;
    private boolean isEnable;
    private String location;
    private LocationPriority priority;

    ServerAddress(String address) {
        this.address = address;
        this.isLocal = false;
        this.isEnable = true;
        this.location = "";
        this.priority = LocationPriority.LOW;
    }

    String getAddress() {
        return this.address;
    }

    void setLocal(boolean value) {
        this.isLocal = value;
    }

    void setEnable(boolean value) {
        this.isEnable = value;
    }

    void setLocation(String location) {
        this.location = location;
    }

    void setLocationPriority(LocationPriority priority) {
        this.priority = priority;
    }

    boolean isLocal() {
        return isLocal;
    }

    boolean isEnable() {
        return isEnable;
    }

    String getLocation() {
        return location;
    }

    LocationPriority getLocationPriority() {
        return priority;
    }
}
