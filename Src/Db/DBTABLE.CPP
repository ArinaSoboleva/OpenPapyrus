// DBTABLE.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016
// @codepage windows-1251
//
#include <db.h>
#pragma hdrstop

#define MAX_ONEOP_LOBSIZE 32000

//
// Utilities {
//
int FASTCALL Btr2SLibType(int t)
{
	if(t == 26)
		return S_WZSTRING;
	else if(t == 25)
		return S_WCHAR;
	else if(t == 27)
		return S_UUID_;
	else
		return (t + 1);
}

int FASTCALL SLib2BtrType(int t)
{
	if(t == S_WZSTRING)
		return 26;
	else if(t == S_WCHAR)
		return 25;
	else if(t == S_UUID_)
		return 27;
	else {
		if(t == S_RAW) {
			//
			// Note: PSQL ������-�� �� ������������ ��� UBINARY.
			// ��� ���������� ������ ���������� PSQL ����� �������� ��� RAW-����� ������
			// ����� ��� S_CHAR. ������ ������ ��������� ��� �� ��������.
			//
			t = S_UBINARY; // @v8.7.0 S_CHAR-->S_UBINARY
		}
		return t ? (t - 1) : 0;
	}
}

char * SLAPI GetRusNCaseACS(char * b)
{
	char * p = b;
	*p++ = (char)0xAC;
	p = stpcpy(p, "RUSNCASE");
	for(int i = 0; i < 256; i++)
		*p++ = ToUpper866(i);
	return b;
}

DBFileSpec & SLAPI operator + (DBFileSpec & pf, DBIdxSpec & pi)
{
	DBFileSpec * tmp = (DBFileSpec *)catmem(&pf, sizeof(DBFileSpec) + pf.NumSeg * sizeof(DBIdxSpec), &pi, sizeof(DBIdxSpec));
	tmp->NumSeg++;
	if(!(pi.flags & XIF_SEG))
		tmp->NumKeys++;
	delete & pi;
	return *tmp;
}
//
// } Utilities
//
SString & SLAPI XFile::GetTableName(SString & rBuf) const
	{ return rBuf.CopyFromN(XfName, MAXTABLENAME).Strip(); }
//
//
int FASTCALL BRet(int r)
{
	if(r) {
		DbThreadLocalArea & r_tla = DBS.GetTLA();
		r_tla.LastDbErr = SDBERR_BTRIEVE;
		r_tla.LastBtrErr = r;
		return 0;
	}
	else
		return 1;
}

int FASTCALL DBTable::copyBufLobFrom(const void * pBuf, size_t srcBufSize)
{
	int    ok = -1;
	if(pBuf && buf) {
		if(State & sHasLob) {
			const RECORDSIZE frs = FixRecSize;
			size_t s = (frs && frs < bufLen) ? frs : bufLen;
			memcpy(buf, pBuf, s);

			DBField last_fld;
			THROW(getField(fields.getCount()-1, &last_fld));
			if(srcBufSize)
				THROW(writeLobData(last_fld, PTR8(pBuf)+frs, (srcBufSize > frs) ? (srcBufSize-frs) : 0));
		}
		else {
			size_t s = (srcBufSize && srcBufSize < bufLen) ? srcBufSize : bufLen;
			memcpy(buf, pBuf, s);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI DBTable::Btr_Open(const char * pName, int openMode, char * pPassword)
{
	int    ok = 0;
	uint16 bl = (uint16)sstrlen(pPassword);
	index = openMode;
	if(strchr(pName, ' '))
		(OpenedFileName = 0).CatQStr(pName);
	else
		OpenedFileName = pName;
	char   temp_buf[512];
	OpenedFileName.CopyTo(temp_buf, sizeof(temp_buf));
	if(Ret(BTRV(B_OPEN, FPB, pPassword, &bl, temp_buf, WBTRVTAIL_Z))) {
		State |= sOpened_;
		//
		// ���������� ������ ������������� ����� ������.
		// � ����������, ������������������ ����� ������� �������� ����� ������������ //
		// �������� DBTable::getRecSize().
		//
		DBFileSpec * p_info = 0;
		uint16 buf_size;
		if(getStat((void **)&p_info, &buf_size)) {
			FixRecSize = p_info->RecSize;
			delete p_info;
			ok = 1;
		}
		else
			close();
	}
	index = 0;
	return ok;
}

int SLAPI DBTable::Btr_Close()
{
	if(State & sOpened_) {
		OpenedFileName = 0;
		BTRV(B_CLOSE, FPB, 0, 0, 0, WBTRVTAIL_ZZ);
		State &= ~sOpened_;
	}
	return 1;
}

static const int _b_cmd[] = {B_GETFIRST,B_GETLAST,B_GETEQ,B_GETLT,B_GETLE,B_GETGT,B_GETGE,B_GETNEXT,B_GETPREV};

static int _fastcall getSearchComm(int mode)
{
	//return (mode >= 0 && mode < sizeof(_b_cmd) / sizeof(int)) ? _b_cmd[mode] : B_GETFIRST;
	return _b_cmd[mode];
}

int DBTable::Btr_ProcessLobOnReading()
{
	int    ok = -1;
	int    bad_structured = 0;
	const  int preserve_index = index;
	if(State & sHasLob) {
		DBField last_fld;
		DbThreadLocalArea & tla = DBS.GetTLA();
		THROW(getField(fields.getCount()-1, &last_fld));
		if(retBufLen > FixRecSize && oneof2(tla.LastBtrErr, 0, BE_UBUFLEN)) {
			STempBuffer temp_buf(0);
			SBuffer lob_buffer;
			THROW(lob_buffer.Write(PTR8(buf)+FixRecSize, retBufLen-FixRecSize));
			if(tla.LastBtrErr == BE_UBUFLEN) {
				DBRowId pos;
				THROW(getPosition(&pos));
				THROW(temp_buf.Alloc(32*1024));
				long   rec_offs = retBufLen;
				const  size_t fixed_header_size = 24;
				const  long chunk_size = (long)(temp_buf.GetSize() - fixed_header_size);
				do {
					char   k[MAXKEYLEN];
					size_t temp_buf_offs = 0;
					*(RECORDNUMBER *)(char *)temp_buf = pos;
					temp_buf_offs += sizeof(RECORDNUMBER);
					*(long *)(((char *)temp_buf) + temp_buf_offs) = CHUNK_DIR_RAND;
					temp_buf_offs += sizeof(long);
					*(long *)(((char *)temp_buf) + temp_buf_offs) = 1; // number of chunks
					temp_buf_offs += sizeof(long);
					// chunk definition
					*(long *)(((char *)temp_buf) + temp_buf_offs) = rec_offs; // chunk offset
					temp_buf_offs += sizeof(long);
					*(long *)(((char *)temp_buf) + temp_buf_offs) = chunk_size; // chunk length (buffer size - fixed header)
					temp_buf_offs += sizeof(long);
					*(long *)(((char *)temp_buf) + temp_buf_offs) = 0; // data ptr (ignored for CHUNK_DIR_RAND)
					temp_buf_offs += sizeof(long);
					assert(temp_buf_offs == fixed_header_size);
					retBufLen = (RECORDSIZE)chunk_size;
					index = -2;
					tla.LastBtrErr = BTRV(B_GETDIRECT, FPB, (char *)temp_buf, (uint16 *)&retBufLen, k, WBTRVTAIL);
					if(oneof2(tla.LastBtrErr, 0, BE_CHUNK_OFFSET_TOO_LONG)) {
						THROW(lob_buffer.Write(temp_buf, retBufLen));
					}
					rec_offs += chunk_size;
				} while(tla.LastBtrErr == 0 && retBufLen == chunk_size); // ���� ��������� �� ��� ��� ���� �� ������� ������ BE_CHUNK_OFFSET_TOO_LONG (103)
			}
			if(oneof2(tla.LastBtrErr, 0, BE_CHUNK_OFFSET_TOO_LONG))
				ok = 1;
			//const void * p_lob_data = PTR8(temp_size ? temp_buf : buf) + FixRecSize;
			{
				//
				// ������� �������� � ���������� ������������������ Lob-������,
				// �� ������ ���������� � ���� ������ (� ������� �� @v6.7.0).
				//
				size_t sz;
				if(LobB.GetSize((uint)last_fld.fld, &sz) > 0)
					if(((SLob *)last_fld.getValuePtr())->EnsureUnstructured() > 0)
						bad_structured = 1;
			}
			//THROW(writeLobData(last_fld, p_lob_data, retBufLen - FixRecSize));
			THROW(writeLobData(last_fld, lob_buffer, lob_buffer.GetAvailableSize()));
		}
		else {
			THROW(setLobSize(last_fld, 0));
		}
	}
	CATCHZOK
	index = preserve_index;
	return ok;
}

int DBTable::Btr_Implement_Search(int idx, void * pKey, int srchMode, long sf)
{
	DbSession::Config dbcfg;
	DBS.GetConfig(dbcfg);
	//
	// ��� ��� ������� ���������� ����� �����, �� ��� ������������� �� ����
	// ������ ������ ���������������� ���������� � �������
	//
	if(idx >= 0)
		index = (int16)idx;
	DbThreadLocalArea & tla = DBS.GetTLA();
	int    op = -1;
	char   temp_buf[MAXKEYLEN];
	char * p_buf = (char *)buf;
	retBufLen = bufLen;
	if(sf & sfDirect) {
		*(RECORDNUMBER *)p_buf = *(DBRowId *)pKey;
		op = B_GETDIRECT;
		if(sf & sfForUpdate) {
			if(dbcfg.NWaitLockTries != BTR_RECLOCKDISABLE) { // @v8.6.3
				if(dbcfg.NWaitLockTries > 0) {
					op += BL_SNGL_NWT;
				}
				else {
					op += BL_SNGL_WT;
				}
			}
		}
	}
	else {
		op = _b_cmd[srchMode];
		if(sf & sfKeyOnly) {
			op += 50;
			p_buf = temp_buf;
			retBufLen = sizeof(temp_buf);
		}
		else if(sf & sfForUpdate) {
			if(dbcfg.NWaitLockTries != BTR_RECLOCKDISABLE) { // @v8.6.3
				if(dbcfg.NWaitLockTries > 0) {
					op += BL_SNGL_NWT;
				}
				else {
					op += BL_SNGL_WT;
				}
			}
		}
	}
	assert(op != -1);
	int    _err = 0;
	int    nwl_try_n = 0;
	do {
		if(nwl_try_n && dbcfg.NWaitLockTryTimeout > 0) {
			SDelay(dbcfg.NWaitLockTryTimeout);
		}
		_err = BTRV(op, FPB, p_buf, (uint16 *)&retBufLen, (char *)pKey, WBTRVTAIL);
	} while(_err == BE_RECLOCK && ++nwl_try_n < dbcfg.NWaitLockTries);
	if(_err == BE_RECLOCK) {
		SString msg_buf;
		msg_buf.Cat(getcurdatetime_()).Tab().Cat("Btrieve locking failed").CatDiv(':', 2).
			Cat(tableName).CatParStr(fileName).Space().
			CatChar('[').Cat(dbcfg.NWaitLockTries).CatChar(',').Cat(dbcfg.NWaitLockTryTimeout).CatChar(']');
		SLS.LogMessage("dbwarn.log", msg_buf);
	}
	else if(_err == 0) {
		if(nwl_try_n) {
			SString msg_buf;
			msg_buf.Cat(getcurdatetime_()).Tab().Cat("Btrieve locking acquired after").Space().Cat(nwl_try_n).Space().Cat("tries").CatDiv(':', 2).
				Cat(tableName).CatParStr(fileName).Space().
				CatChar('[').Cat(dbcfg.NWaitLockTries).CatChar(',').Cat(dbcfg.NWaitLockTryTimeout).CatChar(']');
			SLS.LogMessage("dbwarn.log", msg_buf);
		}
	}
	tla.LastBtrErr = _err;
	if(sf & sfKeyOnly) {
		tla.InitErrFileName(OpenedFileName);
		return (tla.LastBtrErr == 0);
	}
	else {
		if(retBufLen < bufLen)
			((char *)buf)[retBufLen] = 0;
		Btr_ProcessLobOnReading();
		tla.InitErrFileName(OpenedFileName);
		return (tla.LastBtrErr == BE_UBUFLEN ? 1 : (tla.LastBtrErr == 0));
	}
}

int FASTCALL DBTable::Ret(int ret)
{
	BtrError = ret;
	InitErrFileName();
	return (ret == 0);
}

int SLAPI DBTable::step(int sp)
{
	static const int cmd[] = {B_STEPFIRST, B_STEPLAST, -1, -1, -1, -1, -1, B_STEPDIRECT, B_STEPPREV};
	retBufLen = bufLen;
	int    err = BTRV(cmd[sp], FPB, (char *)buf, (uint16 *)&retBufLen, 0, WBTRVTAIL_ZZ);
	BtrError = err;
	if(retBufLen < bufLen)
		((char *)buf)[retBufLen] = 0;
	Btr_ProcessLobOnReading();
	InitErrFileName();
	return BIN(err == 0 || err == BE_UBUFLEN);
}

int SLAPI DBTable::getExtended(void * key, int srchMode, int lock)
{
	assert(srchMode == spNext || srchMode == spPrev);
	int    op = ((srchMode == spNext) ? B_GETNEXTEXT : B_GETPREVEXT) + lock;
	retBufLen = bufLen;
	int    err = BTRV(op, FPB, (char *) buf, (uint16 *)&retBufLen, (char *) key, WBTRVTAIL);
	if(err == BE_FILTERLIMIT)
		err = BE_EOF;
	return Ret(err);
}

int SLAPI DBTable::stepExtended(int srchMode, int lock)
{
	assert(srchMode == spNext || srchMode == spPrev);
	int    op = ((srchMode == spNext) ? B_STEPNEXTEXT : B_STEPPREVEXT) + lock;
	retBufLen = bufLen;
	return Ret(BTRV(op, FPB, (char *)buf, (uint16 *)&retBufLen, 0, WBTRVTAIL_ZZ));
}

int FASTCALL DBTable::Btr_Implement_GetPosition(DBRowId * pPos)
{
	uint16 bl = sizeof(RECORDNUMBER);
	RECORDNUMBER rn = 0;
	int    ok = Ret(BTRV(B_GETPOS, FPB, (char *)&rn, &bl, 0, WBTRVTAIL_ZZ));
	ASSIGN_PTR(pPos, rn);
	return ok;
}

int DBTable::Btr_Implement_InsertRec(int idx, void * pKeyBuf, const void * pData)
{
	if(pData)
		copyBufFrom(pData);
	char key_buf[256];
	/*auto*/ int index = (idx < 0) ? DBTable::index : idx;
	SETIFZ(pKeyBuf, key_buf);
	retBufLen = bufLen;
	char * p_data_buf = (char *)buf;
	SBuffer temp_buf;
	SBuffer lob_buffer;
	size_t  lob_size = 0; // ������ ������ ���������� �����
	int     do_use_chunk_op = 0;
	DBField last_fld;
	if(HasLob(&last_fld) > 0) {
		SLob * p_lob_data = (SLob *)last_fld.getValuePtr();
		int    glsr = getLobSize(last_fld, &lob_size);
		assert(glsr);
		if(p_lob_data->IsPtr()) {
			readLobData(last_fld, lob_buffer);
			assert(lob_buffer.GetAvailableSize() == lob_size);
			if((lob_size+FixRecSize) > MAX_ONEOP_LOBSIZE) {
				//
				// ���� ����� ����� �������, �� ������� ������� �������������
				// �����, � ����� ��������� UPDATE CHUNK ������� �����.
				//
				retBufLen = (uint16)FixRecSize;
				do_use_chunk_op = 1;
			}
			else {
				temp_buf.Write(buf, FixRecSize);
				temp_buf.Write(lob_buffer, lob_buffer.GetAvailableSize());
				p_data_buf = (char *)(const void *)temp_buf;
				// @v9.1.12 @fix retBufLen = (uint16)lob_size;
				retBufLen = (RECORDSIZE)temp_buf.GetAvailableSize(); // @v9.1.12 @fix
			}
		}
		else {
			retBufLen = (int16)(FixRecSize + lob_size);
		}
	}
	else if(HasNote(&last_fld) > 0) {
		size_t tail_len = strlen(((char*)buf) + FixRecSize);
		if(tail_len > 0)
			tail_len++;
		retBufLen = (RECORDSIZE)(FixRecSize + tail_len);
	}
	int result = Ret(BTRV(B_INSERT, FPB, p_data_buf, (uint16 *)&retBufLen, (char *)pKeyBuf, WBTRVTAIL));
	if(result) {
		if(do_use_chunk_op) {
			size_t rest_size = lob_buffer.GetAvailableSize();
			long   rec_offs = FixRecSize;
			while(result && rest_size) {
				long   chunk_size = MIN(rest_size, MAX_ONEOP_LOBSIZE);
				temp_buf.Clear();
				long   item_value;
				temp_buf.Write(item_value = CHUNK_DIR_RAND); // sub operation
				temp_buf.Write(item_value = 1); // number of chunks
				//
				temp_buf.Write(item_value = rec_offs); // offset of chunk in record
				temp_buf.Write(item_value = chunk_size); // length of chunk in record
				temp_buf.Write(item_value = 0); // (ignored) data ptr for indirect chunk op
				//
				temp_buf.Write(lob_buffer.GetBuf(lob_buffer.GetRdOffs()), chunk_size);
				lob_buffer.SetRdOffs(lob_buffer.GetRdOffs() + chunk_size);
				retBufLen = (RECORDSIZE)temp_buf.GetAvailableSize();
                result = Ret(BTRV(B_UPDATECHUNK, FPB, (char *)(const void *)temp_buf, (uint16 *)&retBufLen, (char *)pKeyBuf, WBTRVTAIL));
                rest_size -= chunk_size;
                rec_offs += chunk_size;
			}
		}
	}
	return result;
}

int SLAPI DBTable::Btr_Implement_BExtInsert(BExtInsert * pBei)
{
	int    ok = 1;
	SBaseBuffer sav_buf = getBuffer();
	setBuffer(pBei->GetBuf());
	{
		/*auto*/ int index = -1; // No Change Currency
		char key_buf[256];
		retBufLen = bufLen;
		ok = Ret(BTRV(B_INSERTEXT, FPB, (char *)buf, (uint16 *)&retBufLen, key_buf, WBTRVTAIL));
	}
	setBuffer(sav_buf);
	return ok;
}

int DBTable::Btr_Implement_UpdateRec(const void * pDataBuf, int ncc)
{
	copyBufFrom(pDataBuf);
	/*auto*/ int16 index = ncc ? -1 : DBTable::index;
	char   key_buf[256];
	retBufLen = bufLen;

	char * p_data_buf = (char *)buf;
	SBuffer temp_buf;
	SBuffer lob_buffer;
	size_t  lob_size = 0; // ������ ������ ���������� �����
	int     do_use_chunk_op = 0;
	DBField last_fld;
	if(HasLob(&last_fld) > 0) {
		SLob * p_lob_data = (SLob *)last_fld.getValuePtr();
		int    glsr = getLobSize(last_fld, &lob_size);
		assert(glsr);
		if(p_lob_data->IsPtr()) {
			readLobData(last_fld, lob_buffer);
			assert(lob_buffer.GetAvailableSize() == lob_size);
			if((lob_size+FixRecSize) > MAX_ONEOP_LOBSIZE) {
				//
				// ���� ����� ����� �������, �� ������� ������� �������������
				// �����, � ����� ��������� UPDATE CHUNK ������� �����.
				//
				retBufLen = (uint16)FixRecSize;
				do_use_chunk_op = 1;
			}
			else {
				temp_buf.Write(buf, FixRecSize);
				temp_buf.Write(lob_buffer, lob_buffer.GetAvailableSize());
				p_data_buf = (char *)(const void *)temp_buf;
				// @v9.1.12 @fix retBufLen = (uint16)lob_size;
				retBufLen = (RECORDSIZE)temp_buf.GetAvailableSize(); // @v9.1.12 @fix
			}
		}
		else {
			retBufLen = (int16)(FixRecSize + lob_size);
		}
	}
	else if(HasNote(&last_fld) > 0) {
		size_t tail_len = strlen(((char*)buf) + FixRecSize);
		if(tail_len > 0)
			tail_len++;
		retBufLen = (RECORDSIZE)(FixRecSize + tail_len);
	}
	int result = Ret(BTRV(B_UPDATE, FPB, (char *)p_data_buf, (uint16 *)&retBufLen, key_buf, WBTRVTAIL));
	if(result) {
		if(do_use_chunk_op) {
			size_t rest_size = lob_buffer.GetAvailableSize();
			long   rec_offs = FixRecSize;
			while(result && rest_size) {
				long   chunk_size = MIN(rest_size, MAX_ONEOP_LOBSIZE);
				temp_buf.Clear();
				long   item_value;
				temp_buf.Write(item_value = CHUNK_DIR_RAND); // sub operation
				temp_buf.Write(item_value = 1); // number of chunks
				//
				temp_buf.Write(item_value = rec_offs); // offset of chunk in record
				temp_buf.Write(item_value = chunk_size); // length of chunk in record
				temp_buf.Write(item_value = 0); // (ignored) data ptr for indirect chunk op
				//
				temp_buf.Write(lob_buffer.GetBuf(lob_buffer.GetRdOffs()), chunk_size);
				lob_buffer.SetRdOffs(lob_buffer.GetRdOffs() + chunk_size);
				retBufLen = (RECORDSIZE)temp_buf.GetAvailableSize();
                result = Ret(BTRV(B_UPDATECHUNK, FPB, (char *)(const void *)temp_buf, (uint16 *)&retBufLen, key_buf, WBTRVTAIL));
                rest_size -= chunk_size;
                rec_offs += chunk_size;
			}
		}
	}
	return result;
}

int SLAPI DBTable::Btr_Implement_DeleteRec()
{
	uint16 bl = 0;
	return Ret(BTRV(B_DELETE, FPB, 0, &bl, 0, WBTRVTAIL_ZZ));
}

int SLAPI DBTable::unlock(int isAll)
{
	DBRowId pos;
	/*auto*/  int index = isAll ? -2 : -1;
	uint16 bl = sizeof(pos);
	if(isAll || getPosition(&pos))
		BtrError = BTRV(B_UNLOCK, FPB, (char *)&pos, &bl, 0, WBTRVTAIL_Z);
	InitErrFileName();
	return (BtrError == 0);
}

int SLAPI DBTable::Btr_Encrypt(char * pw, int protectMode)
{
	uint16 bl = (uint16)strlen(pw);
	/*auto*/ int index = protectMode;
	return Ret(BTRV(B_SETOWNER, FPB, pw, &bl, pw, WBTRVTAIL_Z));
}

int SLAPI DBTable::Btr_Decrypt()
{
	return Ret(BTRV(B_CLEAROWNER, FPB, 0, 0, 0, WBTRVTAIL_ZZ));
}

int SLAPI DBTable::getStat(void ** ppInfo, uint16 * pBufSize)
{
	EXCEPTVAR(BtrError);
	*pBufSize = 4096;
	*ppInfo   = 0;
	char  ext_name[80];
	char * p_info = new char[*pBufSize];
	THROW_V(p_info, BE_NOMEM);
	BtrError = BTRV(B_STAT, FPB, p_info, (uint16 *)pBufSize, ext_name, WBTRVTAIL_ZZ);
	THROW(!BtrError);
	THROW_V(*ppInfo = new char[*pBufSize], BE_NOMEM);
	memcpy(*ppInfo, p_info, *pBufSize);
	CATCH

	ENDCATCH
	delete p_info;
	InitErrFileName();
	return (BtrError == 0);
}

int SLAPI DBTable::Btr_GetStat(long reqItems, DbTableStat * pStat)
{
	EXCEPTVAR(BtrError);
	int    ok = 1;
	uint16 buf_size = 4096;
	STempBuffer temp_buf(buf_size);
	char  ext_name[80];
	BtrError = BTRV(B_STAT, FPB, temp_buf, &buf_size, ext_name, WBTRVTAIL_ZZ);
	THROW(!BtrError);

	pStat->Flags = ((DBFileSpec *)(char *)temp_buf)->Flags;
	pStat->RetItems |= DbTableStat::iFlags;
	pStat->NumRecs = ((DBFileSpec *)(char *)temp_buf)->NumRecs;
	pStat->RetItems |= DbTableStat::iNumRecs;
	pStat->FixRecSize = ((DBFileSpec *)(char *)temp_buf)->RecSize;
	pStat->RetItems |= DbTableStat::iFixRecSize;
	pStat->IdxCount = ((DBFileSpec *)(char *)temp_buf)->NumKeys;
	pStat->PageSize = (uint32)((DBFileSpec *)(char *)temp_buf)->PageSize; // @v6.6.10
	pStat->RetItems |= DbTableStat::iIdxCount;

	{
		pStat->IdxList.reset();
		DBIdxSpec * p = (DBIdxSpec *)(temp_buf+sizeof(DBFileSpec));
		for(uint j = 0; j < pStat->IdxCount; j++) {
			BNKey key;
			do {
				for(uint k = 0; k < fields.getCount(); k++) {
					if(fields[k].Offs == (p->position-1)) {
						key.addSegment(k, p->flags);
						break;
					}
				}
			} while((p++)->flags & XIF_SEG);
			pStat->IdxList.addKey(key);
		}
		pStat->RetItems |= DbTableStat::iIdxList;
	}
	CATCHZOK
	InitErrFileName();
	return ok;
}

int SLAPI DBTable::getNumRecs(RECORDNUMBER * pNumRecs)
{
	DBFileSpec * p_info;
	uint16 buf_size;
	int    ok;
	if((ok = getStat((void **)&p_info, &buf_size)) != 0) {
		*pNumRecs = p_info->NumRecs;
		delete p_info;
	}
	return ok;
}

int SLAPI DBTable::getNumKeys(int16 * pNumKeys)
{
	int    ok;
	DBFileSpec * p_info;
	uint16 buf_size;
	if((ok = getStat((void **)&p_info, &buf_size)) != FALSE) {
		*pNumKeys = p_info->NumKeys;
		delete p_info;
	}
	return ok;
}

DBIdxSpec * SLAPI DBTable::getIndexSpec(int idxNo, int * pNumSeg)
{
	DBIdxSpec * p_idx = 0;
	DBFileSpec * p_info;
	uint16 buf_size;
	ASSIGN_PTR(pNumSeg, 0);
	if(getStat((void **)&p_info, &buf_size)) {
		if(idxNo < p_info->NumKeys) {
			int i;
			DBIdxSpec * p = (DBIdxSpec *)(p_info+1);
			for(i = 0; i < idxNo; i++) {
				while(p->flags & XIF_SEG)
					p++;
				p++;
			}
			i = 0;
			do {
				i++;
				p_idx = (DBIdxSpec *)SAlloc::R(p_idx, sizeof(DBIdxSpec) * i);
				p_idx[i-1] = *p;
			} while((p++)->flags & XIF_SEG);
			ASSIGN_PTR(pNumSeg, i);
		}
		delete p_info;
	}
	return p_idx;
}

int SLAPI DBTable::getTabFlags(int16 * pFlags)
{
	DBFileSpec * p_info = 0;
	uint16 buf_size;
	int    res;
	if((res = getStat((void **)&p_info, &buf_size)) != 0) {
		*pFlags = p_info->Flags;
		delete p_info;
	}
	return res;
}
//
//
//
SLAPI ChunkHeader::ChunkHeader(long pos, long func, long aNum)
{
	recPos = pos;
	subFunc = func;
	numChunks = aNum;
}

SLAPI RandChunkItem::RandChunkItem(long ofs, long aLen, void * p)
{
	offset = ofs;
	len = aLen;
	ptr = (long)p;
}

SLAPI RectChunk::RectChunk(long ofs, long rs, long pDist, long p, long aDist)
{
	offset = ofs;
	rowSize = rs;
	ptrDist = pDist;
	ptr = p;
	appDist = aDist;
}

ChunkHeader & SLAPI operator + (ChunkHeader & h, RandChunkItem & i)
{
	ChunkHeader * p_tmp = (ChunkHeader *)catmem(&h, sizeof(ChunkHeader), &i, sizeof(RandChunkItem));
	delete & h;
	p_tmp->numChunks++;
	return *p_tmp;
}

ChunkHeader & SLAPI operator + (ChunkHeader & h, RectChunk & c)
{
	ChunkHeader * p_tmp = (ChunkHeader *)catmem(&h, sizeof(ChunkHeader), &c, sizeof(RectChunk));
	delete & h;
	return *p_tmp;
}

int SLAPI DBTable::getChunk(ChunkHeader * pChunk, int lock)
{
	/*auto*/ int index = 0xfffe; // for substitute in WBTRVTAIL_Z
	size_t chunkInfoSize = sizeof(ChunkHeader);
	long   cf = pChunk->subFunc & ~CHUNK_NEXTINREC;
	if(cf == CHUNK_DIR_RAND || cf == CHUNK_INDIR_RAND)
		chunkInfoSize += sizeof(RandChunkItem) * (int)pChunk->numChunks;
	else
		chunkInfoSize += sizeof(RectChunk);
	if(chunkInfoSize <= bufLen) {
		retBufLen = bufLen;
		memcpy(buf, pChunk, chunkInfoSize);
		BtrError = BTRV(B_GETDIRECT + lock, FPB, (char *)buf, (uint16 *)&retBufLen, 0, WBTRVTAIL_Z);
	}
	else
		BtrError = BE_UBUFLEN;
	InitErrFileName();
	return (BtrError == 0);
}

int SLAPI DBTable::updateChunk()
{
	char   key[256];
	retBufLen = bufLen;
	return Ret(BTRV(B_UPDATECHUNK, FPB, (char *)buf, (uint16 *)&retBufLen, key, WBTRVTAIL));
}
//
// ARG(repPos OUT): @#[0..10000]
//
int SLAPI DBTable::findPercentage(void * key, int16 * relPos)
{
	//RECORDNUMBER pos;
	DBRowId pos;
	uint16 bl = sizeof(RECORDNUMBER);
	int saveIndex = index;
	if(key == 0) {
		if(!getPosition(&pos))
			return 0;
		index = -1;
	}
	else
		pos = 0;
	BtrError = BTRV(B_FINDPERCENT, FPB, (char *)&pos, &bl, (char *)key, WBTRVTAIL);
	index = saveIndex;
	InitErrFileName();
	if(BtrError == 0) {
		*relPos = swapw((uint16)pos);
		return 1;
	}
	return 0;
}

/* relPos = 0..10000 keyIndex = keyNo || -1 (for phisical positioning) */
int SLAPI DBTable::getByPercentage(int16 relPos, int keyIndex)
{
	char   keyBuf[255];
	retBufLen = bufLen;
	int    saveIndex = index;
	index  = keyIndex;
	relPos = swapw(relPos);
	BtrError = BTRV(B_GETBYPERCENT, FPB, (char *)&relPos, (uint16 *)&retBufLen, keyBuf, WBTRVTAIL);
	index = saveIndex;
	InitErrFileName();
	return (BtrError == 0);
}

