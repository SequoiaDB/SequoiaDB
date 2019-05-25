/*
 * Copyright 2002-2011 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.mongodb.monitor;

import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.jmx.export.annotation.ManagedMetric;
import org.springframework.jmx.export.annotation.ManagedResource;
import org.springframework.jmx.support.MetricType;

/**
 * JMX Metrics for Global Locks
 * 
 * @author Mark Pollack
 */
@ManagedResource(description = "Global Lock Metrics")
public class GlobalLockMetrics extends AbstractMonitor {

	public GlobalLockMetrics(Mongo mongo) {
		this.mongo = mongo;
	}

	@ManagedMetric(metricType = MetricType.COUNTER, displayName = "Total time")
	public double getTotalTime() {
		return getGlobalLockData("totalTime", java.lang.Double.class);
	}

	@ManagedMetric(metricType = MetricType.COUNTER, displayName = "Lock time", unit = "s")
	public double getLockTime() {
		return getGlobalLockData("lockTime", java.lang.Double.class);
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Lock time")
	public double getLockTimeRatio() {
		return getGlobalLockData("ratio", java.lang.Double.class);
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Current Queue")
	public int getCurrentQueueTotal() {
		return getCurrentQueue("total");
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Reader Queue")
	public int getCurrentQueueReaders() {
		return getCurrentQueue("readers");
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Writer Queue")
	public int getCurrentQueueWriters() {
		return getCurrentQueue("writers");
	}

	@SuppressWarnings("unchecked")
	private <T> T getGlobalLockData(String key, Class<T> targetClass) {
		DBObject globalLock = (DBObject) getServerStatus().get("globalLock");
		return (T) globalLock.get(key);
	}

	private int getCurrentQueue(String key) {
		DBObject globalLock = (DBObject) getServerStatus().get("globalLock");
		DBObject currentQueue = (DBObject) globalLock.get("currentQueue");
		return (Integer) currentQueue.get(key);
	}
}
