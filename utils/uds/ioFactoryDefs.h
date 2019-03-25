/*
 * Copyright (c) 2018 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 *
 * $Id: //eng/uds-releases/jasper/userLinux/uds/ioFactoryDefs.h#1 $
 */

#ifndef LINUX_USER_IO_FACTORY_DEFS_H
#define LINUX_USER_IO_FACTORY_DEFS_H 1

#include "fileUtils.h"

struct ioFactory;

/**
 * Create an IOFactory.  The IOFactory is returned with a reference count of 1.
 *
 * @param path        The path to the block device or file that contains the
 *                    block stream
 * @param access      The requested access kind.
 * @param factoryPtr  The IOFactory is returned here
 *
 * @return UDS_SUCCESS or an error code
 **/
__attribute__((warn_unused_result))
int makeIOFactory(const char        *path,
                  FileAccess         access,
                  struct ioFactory **factoryPtr);

#endif /* LINUX_USER_IO_FACTORY_DEFS_H */