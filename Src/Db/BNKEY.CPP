// BNKEY.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 2003, 2004, 2007, 2008, 2009, 2010, 2013, 2015, 2016
// @codepage windows-1251
//
#include <db.h>
#pragma hdrstop

struct BNKeyHeader {
	int32  numSeg;
	int16  keyNumber;
	int16  acsNumber;
};

struct BNKeySeg {
	int32  fldID;          // Field index in DBTable::fields
	int32  flags;
};

struct BNIndexHeader {
	int32  numKeys;
	uint32 tblBackOffset; // offsetof(DBTable, indexes)
};

union BNKeyCell {
	BNIndexHeader ih;
	BNKeyHeader   h;
	BNKeySeg      s;
};
//
// BNKey implementation
//
#define SEG(i)   ((BNKeyCell *)data)[i+1].s
#define HEAD     ((BNKeyCell *)data)[0].h
#define NUMCELLS (data ? (HEAD.numSeg + 1) : 0)
#define DATALEN  (NUMCELLS * sizeof(BNKeyCell))

SLAPI BNKey::BNKey()
{
	data = 0;
}

void SLAPI BNKey::destroy()
{
	ZFREE(data);
}

int SLAPI BNKey::reset()
{
	if((data = SAlloc::R(data, sizeof(BNKeyCell))) != 0) {
		HEAD.numSeg = HEAD.keyNumber = HEAD.acsNumber = 0;
		return 1;
	}
	else
		return 0;
}

int SLAPI BNKey::addSegment(int fldID, int flags)
{
	BNKeyCell cell;
	if(data == 0)
		reset();
	uint pos = NUMCELLS;
	for(int i = pos - 2; i >= 0; i--)
		if(SEG(i).fldID == fldID)
			return SLS.SetError(SLERR_DUPIDXSEG);
	cell.s.fldID = fldID;
	cell.s.flags = flags;
	if((data = SAlloc::R(data, sizeof(BNKeyCell) * (pos + 1))) != 0) {
		memcpy(((char *)data) + sizeof(BNKeyCell) * pos, &cell, sizeof(BNKeyCell));
		HEAD.numSeg++;
		return 1;
	}
	else
		return 0;
}

int SLAPI BNKey::setFieldID(int seg, int newID)
{
	if(data && seg >= 0 && seg < HEAD.numSeg) {
		SEG(seg).fldID = newID;
		return 1;
	}
	return 0;
}

int SLAPI BNKey::setKeyParams(int keyNumber, int acsNumber)
{
	if(data) {
		if(keyNumber != (int)UNDEF)
			HEAD.keyNumber = keyNumber;
		if(acsNumber != (int)UNDEF)
			HEAD.acsNumber = acsNumber;
		return 1;
	}
	return 0;
}

int SLAPI BNKey::getNumSeg() const
{
	return data ? HEAD.numSeg : 0;
}

int SLAPI BNKey::getKeyNumber() const
{
	return data ? HEAD.keyNumber : UNDEF;
}

int SLAPI BNKey::getACSNumber() const
{
	return data ? HEAD.acsNumber : UNDEF;
}

int SLAPI BNKey::getFieldID(int seg) const
{
	return (data && seg >= 0 && seg < HEAD.numSeg) ? SEG(seg).fldID : UNDEF;
}

int SLAPI BNKey::getFlags(int seg) const
{
	if(seg == (int)UNDEF)
		seg = 0;
	return (data && seg >= 0 && seg < HEAD.numSeg) ? SEG(seg).flags : UNDEF;
}

int SLAPI BNKey::setFlags(int flags, int seg)
{
	int    ok = 0;
	if(data) {
		if(seg == (int)UNDEF) {
			for(int i = 0; i < HEAD.numSeg; i++) {
				int32 * f = &SEG(i).flags;
				*f &= ~XIF_ALLSEGFLAGS;
				*f |= flags;
				if(i < (HEAD.numSeg - 1))
					*f |= XIF_SEG;
				else
					*f &= ~XIF_SEG;
			}
			ok = 1;
		}
		else if(seg >= 0 && seg < HEAD.numSeg) {
			if(seg < (HEAD.numSeg - 1))
				flags |= XIF_SEG;
			SEG(seg).flags |= (flags & ~XIF_ALLSEGFLAGS);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI BNKey::containsField(int fldID, int * pSeg) const
{
	if(data)
		for(int i = 0; i < HEAD.numSeg; i++)
			if(SEG(i).fldID == fldID)
				return ((*pSeg = i), 1);
	return 0;
}
/*
	BNKeyList implementation
*/
SLAPI BNKeyList::BNKeyList()
{
	if((data = SAlloc::M(sizeof(BNKeyCell))) != 0) {
		BNKeyCell * cell = (BNKeyCell *)data;
		cell->ih.numKeys = 0;
		cell->ih.tblBackOffset = 0;
	}
}

SLAPI BNKeyList::~BNKeyList()
{
	reset();
}

void SLAPI BNKeyList::reset()
{
	ZFREE(data);
}

BNKeyList & FASTCALL BNKeyList::operator = (const BNKeyList & s)
{
	copy(&s);
	return *this;
}

int FASTCALL BNKeyList::copy(const BNKeyList * src)
{
	reset();
	uint   sz = sizeof(BNKeyCell) * src->getNumCells();
	if(sz)
		if((data = SAlloc::M(sz)) != 0)
			memcpy(data, src->data, sz);
		else
			return 0;
	return 1;
}

int SLAPI BNKeyList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint32 sz;
	if(dir > 0) {
		sz = sizeof(BNKeyCell) * getNumCells();
		THROW(rBuf.Write(sz));
		if(sz)
			THROW(rBuf.Write(data, sz));
	}
	else if(dir < 0) {
		reset();
		THROW(rBuf.Read(sz));
		if(sz) {
			data = SAlloc::R(data, sz);
			THROW(rBuf.Read(data, sz));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BNKeyList::getNumCells() const
{
	int    count = 0;
	if(data) {
		const  BNKeyCell * cells = (const BNKeyCell *)data;
		int    num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			count += (cells[count].h.numSeg + 1);
	}
	return count;
}

int FASTCALL BNKeyList::findKeyByNumber(int keyNumber) const
{
	int    count = 0;
	if(data) {
#define cells ((BNKeyCell *)data)
		const int num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			if(keyNumber == cells[count].h.keyNumber)
				return count;
			else
				count += (cells[count].h.numSeg + 1);
#undef cells
	}
	return count;
}

int FASTCALL BNKeyList::findKey(int key) const
{
	int    count = 0;
	if(data) {
#define cells ((BNKeyCell *)data)
		const int num_keys = cells[count++].ih.numKeys;
		for(int i = 0; i < num_keys; i++)
			if(key == i)
				return count;
			else
				count += (cells[count].h.numSeg + 1);
#undef cells
	}
	return 0;
}

uint SLAPI BNKeyList::getNumKeys() const
{
	return data ? (uint)((BNKeyCell *)data)->ih.numKeys : 0;
}

int FASTCALL BNKeyList::addKey(BNKey & key)
{
	int    ok = 1;
	BNKeyCell * kc = (BNKeyCell *)key.data;
	if(kc) {
		SArray ary(data, sizeof(BNKeyCell), getNumCells());
		if(data == 0) {
			BNKeyCell cell;
			cell.ih.numKeys = 0;
			cell.ih.tblBackOffset = 0;
			ok = ary.insert(&cell);
		}
		for(int i = 0; ok && i < kc->h.numSeg + 1; i++) {
			if(i > 0)
				if(i < kc->h.numSeg)
					kc[i].s.flags |= XIF_SEG;
				else
					kc[i].s.flags &= ~XIF_SEG;
			if((ok = ary.insert(kc + i)) == 0)
				for(int j = i - 1; j >= 0; i--)	// Rollback work
					ary.atFree(j);
		}
		data = ary.dataPtr();
		if(ok)
			((BNKeyCell *)data)->ih.numKeys++;
	}
	else
		ok = -1;
	key.destroy();
	return ok;
}

int SLAPI BNKeyList::removeKey(int key)
{
	int    ok = 1;
	int    pos = findKey(key);
	if(pos) {
		SArray ary(data, sizeof(BNKeyCell), getNumCells());
		const int num_seg = ((BNKeyCell *)data)[pos].h.numSeg;
		for(int i = 0; i < num_seg + 1; i++)
			ary.atFree(pos);
		data = ary.dataPtr();
	}
	else
		ok = 0;
	return ok;
}

BNKey FASTCALL BNKeyList::getKey(int key) const
{
	int    pos = findKey(key);
	BNKey  k;
	k.data = pos ? (((BNKeyCell *)data) + pos) : 0;
	return k;
}

void FASTCALL BNKeyList::setTableRef(uint ofs)
{
	if(data)
		((BNKeyCell *)data)->ih.tblBackOffset = ofs;
}

DBTable * SLAPI BNKeyList::table() const
{
	return data ? (DBTable *)(((char *)this) - ((BNKeyCell *)data)->ih.tblBackOffset) : 0;
}

const BNField & SLAPI BNKeyList::field(int key, int seg) const
{
	if(data) {
		DBTable * p_tbl = (DBTable *)(((char *)this) - ((BNKeyCell *)data)->ih.tblBackOffset);
		return p_tbl->fields[getKey(key).getFieldID(seg)];
	}
	return *(BNField *)0;
}

uint FASTCALL BNKeyList::getKeySize(int key) const
{
	BNKey  k = getKey(key);
	if(!k)
		return 0;
	else {
		uint   len = 0;
		const  int  ns = k.getNumSeg();
		for(int i = 0; i < ns; i++) {
#ifdef NDEBUG
			len += field(key, i).size();
#else
			const BNField & r_fld = field(key, i);
			len += r_fld.size();
#endif
		}
		return len;
	}
}

uint SLAPI BNKeyList::getSegOffset(int key, int seg) const
{
	if(seg == 0)
		return 0;
	else {
		BNKey  k = getKey(key);
		if(!k)
			return 0;
		else {
			uint   ofs = 0;
			const int n = MIN(seg, k.getNumSeg()-1);
			//
			// ��� ��������� ������������� �� ��, ��� � ������� ������
			// ����� ���������� ���������
			//
			#define MAX_CASE_SEG 5
			switch(n) {
				default:
					{
						for(int i = MAX_CASE_SEG; i < n; i++)
							ofs += field(key, i).size();
					}
				case (MAX_CASE_SEG-0): ofs += field(key, 4).size();
				case (MAX_CASE_SEG-1): ofs += field(key, 3).size();
				case (MAX_CASE_SEG-2): ofs += field(key, 2).size();
				case (MAX_CASE_SEG-3): ofs += field(key, 1).size();
				case (MAX_CASE_SEG-4): ofs += field(key, 0).size();
					break;
			}
			return ofs;
		}
	}
}
//
// Returns length of filled rest of key
//
uint SLAPI BNKeyList::setBound(int key, int seg, int min_max /* 0 - min, !0 - max */ , void * pDest) const
{
	assert(key >= 0 && key < (int)getNumKeys());
	BNKey  k = getKey(key);
	uint   ofs = 0;
	int    ns = k.getNumSeg();
	for(int i = seg; i < ns; i++) {
		const BNField & f = table()->fields[k.getFieldID(i)];
		if(((k.getFlags(i) & XIF_DESC) ? !min_max : min_max) != 0)
			stmaxval(f.T, PTR8(pDest) + ofs);
		else
			stminval(f.T, PTR8(pDest) + ofs);
		ofs += f.size();
	}
	return ofs;
}

#define INVALID_COMPARE_OPERATION 0

int FASTCALL _invertComp(int cmp)
{
	switch(cmp) {
		case _EQ_: return _EQ_;
		case _OUTER_EQ_: return _OUTER_EQ_;
		case _GT_: return _LT_;
		case _LT_: return _GT_;
		case _GE_: return _LE_;
		case _LE_: return _GE_;
		case _NE_: return _NE_;
	}
	//CHECK(INVALID_COMPARE_OPERATION);
	return 0;
}

int SLAPI BNKeyList::makeKey(int key, int seg, int cmp, const void * val, void * dest) const
{
	struct I {
		uchar  low;
		uchar  upp;
	};
	union {
		char * d;
		I    * h;
	};
	int    use = 0;
	uint   ofs, segOfs;
	BNKey  k  = getKey(key);
	uint   ss = table()->fields[k.getFieldID(seg)].size();
	int    ns = k.getNumSeg();
	uint   keyLen;
	d = (char *)dest;
	if(seg == 0) {
		use    = 1;
		segOfs = 0;
		ofs    = sizeof(I);
	}
	/*
		������ � ��������� �������� ����� ���� ������������ � ������ �� ����� ������
		� ��� ������, ���� ���������� ������������ �����������, ������ ����������
		�������� � ����� ������ ����� ����������� ����� ����� �����
	*/
	else if((h->low == _GT_ || h->low == _GE_) && (h->upp == _LT_ || h->upp == _LE_)) {
		uint o = sizeof(I);
		use = 2;
		keyLen = getKeySize(key);
		for(int i = 0; i < seg; i++) {
			const BNField & f = table()->fields[k.getFieldID(i)];
			if(stcomp(f.T, d + o, d + o + keyLen) != 0) {
				use = 0;
				break;
			}
			o += stsize(f.T);
		}
		segOfs = o - sizeof(I);
		ofs = o;
	}
	if(use) {
		h->low = h->upp = 0;
		if(cmp == _EQ_) {
			memmove(d + ofs, val, ss);
			ofs += ss;
			if(ns > (seg + 1)) {
				ofs += setBound(key, seg + 1, 0, d + ofs);
				ofs += segOfs;
				memmove(d + ofs, val, ss);
				ofs += ss;
				ofs += setBound(key, seg + 1, 1, d + ofs);
				h->low = _GE_;
				h->upp = _LE_;
			}
			else
				h->low = _EQ_;
		}
		else {
			if(k.getFlags(seg) & XIF_DESC)
				cmp = _invertComp(cmp);
			if(cmp == _GT_ || cmp == _GE_) {
				memmove(d + ofs, val, ss);
				ofs += ss;
				if(ns > (seg + 1))
					ofs += setBound(key, seg + 1, 0, d + ofs);
				h->low = cmp;
			}
			else if(cmp == _LT_ || cmp == _LE_) {
				if(use == 2)
					ofs += keyLen;
				memmove(d + ofs, val, ss);
				ofs += ss;
				if(ns > (seg + 1))
					ofs += setBound(key, seg + 1, 1, d + ofs);
				if(use == 2)
					memmove(d + sizeof(I), d + sizeof(I) + keyLen, keyLen);
				h->upp = cmp;
			}
			else
				return 0;	// cmp == _NE_
		}
		return 1;
	}
	return 0;
}

int FASTCALL BNKeyList::compareKey(int key, const void * k1, const void * k2) const
{
	return getKey(key).compareKey(table(), k1, k2);
}

int SLAPI BNKey::compareKey(DBTable * tbl, const void * k1, const void * k2) const
{
	int    ns = HEAD.numSeg;
	uint   ofs = 0;
	for(int i = 0; i < ns; i++) {
		BNKeySeg s = SEG(i);
		const BNField & f = tbl->fields[s.fldID];
		int    c = stcomp(f.T, PTR8(k1) + ofs, PTR8(k2) + ofs);
		if(c)
			return (s.flags & XIF_DESC) ? -c : c;
		if(i < (ns - 1))
			ofs += f.size();
	}
	return 0;
}

