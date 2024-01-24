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

package com.sequoiadb.flink.sink.state;

import org.apache.flink.table.data.TimestampData;

/**
 * EventState is to keep the latest primary key update time, and processing time
 * of the corresponding primary key update changelog.
 */
public class EventState {

    private final TimestampData eventTime;
    private TimestampData processingTime;

    public EventState(TimestampData eventTime, TimestampData processingTime) {
        this.eventTime = eventTime;
        this.processingTime = processingTime;
    }

    public TimestampData getEventTime() {
        return eventTime;
    }

    public TimestampData getProcessingTime() {
        return processingTime;
    }

    public void setProcessingTime(TimestampData processingTime) {
        this.processingTime = processingTime;
    }

}
