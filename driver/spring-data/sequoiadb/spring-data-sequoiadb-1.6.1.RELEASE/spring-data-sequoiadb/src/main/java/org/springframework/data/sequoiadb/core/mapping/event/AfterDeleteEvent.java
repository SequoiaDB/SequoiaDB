/*
 * Copyright 2013 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core.mapping.event;


import org.bson.BSONObject;

/**
 * Event being thrown after a single or a set of documents has/have been deleted. The {@link BSONObject} held in the event
 * will be the query document <em>after</am> it has been mapped onto the domain type handled.
 * 

 */
public class AfterDeleteEvent<T> extends AbstractDeleteEvent<T> {

	private static final long serialVersionUID = 1L;

	/**
	 * Creates a new {@link AfterDeleteEvent} for the given {@link BSONObject} and type.
	 * 
	 * @param dbo must not be {@literal null}.
	 * @param type can be {@literal null}.
	 */
	public AfterDeleteEvent(BSONObject dbo, Class<T> type) {
		super(dbo, type);
	}
}
