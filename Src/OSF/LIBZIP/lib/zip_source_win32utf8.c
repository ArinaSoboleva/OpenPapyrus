/*
   zip_source_win32utf8.c -- create data source from Windows file (UTF-8)
   Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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
#include "zipint.h"
#include "zipwin32.h"

ZIP_EXTERN zip_source_t * zip_source_file(zip_t * za, const char * fname, uint64 start, int64 len)
{
	return za ? zip_source_file_create(fname, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_file_create(const char * fname, uint64 start, int64 length, zip_error_t * error)
{
	wchar_t * wfname;
	zip_source_t * source = 0;
	if(fname == NULL || length < -1) {
		zip_error_set(error, ZIP_ER_INVAL, 0);
	}
	else {
		// Convert fname from UTF-8 to Windows-friendly UTF-16
		int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fname, -1, NULL, 0);
		if(size == 0) {
			zip_error_set(error, ZIP_ER_INVAL, 0);
		}
		else if((wfname = (wchar_t*)SAlloc::M(sizeof(wchar_t) * size)) == NULL) {
			zip_error_set(error, ZIP_ER_MEMORY, 0);
		}
		else {
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fname, -1, wfname, size);
			source = zip_source_win32w_create(wfname, start, length, error);
			SAlloc::F(wfname);
		}
	}
	return source;
}

