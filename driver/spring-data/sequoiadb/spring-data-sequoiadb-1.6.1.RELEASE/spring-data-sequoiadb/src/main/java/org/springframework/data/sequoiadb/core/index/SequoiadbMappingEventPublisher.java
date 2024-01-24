/*
 * Copyright 2011-2012 the original author or authors.
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
package org.springframework.data.sequoiadb.core.index;

import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationEvent;
import org.springframework.context.ApplicationEventPublisher;
import org.springframework.data.mapping.context.MappingContextEvent;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.mapping.event.AfterLoadEvent;
import org.springframework.data.sequoiadb.core.mapping.event.AfterSaveEvent;
import org.springframework.util.Assert;

/**
 * An implementation of ApplicationEventPublisher that will only fire {@link MappingContextEvent}s for use by the index
 * creator when SequoiadbTemplate is used 'stand-alone', that is not declared inside a Spring {@link ApplicationContext}.
 * Declare {@link SequoiadbTemplate} inside an {@link ApplicationContext} to enable the publishing of all persistence events
 * such as {@link AfterLoadEvent}, {@link AfterSaveEvent}, etc.
 * 


 */
public class SequoiadbMappingEventPublisher implements ApplicationEventPublisher {

	private final SequoiadbPersistentEntityIndexCreator indexCreator;

	/**
	 * Creates a new {@link SequoiadbMappingEventPublisher} for the given {@link SequoiadbPersistentEntityIndexCreator}.
	 * 
	 * @param indexCreator must not be {@literal null}.
	 */
	public SequoiadbMappingEventPublisher(SequoiadbPersistentEntityIndexCreator indexCreator) {

		Assert.notNull(indexCreator);
		this.indexCreator = indexCreator;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.context.ApplicationEventPublisher#publishEvent(org.springframework.context.ApplicationEvent)
	 */
	@SuppressWarnings("unchecked")
	public void publishEvent(ApplicationEvent event) {
		if (event instanceof MappingContextEvent) {
			indexCreator.onApplicationEvent((MappingContextEvent<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty>) event);
		}
	}
}
