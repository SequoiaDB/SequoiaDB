/*
 * Copyright 2011-2013 the original author or authors.
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
package org.springframework.data.mongodb.core.mapping;

import org.springframework.context.annotation.Bean;
import org.springframework.data.mongodb.config.AbstractMongoConfiguration;
import org.springframework.data.mongodb.core.mapping.event.LoggingEventListener;

import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoClient;

public class GeoIndexedAppConfig extends AbstractMongoConfiguration {

	public static String GEO_DB = "database";
	public static String GEO_COLLECTION = "geolocation";

	@Override
	public String getDatabaseName() {
		return GEO_DB;
	}

	@Override
	@Bean
	public Mongo mongo() throws Exception {
		return new MongoClient("127.0.0.1", 11810);
	}

	@Override
	public String getMappingBasePackage() {
		return "org.springframework.data.mongodb.core.core.mapping";
	}

	@Bean
	public LoggingEventListener mappingEventsListener() {
		return new LoggingEventListener();
	}
}
