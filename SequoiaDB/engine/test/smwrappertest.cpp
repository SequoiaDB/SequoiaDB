/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include <stdio.h>
#include <iostream>
#include "../spt/engine.h"
#include "../bson/bson.h"
#include "../bson/bson-inl.h"

using namespace engine;
using namespace bson;


BSONObj getTimeNative(const BSONObj &args, void* data)
{
	BSONObjBuilder b;
	int numField = args.nFields();
	if(numField>1 || (numField==1 && !args.firstElement().isNumber()))
	{
		b.append("return", "Syntax: gettime([current timestamp since 1970])");
	}
	else
	{
		char timeBuffer[50];
		time_t t;
		if(args.nFields()==0)
		{
			t = time(NULL); 
		}
		else
		{
			t = args[1].number();
		}
		struct tm tm;
#if defined (_LINUX)
		localtime_r(&t, &tm);
		b.append("return", asctime_r(&tm, timeBuffer));
#else
		localtime_s(&tm, &t);
		b.append("return", asctime_s(timeBuffer, 50, &tm));
#endif
	}
	return b.obj();

}
int main(int argc, char** argv)
{
	if(argc!=2)
	{
		printf("Syntax: %s <file name>\n", argv[0]);
		return 0;
	}


	return 0;
}
