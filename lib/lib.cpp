#include <iostream>
#include "lib.h"

int major_version()
{
#ifdef PROJECT_VERSION_MAJOR
	return PROJECT_VERSION_MAJOR;
#endif
	return 0;
	
}
int minor_version()
{
#ifdef PROJECT_VERSION_MINOR
	return PROJECT_VERSION_MINOR;
#endif
	return 0;
}

int patch_version()
{
#ifdef PROJECT_VERSION_PATCH
	return PROJECT_VERSION_PATCH;
#endif
	return 0;
}
