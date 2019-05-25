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
package org.springframework.data.sequoiadb.monitor;


import org.bson.BSONObject;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.jmx.export.annotation.ManagedMetric;
import org.springframework.jmx.export.annotation.ManagedResource;
import org.springframework.jmx.support.MetricType;

/**
 * JMX Metrics for Connections
 * 

 */
@ManagedResource(description = "Connection metrics")
public class ConnectionMetrics extends AbstractMonitor {

	public ConnectionMetrics(Sdb sdb) {
		this.sdb = sdb;
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Current Connections")
	public int getCurrent() {
		return getConnectionData("current", java.lang.Integer.class);
	}

	@ManagedMetric(metricType = MetricType.GAUGE, displayName = "Available Connections")
	public int getAvailable() {
		return getConnectionData("available", java.lang.Integer.class);
	}

	@SuppressWarnings("unchecked")
	private <T> T getConnectionData(String key, Class<T> targetClass) {
		BSONObject mem = (BSONObject) getServerStatus().get("connections");
		return (T) mem.get(key);
	}

}
