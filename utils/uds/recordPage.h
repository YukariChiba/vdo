/*
 * Copyright Red Hat
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
 * $Id: //eng/uds-releases/krusty/src/uds/recordPage.h#9 $
 */

#ifndef RECORDPAGE_H
#define RECORDPAGE_H 1

#include "common.h"
#include "volume.h"

/**
 * Generate the on-disk encoding of a record page from the list of records
 * in the open chapter representation.
 *
 * @param volume       The volume
 * @param records      The records to be encoded
 * @param record_page  The record page
 *
 * @return UDS_SUCCESS or an error code
 **/
int encode_record_page(const struct volume *volume,
		       const struct uds_chunk_record records[],
		       byte record_page[]);

/**
 * Find the metadata for a given block name in this page.
 *
 * @param record_page  The record page
 * @param name         The block name to look for
 * @param geometry     The geometry of the volume
 * @param metadata     an array in which to place the metadata of the
 *                     record, if one was found
 *
 * @return <code>true</code> if the record was found
 **/
bool search_record_page(const byte record_page[],
			const struct uds_chunk_name *name,
			const struct geometry *geometry,
			struct uds_chunk_data *metadata);

#endif /* RECORDPAGE_H */
