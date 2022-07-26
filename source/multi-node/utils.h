#pragma once

#define for_i(i, length) for (unsigned i = 0; i < (length); i++)
#define for_yx(height, width) for_i(y, height) for_i(x, width)

void error(char* message);
