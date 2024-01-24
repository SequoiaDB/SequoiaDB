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

package com.sequoiadb.flink.config;

import com.sequoiadb.flink.common.exception.SDBException;
import org.apache.flink.configuration.ReadableConfig;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

public class SDBSourceOptions extends SDBClientOptions {

    private final int splitBlockNum;
    private PreferredInstance preferredInstance;
    private SplitMode splitMode;

    public SDBSourceOptions(ReadableConfig options) {
        super(options);

        this.splitBlockNum = options.get(SDBConfigOptions.SPLIT_BLOCK_NUM);
        initSplitMode(options.get(SDBConfigOptions.SPLIT_MODE));
        initPreferredInstance(
                options.get(SDBConfigOptions.PREFERRED_INSTANCE),
                options.get(SDBConfigOptions.PREFERRED_INSTANCE_MODE),
                options.get(SDBConfigOptions.PREFERRED_INSTANCE_STRICT)
                );
    }

    public int getSplitBlockNum() {
        return splitBlockNum;
    }

    public SplitMode getSplitMode() {
        return splitMode;
    }

    private void initSplitMode(String splitMode) {
        if (SplitMode.DATA_BLOCK.getMode().equals(splitMode)) {
            this.splitMode = SplitMode.DATA_BLOCK;
        } else if (SplitMode.SHARDING.getMode().equals(splitMode)) {
            this.splitMode = SplitMode.SHARDING;
        } else if (SplitMode.AUTO.getMode().equals(splitMode)) {
            this.splitMode = SplitMode.AUTO;
        } else {
            throw new SDBException(String.format("unknown split mode: %s.", splitMode));
        }
    }

    private void initPreferredInstance(String preferredInstancesStr,
                                       String preferredInstanceMode,
                                       boolean preferredInstanceStrict) {
        String[] instances = preferredInstancesStr.split(",");
        PreferredInstanceMode mode;
        if ("random".equals(preferredInstanceMode)) {
            mode = PreferredInstanceMode.RANDOM;
        } else if ("ordered".equals(preferredInstanceMode)) {
            mode = PreferredInstanceMode.ORDERED;
        } else {
            throw new SDBException(String.format("preferred instance mode %s is not in ['random', 'ordered'].",
                    preferredInstanceMode));
        }

        this.preferredInstance = new PreferredInstance(
                instances,
                mode,
                preferredInstanceStrict);
    }

    @Override
    public String toString() {
        return "SDBSourceOptions{" +
                "hosts= " + getHosts() +
                ", collectionSpace=" + getCollectionSpace() +
                ", collection=" + getCollection() +
                ", username=" + getUsername() +
                ", splitBlockNum=" + splitBlockNum +
                ", preferredInstance=" + preferredInstance +
                ", splitMode=" + splitMode +
                '}';
    }

    public static class PreferredInstance implements Serializable {

        private static final String PREFERRED_ANY = "A";
        private static final String PREFERRED_MASTER = "M";
        private static final String PREFERRED_SLAVE = "S";

        private final List<Integer> instanceIds;
        private String instanceTendency;
        private final PreferredInstanceMode mode;
        private final boolean preferredInstanceStrict;

        public PreferredInstance(String[] preferredInstances,
                                 PreferredInstanceMode preferredInstanceMode,
                                 boolean preferredInstanceStrict) {
            List<Integer> ids = new ArrayList<>();
            for (String preferredInstance : preferredInstances) {
                switch (preferredInstance.toUpperCase()) {
                    case PREFERRED_ANY:
                    case PREFERRED_MASTER:
                    case PREFERRED_SLAVE:
                        if (instanceTendency == null) {
                            instanceTendency = preferredInstance;
                        }
                        break;
                    default:
                        int instanceId = -1;
                        try {
                            instanceId = Integer.parseInt(preferredInstance);
                        } catch (NumberFormatException ex) {
                            throw new SDBException(
                                    String.format("preferred instance role %s is not in range: ['M', 'S', 'A'].",
                                            preferredInstance));
                        }
                        if (instanceId <= 0 || instanceId > 255)
                            throw new SDBException(
                                    String.format("preferred instance id %d is not in range: [1-255].", instanceId));
                        ids.add(instanceId);
                }
            }
            this.instanceIds = ids.stream().distinct().collect(Collectors.toList());
            this.mode = preferredInstanceMode;
            this.preferredInstanceStrict = preferredInstanceStrict;
        }

        public boolean nonPreferred() {
            return instanceIds.isEmpty() &&
                    (!hasInstanceTendency() || PREFERRED_ANY.equals(instanceTendency));
        }

        public boolean hasInstanceTendency() {
            return instanceTendency != null;
        }

        public boolean isMasterTendency() {
            return hasInstanceTendency() && PREFERRED_MASTER.equals(instanceTendency);
        }

        public boolean isSlaveTendency() {
            return hasInstanceTendency() && PREFERRED_SLAVE.equals(instanceTendency);
        }

        public boolean isPreferredInstanceStrict() {
            return preferredInstanceStrict;
        }

        public List<Integer> getInstanceIds() {
            return instanceIds;
        }

        public PreferredInstanceMode getMode() {
            return mode;
        }

        @Override
        public String toString() {
            return "PreferredInstance{" +
                    "instanceIds=" + instanceIds +
                    ", instanceTendency='" + instanceTendency + '\'' +
                    ", mode=" + mode +
                    ", preferredInstanceStrict=" + preferredInstanceStrict +
                    '}';
        }
    }

    public PreferredInstance getPreferredInstance() {
        return preferredInstance;
    }

}
