// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <StateMachine/stdafx.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace std {
#if defined(_UNICODE)
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}
