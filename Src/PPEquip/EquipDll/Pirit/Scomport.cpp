// SCOMPORT.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2006, 2010, 2011
//
#include <slib.h>
#pragma hdrstop
//
//
//
int SLAPI IsComDvcSymb(const char * pSymb, int * pCount)
{
	int    count = 0;
	int    comdvcs = 0;
	char   temp_buf[32];
	if(pSymb) {
		long   s_com = 0x004D4F43L; // "COM"
		long   s_lpt = 0x0054504CL; // "LPT"
		long   s_prn = 0x004E5250L; // "PRN"
		long   s_con = 0x004E4F43L; // "CON"
		memzero(temp_buf, sizeof(temp_buf));
		if(stricmp(pSymb, (char *)&s_prn) == 0)
			comdvcs = comdvcsPrn;
		else if(stricmp(pSymb, (char *)&s_con) == 0)
			comdvcs = comdvcsCon;
		else if(strnicmp(pSymb, (char *)&s_com, 3) == 0) {
			comdvcs = comdvcsCom;
			if(pSymb[3])
				temp_buf[0] = pSymb[3];
			if(pSymb[4])
				temp_buf[1] = pSymb[4];
			count = atoi(temp_buf);
		}
		else if(strnicmp(pSymb, (char *)&s_lpt, 3) == 0) {
			comdvcs = comdvcsLpt;
			if(pSymb[3])
				temp_buf[0] = pSymb[3];
			count = atoi(temp_buf);
		}
	}
	ASSIGN_PTR(pCount, count);
	return comdvcs;
}

char * SLAPI GetComDvcSymb(int comdvcs, int count, int option, char * pBuf, size_t bufLen)
{
	long   s_com = 0x004D4F43L; // "COM"
	long   s_lpt = 0x0054504CL; // "LPT"
	long   s_prn = 0x004E5250L; // "PRN"
	long   s_con = 0x004E4F43L; // "CON"
	SString temp_buf;
	if(option & 0x0001)
		temp_buf.CatCharN('\\', 2).Dot().CatChar('\\');
	if(comdvcs == comdvcsPrn)
		temp_buf.Cat((char *)&s_prn);
	else if(comdvcs == comdvcsCon)
		temp_buf.Cat((char *)&s_con);
	else if(comdvcs == comdvcsCom)
		temp_buf.Cat((char *)&s_com).Cat(count);
	else if(comdvcs == comdvcsLpt)
		temp_buf.Cat((char *)&s_lpt).Cat(count);
	return temp_buf.CopyTo(pBuf, bufLen);
}
//
//
//
#define SETS            0
#define SEND            1
#define RECV            2
#define RETN            3
//
// combination of abyte
//
#define SCP_DATA_7          0x02
#define SCP_DATA_8          0x03
#define SCP_STOP_1          0x00
#define SCP_STOP_2          0x04
#define SCP_PARITY_NO       0x00
#define SCP_PARITY_ODD      0x08
#define SCP_PARITY_EVEN     0x18
#define SCP_BAUD_110        0x00
#define SCP_BAUD_150        0x20
#define SCP_BAUD_300        0x40
#define SCP_BAUD_600        0x60
#define SCP_BAUD_1200       0x80
#define SCP_BAUD_2400       0xA0
#define SCP_BAUD_4800       0xC0
#define SCP_BAUD_9600       0xE0
//
// upper byte of return cmd value
//
#define TIME_OUT        0x8000
#define TSR_EMPTY       0x4000
#define THR_EMPTY       0x2000
#define BREAK_DETECT    0x1000
#define FRAMING_ERROR   0x0800
#define PARITY_ERROR    0x0400
#define OVERRUN_ERROR   0x0200
#define DATA_READY      0x0100
//
// lower byte of return cmd value
//
#define RLS_DETECT      0x80
#define RING_INDICATOR  0x40
#define DATA_SET_READY  0x20
#define CLEAR_TO_SEND   0x10
#define CHANGE_IN_RLS   0x08
#define TER_DETECTOR    0x04
#define CHANGE_DSR      0x02
#define CHANGE_CTS      0x01

SLAPI SCommPort::SCommPort()
{
	CPP.Cbr = cbr9600;
	CPP.ByteSize = 8;
	CPP.Parity = 0;
	CPP.StopBits = 0;

	MEMSZERO(CPT);
	CPT.Get_NumTries = 2000;
	CPT.Get_Delay = 1;
	CPT.Put_NumTries = 400;
	CPT.Put_Delay = 5;
	CPT.W_Get_Delay = 150;

	ReadCycleCount = 0;
	ReadCycleDelay = 0;
#ifdef __WIN32__
	H_Port = INVALID_HANDLE_VALUE;
#endif
}

SLAPI SCommPort::~SCommPort()
{
#ifdef __WIN32__
	if(H_Port != INVALID_HANDLE_VALUE)
		CloseHandle(H_Port);
#endif
}

int SLAPI SCommPort::SetReadCyclingParams(int cycleCount, int cycleDelay)
{
	ReadCycleCount = cycleCount;
	ReadCycleDelay = cycleDelay;
	return 1;
}

int SLAPI SCommPort::GetParams(CommPortParams * pParams) const
{
	*pParams = CPP;
	return 1;
}

int SLAPI SCommPort::GetTimeouts(CommPortTimeouts * pParam) const
{
	*pParam = CPT;
	return 1;
}

int SLAPI SCommPort::SetParams(const CommPortParams * pParam)
{
	CPP = *pParam;
	return 1;
}

int SLAPI SCommPort::SetTimeouts(const CommPortTimeouts * pParam)
{
	CPT = *pParam;
	return 1;
}

#ifdef __WIN32__

static void __OutLastErr()
{
	DWORD  last_err = GetLastError();
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);
	LocalFree(lpMsgBuf);
}

int SLAPI SCommPort::InitPort(int portNo)
{
	PortNo = portNo;

	int    ok = 1;
	DCB    dcb;
	COMMTIMEOUTS cto;
	char   name[64];

	GetComDvcSymb(comdvcsCom, portNo+1, 1, name, sizeof(name));
	if(H_Port != INVALID_HANDLE_VALUE) {
		CloseHandle(H_Port);
		H_Port = INVALID_HANDLE_VALUE;
	}
	H_Port = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	SLS.SetAddedMsgString(name);
	THROW(H_Port != INVALID_HANDLE_VALUE);
	THROW(GetCommState(H_Port, &dcb));
	switch(CPP.Cbr) {
		case cbr110:    dcb.BaudRate = CBR_110;    break;
		case cbr300:    dcb.BaudRate = CBR_300;    break;
		case cbr600:    dcb.BaudRate = CBR_600;    break;
		case cbr1200:   dcb.BaudRate = CBR_1200;   break;
		case cbr2400:   dcb.BaudRate = CBR_2400;   break;
		case cbr4800:   dcb.BaudRate = CBR_4800;   break;
		case cbr9600:   dcb.BaudRate = CBR_9600;   break;
		case cbr14400:  dcb.BaudRate = CBR_14400;  break;
		case cbr19200:  dcb.BaudRate = CBR_19200;  break;
		case cbr38400:  dcb.BaudRate = CBR_38400;  break;
		case cbr56000:  dcb.BaudRate = CBR_56000;  break;
		case cbr57600:  dcb.BaudRate = CBR_57600;  break;
		case cbr115200: dcb.BaudRate = CBR_115200; break;
		case cbr128000: dcb.BaudRate = CBR_128000; break;
		case cbr256000: dcb.BaudRate = CBR_256000; break;
		default: dcb.BaudRate = CBR_9600; break;
	}
	dcb.fParity  = FALSE;
	dcb.fBinary = 1;
	dcb.Parity   = CPP.Parity;
	dcb.ByteSize = CPP.ByteSize;
	dcb.StopBits = CPP.StopBits;
	// @vmiller {
	/*dcb.fOutxCtsFlow = TRUE;
	dcb.fOutxDsrFlow  = TRUE;
	dcb.fDtrControl = 1;
	dcb.fRtsControl = 1;*/
	// } @vmiller
	THROW(SetCommState(H_Port, &dcb));
	//THROW(GetCommState(H_Port, &dcb)); // @vmiller
	cto.ReadIntervalTimeout = MAXDWORD;
	cto.ReadTotalTimeoutMultiplier = MAXDWORD;
	cto.ReadTotalTimeoutConstant = CPT.W_Get_Delay;
	cto.WriteTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;
	THROW(SetCommTimeouts(H_Port, &cto));
	
	CATCH
		ok = (SLibError = SLERR_COMMINIT, 0);
	ENDCATCH
	return ok;
}

int FASTCALL SCommPort::GetChr(int * pChr)
{
	int    ok = 0;
	int    chr = 0;
	char   buf[32];
	DWORD  sz = 1;
	if(H_Port != INVALID_HANDLE_VALUE) {
		int    r = 0;
		int    cycle_count = (ReadCycleCount > 0) ? ReadCycleCount : 1;
		int    cycle_delay = (ReadCycleDelay > 0) ? ReadCycleDelay : 0;
		int    collision = 0;
		for(int i = 0; !ok && i < cycle_count; i++) {
			r = ReadFile(H_Port, buf, 1, &sz, 0);
			if(r && sz == 1) {
				// @debug {
				if(collision) {
					NumGetColl++;
					if(collision > MaxGetCollIters)
						MaxGetCollIters = collision;
				}
				// } @debug
				chr = buf[0];
				ok = 1;
			}
			else if(cycle_delay) {
				delay(cycle_delay);
				collision++;
			}
		}
		if(r == 0)
			SLS.SetOsError();
		else
			SLS.SetError(SLERR_COMMRCV);
	}
	else
		SLS.SetError(SLERR_COMMRCV);
	ASSIGN_PTR(pChr, chr);
	return ok;
}

int SLAPI SCommPort::GetChr()
{
	int    chr = 0;
	GetChr(&chr);
	return chr;
#if 0 //  {
	int    ok = 0;
	char   buf[32];
	DWORD  sz = 1;
	if(H_Port != INVALID_HANDLE_VALUE) {
		int r = 0;
		int cycle_count = (ReadCycleCount > 0) ? ReadCycleCount : 1;
		int cycle_delay = (ReadCycleDelay > 0) ? ReadCycleDelay : 0;
		int collision = 0;
		for(int i = 0; i < cycle_count; i++) {
			r = ReadFile(H_Port, buf, 1, &sz, 0);
			if(r && sz == 1) {
				// @debug {
				if(collision) {
					NumGetColl++;
					if(collision > MaxGetCollIters)
						MaxGetCollIters = collision;
				}
				// } @debug
				return buf[0];
			}
			else if(cycle_delay) {
				delay(cycle_delay);
				collision++;
			}
		}
		if(r == 0)
			SLS.SetOsError();
		else
			SLS.SetError(SLERR_COMMRCV);
		return 0;
	}
	else
		return (SLibError = SLERR_COMMRCV, 0);
#endif // } 0
}

#if 0 // {

int SLAPI SCommPort::GetChr()
{
	int    ok = 0;
	char   buf[32];
	DWORD  sz = 1;
	if(H_Port != INVALID_HANDLE_VALUE) {
		int r = ReadFile(H_Port, buf, sz, &sz, 0);
		if(r) {
			if(sz == 1)
				return buf[0];
		}
		// else __OutLastErr(); // @debug
	}
	return (SLibError = SLERR_COMMRCV, 0);
}

#endif // } 0

int FASTCALL SCommPort::PutChr(int c)
{
	char   buf[32];
	DWORD  sz = 1;
	buf[0] = c;
	buf[1] = 0;
	int    r = (H_Port != INVALID_HANDLE_VALUE &&
		WriteFile(H_Port, buf, sz, &sz, 0) && sz == 1) ? 1 : (SLibError = SLERR_COMMSEND, 0);
	return r;
}

#else // !__WIN32__

int SLAPI SCommPort::InitPort(int portNo)
{
	PortNo = portNo;
	char st = 0;

	switch(CPP.Cbr) {
		case cbr110:  st |= SCP_BAUD_110;  break;
		case cbr300:  st |= SCP_BAUD_300;  break;
		case cbr600:  st |= SCP_BAUD_600;  break;
		case cbr1200: st |= SCP_BAUD_1200; break;
		case cbr2400: st |= SCP_BAUD_2400; break;
		case cbr4800: st |= SCP_BAUD_4800; break;
		case cbr9600: st |= SCP_BAUD_9600; break;
		default:      st |= SCP_BAUD_9600; break;
	}
	if(CPP.ByteSize == 7)
		st |= SCP_DATA_7;
	else if(CPP.ByteSize == 8)
		st |= SCP_DATA_8;
	else
		st |= SCP_DATA_8;

	if(CPP.Parity == 0)
		st |= SCP_PARITY_NO;
	else if(CPP.Parity == 1)
		st |= SCP_PARITY_ODD;
	else if(CPP.Parity == 2)
		st |= SCP_PARITY_EVEN;
	else
		st |= SCP_PARITY_NO;

	if(CPP.StopBits == 0)
		st |= SCP_STOP_1;
	else if(CPP.StopBits == 2)
		st |= SCP_STOP_2;
	else
		st |= SCP_STOP_1;

	bioscom(SETS, st, PortNo);
	return 1;
}

int SLAPI SCommPort::GetChr()
{
	for(uint wait_count = 0; wait_count++ < CPT.Get_NumTries; delay(CPT.Get_Delay)) {
		int r = bioscom(RETN, 0, PortNo);
		if(r & DATA_READY) {
			r = bioscom(RECV, 0, PortNo);
			return (r & 0x00ff);
		}
		if(CPT.Get_Delay == 0)
			break;
	}
	return (SLibError = SLERR_COMMRCV, 0);
}

int FASTCALL SCommPort::PutChr(int c/*, int direct*/)
{
	/*
	if(direct) {
		bioscom(SEND, c, PortNo);
		return 1;
	}
	else { */
		for(uint wait_count = 0; wait_count++ < CPT.Put_NumTries; delay(CPT.Put_Delay)) {
			int r = bioscom(RETN, 0, PortNo);
			if(r & THR_EMPTY) {
				r = bioscom(SEND, c, PortNo);
				return 1;
			}
			if(CPT.Put_Delay == 0)
				break;
		}
		return (SLibError = SLERR_COMMSEND, 0);
	/* } */
}

#endif // __WIN32__
