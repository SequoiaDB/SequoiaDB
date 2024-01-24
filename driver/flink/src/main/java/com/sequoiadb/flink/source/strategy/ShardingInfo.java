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

public class ShardingInfo {

    String groupName;
    String nodeName;
    String csName;
    String clName;
    String scanType;

    public ShardingInfo(String groupName, String nodeName, String scanType, String clFullName) {
        this.groupName = groupName;
        this.nodeName = nodeName;
        this.scanType = scanType;

        String[] split = clFullName.split("\\.");
        this.csName = split[0];
        this.clName = split[1];
    }

    @Override
    public String toString() {
        return "ShardingInfo{" +
                "groupName='" + groupName + '\'' +
                ", nodeName='" + nodeName + '\'' +
                ", csName='" + csName + '\'' +
                ", clName='" + clName + '\'' +
                ", scanType='" + scanType + '\'' +
                '}';
    }

}