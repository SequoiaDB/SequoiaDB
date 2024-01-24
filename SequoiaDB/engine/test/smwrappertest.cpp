/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

#include <stdio.h>
#include <iostream>
#include "../spt/engine.h"
#include "../bson/bson.h"
//#include "../bson/util/json.h"
#include "../bson/bson-inl.h"

using namespace engine;
using namespace bson;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create a sample native function here
// any native function (C function that can be called in javascript) need to take 2 arguments
// 1) const BSONObj & args
//	this variable contains the user input data in javascript
//	we always need to do few things
//	a) check the number of arguments
//	b) check the data type of each arguments
//	c) if everything satisfy what we need, then we proceed
// 2) void* data
//	this is the memory address we could optionally pass to variable when we create the function
//	for example some functions may need to use static memory information (say database control block)
//	whenever we need to use such thing, we need to construct a memory block contains everything the function need
//	and pass the pointer of the block when calling injectNative() function
// Return
//	this function returns BSONObj that received by javascript client
//	in simplest case we give a string "success" or "failure" to a new BSONObj and return to app
//	in native helper function, we simply retrieve the value of the first element in the return object
//	that means the string "return" is not important, it will ignore such thing and always pickup the first
//	element in the obj
// Remark
//	Since the purpose of engine.h wrapper, the application should NOT directly talk with JS engine at all
//	In this case, we should avoid using JS_DefineFunctions to define native function that can be directly
//	called by SM engine
//	That's why there's native_helper function helps map a native C function to javascript
//	The basic idea of such mapping is that creating an object in javascript contains the function address
//	And then create a javascript function that always call native_helper function and pass the function addr
//	Once native helper function received the function addr, it will parser the arguments and construct BSONObj
//	Then will call func() pointer to run our native function
//	For the return value, it's always parsed by native_helper and construct JS_RVAL(cx, vp) return variable to SM
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		// when user didn't input any variable, we get default system time
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

   /// TODO

	return 0;
}
