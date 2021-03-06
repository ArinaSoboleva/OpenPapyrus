// SPIP.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2012, 2015, 2016
// Program Interface Paradigm
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

//uuid(52D5E7CA-F613-4333-A04E-125DE29D715F)
//uuid(52D5E7CAF6134333A04E125DE29D715F)

S_GUID & FASTCALL S_GUID::Init(REFIID rIID)
{
	memcpy(Data, &rIID, sizeof(Data));
	return *this;
}

S_GUID::operator GUID & ()
{
	return *(GUID *)Data;
}

int FASTCALL S_GUID::operator == (const S_GUID & s) const
{
	return BIN(memcmp(Data, s.Data, sizeof(Data)) == 0);
}

int FASTCALL S_GUID::operator != (const S_GUID & s) const
{
	return BIN(memcmp(Data, s.Data, sizeof(Data)) != 0);
}

int S_GUID::IsZero() const
{
	return BIN(Data[0] == 0 && Data[1] == 0 && Data[2] == 0 && Data[3] == 0);
}

void S_GUID::SetZero()
{
	memzero(Data, sizeof(Data));
}

static char * format_uuid_part(const uint8 * pData, size_t numBytes, char * pBuf)
{
	if(numBytes == 2)
		sprintf(pBuf, "%04X", *(uint16*)pData);
	else if(numBytes == 4)
		sprintf(pBuf, "%08X", *(uint32*)pData);
	else if(numBytes == 1)
		sprintf(pBuf, "%02X", *pData);
	return pBuf;
}

SString & S_GUID::ToStr(long fmt, SString & rBuf) const
{
	char   temp_buf[64];
	uint   i;
	const uint8 * p_data = (const uint8 *)Data;
	rBuf = 0;
	if(fmt == fmtIDL) {
		rBuf.Cat(format_uuid_part(p_data, 4, temp_buf)).CatChar('-');
		p_data += 4;
		rBuf.Cat(format_uuid_part(p_data, 2, temp_buf)).CatChar('-');
		p_data += 2;
		rBuf.Cat(format_uuid_part(p_data, 2, temp_buf)).CatChar('-');
		p_data += 2;
		for(i = 0; i < 2; i++) {
			rBuf.Cat(format_uuid_part(p_data, 1, temp_buf));
			p_data++;
		}
		rBuf.CatChar('-');
		for(i = 0; i < 6; i++) {
			rBuf.Cat(format_uuid_part(p_data, 1, temp_buf));
			p_data++;
		}
	}
	else if(fmt == fmtPlain) {
		rBuf.Cat(format_uuid_part(p_data, 4, temp_buf));
		p_data += 4;
		rBuf.Cat(format_uuid_part(p_data, 2, temp_buf));
		p_data += 2;
		rBuf.Cat(format_uuid_part(p_data, 2, temp_buf));
		p_data += 2;
		for(i = 0; i < 2; i++) {
			rBuf.Cat(format_uuid_part(p_data, 1, temp_buf));
			p_data++;
		}
		for(i = 0; i < 6; i++) {
			rBuf.Cat(format_uuid_part(p_data, 1, temp_buf));
			p_data++;
		}
	}
	else if(fmt == fmtC) {
		const char * ox = "0x";
		rBuf.Cat(ox).Cat(format_uuid_part(p_data, 4, temp_buf)).Comma();
		p_data += 4;
		rBuf.Cat(ox).Cat(format_uuid_part(p_data, 2, temp_buf)).Comma();
		p_data += 2;
		rBuf.Cat(ox).Cat(format_uuid_part(p_data, 2, temp_buf)).Comma();
		p_data += 2;
		for(uint i = 0; i < 8; i++) {
			rBuf.Cat(ox).Cat(format_uuid_part(p_data, 1, temp_buf));
			if(i < 7)
				rBuf.Comma();
			p_data++;
		}
	}
	return rBuf;
}

int FASTCALL S_GUID::FromStr(const char * pBuf)
{
	int    ok = 1;
	const  char * p = pBuf;
	uint   t = 0;
	char   temp_buf[64];
	while(*p) {
		char   c = *p;
		if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
			temp_buf[t++] = c;
		if(c == ')' || t > (sizeof(temp_buf)-4))
			break;
		p++;
	}
	if(t == 32) {
		size_t i;
		uint8 * p_data = (uint8 *)Data;
		for(i = 0; i < 16; i++)
			p_data[i] = hextobyte(temp_buf + (i << 1));
		for(i = 0; i < 4; i++)
			*(uint16 *)(p_data+i*2) = swapw(*(uint16 *)(p_data+i*2));
		*(uint32 *)p_data = swapdw(*(uint32 *)p_data);
	}
	else {
		memzero(Data, sizeof(Data));
		SLS.SetError(SLERR_INVGUIDSTR, pBuf);
		ok = 0;
	}
	return ok;
}

int S_GUID::Generate()
{
#ifdef _DEBUG
	GUID guid;
	if(CoCreateGuid(&guid) == S_OK) {
		Init(guid);
		S_GUID test;
		SString s_buf;
		ToStr(fmtIDL, s_buf);
		test.FromStr(s_buf);
		assert(test == *this);
		return 1;
	}
	else
		return 0;
#else
	return BIN(CoCreateGuid((GUID *)Data) == S_OK);
#endif
}
//
//
//
SLAPI SVerT::SVerT(int j, int n, int r)
{
	Set(j, n, r);
}

SVerT::operator uint32() const
{
	return ((((uint32)V) << 16) | R);
}

void SLAPI SVerT::Set(uint32 n)
{
	V = (uint16)(n >> 16);
	R = (uint16)(n & 0x0000ffff);
}

int SLAPI SVerT::Get(int * pJ, int * pN, int * pR) const
{
	int j = (int)(V >> 8);
	int n = (int)(V & 0x00ff);
	int r = (int)R;
	ASSIGN_PTR(pJ, j);
	ASSIGN_PTR(pN, n);
	ASSIGN_PTR(pR, r);
	return 1;
}

int SLAPI SVerT::Set(int j, int n, int r)
{
	V = (((uint16)j) << 8) | ((uint16)n);
	R = (uint16)r;
	return 1;
}

int SLAPI SVerT::IsLt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	if(V < v2.V)
		return 1;
	if(V == v2.V && R < v2.R)
		return 1;
	return 0;
}

int SLAPI SVerT::IsGt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	if(V > v2.V)
		return 1;
	if(V == v2.V && R > v2.R)
		return 1;
	return 0;
}

int SLAPI SVerT::IsEq(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return (V == v2.V && R == v2.R) ? 1 : 0;
}

int SLAPI SVerT::Cmp(const SVerT * pVer) const
{
	int ok = 0;
	if(pVer) {
		if(V > pVer->V)
			ok = 1;
		else if(V < pVer->V)
			ok = -1;
		else if(R > pVer->R)
			ok = 1;
		else if(R < pVer->R)
			ok = -1;
		else
			ok = 0;
	}
	return ok;
}

SString FASTCALL SVerT::ToStr(SString & rBuf) const
{
	int    j, n, r;
	Get(&j, &n, &r);
	return (rBuf = 0).CatDotTriplet(j, n, r);
}

int FASTCALL SVerT::FromStr(const char * pStr)
{
	int    ok = 0;
	int    j = 0, n = 0, r = 0;
	SString temp_buf;
	SStrScan scan(pStr);
	Set(0, 0, 0);
	if(scan.Skip().GetDigits(temp_buf)) {
		j = (int)temp_buf.ToLong();
		if(scan.Skip()[0] == '.') {
			scan.Incr();
			if(scan.Skip().GetDigits(temp_buf)) {
				n = (int)temp_buf.ToLong();
				if(scan.Skip()[0] == '.') {
					scan.Incr();
					if(scan.Skip().GetDigits(temp_buf)) {
						r = (int)temp_buf.ToLong();
						Set(j, n, r);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI SVerT::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, V, rBuf));
	THROW(pCtx->Serialize(dir, R, rBuf));
	CATCHZOK
	return ok;
}
