
#include "loadfuncpp.h"

extern "C" int integertobytes(value argv[]) {
    argv[0] = integertobytes(argv[1]);
    return SUCCEEDED;
}

extern "C" int bytestointeger(value argv[]) {
	argv[0] = bytestointeger(argv[1]);
	return SUCCEEDED;
}

extern "C" int base64(value argv[]) {
	argv[0] = base64(argv[1]);
	return SUCCEEDED;
}

extern "C" int base64tostring(value argv[]) {
	argv[0] = base64tostring(argv[1]);
	return SUCCEEDED;	
}

extern "C" int base64tointeger(value argv[]) {
	argv[0] = base64tointeger(argv[1]);
	return SUCCEEDED;	
}
