#ifndef _ZNC_SMJS_H
#define _ZNC_SMJS_H

#ifdef _WIN32
#define XP_WIN
#else
#define XP_UNIX
#endif

#ifndef JSAPI_SUBDIR
#include "jsapi.h"
#else
#include "js/jsapi.h"
#endif

#ifndef JS_FS
/* only defined starting with SpiderMonkey 1.8 */
#define JS_FS_END JS_FS(NULL,NULL,0,0,0)
#define JS_FS(name,call,nargs,flags,extra) {name, call, nargs, flags, extra}
#endif

#if JS_VERSION <= 180
#define JS_AddObjectRoot JS_AddRoot
#define JS_AddValueRoot JS_AddRoot
#define JS_RemoveValueRoot JS_RemoveRoot
#define JS_RemoveObjectRoot JS_RemoveRoot
#endif

#endif
