# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, 
# Boston, MA 02111-1307, USA.

include(FindPkgConfig)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_AVAHI_CLIENT avahi-client)
endif()


find_library(AVAHI_COMMON_LIBRARY NAMES avahi-common PATHS ${PC_AVAHI_CLIENT_LIBRARY_DIRS})
if(AVAHI_COMMON_LIBRARY)
	set(AVAHI_COMMON_FOUND TRUE)
endif()

find_library(AVAHI_CLIENT_LIBRARY NAMES avahi-client PATHS ${PC_AVAHI_CLIENT_LIBRARY_DIRS})
if(AVAHI_CLIENT_LIBRARY)
	set(AVAHI_CLIENT_FOUND TRUE)
endif()

find_path(AVAHI_INCLUDE_DIR avahi-client/client.h PATHS ${PC_AVAHI_CLIENT_INCLUDE_DIRS})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(AVAHI DEFAULT_MSG AVAHI_COMMON_FOUND AVAHI_CLIENT_FOUND)

if (AVAHI_FOUND)
	set(AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDE_DIR})
	set(AVAHI_LIBRARIES ${AVAHI_COMMON_LIBRARY} ${AVAHI_CLIENT_LIBRARY})
endif()

mark_as_advanced(AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)
