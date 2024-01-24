///*
// * Copyright 2011-2014 the original author or authors.
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *      http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */
//package org.springframework.data.sequoiadb.repository.support;
//
//import java.util.Collections;
//import java.util.HashSet;
//import java.util.Set;
//import java.util.regex.Pattern;
//
//import org.springframework.data.mapping.context.MappingContext;
//import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
//import org.springframework.data.sequoiadb.core.convert.QueryMapper;
//import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
//import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
//import org.springframework.util.Assert;
//
//
//import org.springframework.data.sequoiadb.assist.DBRef;
//import com.mysema.query.sequoiadb.SequoiadbSerializer;
//import com.mysema.query.types.Constant;
//import com.mysema.query.types.Operation;
//import com.mysema.query.types.Path;
//import com.mysema.query.types.PathMetadata;
//import com.mysema.query.types.PathType;
//
///**
// * Custom {@link SequoiadbSerializer} to take mapping information into account when building keys for constraints.
// *
//
//
// */
//class SpringDataSequoiadbSerializer extends SequoiadbSerializer {
//
//	private static final String ID_KEY = "_id";
//	private static final Set<PathType> PATH_TYPES;
//
//	static {
//
//		Set<PathType> pathTypes = new HashSet<PathType>();
//		pathTypes.add(PathType.VARIABLE);
//		pathTypes.add(PathType.PROPERTY);
//
//		PATH_TYPES = Collections.unmodifiableSet(pathTypes);
//	}
//
//	private final SequoiadbConverter converter;
//	private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;
//	private final QueryMapper mapper;
//
//	/**
//	 * Creates a new {@link SpringDataSequoiadbSerializer} for the given {@link MappingContext}.
//	 *
//	 * @param mappingContext must not be {@literal null}.
//	 */
//	public SpringDataSequoiadbSerializer(SequoiadbConverter converter) {
//
//		Assert.notNull(converter, "SequoiadbConverter must not be null!");
//
//		this.mappingContext = converter.getMappingContext();
//		this.converter = converter;
//		this.mapper = new QueryMapper(converter);
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbSerializer#getKeyForPath(com.mysema.query.types.Path, com.mysema.query.types.PathMetadata)
//	 */
//	@Override
//	protected String getKeyForPath(Path<?> expr, PathMetadata<?> metadata) {
//
//		if (!metadata.getPathType().equals(PathType.PROPERTY)) {
//			return super.getKeyForPath(expr, metadata);
//		}
//
//		Path<?> parent = metadata.getParent();
//		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(parent.getType());
//		SequoiadbPersistentProperty property = entity.getPersistentProperty(metadata.getName());
//
//		return property == null ? super.getKeyForPath(expr, metadata) : property.getFieldName();
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbSerializer#asDBObject(java.lang.String, java.lang.Object)
//	 */
//	@Override
//	protected BSONObject asDBObject(String key, Object value) {
//
//		if (ID_KEY.equals(key)) {
//			return mapper.getMappedObject(super.asDBObject(key, value), null);
//		}
//
//		return super.asDBObject(key, value instanceof Pattern ? value : converter.convertToSequoiadbType(value));
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbSerializer#isReference(com.mysema.query.types.Path)
//	 */
//	@Override
//	protected boolean isReference(Path<?> path) {
//
//		SequoiadbPersistentProperty property = getPropertyFor(path);
//		return property == null ? false : property.isAssociation();
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbSerializer#asReference(java.lang.Object)
//	 */
//	@Override
//	protected DBRef asReference(Object constant) {
//		return converter.toDBRef(constant, null);
//	}
//
//	/*
//	 * (non-Javadoc)
//	 * @see com.mysema.query.sequoiadb.SequoiadbSerializer#asReference(com.mysema.query.types.Operation, int)
//	 */
//	@Override
//	protected DBRef asReference(Operation<?> expr, int constIndex) {
//
//		for (Object arg : expr.getArgs()) {
//
//			if (arg instanceof Path) {
//
//				SequoiadbPersistentProperty property = getPropertyFor((Path<?>) arg);
//				Object constant = ((Constant<?>) expr.getArg(constIndex)).getConstant();
//
//				return converter.toDBRef(constant, property);
//			}
//		}
//
//		return super.asReference(expr, constIndex);
//	}
//
//	private SequoiadbPersistentProperty getPropertyFor(Path<?> path) {
//
//		Path<?> parent = path.getMetadata().getParent();
//
//		if (parent == null || !PATH_TYPES.contains(path.getMetadata().getPathType())) {
//			return null;
//		}
//
//		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(parent.getType());
//		return entity != null ? entity.getPersistentProperty(path.getMetadata().getName()) : null;
//	}
//}
