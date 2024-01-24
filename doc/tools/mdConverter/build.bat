@echo on

set sourcePath=.\src

cl /Fo".\build\mdConverter.obj" /c %sourcePath%\mdConverter.cpp /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1 /D_CRT_RAND_S
if %errorlevel% GTR 0 goto end

cl /Fo".\build\iniReader.obj" /c %sourcePath%\iniReader.cpp /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\mdParser.obj" /c %sourcePath%\mdParser.cpp /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\system.obj" /c %sourcePath%\system.cpp /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\options.obj" /c %sourcePath%\options.cpp /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\ini.obj" /c %sourcePath%\inih\ini.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\cJSON2.obj" /c %sourcePath%\json\cJSON2.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\cJSON_iterator.obj" /c %sourcePath%\json\cJSON_iterator.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\oss.obj" /c %sourcePath%\oss\oss.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\ossUtil.obj" /c %sourcePath%\oss\ossUtil.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\ossMem.obj" /c %sourcePath%\oss\ossMem.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\autolink.obj" /c %sourcePath%\sundown\autolink.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\buffer.obj" /c %sourcePath%\sundown\buffer.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\houdini_href_e.obj" /c %sourcePath%\sundown\houdini_href_e.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\houdini_html_e.obj" /c %sourcePath%\sundown\houdini_html_e.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\html.obj" /c %sourcePath%\sundown\html.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\html_smartypants.obj" /c %sourcePath%\sundown\html_smartypants.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\markdown.obj" /c %sourcePath%\sundown\markdown.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

cl /Fo".\build\stack.obj" /c %sourcePath%\sundown\stack.c /I%sourcePath%\json /I%sourcePath%\include /nologo /EHsc /W3 /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /RTC1 /MT /Z7 /errorReport:none /Od /D__STDC_LIMIT_MACROS /DHAVE_CONFIG_H /DBOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC /DSDB_DLL_BUILD /DXP_WIN /DJS_HAVE_STDINT_H /D_UNICODE /DUNICODE /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS /DPSAPI_VERSION=1
if %errorlevel% GTR 0 goto end

link /DEBUG /out:.\build\mdConverter.exe build\mdConverter.obj build\system.obj build\options.obj build\ini.obj build\iniReader.obj build\mdParser.obj build\cJSON2.obj build\cJSON_iterator.obj build\oss.obj build\ossUtil.obj build\ossMem.obj build\autolink.obj build\buffer.obj build\houdini_href_e.obj build\houdini_html_e.obj build\html.obj build\html_smartypants.obj build\markdown.obj build\stack.obj
if %errorlevel% GTR 0 goto end

:end