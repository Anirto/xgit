#pragma once

#include <stdio.h>
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
#include <sys/stat.h>
#include <malloc.h>
#include <fcntl.h>
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
#include <cstdlib>
#include <initializer_list>
#include <fstream>
#include <iosfwd>
#include <ostream>
#include <sstream>


#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <fileapi.h>
    #include <winbase.h>
    #include <io.h>

    #define TESTDIR     "D:\\Repo\\xgit\\test"
    #define GIT_DIR     "\\.git"
    #define CACHE_DIR   "\\.git\\objs"
    #define CACHE_FILE  "\\.git\\cache"
    #define PATH_SEP    "\\"

#else
    #include <sys/io.h>
    #include <sys/types.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/mman.h>

    #define _open open
    #define _close close
    #define _mkdir(path) mkdir(path, 0777)
    #define _access eaccess

    #define TESTDIR     "/home/gec/xc/xgit/test"
    #define GIT_DIR     "/.git"
    #define CACHE_DIR   "/.git/objs"
    #define CACHE_FILE  "/.git/cache"
    #define PATH_SEP    "/"
#endif


using Strings = std::vector<std::string>;

