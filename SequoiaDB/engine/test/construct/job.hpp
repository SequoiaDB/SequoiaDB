/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#ifndef JOB_HPP_
#define JOB_HPP_

#include <string>

using namespace std;

enum JOB_TYPE
{
   JOB_TYPE_QUIT = 0,
   JOB_TYPE_INSERT,
   JOB_TYPE_DELETE,
   JOB_TYPE_UPDATE,
   JOB_TYPE_QUERY
};

class job
{
   public:
      job(){}
      ~job(){}
   public:
      JOB_TYPE _type;
      string _cs;
      string _collection;
};

#endif

