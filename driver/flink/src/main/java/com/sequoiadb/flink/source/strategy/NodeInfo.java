/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.source.strategy;

public class NodeInfo {

    final String groupName;
    final String hostName;
    final int port;
    final int instanceId;
    final boolean isPrimary;

    final String url;

    public NodeInfo(String groupName, String hostName, int port, int instanceId, boolean isPrimary) {
        this.groupName = groupName;
        this.hostName = hostName;
        this.port = port;
        this.instanceId = instanceId;
        this.isPrimary = isPrimary;

        this.url = hostName.concat(":").concat(String.valueOf(port));
    }

    public boolean isPrimary() {
        return isPrimary;
    }

    @Override
    public String toString() {
        return "NodeInfo{" +
                "groupName='" + groupName + '\'' +
                ", hostname='" + hostName + '\'' +
                ", port=" + port +
                ", instanceId=" + instanceId +
                ", isPrimary=" + isPrimary +
                ", url='" + url + '\'' +
                '}';
    }

}