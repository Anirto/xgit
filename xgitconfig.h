#pragma once

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <algorithm>
#include <iterator>
#include <stdint.h>
#include <cstdint>
#include <tuple>
#include <array>
#include <map>
#include <sys\stat.h>
#include <malloc.h>
#include <concrt.h>
#include <fcntl.h>
#include <io.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <type_traits>
#include <ctype.h>
#include <stdlib.h>
#include <initializer_list>
#include <fstream>
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <direct.h>

#include <windows.h>
#include <fileapi.h>
#include <winbase.h>

using Strings = std::vector<std::string>;

#define CACHE_DIR	 "\\.git\\objs"
#define CACHE_FILE	 "\\.git\\cache"