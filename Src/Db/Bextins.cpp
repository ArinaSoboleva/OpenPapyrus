// BEXTINS.CPP
// Copyright (c) A.Sobolev 1997-1999, 2000, 2004, 2008, 2009, 2010, 2015
//
// ��������� �������� ����������� ������� �������
//
#include <db.h>
#pragma hdrstop

const size_t BExtInsert::DefBufSize = (28*1024U);

SLAPI BExtInsert::BExtInsert(DBTable * pTbl, size_t aBufSize) : SdRecordBuffer(NZOR(aBufSize, DefBufSize))
{
	State = stValid;
	P_Tbl = pTbl;
	ActualCount = 0xffffU;
	if(!GetBuf().P_Buf)
		State &= ~stValid;
	if(!P_Tbl || !P_Tbl->getRecSize(&FixRecSize)) {
		FixRecSize = 0;
		State &= ~stValid;
	}
	else {
		if(P_Tbl->HasNote(0) > 0)
			State |= stHasNote;
	}
}

SLAPI BExtInsert::~BExtInsert()
{
	flash();
}

int FASTCALL BExtInsert::insert(const void * b)
{
	size_t s;
	if(State & stValid) {
		if(State & stHasNote) {
			const char * p_note = ((const char *)b) + FixRecSize;
			size_t note_len = p_note[0] ? (strlen(p_note)+1) : 0;
			s = FixRecSize + note_len;
		}
		else
			s = FixRecSize;
		int    r = Add(b, s);
		if(r < 0)
			r = flash() ? Add(b, s) : 0;
		if(!r)
			State &= ~stValid;
	}
	return BIN(State & stValid);
}

int SLAPI BExtInsert::flash()
{
	if(State & stValid && GetCount()) {
		int    ok = 1;
		if(P_Tbl->GetDb()) {
			ok = P_Tbl->GetDb()->Implement_BExtInsert(this);
		}
		else {
			ok = P_Tbl->Btr_Implement_BExtInsert(this);
		}
		SETFLAG(State, stValid, ok);
		ActualCount = (State & stValid) ? GetCount() : 0xffffU;
		Reset();
	}
	return BIN(State & stValid);
}

DBTable * SLAPI BExtInsert::getTable()
{
	return P_Tbl;
}

uint SLAPI BExtInsert::getActualCount() const
{
	return ActualCount;
}

