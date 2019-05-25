/*
 * Copyright (c) 2011 by the original author(s).
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

import java.util.ArrayList;



public class SimpleMappingEventListener extends AbstractSequoiadbEventListener<Object> {

	public final ArrayList<BeforeConvertEvent<Object>> onBeforeConvertEvents = new ArrayList<BeforeConvertEvent<Object>>();
	public final ArrayList<BeforeSaveEvent<Object>> onBeforeSaveEvents = new ArrayList<BeforeSaveEvent<Object>>();
	public final ArrayList<AfterSaveEvent<Object>> onAfterSaveEvents = new ArrayList<AfterSaveEvent<Object>>();
	public final ArrayList<AfterLoadEvent<Object>> onAfterLoadEvents = new ArrayList<AfterLoadEvent<Object>>();
	public final ArrayList<AfterConvertEvent<Object>> onAfterConvertEvents = new ArrayList<AfterConvertEvent<Object>>();

	@Override
	public void onBeforeConvert(Object source) {
		onBeforeConvertEvents.add(new BeforeConvertEvent<Object>(source));
	}

	@Override
	public void onBeforeSave(Object source, BSONObject dbo) {
		onBeforeSaveEvents.add(new BeforeSaveEvent<Object>(source, dbo));
	}

	@Override
	public void onAfterSave(Object source, BSONObject dbo) {
		onAfterSaveEvents.add(new AfterSaveEvent<Object>(source, dbo));
	}

	@Override
	public void onAfterLoad(BSONObject dbo) {
		onAfterLoadEvents.add(new AfterLoadEvent<Object>(dbo, Object.class));
	}

	@Override
	public void onAfterConvert(BSONObject dbo, Object source) {
		onAfterConvertEvents.add(new AfterConvertEvent<Object>(dbo, source));
	}
}
