// ATOLDRV.CPP
// Copyright (c) A.Starodub 2010, 2011, 2013, 2015, 2016
// @codepage windows-1251
// ��������� ��������� ����������� ��������� ��������
//
#include <pp.h>
#pragma hdrstop
#include <comdisp.h>
#include <process.h>
//
//
//
#define RESCODE_NO_ERROR             0L   // ��� ������
#define RESCODE_DVCCMDUNSUPP       -12L   // ��� �������� "������� �� �������������� � ������ ������������ ������������"
#define RESCODE_MODE_OFF           -16L   // �� �������������� � ������ ������ ����������
#define RESCODE_NOPAPER          -3807L   // ��� ������
#define RESCODE_PAYM_LESS_SUM    -3835L   // �������� ����� ������ ����� ����
#define RESCODE_OPEN_SESS_LONG   -3822L   // C���� ������� ����� 24 �����
#define RESCODE_PRINTCONTROLTAPE -3827L   // ���� ������ ����������� �����
#define RESCODE_PRINTREPORT      -3829L   // ���� ������ ������
#define RESCODE_PAYM_LESS_SUM    -3835L   // �������� ����� ������ ����� ����

#define CHKST_CLOSE            0L

#define PRNMODE_PRINT          2L

#define MODE_REGISTER          1L
#define MODE_XREPORT           2L
#define MODE_ZREPORT           3L
#define MODE_EKLZ_REPORT       6L

#define REPORT_TYPE_Z          1L
#define REPORT_TYPE_X          2L

#define PAYTYPE_CASH           0L
#define PAYTYPE_BANKING        3L
//
//
//
struct PrnLineStruc {
	SString PrnBuf;
	SlipLineParam Param;
};

typedef TSCollection<PrnLineStruc> PrnLinesArray;

static void SLAPI WriteLogFile_PageWidthOver(const char * pFormatName)
{
	SString msg_fmt, msg;
	PPLoadText(PPTXT_SLIPFMT_WIDTHOVER, msg_fmt);
	msg.Printf(msg_fmt, pFormatName);
	PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
}

class SCS_ATOLDRV : public PPSyncCashSession {
public:
	SLAPI  SCS_ATOLDRV(PPID n, char * name, char * port);
	SLAPI ~SCS_ATOLDRV();
	virtual int SLAPI PrintCheck(CCheckPacket *, uint flags);
	virtual int SLAPI PrintCheckCopy(CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int SLAPI PrintXReport(const CSessInfo *);
	virtual int SLAPI PrintZReportCopy(const CSessInfo *);
	virtual int SLAPI PrintIncasso(double sum, int isIncome);
	virtual int SLAPI OpenBox();
	virtual int SLAPI CloseSession(PPID sessID);
	virtual int SLAPI GetSummator(double * val);
	virtual int SLAPI AddSummator(double add);
	virtual int SLAPI EditParam(void *);
	virtual int SLAPI CheckForSessionOver();
private:
	virtual int SLAPI InitChannel();
	int  SLAPI Connect();
	int  SLAPI CheckForCash(double sum);
	int  SLAPI Annulate(long mode);
	void SLAPI WriteLogFile(PPID id);
	void SLAPI CutLongTail(char * pBuf);
	void SLAPI CutLongTail(SString & rBuf);
	int  SLAPI AllowPrintOper(uint id);
	int  SLAPI PrintDiscountInfo(CCheckPacket * pPack, uint flags);
	int  SLAPI PrintReport(int withCleaning);
	//int  SLAPI GetCheckInfo(PPBillPacket * pPack, BillTaxArray * pAry, long * pFlags, SString &rName);
	ComDispInterface * SLAPI InitDisp();
	int  SLAPI SetErrorMessage();
	int  SLAPI SetProp(uint propID, bool         propValue);
	int  SLAPI SetProp(uint propID, int          propValue);
	int  SLAPI SetProp(uint propID, long         propValue);
	int  SLAPI SetProp(uint propID, double       propValue);
	int  SLAPI SetProp(uint propID, const char * pPropValue);
	int  SLAPI GetProp(uint propID, bool  * pPropValue);
	int  SLAPI GetProp(uint propID, int   * pPropValue);
	int  SLAPI GetProp(uint propID, long  * pPropValue);
	int  SLAPI GetProp(uint propID, double  * pPropValue);
	int  SLAPI Exec(uint id);
	int  SLAPI ExecOper(uint id);
	enum {
		ShowProperties, // ������
		OpenCheck,
		CloseCheck,
		CancelCheck,
		SetMode,
		ResetMode,
		PrintString,
		PrintHeader,
		Registration,
		Return,
		NewDocument,
		FullCut,
		GetSumm,
		Beep,
		Payment,
		GetCurrentMode,
		GetStatus,
		CashIncome,
		CashOutcome,
		OpenDrawer,
		Report,
		BeginDocument,
		EndDocument,
		SlipDocCharLineLength,
		SlipDocTopMargin,
		SlipDocLeftMargin,
		SlipDocOrientation,
		Mode,           // ���������
		AdvancedMode,
		CheckNumber,
		Caption,
		Quantity,
		Price,
		Department,
		Name,
		CharLineLength,
		TextWrap,
		CheckType,
		TestMode,
		ResultCode,
		ResultDescription,
		CurrentDeviceNumber,
		CheckPaperPresent,
		ControlPaperPresent,
		Password,
		DeviceEnabled,
		Summ,
		CheckState,
		TypeClose,
		OutOfPaper,
		ECRError,
		DrawerOpened,
		ReportType,
		DocNumber,
		PointPosition,
		PrintBarcode,      // @v9.1.4
		Barcode,           // @v9.1.4
		BarcodeType,       // @v9.1.4
		Height,            // @v9.1.4
		PrintBarcodeText,  // @v9.1.4
		AutoSize,          // @v9.1.8
		Alignment,         // @v9.1.8
		Scale,             // @v9.1.8
		PrintPurpose,      // @v9.1.8
		BarcodeControlCode // @v9.1.8
	};

	enum AtolDrvFlags {
		sfConnected     = 0x0001,     // ����������� ����� � �����������, COM-���� �����
		sfOpenCheck     = 0x0002,     // ��� ������
		sfCancelled     = 0x0004,     // �������� ������ ���� �������� �������������
		sfPrintSlip     = 0x0008,     // ������ ����������� ���������
		sfNotUseCutter  = 0x0010,     // �� ������������ �������� �����
		sfUseWghtSensor = 0x0020      // ������������ ������� ������
	};

	static ComDispInterface * P_Disp;
	static int RefToIntrf;

	long   ResCode;
	long   ErrCode;
	long   CheckStrLen;
	long   Flags;
	SString CashierPassword;
	SString AdmPassword;
	// @v7.4.1 PPObjCashNode ObjCashn;
	// @v8.5.3 CCheckCore CCheckTbl;
	// @v8.5.3 CSessionCore CSessTbl;
};

ComDispInterface * SCS_ATOLDRV::P_Disp = 0; // @global
int  SCS_ATOLDRV::RefToIntrf = 0;          // @global

class CM_ATOLDRV : public PPCashMachine {
public:
	SLAPI CM_ATOLDRV(PPID cashID) : PPCashMachine(cashID) {}
	PPSyncCashSession * SLAPI SyncInterface();
};

PPSyncCashSession * SLAPI CM_ATOLDRV::SyncInterface()
{
	PPSyncCashSession * cs = 0;
	if(IsValid()) {
		cs = (PPSyncCashSession*)new SCS_ATOLDRV(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return cs;
}

REGISTER_CMT(ATOLDRV,1,0);

SLAPI SCS_ATOLDRV::SCS_ATOLDRV(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port)
{
	RefToIntrf++;
	SETIFZ(P_Disp, InitDisp());
	Flags = 0;
	if(SCn.Flags & CASHF_NOTUSECHECKCUTTER)
		Flags |= sfNotUseCutter;
	ResCode = RESCODE_NO_ERROR;
}

SLAPI SCS_ATOLDRV::~SCS_ATOLDRV()
{
	if(--RefToIntrf == 0)
		ZDELETE(P_Disp);
}

ComDispInterface * SLAPI SCS_ATOLDRV::InitDisp()
{
	int    r = 0;
	ComDispInterface * p_disp = 0;
	THROW_MEM(p_disp = new ComDispInterface);
	THROW(p_disp->Init("AddIn.FprnM45"));
	THROW(ASSIGN_ID_BY_NAME(p_disp, ShowProperties) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, OpenCheck) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CloseCheck) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CancelCheck) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, SetMode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ResetMode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, PrintString) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, PrintHeader) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Registration) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Return) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, NewDocument) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, FullCut) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, GetSumm) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Beep) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Payment) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, GetCurrentMode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, GetStatus) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CashIncome) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CashOutcome) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, OpenDrawer) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Report) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, BeginDocument) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, EndDocument) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocCharLineLength) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocTopMargin) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocLeftMargin) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocOrientation) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, PrintBarcode) > 0); // @v9.1.4
	// ���������
	THROW(ASSIGN_ID_BY_NAME(p_disp, Mode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, AdvancedMode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CheckNumber) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Caption) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Quantity) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Price) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Department) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Name) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Caption) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CharLineLength) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, TextWrap) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CheckType) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, TestMode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CheckPaperPresent) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ControlPaperPresent) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CheckNumber) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ResultCode) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ResultDescription) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CurrentDeviceNumber) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Password) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, DeviceEnabled) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Summ) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, CheckState) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, TypeClose) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, OutOfPaper) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ECRError) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, DrawerOpened) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, ReportType) > 0);
	THROW(ASSIGN_ID_BY_NAME(p_disp, Barcode) > 0); // @v9.1.4
	THROW(ASSIGN_ID_BY_NAME(p_disp, BarcodeType) > 0); // @v9.1.4
	THROW(ASSIGN_ID_BY_NAME(p_disp, Height) > 0); // @v9.1.4
	THROW(ASSIGN_ID_BY_NAME(p_disp, PrintBarcodeText) > 0); // @v9.1.4
	THROW(ASSIGN_ID_BY_NAME(p_disp, AutoSize) > 0);  // @v9.1.8
	THROW(ASSIGN_ID_BY_NAME(p_disp, Alignment) > 0); // @v9.1.8
	THROW(ASSIGN_ID_BY_NAME(p_disp, Scale) > 0);     // @v9.1.8
	THROW(ASSIGN_ID_BY_NAME(p_disp, PrintPurpose) > 0);       // @v9.1.8
	THROW(ASSIGN_ID_BY_NAME(p_disp, BarcodeControlCode) > 0); // @v9.1.8
	CATCH
		ZDELETE(p_disp);
	ENDCATCH
	return p_disp;
}

// virtual
int SLAPI SCS_ATOLDRV::EditParam(void * pDevNum)
{
	int    ok = 1;
	long   dev_num = (pDevNum) ? *(long*)pDevNum : 1;
	THROW_INVARG(P_Disp);
	THROW(P_Disp->SetProperty(CurrentDeviceNumber, dev_num) > 0);
	THROW(P_Disp->CallMethod(ShowProperties) > 0);
	THROW(P_Disp->GetProperty(CurrentDeviceNumber, &dev_num) > 0);
	ASSIGN_PTR((long*)pDevNum, dev_num);
	CATCH
		SetErrorMessage();
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SCS_ATOLDRV::SetErrorMessage()
{
	int    ok = -1;
	char   err_buf[MAXPATH];
	memzero(err_buf, sizeof(err_buf));
	THROW_INVARG(P_Disp)
	THROW(P_Disp->GetProperty(ResultCode, &ResCode) > 0);
	if(ResCode != RESCODE_NO_ERROR) {
		THROW(P_Disp->GetProperty(ResultDescription, err_buf, sizeof(err_buf) - 1) > 0);
		SCharToOem(err_buf);
		PPSetError(PPERR_ATOL_DRV);
		PPSetAddedMsgString(err_buf);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::InitChannel()
{
	return -1;
}

int SLAPI SCS_ATOLDRV::SetProp(uint propID, bool propValue)
{
	return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::SetProp(uint propID, int propValue)
{
	return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::SetProp(uint propID, long propValue)
{
	return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::SetProp(uint propID, double propValue)
{
	return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::SetProp(uint propID, const char * pPropValue)
{
	return BIN(P_Disp && P_Disp->SetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::GetProp(uint propID, bool  * pPropValue)
{
	return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::GetProp(uint propID, int * pPropValue)
{
	return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::GetProp(uint propID, long * pPropValue)
{
	return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1);
}

int SLAPI SCS_ATOLDRV::GetProp(uint propID, double * pPropValue)
{
	return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1);
}

int	SLAPI SCS_ATOLDRV::PrintDiscountInfo(CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	double amt = R2(fabs(MONEYTOLDBL(pPack->Rec.Amount)));
	double dscnt = R2(MONEYTOLDBL(pPack->Rec.Discount));
	if(flags & PRNCHK_RETURN)
		dscnt = -dscnt;
	if(dscnt > 0.0) {
		double  pcnt = round(dscnt * 100.0 / (amt + dscnt), 1);
		SString prn_str, temp_str;
		SCardCore scc;
		THROW(SetProp(Caption, (prn_str = 0).CatCharN('-', CheckStrLen)));
		THROW(ExecOper(PrintString));
		(temp_str = 0).Cat(amt + dscnt, SFMT_MONEY);
		prn_str = "����� ��� ������"; // @cstr #0
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		THROW(SetProp(Caption, prn_str));
		THROW(ExecOper(PrintString));
		if(scc.Search(pPack->Rec.SCardID, 0) > 0) {
			THROW(SetProp(Caption, (prn_str = "�����").Space().Cat(scc.data.Code))); // @cstr #1
			THROW(ExecOper(PrintString));
			if(scc.data.PersonID && GetPersonName(scc.data.PersonID, temp_str) > 0) {
				(prn_str = "��������").Space().Cat(temp_str.Transf(CTRANSF_INNER_TO_OUTER)); // @cstr #2
				CutLongTail(prn_str);
				THROW(SetProp(Caption, prn_str));
				THROW(ExecOper(PrintString));
			}
		}
		(temp_str = 0).Cat(dscnt, SFMT_MONEY);
		(prn_str = "������").Space().Cat(pcnt, MKSFMTD(0, (flags & PRNCHK_ROUNDINT) ? 0 : 1, NMBF_NOTRAILZ)).CatChar('%'); // @cstr #3
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		THROW(SetProp(Caption, prn_str));
		THROW(ExecOper(PrintString));
	}
	CATCHZOK
	return ok;
}

void SLAPI SCS_ATOLDRV::CutLongTail(char * pBuf)
{
	char * p = 0;
	if(pBuf && strlen(pBuf) > (uint)CheckStrLen) {
		pBuf[CheckStrLen + 1] = 0;
		if((p = strrchr(pBuf, ' ')) != 0)
			*p = 0;
		else
			pBuf[CheckStrLen] = 0;
	}
}

void SLAPI SCS_ATOLDRV::CutLongTail(SString & rBuf)
{
	char   buf[256];
	rBuf.CopyTo(buf, sizeof(buf));
	CutLongTail(buf);
	rBuf = buf;
}

void SLAPI SCS_ATOLDRV::WriteLogFile(PPID id)
{
	if(P_Disp && (CConfig.Flags & CCFLG_DEBUG)) {
		long   mode = 0, adv_mode = 0;
		size_t pos = 0;
		SString msg_fmt, msg, oper_name, mode_descr;
		SString err_msg = DS.GetConstTLA().AddedMsgString;
		PPLoadText(PPTXT_LOG_SHTRIH, msg_fmt);
		P_Disp->GetNameByID(id, oper_name);
		GetProp(Mode, &mode);
		mode_descr.Cat(mode);
		mode_descr.ToOem();
		if(err_msg.StrChr('\n', &pos))
			err_msg.Trim(pos);
		GetProp(AdvancedMode, &adv_mode);
		msg.Printf(msg_fmt, (const char *)oper_name, (const char *)err_msg, (const char*)mode_descr, adv_mode);
		PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
	}
}

static int IsModeOffPrint(int mode)
{
	return BIN(oneof4(mode, MODE_REGISTER, MODE_XREPORT, MODE_ZREPORT, MODE_EKLZ_REPORT));
}

int SLAPI SCS_ATOLDRV::CheckForSessionOver()
{
	int    ok = -1;
	long   err_code = 0L;
	THROW(Connect());
	SetProp(Mode, MODE_REGISTER);
	Exec(SetMode);
	// Exec(NewDocument);
	if(ResCode == RESCODE_OPEN_SESS_LONG)
		ok = 1;
	CATCHZOK
	Exec(ResetMode);
	return ok;
}

int SLAPI SCS_ATOLDRV::AllowPrintOper(uint id)
{
	//
	// ������� AllowPrintOper ����������� �� ����� ����������,
	//  ������� ����� ���������� ��� ������ ����.
	// ��� ��������: 1 - �������� ������ ���������, 0 - ���������.
	//
	int    ok = 1;
	bool   is_chk_rbn = true, is_jrn_rbn = true, out_paper = false;
	int    wait_prn_err = 0, continue_print = 0;
	long   mode = 0L, adv_mode = 0L, chk_state = CHKST_CLOSE, last_res_code = ResCode;

	SetErrorMessage();
	// �������� ��������� �������� ������
	do {
		THROW(Exec(GetStatus));
		THROW(GetProp(Mode, &mode));
		if(last_res_code == RESCODE_NOPAPER)
			wait_prn_err = 1;
	} while(oneof2(last_res_code, RESCODE_PRINTCONTROLTAPE, RESCODE_PRINTREPORT));
	{
		continue_print = BIN(last_res_code == RESCODE_NOPAPER);
		// �� ������ ������ ��������, ��� ��� ������
		// (����� ��� ���� �������� �������� ���� ����������: ��� ��� ������ ��� ���)
		THROW(GetProp(CheckState, &chk_state));
		if(chk_state != CHKST_CLOSE)
			Flags |= sfOpenCheck;
		// �������� �������� ������� ����� ��� ������ �� ������, ����� ������ �������� ���
		while(ok && last_res_code == RESCODE_NOPAPER || (last_res_code == RESCODE_MODE_OFF && IsModeOffPrint(mode))) {
			int  send_msg = 0, r = 0;
			THROW(Exec(GetStatus));
			THROW(GetProp(CheckPaperPresent,   &is_chk_rbn));
			THROW(GetProp(ControlPaperPresent, &is_jrn_rbn));
			if(!is_chk_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_CHK_RBN);
				send_msg = 1;
			}
			else if(!is_jrn_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_JRN_RBN);
				send_msg = 1;
			}
			else
				WriteLogFile(id);
			Exec(Beep);
			wait_prn_err = 1;
			r = PPError();
			if((!send_msg && r != cmOK) || (send_msg && Exec(Beep) &&
				PPMessage(mfConf|mfYesNo, PPCFM_SETPAPERTOPRINT) != cmYes)) {
				Flags |= sfCancelled;
				ok = 0;
			}
			THROW(Exec(GetStatus));
			THROW(GetProp(ECRError, &last_res_code));
		}
		// ���������, ���� �� ��������� ������ ����� �������� �����
		if(continue_print) {
			WriteLogFile(id);
			THROW(ExecOper(id));
			THROW(Exec(GetStatus));
			THROW(GetProp(Mode,         &mode));
			THROW(GetProp(AdvancedMode, &adv_mode));
			wait_prn_err = 1;
		}
		//
		// ���, �������, �� ������� �� "������", � ��������� �������
		// ���������� � ���������� ��������, ������������ ��-�� ���� ��������� ����.
		//
		if(chk_state != CHKST_CLOSE && id == FullCut) {
			WriteLogFile(id);
			SString  err_msg(DS.GetConstTLA().AddedMsgString), added_msg;
			if(PPLoadText(PPTXT_APPEAL_CTO, added_msg))
				err_msg.CR().Cat("\003").Cat(added_msg);
			PPSetAddedMsgString(err_msg);
			Exec(Beep);
			PPError();
			ok = -1;
		}
	}
	//
	// ���� �������� �� ������� ��������������� � ��������� ������, ������ ��������� �� ������
	// ��� �������� ���� - ����� ������ ������ ����� ���� - �� ������� � ��������� ������, �� wait_prn_err == 1
	//
	if(!wait_prn_err || last_res_code == RESCODE_PAYM_LESS_SUM) {
		WriteLogFile(id);
		Exec(Beep);
		PPError();
		Flags |= sfCancelled;
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::Exec(uint id)
{
	int ok = 1;
	THROW_INVARG(P_Disp);
	THROW(SetProp(Password, CashierPassword));
	THROW(P_Disp->CallMethod(id) > 0);
	THROW(SetErrorMessage() == -1);
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::ExecOper(uint id)
{
	int    ok = 1;
	THROW(P_Disp/* && P_Disp->SetProperty(Password, CashierPassword) > 0 && SetErrorMessage() == -1*/); // @debug
	do {
		THROW(P_Disp->CallMethod(id) > 0);
		THROW(P_Disp->GetProperty(ResultCode, &ResCode) > 0);
		if(ResCode == RESCODE_DVCCMDUNSUPP) {
			ok = -1;
			break;
		}
	} while(ResCode != RESCODE_NO_ERROR && (ok = AllowPrintOper(id)) > 0);
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::Annulate(long mode)
{
	int    ok = 1;
	long   check_state = 0L;
	THROW(GetProp(CheckState, &check_state));
	if(check_state != 0) {
		if(mode)
			THROW(SetProp(Mode, mode));
		THROW(ExecOper(SetMode));
		THROW(ExecOper(CancelCheck) > 0);
		if(mode)
			THROW(ExecOper(ResetMode));
		THROW(ExecOper(FullCut));
	}
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::Connect()
{
	int    ok = 1;
	bool   enabled = false;
	SString buf, buf1;
	PPIniFile ini_file;
	THROW_PP(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_SHTRIHFRPASSWORD, buf) > 0, PPERR_SHTRIHFRADMPASSW);
	buf.Divide(',', CashierPassword, buf1);
	AdmPassword = CashierPassword;
	GetProp(DeviceEnabled, &enabled);
	if(enabled == false) {
		THROW(SetProp(CurrentDeviceNumber, atol(Port)) > 0);
		THROW(SetProp(DeviceEnabled, true) > 0);
		THROW(SetProp(Password, CashierPassword));
		THROW(Annulate(MODE_REGISTER));
	}
	CATCHZOK
	return ok;
}

int SLAPI SCS_ATOLDRV::PrintCheck(CCheckPacket * pPack, uint flags)
{
	int    ok = 1, is_format = 0;
	bool   enabled = true;
	SString buf, temp_buf;
	SString debug_log_buf;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW_INVARG(pPack);
	debug_log_buf.CatEq("CCID", pPack->Rec.ID).CatDiv(';', 2).CatEq("CCCode", pPack->Rec.Code);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		SlipDocCommonParam sdc_param;
		double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
		double sum = fabs(pPack->_Cash);
		double running_total = 0.0;
		double fiscal = 0.0, nonfiscal = 0.0;
		THROW(Connect());
		pPack->HasNonFiscalAmount(&fiscal, &nonfiscal);
		fiscal = fabs(fiscal); // @v8.5.9
		nonfiscal = fabs(nonfiscal); // @v8.5.9
		if(flags & PRNCHK_LASTCHKANNUL) {
			THROW(Annulate(MODE_REGISTER));
		}
		if(flags & PRNCHK_RETURN && !(flags & PRNCHK_BANKING)) {
			int is_cash = 0;
			THROW(is_cash = CheckForCash(amt));
			THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
		}
		THROW(SetProp(Mode, MODE_REGISTER));
		THROW(ExecOper(NewDocument));
		THROW(GetProp(CharLineLength, &CheckStrLen));
		{
			int chk_no = 0;
			THROW(GetProp(CheckNumber, &chk_no));
			pPack->Rec.Code = chk_no;
		}
		if(P_SlipFmt) {
			int    prn_total_sale = 1;
			int    r = 0;
			SString line_buf, format_name = (flags & PRNCHK_RETURN) ? "CCheckRet" : "CCheck";
			SlipLineParam sl_param;
			THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
			if(r > 0) {
				is_format = 1;
				if(sdc_param.PageWidth > (uint)CheckStrLen)
					WriteLogFile_PageWidthOver(format_name);
				THROW(SetProp(CheckType, (flags & PRNCHK_RETURN) ? 2L : 1L));
				THROW(ExecOper(OpenCheck));
				Flags |= sfOpenCheck;
				debug_log_buf.Space().CatChar('{');
				for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					if(sl_param.Flags & SlipLineParam::fRegFiscal) {
						const double _q = sl_param.Qtty;
						const double _p = fabs(sl_param.Price); // @v8.5.9 fabs
						running_total += (_q * _p);
						{
							(temp_buf = sl_param.Text).Strip();
							if(!temp_buf.NotEmptyS())
								temp_buf = "WARE";
							else
								temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
							temp_buf.Trim(CheckStrLen);
							THROW(SetProp(Name, temp_buf)); // ������������ ������
						}
						{
							const double pq = R3(_q);
							const double pp = R2(_p);
							debug_log_buf.CatChar('[').CatEq("QTY", pq).Space().CatEq("PRICE", pp).CatChar(']');
							THROW(SetProp(Quantity, pq));
							THROW(SetProp(Price, pp));
						}
						THROW(SetProp(Department, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 :  (int32)sl_param.DivID));
						THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
						Flags |= sfOpenCheck;
						prn_total_sale = 0;
					}
					// @v9.1.4 {
					else if(sl_param.Kind == sl_param.lkBarcode) {
						;
					}
					else if(sl_param.Kind == sl_param.lkSignBarcode) {
						if(line_buf.NotEmptyS()) {
							{
								int    atol_bctype = 0;
								if(sl_param.BarcodeStd == BARCSTD_CODE39)
									atol_bctype = 1;
								else if(sl_param.BarcodeStd == BARCSTD_UPCA)
									atol_bctype = 0;
								else if(sl_param.BarcodeStd == BARCSTD_UPCE)
									atol_bctype = 4;
								else if(sl_param.BarcodeStd == BARCSTD_EAN13)
									atol_bctype = 2;
								else if(sl_param.BarcodeStd == BARCSTD_EAN8)
									atol_bctype = 3;
								else if(sl_param.BarcodeStd == BARCSTD_ANSI)
									atol_bctype = 6;
								else if(sl_param.BarcodeStd == BARCSTD_CODE93)
									atol_bctype = 7;
								else if(sl_param.BarcodeStd == BARCSTD_CODE128)
									atol_bctype = 8;
								else if(sl_param.BarcodeStd == BARCSTD_PDF417)
									atol_bctype = 10;
								else if(sl_param.BarcodeStd == BARCSTD_QR)
									atol_bctype = 84;
								else
									atol_bctype = 84; // QR by default
								THROW(SetProp(BarcodeType, atol_bctype));
							}
							THROW(SetProp(Barcode, line_buf));
							if(sl_param.BarcodeHt > 0) {
								THROW(SetProp(Height, sl_param.BarcodeHt));
							}
							else {
								THROW(SetProp(Height, 50));
							}
							if(sl_param.Flags & sl_param.fBcTextBelow) {
								THROW(SetProp(PrintBarcodeText, 2));
							}
							else if(sl_param.Flags & sl_param.fBcTextAbove) {
								THROW(SetProp(PrintBarcodeText, 1));
							}
							else {
								THROW(SetProp(PrintBarcodeText, 0));
							}
							THROW(SetProp(AutoSize, true)); // @v9.1.8
							THROW(SetProp(Alignment, (int)1)); // @v9.1.8
							THROW(SetProp(Scale, (int)300)); // @v9.1.8
							THROW(SetProp(PrintPurpose, (int)1));       // @v9.1.8
							THROW(SetProp(BarcodeControlCode, false)); // @v9.1.8
							THROW(ExecOper(PrintBarcode));
						}
					}
					// } @v9.1.4
					else {
						THROW(SetProp(TextWrap, 1L));
						THROW(SetProp(Caption, line_buf.Trim(CheckStrLen)));
						THROW(ExecOper(PrintString));
					}
				}
				if(prn_total_sale) {
					if(!pPack->GetCount()) {
						{
							const double pq = 1.0;
							const double pp = fabs(amt); // @v8.5.9 fabs
							debug_log_buf.CatChar('[').CatEq("QTY", pq).Space().CatEq("PRICE", pp).CatChar(']');
							THROW(SetProp(Quantity, pq));
							THROW(SetProp(Price, pp));
						}
						THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
						Flags |= sfOpenCheck;
						running_total += amt;
					}
					else if(fiscal) {
						{
							const double pq = 1.0;
							const double pp = fabs(fiscal); // @v8.5.9 fabs
							debug_log_buf.CatChar('[').CatEq("QTY", pq).Space().CatEq("PRICE", pp).CatChar(']');
							THROW(SetProp(Quantity, pq));
							THROW(SetProp(Price, pp));
						}
						THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
						Flags |= sfOpenCheck;
						running_total += fiscal;
					}
				}
				else if(running_total > amt) {
					SString fmt_buf, msg_buf, added_buf;
					PPLoadText(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf);
					(added_buf = 0).Cat(running_total, MKSFMTD(0, 20, NMBF_NOTRAILZ)).CatChar('>').Cat(amt, MKSFMTD(0, 20, NMBF_NOTRAILZ));
					msg_buf.Printf(fmt_buf, added_buf.cptr());
					PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				}
				debug_log_buf.CatChar('}');
			}
		}
		if(!is_format) {
			CCheckLineTbl::Rec ccl;
			(buf = 0).CatCharN('-', CheckStrLen);
			for(uint pos = 0; pPack->EnumLines(&pos, &ccl) > 0;) {
				int  division = (ccl.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? ccl.DivID - CHECK_LINE_IS_PRINTED_BIAS : ccl.DivID;
				GetGoodsName(ccl.GoodsID, buf);
				buf.Strip().Transf(CTRANSF_INNER_TO_OUTER).Trim(CheckStrLen);
				THROW(SetProp(Name, buf));                                     // ������������ ������
				THROW(SetProp(Price, R2(intmnytodbl(ccl.Price) - ccl.Dscnt))); // ����
				THROW(SetProp(Quantity, R3(fabs(ccl.Quantity))));              // ����������
				THROW(SetProp(Department, (division > 16 || division < 0) ? 0 : division));
				THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
				Flags |= sfOpenCheck;
			}
			// ���������� � ������
			THROW(PrintDiscountInfo(pPack, flags));
			(buf = 0).CatCharN('-', CheckStrLen);
			THROW(SetProp(Caption, buf.Trim(CheckStrLen)));
			THROW(ExecOper(PrintString));
		}
		debug_log_buf.Space().CatEq("SUM", sum).Space().CatEq("RUNNINGTOTAL", running_total);
		if(running_total > sum)
			sum = running_total;
		{
			double _paym_bnk = 0;
			if(pPack->AL_Const().getCount()) {
				_paym_bnk = R2(fabs(pPack->AL_Const().Get(CCAMTTYP_BANK)));
			}
			else if(pPack->Rec.Flags & CCHKF_BANKING) {
				if(nonfiscal > 0.0) {
					if(fiscal > 0.0)
						_paym_bnk = R2(fiscal);
				}
				else
					_paym_bnk = R2(sum);
			}
			if(nonfiscal > 0.0) {
				if(fiscal > 0.0) {
					const double _paym_cash = R2(fiscal - _paym_bnk);
					debug_log_buf.Space().CatEq("PAYMBANK", _paym_bnk).Space().CatEq("PAYMCASH", _paym_cash);
					if(_paym_cash > 0.0) {
						THROW(SetProp(Summ, R2(_paym_cash)));
						THROW(SetProp(TypeClose, PAYTYPE_CASH));
						THROW(ExecOper(Payment));
					}
					if(_paym_bnk > 0.0) {
						THROW(SetProp(Summ, R2(_paym_bnk)));
						THROW(SetProp(TypeClose, PAYTYPE_BANKING));
						THROW(ExecOper(Payment));
					}
				}
			}
// @v8.5.9 {
			else {
				const double _paym_cash = R2(sum - _paym_bnk);
				debug_log_buf.Space().CatEq("PAYMBANK", _paym_bnk).Space().CatEq("PAYMCASH", _paym_cash);
				if(_paym_cash > 0.0) {
					THROW(SetProp(Summ, _paym_cash));
					THROW(SetProp(TypeClose, PAYTYPE_CASH));
					THROW(ExecOper(Payment));
				}
				if(_paym_bnk > 0.0) {
					THROW(SetProp(Summ, _paym_bnk));
					THROW(SetProp(TypeClose, PAYTYPE_BANKING));
					THROW(ExecOper(Payment));
				}
			}
// } @v8.5.9
#if 0 // @v8.5.9 {
			else if(flags & PRNCHK_BANKING) {
				double  add_paym = intmnytodbl(pPack->Ext.AddPaym);
				if(add_paym) {
					THROW(SetProp(Summ, R2(sum - amt + add_paym)));
					THROW(SetProp(TypeClose, PAYTYPE_CASH));
					THROW(ExecOper(Payment));
					THROW(SetProp(Summ, R2(amt - add_paym)));
					THROW(SetProp(TypeClose, PAYTYPE_BANKING));
					THROW(ExecOper(Payment));
				}
				else {
					THROW(SetProp(Summ, R2(sum)));
					THROW(SetProp(TypeClose, PAYTYPE_BANKING));
					THROW(ExecOper(Payment));
				}
			}
			else {
				THROW(SetProp(Summ, R2(sum)));
				THROW(SetProp(TypeClose, PAYTYPE_CASH));
				THROW(ExecOper(Payment));
			}
#endif // } 0 @v8.5.9
		}
		THROW(ExecOper(CloseCheck));
		THROW(Exec(ResetMode));
		Flags &= ~sfOpenCheck;
		ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
		if(!(Flags & sfNotUseCutter))
			THROW(ExecOper(FullCut));
		ErrCode = SYNCPRN_NO_ERROR;
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				SString no_print_txt;
				PPLoadText(PPTXT_CHECK_NOT_PRINTED, no_print_txt);
				ErrCode = (Flags & sfOpenCheck) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				PPLogMessage(PPFILNAM_SHTRIH_LOG, CCheckCore::MakeCodeString(&pPack->Rec, no_print_txt), LOGMSGF_TIME|LOGMSGF_USER);
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			Exec(Beep);
			if(Flags & sfOpenCheck)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			PPLogMessage(PPFILNAM_SHTRIH_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER); // @v8.5.3
			ok = 0;
		}
		if(Flags & sfOpenCheck)
			ExecOper(CancelCheck);
		Exec(ResetMode);
	ENDCATCH
	if(CConfig.Flags & CCFLG_DEBUG && debug_log_buf.NotEmptyS()) {
		PPLogMessage(PPFILNAM_DEBUG_LOG, debug_log_buf, LOGMSGF_USER|LOGMSGF_TIME);
	}
	return ok;
}

// virtual
int SLAPI SCS_ATOLDRV::PrintCheckCopy(CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	int     ok = 1;
	{
		SlipDocCommonParam sdc_param;
		THROW_INVARG(pPack);
		THROW(Connect());
		THROW(SetProp(Mode, MODE_REGISTER));
		// THROW(ExecOper(NewDocument));
		THROW(GetProp(CharLineLength, &CheckStrLen));
		if(P_SlipFmt) {
			int   r = 0;
			SString  line_buf, format_name = (pFormatName && pFormatName[0]) ? pFormatName : ((flags & PRNCHK_RETURN) ? "CCheckRetCopy" : "CCheckCopy");
			SlipLineParam sl_param;
			THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
			if(r > 0) {
				SString buf;
				for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					THROW(SetProp(TextWrap, 1L));
					THROW(SetProp(Caption, line_buf.Trim(CheckStrLen)));
					THROW(ExecOper(PrintString));
				}
				THROW(ExecOper(PrintHeader));
				if(!(Flags & sfNotUseCutter))
					THROW(ExecOper(FullCut));
			}
		}
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (Exec(Beep), 0);
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SCS_ATOLDRV::PrintReport(int withCleaning)
{
	int     ok = 1, mode = 0;
	SString cshr_pssw;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	// ������� ������ ����� ������ ��� ������� ��������������
	cshr_pssw = CashierPassword;
	CashierPassword = AdmPassword;
	//
	Flags |= sfOpenCheck;
	THROW(SetProp(Mode, (withCleaning) ? MODE_ZREPORT : MODE_XREPORT));
	THROW(Exec(SetMode));
	THROW(SetProp(ReportType, (withCleaning) ? REPORT_TYPE_Z : REPORT_TYPE_X));
	THROW(ExecOper(Report));
	if(!(Flags & sfNotUseCutter))
		THROW(ExecOper(FullCut));
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = 0;
		}
		else {
			SetErrorMessage();
			ok = (Exec(Beep), 0);
		}
	ENDCATCH
	if(Flags & sfOpenCheck) {
		Flags &= ~sfOpenCheck;
		CashierPassword = cshr_pssw;
	}
	return ok;
}

int SLAPI SCS_ATOLDRV::PrintXReport(const CSessInfo *)
{
	return PrintReport(0);
}

int SLAPI SCS_ATOLDRV::PrintZReportCopy(const CSessInfo * pInfo)
{
	int  ok = -1;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW_INVARG(pInfo);
	THROW(Connect());

	THROW(Connect());
	THROW(SetProp(Mode, MODE_REGISTER));
	THROW(GetProp(CharLineLength, &CheckStrLen));
	if(P_SlipFmt) {
		int   r = 0;
		SString  line_buf, format_name = "ZReportCopy";
		SlipDocCommonParam sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SlipLineParam sl_param;
			if(sdc_param.PageWidth > (uint)CheckStrLen)
				WriteLogFile_PageWidthOver(format_name);
			for(P_SlipFmt->InitIteration(pInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				THROW(SetProp(TextWrap, 1L));
				THROW(SetProp(Caption, line_buf.Trim(CheckStrLen)));
				THROW(ExecOper(PrintString));
			}
			THROW(ExecOper(PrintHeader));
			// THROW(Exec(EndDocument)); // ��������� � ���������� ��������
			if(!(Flags & sfNotUseCutter))
				THROW(ExecOper(FullCut));
		}
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (Exec(Beep), 0);
		}
	ENDCATCH
	return ok;
}

int SLAPI SCS_ATOLDRV::CloseSession(PPID sessID)
{
	return PrintReport(1);
}

int SLAPI SCS_ATOLDRV::GetSummator(double * val)
{
	int    ok = 1;
	double cash_amt = 0.0;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	THROW(ExecOper(GetSumm));
	THROW(GetProp(Summ, &cash_amt));
	CATCH
		ok = (SetErrorMessage(), 0);
	ENDCATCH
	ASSIGN_PTR(val, cash_amt);
	return ok;
}

int SLAPI SCS_ATOLDRV::AddSummator(double)
{
	return 1;
}

int SLAPI SCS_ATOLDRV::CheckForCash(double sum)
{
	double cash_sum = 0.0;
	return GetSummator(&cash_sum) ? ((cash_sum < sum) ? -1 : 1) : 0;
}

int SLAPI SCS_ATOLDRV::OpenBox()
{
	int     ok = -1, is_drawer_open = 0;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW(Connect());
	THROW(Exec(GetStatus));
	THROW(GetProp(DrawerOpened, &is_drawer_open));
	if(!is_drawer_open) {
		THROW(ExecOper(OpenDrawer));
		ok = 1;
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				ErrCode = (Flags & sfOpenCheck) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			Exec(Beep);
			if(Flags & sfOpenCheck)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SLAPI SCS_ATOLDRV::PrintIncasso(double sum, int isIncome)
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	if(!isIncome) {
		int is_cash = 0;
		THROW(is_cash = CheckForCash(sum));
		THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
	}
	THROW(Connect());
	THROW(SetProp(Mode, MODE_REGISTER));
	THROW(ExecOper(SetMode));
	Flags |= sfOpenCheck;
	THROW(SetProp(Summ, sum));
	THROW(ExecOper((isIncome) ? CashIncome : CashOutcome));
	if(!(Flags & sfNotUseCutter)) {
		THROW(ExecOper(FullCut));
	}
	THROW(Exec(ResetMode));
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = (Exec(Beep), 0);
		}
		Exec(ResetMode);
	ENDCATCH
	Flags &= ~sfOpenCheck;
	return ok;
}
