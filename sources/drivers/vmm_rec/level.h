/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdio.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

#ifdef _DEBUG_SERVER
#define LEVEL(level, ...)  \
  if (debug_server_level == level) { \
    PRINTK(0, "[level]",   __VA_ARGS__); \
  }
#else
#define LEVEL(level, ...)
#endif
