 /*
  * Copyright 2018 SequoiaDB Inc.
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

 package com.sequoiadb.datasource;

 import com.sequoiadb.base.Sequoiadb;

 import java.util.Iterator;
 import java.util.List;


 class Pair {
     private ConnItem _item;
     private Sequoiadb _sdb;

     public Pair(ConnItem item, Sequoiadb sdb) {
         _item = item;
         _sdb = sdb;
     }

     public ConnItem first() {
         return _item;
     }

     public Sequoiadb second() {
         return _sdb;
     }
 }


 interface IConnectionPool {
     public Sequoiadb peek(ConnItem connItem);

     public Sequoiadb poll(ConnItem connItem);

     public ConnItem poll(Sequoiadb sdb);

     public void insert(ConnItem item, Sequoiadb sdb);

     public Iterator<Pair> getIterator();

     public int count();

     public boolean contains(Sequoiadb sdb);

     public List<ConnItem> clear();
 }
