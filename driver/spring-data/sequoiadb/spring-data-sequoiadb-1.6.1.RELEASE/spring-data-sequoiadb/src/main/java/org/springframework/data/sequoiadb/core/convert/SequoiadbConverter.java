/*
 * Copyright 2010-2013 the original author or authors.
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
package org.springframework.data.sequoiadb.core.convert;

import org.bson.BSONObject;
import org.springframework.data.convert.EntityConverter;
import org.springframework.data.convert.EntityReader;
import org.springframework.data.convert.TypeMapper;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;



/**
 * Central Sdb specific converter interface which combines {@link SequoiadbWriter} and {@link SequoiadbReader}.
 * 


 */
public interface SequoiadbConverter extends
		EntityConverter<SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty, Object, BSONObject>, SequoiadbWriter<Object>,
		EntityReader<Object, BSONObject> {

	/**
	 * Returns thw {@link TypeMapper} being used to write type information into {@link BSONObject}s created with that
	 * converter.
	 * 
	 * @return will never be {@literal null}.
	 */
	SequoiadbTypeMapper getTypeMapper();
}
