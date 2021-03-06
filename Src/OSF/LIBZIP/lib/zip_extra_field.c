/*
   zip_extra_field.c -- manipulate extra fields
   Copyright (C) 2012-2015 Dieter Baron and Thomas Klausner

   This file is part of libzip, a library to manipulate ZIP archives.
   The authors can be contacted at <libzip@nih.at>

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
   IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include "zipint.h"

zip_extra_field_t * _zip_ef_clone(const zip_extra_field_t * ef, zip_error_t * error)
{
	zip_extra_field_t * head = 0;
	zip_extra_field_t * prev = 0;
	zip_extra_field_t * def;
	while(ef) {
		if((def = _zip_ef_new(ef->id, ef->size, ef->data, ef->flags)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			_zip_ef_free(head);
			return NULL;
		}
		else {
			SETIFZ(head, def);
			if(prev)
				prev->next = def;
			prev = def;
			ef = ef->next;
		}
	}
	return head;
}

zip_extra_field_t * _zip_ef_delete_by_id(zip_extra_field_t * ef, uint16 id, uint16 id_idx, zip_flags_t flags)
{
	int    i = 0;
	zip_extra_field_t * head = ef;
	zip_extra_field_t * prev = NULL;
	for(; ef; ef = (prev ? prev->next : head)) {
		if((ef->flags & flags & ZIP_EF_BOTH) && ((ef->id == id) || (id == ZIP_EXTRA_FIELD_ALL))) {
			if(id_idx == ZIP_EXTRA_FIELD_ALL || i == id_idx) {
				ef->flags &= ~(flags & ZIP_EF_BOTH);
				if(!(ef->flags & ZIP_EF_BOTH)) {
					if(prev)
						prev->next = ef->next;
					else
						head = ef->next;
					ef->next = NULL;
					_zip_ef_free(ef);
					if(id_idx == ZIP_EXTRA_FIELD_ALL)
						continue;
				}
			}
			i++;
			if(i > id_idx)
				break;
		}
		prev = ef;
	}
	return head;
}

void _zip_ef_free(zip_extra_field_t * ef)
{
	while(ef) {
		zip_extra_field_t * ef2 = ef->next;
		SAlloc::F(ef->data);
		SAlloc::F(ef);
		ef = ef2;
	}
}

const uint8 * _zip_ef_get_by_id(const zip_extra_field_t * ef, uint16 * lenp, uint16 id, uint16 id_idx, zip_flags_t flags, zip_error_t * error)
{
	static const uint8 empty[1] = { '\0' };
	int i = 0;
	for(; ef; ef = ef->next) {
		if(ef->id == id && (ef->flags & flags & ZIP_EF_BOTH)) {
			if(i < id_idx)
				i++;
			else {
				ASSIGN_PTR(lenp, ef->size);
				return (ef->size > 0) ? ef->data : empty;
			}
		}
	}
	zip_error_set(error, SLERR_ZIP_NOENT, 0);
	return NULL;
}

zip_extra_field_t * _zip_ef_merge(zip_extra_field_t * to, zip_extra_field_t * from)
{
	if(to == NULL)
		return from;
	else {
		zip_extra_field_t * ef2, * tt, * tail;
		for(tail = to; tail->next; tail = tail->next)
			;
		for(; from; from = ef2) {
			int duplicate = 0;
			ef2 = from->next;
			for(tt = to; tt; tt = tt->next) {
				if(tt->id == from->id && tt->size == from->size && memcmp(tt->data, from->data, tt->size) == 0) {
					tt->flags |= (from->flags & ZIP_EF_BOTH);
					duplicate = 1;
					break;
				}
			}
			from->next = NULL;
			if(duplicate)
				_zip_ef_free(from);
			else
				tail = tail->next = from;
		}
		return to;
	}
}

zip_extra_field_t * _zip_ef_new(uint16 id, uint16 size, const uint8 * data, zip_flags_t flags)
{
	zip_extra_field_t * ef = (zip_extra_field_t*)SAlloc::M(sizeof(*ef));
	if(ef) {
		ef->next = NULL;
		ef->flags = flags;
		ef->id = id;
		ef->size = size;
		if(size > 0) {
			ef->data = (uint8*)_zip_memdup(data, size, NULL);
			if(!ef->data) {
				ZFREE(ef);
			}
		}
		else
			ef->data = NULL;
	}
	return ef;
}

bool _zip_ef_parse(const uint8 * data, uint16 len, zip_flags_t flags, zip_extra_field_t ** ef_head_p, zip_error_t * error)
{
	zip_buffer_t * buffer;
	zip_extra_field_t * ef2;
	if((buffer = _zip_buffer_new((uint8*)data, len)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	else {
		zip_extra_field_t * ef_head = 0;
		zip_extra_field_t * ef = 0;
		while(_zip_buffer_ok(buffer) && _zip_buffer_left(buffer) >= 4) {
			uint16 fid = _zip_buffer_get_16(buffer);
			uint16 flen = _zip_buffer_get_16(buffer);
			uint8 * ef_data = _zip_buffer_get(buffer, flen);
			if(ef_data == NULL) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				_zip_buffer_free(buffer);
				_zip_ef_free(ef_head);
				return false;
			}
			else if((ef2 = _zip_ef_new(fid, flen, ef_data, flags)) == NULL) {
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
				_zip_buffer_free(buffer);
				_zip_ef_free(ef_head);
				return false;
			}
			else { 
				if(ef_head) {
					ef->next = ef2;
					ef = ef2;
				}
				else
					ef_head = ef = ef2;
			}
		}
		if(!_zip_buffer_eof(buffer)) {
			/* Android APK files align stored file data with padding in extra fields; ignore. */
			/* see https://android.googlesource.com/platform/build/+/master/tools/zipalign/ZipAlign.cpp */
			size_t glen = (size_t)_zip_buffer_left(buffer);
			uint8 * garbage;
			garbage = _zip_buffer_get(buffer, glen);
			if(glen >= 4 || garbage == NULL || memcmp(garbage, "\0\0\0", glen) != 0) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				_zip_buffer_free(buffer);
				_zip_ef_free(ef_head);
				return false;
			}
		}
		_zip_buffer_free(buffer);
		if(ef_head_p) {
			*ef_head_p = ef_head;
		}
		else {
			_zip_ef_free(ef_head);
		}
		return true;
	}
}

zip_extra_field_t * _zip_ef_remove_internal(zip_extra_field_t * ef)
{
	zip_extra_field_t * ef_head = ef;
	zip_extra_field_t * prev = NULL;
	while(ef) {
		if(ZIP_EF_IS_INTERNAL(ef->id)) {
			zip_extra_field_t * next = ef->next;
			if(ef_head == ef)
				ef_head = next;
			ef->next = NULL;
			_zip_ef_free(ef);
			if(prev)
				prev->next = next;
			ef = next;
		}
		else {
			prev = ef;
			ef = ef->next;
		}
	}
	return ef_head;
}

uint16 _zip_ef_size(const zip_extra_field_t * ef, zip_flags_t flags)
{
	uint16 size = 0;
	for(; ef; ef = ef->next) {
		if(ef->flags & flags & ZIP_EF_BOTH)
			size = (uint16)(size+4+ef->size);
	}
	return size;
}

int _zip_ef_write(zip_t * za, const zip_extra_field_t * ef, zip_flags_t flags)
{
	uint8 b[4];
	zip_buffer_t * buffer = _zip_buffer_new(b, sizeof(b));
	if(!buffer) {
		return -1;
	}
	for(; ef; ef = ef->next) {
		if(ef->flags & flags & ZIP_EF_BOTH) {
			_zip_buffer_set_offset(buffer, 0);
			_zip_buffer_put_16(buffer, ef->id);
			_zip_buffer_put_16(buffer, ef->size);
			if(!_zip_buffer_ok(buffer)) {
				zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
				_zip_buffer_free(buffer);
				return -1;
			}
			if(_zip_write(za, b, 4) < 0) {
				_zip_buffer_free(buffer);
				return -1;
			}
			if(ef->size > 0) {
				if(_zip_write(za, ef->data, ef->size) < 0) {
					_zip_buffer_free(buffer);
					return -1;
				}
			}
		}
	}
	_zip_buffer_free(buffer);
	return 0;
}

int _zip_read_local_ef(zip_t * za, uint64 idx)
{
	uchar b[4];
	zip_buffer_t * buffer;
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else {
		zip_entry_t * e = za->entry+idx;
		if(e->orig == NULL || e->orig->local_extra_fields_read)
			return 0;
		else if(e->orig->offset + 26 > ZIP_INT64_MAX)
			return zip_error_set(&za->error, SLERR_ZIP_SEEK, EFBIG);
		else if(zip_source_seek(za->src, (int64)(e->orig->offset + 26), SEEK_SET) < 0) {
			_zip_error_set_from_source(&za->error, za->src);
			return -1;
		}
		else if((buffer = _zip_buffer_new_from_source(za->src, sizeof(b), b, &za->error)) == NULL) {
			return -1;
		}
		else {
			uint16 fname_len = _zip_buffer_get_16(buffer);
			uint16 ef_len = _zip_buffer_get_16(buffer);
			if(!_zip_buffer_eof(buffer)) {
				_zip_buffer_free(buffer);
				return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			}
			else {
				_zip_buffer_free(buffer);
				if(ef_len > 0) {
					if(zip_source_seek(za->src, fname_len, SEEK_CUR) < 0)
						return zip_error_set(&za->error, SLERR_ZIP_SEEK, errno);
					else {
						zip_extra_field_t * ef;
						uint8 * ef_raw = _zip_read_data(NULL, za->src, ef_len, 0, &za->error);
						if(ef_raw == NULL)
							return -1;
						else if(!_zip_ef_parse(ef_raw, ef_len, ZIP_EF_LOCAL, &ef, &za->error)) {
							SAlloc::F(ef_raw);
							return -1;
						}
						else {
							SAlloc::F(ef_raw);
							if(ef) {
								ef = _zip_ef_remove_internal(ef);
								e->orig->extra_fields = _zip_ef_merge(e->orig->extra_fields, ef);
							}
						}
					}
				}
				e->orig->local_extra_fields_read = 1;
				if(e->changes && e->changes->local_extra_fields_read == 0) {
					e->changes->extra_fields = e->orig->extra_fields;
					e->changes->local_extra_fields_read = 1;
				}
				return 0;
			}
		}
	}
}

