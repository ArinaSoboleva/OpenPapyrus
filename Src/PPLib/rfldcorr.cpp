// RFLDCORR.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
// @v9.6.2 (moved to pp.h) #include <ppidata.h>
//
// ������������� ����� �������/�������� �� ���������� SdRecord
//
int SLAPI SetupSdRecFieldCombo(TDialog * dlg, uint ctlID, uint id, const SdRecord * pRec)
{
	StrAssocArray list;
	SdbField fld;
	for(uint i = 0; pRec->EnumFields(&i, &fld);) {
		if(fld.Descr.NotEmpty()) {
			fld.Descr.Transf(CTRANSF_OUTER_TO_INNER);
			list.Add(fld.ID, fld.Descr);
		}
		else
			list.Add(fld.ID, fld.Name);
	}
	return SetupStrAssocCombo(dlg, ctlID, &list, (long)id, 0);
}

int SLAPI SetupBaseSTypeCombo(TDialog * dlg, uint ctlID, int typ)
{
	StrAssocArray list;
	LongArray bt_list;
	bt_list.addzlist(BTS_STRING, BTS_INT, BTS_REAL, BTS_DATE, BTS_TIME, BTS_BOOL, 0);
	SString text;
	for(uint i = 0; i < bt_list.getCount(); i++) {
		const long bt = bt_list.get(i);
		list.Add(bt, GetBaseTypeString(bt, BTSF_NATIVE|BTSF_OEM, text));
	}
	return SetupStrAssocCombo(dlg, ctlID, &list, (long)typ, 0);
}

int SLAPI EditFieldCorr(const SdRecord * pInnerRec, SdbField * pOuterField)
{
	class SdFieldCorrDialog : public TDialog {
	public:
		SdFieldCorrDialog(const SdRecord * pInnerRec) : TDialog(DLG_FLDCORR)
		{
			P_Rec = pInnerRec;
		}
		int    setDTS(const SdbField * pData)
		{
			Data = *pData;
			SString temp_buf;
			SetupSdRecFieldCombo(this, CTLSEL_FLDCORR_INNERFLD, Data.ID, P_Rec);
			if(Data.T.Typ)
				Data.T.Typ = stbase(Data.T.Typ);
			ushort use_formula = BIN(Data.T.Flags & STypEx::fFormula);
			setCtrlData(CTL_FLDCORR_USEFORMULA, &use_formula);
			setCtrlString(CTL_FLDCORR_FORMULA, Data.Formula);
			disableCtrl(CTLSEL_FLDCORR_INNERFLD, use_formula);
			disableCtrl(CTL_FLDCORR_FORMULA, !use_formula);
			SetupBaseSTypeCombo(this, CTLSEL_FLDCORR_OUTERTYPE, Data.T.Typ);
			setCtrlString(CTL_FLDCORR_OUTERFLD, (temp_buf = Data.Name).Transf(CTRANSF_OUTER_TO_INNER));
			setupOuterLen();
			return 1;
		}
		int    getDTS(SdbField * pData)
		{
			int16  sz = 0, prec = 0;
			long   temp_long = getCtrlLong(CTLSEL_FLDCORR_INNERFLD);
			ushort use_formula = getCtrlUInt16(CTL_FLDCORR_USEFORMULA);
			SString temp_buf;
			if(use_formula) {
				getCtrlString(CTL_FLDCORR_FORMULA, Data.Formula);
				if(Data.Formula.Strip().Empty())
					Data.Formula = "@empty";
				temp_long = 0;
			}
			Data.ID = (uint)temp_long;
			SETFLAG(Data.T.Flags, STypEx::fFormula, use_formula);
			getOuterType();
			Data.T.Typ = Data.T.Typ ? bt2st(Data.T.Typ) : 0;
			getCtrlString(CTL_FLDCORR_OUTERFLD, temp_buf);
			Data.Name = temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			getCtrlData(CTL_FLDCORR_OUTERSZ, &sz);
			getCtrlData(CTL_FLDCORR_OUTERPRC, &prec);
			Data.OuterFormat = MKSFMTD(sz, prec, 0);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND) {
				if(event.isCbSelected(CTLSEL_FLDCORR_OUTERTYPE))
					setupOuterLen();
				else if(event.isCbSelected(CTLSEL_FLDCORR_INNERFLD)) {
					long   temp_long = getCtrlLong(CTLSEL_FLDCORR_INNERFLD);
					if(Data.ID != (uint)temp_long) {
						SdbField fld;
						if(P_Rec->GetFieldByID(temp_long, 0, &fld) > 0) {
							Data.ID = (uint)temp_long;
							getCtrlString(CTL_FLDCORR_OUTERFLD, Data.Name);
							if(Data.Name.Empty()) {
								Data.Name = fld.Name;
								setCtrlString(CTL_FLDCORR_OUTERFLD, Data.Name);
							}
							//getOuterType();
							Data.T.Typ = stbase(fld.T.Typ);
							setCtrlLong(CTLSEL_FLDCORR_OUTERTYPE, Data.T.Typ);
							setupOuterLen();
						}
					}
				}
				else if(event.isClusterClk(CTL_FLDCORR_USEFORMULA)) {
					ushort use_formula = getCtrlUInt16(CTL_FLDCORR_USEFORMULA);
					disableCtrl(CTLSEL_FLDCORR_INNERFLD, use_formula);
					disableCtrl(CTL_FLDCORR_FORMULA, !use_formula);
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		void   setupOuterLen()
		{
			int16  sz  = (int16)SFMTLEN(Data.OuterFormat);
			int16  prc = (int16)SFMTPRC(Data.OuterFormat);
			getOuterType();
			if(Data.T.Typ != BTS_REAL) {
				prc = 0;
				disableCtrl(CTL_FLDCORR_OUTERPRC, 1);
			}
			else
				disableCtrl(CTL_FLDCORR_OUTERPRC, 0);
			setCtrlData(CTL_FLDCORR_OUTERSZ,  &sz);
			setCtrlData(CTL_FLDCORR_OUTERPRC, &prc);
		}
		int    getOuterType()
		{
			long   temp_long = getCtrlLong(CTLSEL_FLDCORR_OUTERTYPE);
			Data.T.Typ = (TYPEID)temp_long;
			if(Data.T.Typ == 0) {
				getCtrlData(CTLSEL_FLDCORR_INNERFLD, &Data.ID);
				SdbField inner_fld;
				if(P_Rec->GetFieldByID(Data.ID, 0, &inner_fld) > 0)
					Data.T.Typ = stbase(inner_fld.T.Typ);
			}
			return 1;
		}
		SdbField Data;
		const SdRecord * P_Rec;
	};
	DIALOG_PROC_BODY_P1(SdFieldCorrDialog, pInnerRec, pOuterField);
}
//
//
//
class SdFieldCorrListDialog : public PPListDialog {
public:
	SdFieldCorrListDialog(const SdRecord * pInnerRec) : PPListDialog(DLG_FLDCORRLIST, CTL_FLDCORRLIST_LIST)
	{
		P_Rec = pInnerRec;
	}
	int    setDTS(const SdRecord * pData);
	int    getDTS(SdRecord * pData);
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up);
	const SdRecord * P_Rec;
	SdRecord Data;
};

int SdFieldCorrListDialog::setDTS(const SdRecord * pData)
{
	Data = *pData;
	updateList(-1);
	return 1;
}

int SdFieldCorrListDialog::getDTS(SdRecord * pData)
{
	ASSIGN_PTR(pData, Data);
	return 1;
}

int SdFieldCorrListDialog::moveItem(long pos, long id, int up)
{
	int    ok = -1;
	if(P_Box && P_Box->def) {
		uint   u_pos = (uint)pos;
		if(Data.MoveField(u_pos, up, &u_pos) > 0) {
			updateList(u_pos);
			ok = 1;
		}
	}
	return ok;
}

int SdFieldCorrListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	SdbField fld;
	if(EditFieldCorr(P_Rec, &fld) > 0) {
		SETIFZ(fld.ID, 30000);
		if(Data.AddField(&fld.ID, &fld)) {
			ASSIGN_PTR(pPos, Data.GetCount()-1);
			ASSIGN_PTR(pID, (long)fld.ID);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int SdFieldCorrListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	SdbField fld;
	if(Data.GetFieldByPos(pos, &fld) > 0) {
		// @v7.4.1 fld.Name.ToOem(); // @v7.0.3
		while(ok <= 0 && EditFieldCorr(P_Rec, &fld) > 0) {
			// @v7.4.1 fld.Name.Transf(CTRANSF_INNER_TO_OUTER); // @v7.0.3
			SETFLAG(fld.T.Flags, STypEx::fZeroID, fld.T.Flags & STypEx::fFormula);
			if(fld.ID == 0 && !(fld.T.Flags & STypEx::fZeroID))
				fld.ID = 30000;
			if(Data.UpdateField(pos, &fld))
				ok = 1;
			else
				ok = (PPError(PPERR_SLIB), 0);
		}
	}
	return ok;
}

int SdFieldCorrListDialog::delItem(long pos, long id)
{
	SdbField fld;
	return (Data.GetFieldByPos(pos, &fld) > 0) ? (Data.RemoveField(pos) ? 1 : PPErrorZ()) : -1;
}

int SdFieldCorrListDialog::setupList()
{
	int    ok = 1;
	uint   len, offs = 0;
	SString sub;
	StringSet ss(SLBColumnDelim);
	SdbField fld, inner_fld;
	for(uint i = 0; ok && Data.EnumFields(&i, &fld);) {
		ss.clear(1);
		sub = 0;
		if(fld.T.Flags & STypEx::fFormula)
			sub.CatChar('F').CatChar(':').Cat(fld.Formula);
		else if(P_Rec->GetFieldByID(fld.ID, 0, &inner_fld) > 0)
			sub = inner_fld.Name;
		ss.add(sub);
		// @v7.0.3 ss.add(fld.Name);
		ss.add((sub = fld.Name).Transf(CTRANSF_OUTER_TO_INNER)); // @v7.0.3
		ss.add(GetBaseTypeString(stbase(fld.T.Typ), BTSF_NATIVE|BTSF_OEM, sub));
		len = SFMTLEN(fld.OuterFormat);
		(sub = 0).Cat(len);
		if(SFMTPRC(fld.OuterFormat))
			sub.Dot().Cat(SFMTPRC(fld.OuterFormat));
		ss.add(sub);
		ss.add((sub = 0).Cat(offs));
		offs += len;
		if(!addStringToList(i, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int SLAPI EditFieldCorrList(const SdRecord * pInnerRec, SdRecord * pCorrRec)
{
	DIALOG_PROC_BODY_P1(SdFieldCorrListDialog, pInnerRec, pCorrRec);
}
//
//
//
int EditTextDbFileParam(/*TextDbFile::Param * pData*/ PPImpExpParam * pIeParam)
{
	class TextDbFileParamDialog : public TDialog {
	public:
		TextDbFileParamDialog(PPImpExpParam * pParam) : TDialog(DLG_TXTDBPARAM)
		{
			P_Param = pParam;
			FileBrowseCtrlGroup::Setup(this, CTLBRW_TXTDBPARAM_FNAME, CTL_TXTDBPARAM_FILENAME, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
		}
		int    setDTS(const TextDbFile::Param * pData)
		{
			Data = *pData;
			SString temp_buf;
			setCtrlString(CTL_TXTDBPARAM_FILENAME, Data.DefFileName);
			ushort orient = BIN(Data.Flags & TextDbFile::fVerticalRec);
			setCtrlData(CTL_TXTDBPARAM_ORIENT, &orient);
			AddClusterAssoc(CTL_TXTDBPARAM_FLAGS, 0, TextDbFile::fFldNameRec);
			AddClusterAssoc(CTL_TXTDBPARAM_FLAGS, 1, TextDbFile::fFixedFields);
			AddClusterAssoc(CTL_TXTDBPARAM_FLAGS, 2, TextDbFile::fFldEqVal);
			AddClusterAssoc(CTL_TXTDBPARAM_FLAGS, 3, TextDbFile::fOemText);
			AddClusterAssoc(CTL_TXTDBPARAM_FLAGS, 4, TextDbFile::fQuotText);
			SetClusterData(CTL_TXTDBPARAM_FLAGS, Data.Flags);
			setCtrlData(CTL_TXTDBPARAM_HDRCOUNT, &Data.HdrLinesCount);
			setCtrlString(CTL_TXTDBPARAM_FOOTER, (temp_buf = Data.FooterLine).Transf(CTRANSF_OUTER_TO_INNER)); // @v7.4.1
			onOrientSelection();
			return 1;
		}
		int    getDTS(TextDbFile::Param * pData)
		{
			getCtrlString(CTL_TXTDBPARAM_FILENAME, Data.DefFileName);
			ushort orient = getCtrlUInt16(CTL_TXTDBPARAM_ORIENT);
			SETFLAG(Data.Flags, TextDbFile::fVerticalRec, orient);
			GetClusterData(CTL_TXTDBPARAM_FLAGS, &Data.Flags);
			Data.VertRecTerm = 0;
			Data.FldDiv = 0;
			SString temp_buf;
			getCtrlString(CTL_TXTDBPARAM_DIV, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			if(orient) {
				Data.VertRecTerm = temp_buf;
				getCtrlString(CTL_TXTDBPARAM_FOOTER, temp_buf); // @v7.4.1
				Data.FooterLine = temp_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER);    // @v7.4.1
			}
			else {
				Data.FldDiv = temp_buf;
				Data.FooterLine = 0; // @v7.4.1
			}
			getCtrlData(CTL_TXTDBPARAM_HDRCOUNT, &Data.HdrLinesCount);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmHeaderFields)) {
				if(P_Param)
					EditFieldCorrList(&P_Param->HdrInrRec, &P_Param->HdrOtrRec);
				clearEvent(event);
			}
			else if(event.isClusterClk(CTL_TXTDBPARAM_ORIENT)) {
				ushort orient = getCtrlUInt16(CTL_TXTDBPARAM_ORIENT);
				SETFLAG(Data.Flags, TextDbFile::fVerticalRec, orient);
				onOrientSelection();
				clearEvent(event);
			}
		}
		void   onOrientSelection()
		{
			ushort orient = getCtrlUInt16(CTL_TXTDBPARAM_ORIENT);
			SETFLAG(Data.Flags, TextDbFile::fVerticalRec, orient);
			DisableClusterItem(CTL_TXTDBPARAM_FLAGS, 1, (Data.Flags & TextDbFile::fVerticalRec));
			DisableClusterItem(CTL_TXTDBPARAM_FLAGS, 2, !(Data.Flags & TextDbFile::fVerticalRec));

			SString label_text;
			PPGetSubStr(PPTXT_LAB_TXTDBPARAM_DIV, orient, label_text);
			setLabelText(CTL_TXTDBPARAM_DIV, label_text);
			label_text = orient ? Data.VertRecTerm : Data.FldDiv;
			setCtrlString(CTL_TXTDBPARAM_DIV, label_text.Transf(CTRANSF_OUTER_TO_INNER));
			disableCtrl(CTL_TXTDBPARAM_FOOTER, orient != 1); // @v7.4.1
		}
		TextDbFile::Param Data;
		PPImpExpParam * P_Param;
	};
	DIALOG_PROC_BODY_P1(TextDbFileParamDialog, pIeParam, &pIeParam->TdfParam);
}

int EditXmlDbFileParam(/*XmlDbFile::Param * pData*/PPImpExpParam * pIeParam)
{
	class XmlDbFileParamDialog : public TDialog {
	public:
		XmlDbFileParamDialog(PPImpExpParam * pParam) : TDialog(DLG_XMLDBPARAM), Data(0, 0, 0, 0)
		{
			P_Param = pParam;
		}
		int    setDTS(const XmlDbFile::Param * pData)
		{
			if(!RVALUEPTR(Data, pData))
				Data.Init(0, 0, 0, 0);
			setCtrlString(CTL_XMLDBPARAM_ROOTTAG, Data.RootTag.Transf(CTRANSF_OUTER_TO_INNER));
			setCtrlString(CTL_XMLDBPARAM_RECTAG,  Data.RecTag.Transf(CTRANSF_OUTER_TO_INNER));
			setCtrlString(CTL_XMLDBPARAM_HDRTAG,  Data.HdrTag.Transf(CTRANSF_OUTER_TO_INNER));
			AddClusterAssoc(CTL_XMLDBPARAM_FLAGS, 0, XmlDbFile::Param::fUseDTD);
			AddClusterAssoc(CTL_XMLDBPARAM_FLAGS, 1, XmlDbFile::Param::fUtf8Codepage);
			AddClusterAssoc(CTL_XMLDBPARAM_FLAGS, 2, XmlDbFile::Param::fHaveSubRec);
			SetClusterData(CTL_XMLDBPARAM_FLAGS, Data.Flags);
			return 1;
		}
		int    getDTS(XmlDbFile::Param * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlString(CTL_XMLDBPARAM_ROOTTAG, Data.RootTag);
			Data.RootTag.Transf(CTRANSF_INNER_TO_OUTER);
			getCtrlString(CTL_XMLDBPARAM_RECTAG,  Data.RecTag);
			Data.RecTag.Transf(CTRANSF_INNER_TO_OUTER);
			getCtrlString(CTL_XMLDBPARAM_HDRTAG,  Data.HdrTag);
			Data.HdrTag.Transf(CTRANSF_INNER_TO_OUTER);
			if(Data.RootTag.Len() == 0 || Data.RecTag.Len() == 0)
				Data.Init(Data.RootTag, Data.HdrTag, Data.RecTag, 0);
			sel = CTL_XMLDBPARAM_ROOTTAG;
			THROW_SL(XmlDbFile::CheckParam(Data));
			GetClusterData(CTL_XMLDBPARAM_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmHeaderFields)) {
				if(P_Param) {
					EditFieldCorrList(&P_Param->HdrInrRec, &P_Param->HdrOtrRec);
				}
				clearEvent(event);
			}
		}
		XmlDbFile::Param Data;
		PPImpExpParam * P_Param;
	};
	DIALOG_PROC_BODY_P1(XmlDbFileParamDialog, pIeParam, &pIeParam->XdfParam);
}
//
//
//
int EditXlsDbFileParam(ExcelDbFile::Param * pData)
{
	class XlsDbFileParamDialog : public TDialog {
	public:
		XlsDbFileParamDialog() : TDialog(DLG_XLSDBPARAM)
		{
		}
		int    setDTS(const ExcelDbFile::Param * pData)
		{
			Data = *pData;
			AddClusterAssoc(CTL_XLSDBPARAM_FLAGS, 0, ExcelDbFile::fFldNameRec);
			AddClusterAssoc(CTL_XLSDBPARAM_FLAGS, 1, ExcelDbFile::fQuotText);
			AddClusterAssoc(CTL_XLSDBPARAM_FLAGS, 2, ExcelDbFile::fOneRecPerFile);
			SetClusterData(CTL_XLSDBPARAM_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_XLSDBPARAM_ORIENT,  0, 0);
			AddClusterAssoc(CTL_XLSDBPARAM_ORIENT,  1, ExcelDbFile::fVerticalRec);
			SetClusterData(CTL_XLSDBPARAM_ORIENT, Data.Flags & ExcelDbFile::fVerticalRec);
			setCtrlData(CTL_XLSDBPARAM_HDRCOUNT, &Data.HdrLinesCount);
			setCtrlData(CTL_XLSDBPARAM_COLCOUNT, &Data.ColumnsCount);
			setCtrlData(CTL_XLSDBPARAM_SHEETNUM, &Data.SheetNum);
			setCtrlString(CTL_XLSDBPARAM_SHEETNAME, Data.SheetName_);
			setCtrlString(CTL_XLSDBPARAM_ENDSTR,    Data.EndStr_);
			return 1;
		}
		int    getDTS(ExcelDbFile::Param * pData)
		{
			long   orient = 0;
			GetClusterData(CTL_XLSDBPARAM_FLAGS,  &Data.Flags);
			GetClusterData(CTL_XLSDBPARAM_ORIENT, &orient);
			SETFLAG(Data.Flags, ExcelDbFile::fVerticalRec, orient);
			getCtrlData(CTL_XLSDBPARAM_HDRCOUNT,  &Data.HdrLinesCount);
			getCtrlData(CTL_XLSDBPARAM_COLCOUNT, &Data.ColumnsCount);
			getCtrlData(CTL_XLSDBPARAM_SHEETNUM,  &Data.SheetNum);
			getCtrlString(CTL_XLSDBPARAM_SHEETNAME, Data.SheetName_);
			Data.SheetNum = MAX(Data.SheetNum, 1);
			if(!Data.SheetName_.NotEmptyS()) {
				Data.SheetName_ = 0;
				PPGetWord(PPWORD_SHEET, 0, Data.SheetName_);
				Data.SheetName_.CatChar('1');
			}
			getCtrlString(CTL_XLSDBPARAM_ENDSTR, Data.EndStr_);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		ExcelDbFile::Param Data;
	};
	DIALOG_PROC_BODYERR(XlsDbFileParamDialog, pData);
}
//
//
//
//static
PPImpExpParam * FASTCALL PPImpExpParam::CreateInstance(uint recId, long flags)
{
	PPImpExpParam * p_param = 0;
	SdRecord rec;
	SString temp_buf;
	THROW(LoadSdRecord(recId, &rec, 1));
	p_param = CreateInstance((temp_buf = rec.Name).ToUpper(), flags);
	CATCH
		ZDELETE(p_param);
	ENDCATCH
	return p_param;
}

//static
PPImpExpParam * FASTCALL PPImpExpParam::CreateInstance(const char * pSymb, long flags)
{
	PPImpExpParam * p_param = 0;
	if(!isempty(pSymb)) {
		SString ffn;
		ffn.Cat("IEHF").CatChar('_').Cat(pSymb);
		FN_IMPEXPHDL_FACTORY f = (FN_IMPEXPHDL_FACTORY)GetProcAddress(SLS.GetHInst(), ffn);
		if(f) {
			p_param = f(flags);
			if(p_param)
				p_param->DataSymb = pSymb;
		}
		else
			PPSetError(PPERR_PPIMPEXPHDLUNIMPL, pSymb);
	}
	else
		PPSetError(PPERR_PPIMPEXPHDLSYMBUNDEF);
	return p_param;
}

ImpExpParamDllStruct::ImpExpParamDllStruct()
{
	BeerGrpID = 0;
	AlcoGrpID = 0;
	AlcoLicenseRegID = 0;
	TTNTagID = 0;
	ManufTagID = 0;
	ManufKPPRegTagID = 0;
	RcptTagID = 0;
	ManufRegionCode = 0;
	IsManufTagID = 0;
	GoodsKindTagID = 0;
	ManufINNID = 0;
}

PPImpExpParam::PPImpExpParam(uint recId, long flags) : OtrRec(SdRecord::fAllowDupID), XdfParam(0, 0, 0, 0)
{
	RecId = recId;
	Init();
}

PPImpExpParam::~PPImpExpParam()
{
}

int PPImpExpParam::Init(int import)
{
	int    ok = 1;
	BaseFlags = 0;
	Direction  = BIN(import);
	// @v8.4.6 ImpExpByDll = 0; // @vmiller
	DataFormat = 0;
	FtpAccID = 0;
	Name = 0;
	FileName = 0;
	HdrInrRec.Clear();
	HdrOtrRec.Clear();
	InrRec.Clear();
	OtrRec.Clear();
	TdfParam.Init();
	XdfParam.Init(0, 0, 0, 0);
	XlsdfParam.Init();
	THROW(LoadSdRecord(PPREC_IMPEXPHEADER, &HdrInrRec)); // @v7.2.6
	CATCHZOK
	return ok;
}

int PPImpExpParam::ProcessName(int op, SString & rName) const
{
	//#define IMP_PREFX  "IMP"
	//#define EXP_PREFX  "EXP"
	const char * p_prefix_imp = "IMP";
	const char * p_prefix_exp = "EXP";

	int    ok = -1;
	SString rec_prefx, temp_buf;
	switch(op) {
		case 1: // decorate name
			(rec_prefx = InrRec.Name).ToUpper().CatChar('@');
			temp_buf = (Direction ? p_prefix_imp : p_prefix_exp);
			rName = temp_buf.CatChar('@').Cat(rec_prefx).Cat(rName);
			ok = 1;
			break;
		case 2: // undecorate name
			(rec_prefx = InrRec.Name).ToUpper().CatChar('@');
			temp_buf = rName;
			if(temp_buf.CmpPrefix(p_prefix_imp, 1) == 0 || temp_buf.CmpPrefix(p_prefix_exp, 1) == 0)
				temp_buf.ShiftLeft(strlen(Direction ? p_prefix_imp : p_prefix_exp)+1);
			if(temp_buf.CmpPrefix(rec_prefx, 1) == 0)
				temp_buf.ShiftLeft(rec_prefx.Len());
			rName = temp_buf;
			ok = 1;
			break;
		case 3: // check decorated name
			(rec_prefx = InrRec.Name).ToUpper().CatChar('@');
			temp_buf  = rName;
			ok = 0;
			if(temp_buf.CmpPrefix(p_prefix_imp, 1) == 0 || temp_buf.CmpPrefix(p_prefix_exp, 1) == 0) {
				temp_buf.ShiftLeft(strlen(Direction ? p_prefix_imp : p_prefix_exp)+1);
				if(temp_buf.CmpPrefix(rec_prefx, 1) == 0)
					ok = 1;
			}
			break;
		case 4:
			break;
	}
	return ok;
}

//virtual
int PPImpExpParam::MakeExportFileName(const void * extraPtr, SString & rResult) const
{
	rResult = 0;

	int    ok = 1;
	int    use_ps = 0;
	char   cntr[128];
	uint   cn = 0;
	if(FileName.CmpNC(":buffer:") == 0) {
		rResult = FileName;
	}
	else {
		SString temp_buf;
		SPathStruc ps, ps_temp;
		ps.Split(FileName);
		if(ps.Drv.Empty() && ps.Dir.Empty()) {
			PPGetPath(PPPATH_OUT, temp_buf);
			ps_temp.Split(temp_buf);
			ps.Drv = ps_temp.Drv;
			ps.Dir = ps_temp.Dir;
			use_ps = 1;
		}
		{
			size_t gp = 0;
			if(ps.Nam.Search("#guid", 0, 1, &gp)) {
				S_GUID guid;
				guid.Generate();
				guid.ToStr(S_GUID::fmtIDL, temp_buf = 0);
				ps.Nam.ReplaceStr("#guid", temp_buf, 0);
				use_ps = 1;
			}
		}
		const uint fnl = ps.Nam.Len();
		for(uint i = 0; i < fnl; i++)
			if(ps.Nam.C(i) == '?')
				cntr[cn++] = '0';
		if(cn) {
			SString nam;
			int    overflow = 1;
			do {
				overflow = 1;
				for(int j = cn-1; overflow && j >= 0; j--) {
					if(cntr[j] < '9') {
						cntr[j]++;
						// @v8.6.6 @fix {
						for(int z = j+1; z < (int)cn; z++)
							cntr[z] = '0';
						// } @v8.6.6 @fix
						overflow = 0;
					}
				}
				nam = 0;
				for(uint i = 0, k = 0; i < fnl; i++) {
					if(ps.Nam.C(i) == '?')
						nam.CatChar(cntr[k++]);
					else
						nam.CatChar(ps.Nam.C(i));
				}
				ps_temp.Copy(&ps, 0xffff);
				ps_temp.Nam = nam;
				ps_temp.Merge(rResult);
			} while(!overflow && fileExists(rResult));
			if(overflow)
				ok = PPSetError(PPERR_EXPFNTEMPLATEOVERFLOW, FileName);
			else
				ok = 100; // ��� ������� �� �������
		}
		else {
			if(use_ps)
				ps.Merge(rResult);
			else
				rResult = FileName;
			ok = 1; // ��� ����� �������� ��� ����
		}
	}
	return ok;
}

//virtual
int PPImpExpParam::PreprocessImportFileSpec(const SString & rFileSpec, PPFileNameArray & rList)
{
	SPathStruc ps;
	ps.Split(rFileSpec);
	SString wildcard, path;
	(wildcard = ps.Nam).Dot().Cat(ps.Ext);
	ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, path);
	return rList.Scan(path, wildcard);
}

//virtual
int PPImpExpParam::PreprocessImportFileName(const SString & rFileName, StrAssocArray & rResultList)
{
	return -1;
}

int PPImpExpParam::GetFilesFromSource(const char * pWildcard, PPLogger * pLogger)
{
	int    ok = -1;
	if(Direction /*import*/ && FtpAccID) {
		PPObjInternetAccount ia_obj;
		PPInternetAccount ia_pack;
		if(ia_obj.Get(FtpAccID, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
			StrAssocArray file_list;
			SString ftp_path, file_name;
			SPathStruc ps;
			ps.Split(FileName);
			if(pWildcard)
				file_name = pWildcard;
			else {
                (file_name = ps.Nam).Dot().Cat(ps.Ext);
			}
			ia_pack.GetExtField(FTPAEXSTR_HOST, ftp_path);
			ftp_path.SetLastSlash();
			//
			WinInetFTP ftp;
			THROW(ftp.Init());
			THROW(ftp.Connect(&ia_pack));
			THROW(ftp.GetFileList(ftp_path, &file_list, file_name));
			{
				SString dest_file_name;
				for(uint i = 0; i < file_list.getCount(); i++) {
					StrAssocArray::Item item = file_list.at(i);
                    (file_name = ftp_path).Cat(item.Txt);
					PPGetFilePath(PPPATH_IN, item.Txt, dest_file_name);
                    THROW(ftp.SafeGet(dest_file_name, file_name, 1, 0, pLogger));
                    ok = 1;
				}
			}
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	return ok;
}

int PPImpExpParam::DistributeFile(PPLogger * pLogger)
{
	int    ok = -1;
	if(!Direction /*export*/ && FtpAccID) {
		PPObjInternetAccount ia_obj;
		PPInternetAccount ia_pack;
		if(ia_obj.Get(FtpAccID, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
			if(fileExists(FileName)) {
				SString ftp_path, naked_file_name;
				{
					SPathStruc ps;
					ps.Split(FileName);
					ps.Drv = 0;
					ps.Dir = 0;
					ps.Merge(naked_file_name);
					ia_pack.GetExtField(FTPAEXSTR_HOST, ftp_path);
					ftp_path.SetLastSlash().Cat(naked_file_name);
				}
				WinInetFTP ftp;
				THROW(ftp.Init());
				THROW(ftp.Connect(&ia_pack));
				THROW(ftp.SafePut(FileName, ftp_path, 0, 0, pLogger));
				ok = 1;
			}
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	return ok;
}

enum {
	iefFileName=1,    // filename    ��� ����� �������/��������
	iefImpExp,        // impexp      (import,imp;export,exp)
	iefFormat,        // format      ������ ����� (txt,text,csv;dbf)
	iefOrient,        // orient      ���������� ������� ���������� ����� (horizontal,horiz;vertical,vert)
	iefCodepage,      // codepage    ������� ��������
	iefOemText,       // oemcoding   ��������� ���� � OEM-���������
	iefQuotStr,       // quotstr     ��������� ���� ��������� �������� ���������
	iefFieldDiv,      // fielddiv    ������-����������� �����
	iefFixedFields,   // fixedfields ���� �������������� �������
	iefFieldEqVal,    // fieldeqval  ������������ ���� � �������� ��������� �������� '='
	iefFormula,       // formula()   �������� ���� �������������� �� �������
	iefRootTag,       // roottag     ������������ ��������� ���� (XML)
	iefRecTag,        // rectag      ������������ ���� ������ (XML)
	iefFldNameRec,    // fldnamerec  fFldNameRec ������ ������������ ����� ��� ���������� �����
	iefUseDTD,        // fusedtd     ������������ DTD � xml ������
	iefUtf8Codepage,  // fuseutf8    ������������ ��������� UTF8 � xml ������
	iefSubRec,        // fsubrec     ������ ������ �������� ��������� (�����������) �� ��������� � ��������
	iefHdrLinesCount, // hdrlinescount ���������� ���������� ����� � ������ ���������� �����
	iefSheetNum,      // sheetnum    ����� ����� excel
	iefSheetName,     // sheetname   ������������ ����� excel
	iefXlsFldNameRec, // xlsfldnamerec fFldNameRec ������ ������������ ����� ��� ���������� ����� (excel)
	iefXlsQuotStr,    // xlsquotstr   ��������� ���� ��������� �������� ��������� (excel)
	iefXlsOrient,     // xlsorient ���������� ������� � ����� (horizontal,horiz;vertical,vert)
	iefOneRecPerFile, // onerecperfile ���� ������ � �����
	iefEndStr,        // endstr ������� ��������� ������
	iefXlsHdrLinesCount, // xlshdrlinescount ���������� ���������� ����� � ������ ����� excel
	iefColumnsCount,     // columnscount ���������� ���������� �������� � ������ ����� excel
	iefHdrTag,           // hdrtag      ������������ ���� ��������� (XML)
	iefHdrFormula,       // hdrformula() �������� ���� ��������� �������������� �� �������
	iefFooterLine,        // footerline   ����������� ������ ���������� ����� � ������������ ���������� ����� (TXT)
	iedllDllPath,	  // dllpath	  ���� � dll-����� �������/��������
	iedllReceiptTag,  // receipttag	  �� ����, �� �������� � �������� ����� ��������� �����
	iedllGlobalName,  // globalname	  ����� ��� ����� � ������� ���������� ���
	iedllBeerGrpId,	  // beergrpid	  �� ������ ������� "����"
	iedllAlcoLicenseRegId,  // alcolicseregid   �� �������� ������������� � ������� "�������� �� ��������"
	iedllTTNTagId,    // ttntagid   �� ���� � ������� ���
	iedllManufTagId,  // manuftagid   �� ���� ���������/������������
	iedllManufKPPRegTagId,  // manufkppregtagid   �� �������� ������������� � ������� ���
	iedllManufRegionCode,  // manufregioncode   ��� ������� �� ������ �������������/���������
	iedllIsManufTagId,	  // ismanuftagid   ���� 1, �� ����������-�������������, 2 - ����������-��������
	iedllGoodsKindSymb,  // goodskindsymb   �� ������ ��������� ������ ��� ������: x, y, z, w
	iedllXmlPrefix,   // xmlprefix   ������� ����� xml-���������
	iedllAlcoGrpId,	  // alcogrpid	  �� ������ ������� "��������"
	iedllGoodsKindTagID, // goodskindtagid   �� ���� ���� � ����� � ���� ������
	iedllOperType,   // opertype   ��� �������� �������/�������� (ORDER, ORDRSP, APERAK, DESADV, RESADV)
	iedllGoodsVolSymb, // goodsvolsymb   �� ������ ��������� ������ ������� ������: x, y, z ,w
	iedllManufINNID,   // manufinnid   �� ���� ���� � ����� �� ��� ����������/���������
	iefBaseFlags,      // ����, �������������� ������� �������� ����� ����� �������� �������
	iefFtpAccSymb,     // @v8.6.1 ������ FTP-��������
	iefFtpAccID        // @v8.6.1 ������������� FTP-��������
};

//virtual
int PPImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		rHdr.ID = 0;
		rHdr.Ver = 0;
		rHdr.Type = Direction ? PPConfigDatabase::tImport : PPConfigDatabase::tExport;
		rHdr.Flags = 0;
		rHdr.Name = Name;
		rHdr.SubSymb = InrRec.Name;
		rHdr.DbSymb = 0;
		rHdr.OwnerSymb = GlobalUserName;
		rTail.Clear();
	}
	else if(dir < 0) {
		THROW(oneof2(rHdr.Type, PPConfigDatabase::tImport, PPConfigDatabase::tExport));
		Direction = (rHdr.Type == PPConfigDatabase::tImport) ? 1 : 0;
		Name = rHdr.Name;
		GlobalUserName = rHdr.OwnerSymb;
	}
	THROW_SL(pSCtx->Serialize(dir, DataFormat, rTail));
	THROW_SL(pSCtx->Serialize(dir, BaseFlags, rTail)); // @v8.4.6
	if(DataFormat == dfText) {
		THROW_SL(TdfParam.Serialize(dir, rTail, pSCtx));
	}
	else if(DataFormat == dfDbf) {
	}
	else if(DataFormat == dfXml) {
		THROW_SL(XdfParam.Serialize(dir, rTail, pSCtx));
	}
	else if(DataFormat == dfSoap) {
	}
	else if(DataFormat == dfExcel) {
		THROW_SL(XlsdfParam.Serialize(dir, rTail, pSCtx));
	}
	THROW_SL(pSCtx->Serialize(dir, FileName, rTail));
	THROW_SL(HdrOtrRec.Serialize(dir, rTail, pSCtx));
	THROW_SL(HdrInrRec.Serialize(dir, rTail, pSCtx));
	THROW_SL(OtrRec.Serialize(dir, rTail, pSCtx));
	THROW_SL(InrRec.Serialize(dir, rTail, pSCtx));
	if(dir < 0) {
		//
		// ������������� ������������ ����� ������ ������� � ���������� �������� �� �������������.
		//
		uint i;
		SdbField fld, inner_fld;
		for(i = 0; HdrOtrRec.EnumFields(&i, &fld);) {
			uint fld_pos = 0;
			if(fld.T.Flags & STypEx::fFormula) {
				if(fld.ID != 0) {
					fld.ID = 0;
					HdrOtrRec.UpdateField(i-1, &fld);
				}
			}
			else if(!HdrInrRec.GetFieldByID(fld.ID, &fld_pos, &inner_fld)) {
				if(fld.ID != 0) {
					fld.ID = 0;
					HdrOtrRec.UpdateField(i-1, &fld);
				}
			}
		}
		for(i = 0; OtrRec.EnumFields(&i, &fld);) {
			uint fld_pos = 0;
			if(fld.T.Flags & STypEx::fFormula) {
				if(fld.ID != 0) {
					fld.ID = 0;
					OtrRec.UpdateField(i-1, &fld);
				}
			}
			else if(!InrRec.GetFieldByID(fld.ID, &fld_pos, &inner_fld)) {
				if(fld.ID != 0) {
					fld.ID = 0;
					OtrRec.UpdateField(i-1, &fld);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	const  int preserve_win_coding = BIN(pFile->GetFlags() & SIniFile::fWinCoding); // @v7.0.3
	SString temp_buf, fld_buf, symb_buf;
	SdbField fld, inner_fld;
	PPSymbTranslator tsl_par(PPSSYM_IMPEXPPAR);
	pFile->SetFlag(SIniFile::fWinCoding, 0); // @v7.0.3
	pFile->ClearSection(pSect);
	if(FileName.NotEmpty()) {
		THROW(tsl_par.Retranslate(iefFileName, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, FileName, 1));
	}
	if(Direction >= 0) {
		THROW(tsl_par.Retranslate(iefImpExp, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, (Direction ? "IMP" : "EXP"), 1));
	}
	// @v8.4.6 {
	if(BaseFlags) {
		THROW(tsl_par.Retranslate(iefBaseFlags, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BaseFlags, 1));
	}
	// } @v8.4.6
	// @v8.6.1 {
	if(FtpAccID) {
        PPObjInternetAccount ia_obj;
        PPInternetAccount ia_pack;
        if(ia_obj.Get(FtpAccID, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
			temp_buf = ia_pack.Symb;
			if(temp_buf.NotEmptyS()) {
				THROW(tsl_par.Retranslate(iefFtpAccSymb, symb_buf));
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));
			}
			else {
				THROW(tsl_par.Retranslate(iefFtpAccID, symb_buf));
				THROW(pFile->AppendIntParam(pSect, symb_buf, FtpAccID, 1));
			}
        }
        else {
        	//
        	// ������ ��������, ��� ����� �������� ����������� ������ ���� ������, ������ ��������� ��� ��� ���������
        	//
			THROW(tsl_par.Retranslate(iefFtpAccID, symb_buf));
			THROW(pFile->AppendIntParam(pSect, symb_buf, FtpAccID, 1));
        }
	}
	// } @v8.6.1
	{
		THROW(tsl_par.Retranslate(iefFormat, symb_buf));
		if(DataFormat == dfText)
			temp_buf = "TXT";
		else if(DataFormat == dfDbf)
			temp_buf = "DBF";
		else if(DataFormat == dfXml)
			temp_buf = "XML";
		else if(DataFormat == dfExcel)
			temp_buf = "XLS";
		else
			temp_buf = 0;
		if(temp_buf.NotEmpty())
			THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));
	}
	if(TdfParam.Flags & TextDbFile::fVerticalRec) {
		THROW(tsl_par.Retranslate(iefOrient, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, "VERTICAL", 1));
		if(TdfParam.FooterLine.NotEmpty()) {
			THROW(tsl_par.Retranslate(iefFooterLine, symb_buf));
			THROW(pFile->AppendParam(pSect, symb_buf, TdfParam.FooterLine, 1));
		}
	}
	{
		THROW(tsl_par.Retranslate(iefOemText, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(TdfParam.Flags & TextDbFile::fOemText)));
		THROW(tsl_par.Retranslate(iefFldNameRec, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(TdfParam.Flags & TextDbFile::fFldNameRec)));
	}
	if(TdfParam.Flags & TextDbFile::fQuotText) {
		THROW(tsl_par.Retranslate(iefQuotStr, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, 1));
	}
	{
		THROW(tsl_par.Retranslate(iefHdrLinesCount, symb_buf));
		(temp_buf = 0).Cat(TdfParam.HdrLinesCount);
		THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));
	}
	{
		THROW(tsl_par.Retranslate(iefFieldDiv, symb_buf));
		temp_buf = (TdfParam.Flags & TextDbFile::fVerticalRec) ? TdfParam.VertRecTerm : TdfParam.FldDiv;
		THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));
	}
	if(TdfParam.Flags & TextDbFile::fFixedFields) {
		THROW(tsl_par.Retranslate(iefFixedFields, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, 1));
	}
	{
		THROW(tsl_par.Retranslate(iefFieldEqVal, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(TdfParam.Flags & TextDbFile::fFldEqVal)));
	}
	{
		THROW(tsl_par.Retranslate(iefRootTag, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, XdfParam.RootTag, 0));
		THROW(tsl_par.Retranslate(iefRecTag, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, XdfParam.RecTag, 0));
		THROW(tsl_par.Retranslate(iefHdrTag, symb_buf)); // @v7.2.6
		THROW(pFile->AppendParam(pSect, symb_buf, XdfParam.HdrTag, 0));      // @v7.2.6
		THROW(tsl_par.Retranslate(iefUseDTD, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XdfParam.Flags & XmlDbFile::Param::fUseDTD)));
		THROW(tsl_par.Retranslate(iefUtf8Codepage, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XdfParam.Flags & XmlDbFile::Param::fUtf8Codepage)));
		THROW(tsl_par.Retranslate(iefSubRec, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XdfParam.Flags & XmlDbFile::Param::fHaveSubRec)));
		{
			for(uint i = 0; HdrOtrRec.EnumFields(&i, &fld);)
				if(fld.T.Flags & STypEx::fFormula) {
					THROW(tsl_par.Retranslate(iefHdrFormula, symb_buf));
					temp_buf = symb_buf;
					temp_buf.Space().CatParStr(fld.Formula);
					fld.PutToString(3, fld_buf);
					THROW(pFile->AppendParam(pSect, temp_buf, fld_buf, 0));
				}
				else {
					THROW_SL(HdrInrRec.GetFieldByID(fld.ID, 0, &inner_fld));
					fld.PutToString(3, fld_buf);
					THROW(pFile->AppendParam(pSect, inner_fld.Name, fld_buf, 1));
				}
		}
	}
	{
		for(uint i = 0; OtrRec.EnumFields(&i, &fld);)
			if(fld.T.Flags & STypEx::fFormula) {
				THROW(tsl_par.Retranslate(iefFormula, symb_buf));
				temp_buf = symb_buf;
				temp_buf.Space().CatParStr(fld.Formula);
				fld.PutToString(3, fld_buf);
				THROW(pFile->AppendParam(pSect, temp_buf, fld_buf, 0));
			}
			else {
				THROW_SL(InrRec.GetFieldByID(fld.ID, 0, &inner_fld));
				fld.PutToString(3, fld_buf);
				THROW(pFile->AppendParam(pSect, inner_fld.Name, fld_buf, 1));
			}
	}
	// Excel Params {
	{
		if(XlsdfParam.Flags & ExcelDbFile::fVerticalRec) {
			THROW(tsl_par.Retranslate(iefXlsOrient, symb_buf));
			THROW(pFile->AppendParam(pSect, symb_buf, "VERTICAL", 1));
		}

		THROW(tsl_par.Retranslate(iefXlsFldNameRec, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XlsdfParam.Flags & ExcelDbFile::fFldNameRec)));

		THROW(tsl_par.Retranslate(iefXlsQuotStr, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XlsdfParam.Flags & ExcelDbFile::fQuotText)));

		THROW(tsl_par.Retranslate(iefOneRecPerFile, symb_buf));
		THROW(pFile->AppendIntParam(pSect, symb_buf, BIN(XlsdfParam.Flags & ExcelDbFile::fOneRecPerFile)));

		THROW(tsl_par.Retranslate(iefXlsHdrLinesCount, symb_buf));
		(temp_buf = 0).Cat(XlsdfParam.HdrLinesCount);
		THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));

		THROW(tsl_par.Retranslate(iefColumnsCount, symb_buf));
		(temp_buf = 0).Cat(XlsdfParam.ColumnsCount);
		THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));

		THROW(tsl_par.Retranslate(iefSheetNum, symb_buf));
		(temp_buf = 0).Cat(XlsdfParam.SheetNum);
		THROW(pFile->AppendParam(pSect, symb_buf, temp_buf, 1));

		THROW(tsl_par.Retranslate(iefSheetName, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, XlsdfParam.SheetName_, 1));

		THROW(tsl_par.Retranslate(iefEndStr, symb_buf));
		THROW(pFile->AppendParam(pSect, symb_buf, XlsdfParam.EndStr_, 1));
	}
	// }
	CATCHZOK
	pFile->SetFlag(SIniFile::fWinCoding, preserve_win_coding);
	return ok;
}

//virtual
int PPImpExpParam::Edit()
{
	return -1;
}

//virtual
int PPImpExpParam::Select()
{
	return -1;
}

int PPImpExpParam::ParseFormula(int hdr, SString & rPar, SString & rVal)
{
	int    ok = 1;
	size_t pos = 0;
	SdbField outer_fld;
	const char * p = rPar.StrChr('(', &pos);
	if(p) {
		p++;
		while(*p && *(p + 1) != 0) // ���������� ��������� ������ ')'
			outer_fld.Formula.CatChar(*p++);
	}
	SStrScan scan(rVal);
	outer_fld.ID = 0;
	outer_fld.T.Flags = (STypEx::fFormula | STypEx::fZeroID);
	THROW_SL(outer_fld.TranslateString(scan));
	if(GETSTYPE(outer_fld.T.Typ) == S_ZSTRING && GETSSIZE(outer_fld.T.Typ) == 0) {
		size_t len = SFMTLEN(outer_fld.OuterFormat);
		len++;
		if(len & 0x3)
			len = ((len >> 2) << 2) + 4;
		if(len > 255)
			len = 255;
		outer_fld.T.Typ = MKSTYPE(S_ZSTRING, len);
	}
	if(hdr) {
		THROW_SL(HdrOtrRec.AddField(&outer_fld.ID, &outer_fld));
	}
	else {
		THROW_SL(OtrRec.AddField(&outer_fld.ID, &outer_fld));
	}
	CATCHZOK
	return ok;
}

int PPImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	const  int preserve_win_coding = BIN(pFile->GetFlags() & SIniFile::fWinCoding);
	SString ini_param, par, val, fld_div, msg_buf, footer_line;
	StringSet param_list;
	SStrScan scan;
	SdbField outer_fld, fld;
	PPSymbTranslator tsl_par(PPSSYM_IMPEXPPAR);
	Name = pSect;
	memzero(&ImpExpParamDll, sizeof(ImpExpParamDllStruct));
	FtpAccID = 0; // @v9.3.7.
	pFile->SetFlag(SIniFile::fWinCoding, 0);
	pFile->GetEntries(Name, &param_list, 1);
	for(uint pos = 0; param_list.get(&pos, ini_param);) {
		uint   idx = 0;
		size_t next_pos = 0;
		ini_param.Divide('=', par, val);
		par.Strip();
		if(pExclParamList && pExclParamList->search(par, 0, 1))
			continue;
		switch(tsl_par.Translate(par.Strip(), &next_pos)) {
			case iefFileName: FileName = val; break;
			case iefImpExp:
				if(val.CmpPrefix("IMP", 1) == 0)
					Direction = 1;
				else if(val.CmpPrefix("EXP", 1) == 0)
					Direction = 0;
				break;
			case iefFormat:
				if(val.CmpPrefix("DBF", 1) == 0)
					DataFormat = dfDbf;
				else if(val.CmpPrefix("TXT", 1) == 0 || val.CmpPrefix("TEXT", 1) == 0) {
					DataFormat = dfText;
					if(fld_div.Empty()) {
						size_t pos = 0;
						const char * p = val.StrChr('(', &pos);
						if(p) {
							p++;
							while(*p && *p != ')')
								fld_div.CatChar(*p++);
						}
					}
				}
				else if(val.CmpPrefix("XML", 1) == 0)
					DataFormat = dfXml;
				else if(val.CmpPrefix("XLS", 1) == 0)
					DataFormat = dfExcel;
				break;
			case iefOrient:
				if(val.CmpPrefix("HOR", 1) == 0)
					TdfParam.Flags &= ~TextDbFile::fVerticalRec;
				else if(val.CmpPrefix("VER", 1) == 0)
					TdfParam.Flags |= TextDbFile::fVerticalRec;
				break;
			case iefCodepage:
				if(val.CmpNC("windows-1251") == 0 || val == "1251") {
					SETFLAG(TdfParam.Flags, TextDbFile::fOemText, 0);
				}
				else
					SETFLAG(TdfParam.Flags, TextDbFile::fOemText, 1);
				break;
			case iefOemText: SETFLAG(TdfParam.Flags, TextDbFile::fOemText, val.ToLong()); break;
			case iefFldNameRec: SETFLAG(TdfParam.Flags, TextDbFile::fFldNameRec, val.ToLong()); break;
			case iefQuotStr: SETFLAG(TdfParam.Flags, TextDbFile::fQuotText, val.ToLong()); break;
			case iefHdrLinesCount: TdfParam.HdrLinesCount = val.ToLong(); break;
			case iefFieldDiv: fld_div = val; break;
			case iefFixedFields: SETFLAG(TdfParam.Flags, TextDbFile::fFixedFields, val.ToLong()); break;
			case iefFieldEqVal: SETFLAG(TdfParam.Flags, TextDbFile::fFldEqVal, val.ToLong()); break;
			case iefFooterLine: footer_line = val; break;
			case iefFormula: THROW(ParseFormula(0, par, val)); break;
			case iefHdrFormula: THROW(ParseFormula(1, par, val)); break;
			case iefRootTag: XdfParam.RootTag = val; break;
			case iefRecTag: XdfParam.RecTag = val; break;
			case iefHdrTag: XdfParam.HdrTag = val; break;
			case iefUseDTD: SETFLAG(XdfParam.Flags, XmlDbFile::Param::fUseDTD, val.ToLong()); break;
			case iefUtf8Codepage: SETFLAG(XdfParam.Flags, XmlDbFile::Param::fUtf8Codepage, val.ToLong()); break;
			case iefSubRec: SETFLAG(XdfParam.Flags, XmlDbFile::Param::fHaveSubRec, val.ToLong()); break;
			case iefSheetNum: XlsdfParam.SheetNum = val.ToLong(); break;
			case iefSheetName: XlsdfParam.SheetName_ = val; break;
			case iefEndStr: XlsdfParam.EndStr_ = val; break;
			case iefXlsFldNameRec: SETFLAG(XlsdfParam.Flags, ExcelDbFile::fFldNameRec, val.ToLong()); break;
			case iefXlsQuotStr: SETFLAG(XlsdfParam.Flags, ExcelDbFile::fQuotText, val.ToLong()); break;
			case iefOneRecPerFile: SETFLAG(XlsdfParam.Flags, ExcelDbFile::fOneRecPerFile, val.ToLong()); break;
			case iefXlsOrient:
				if(val.CmpPrefix("HOR", 1) == 0)
					XlsdfParam.Flags &= ~ExcelDbFile::fVerticalRec;
				else if(val.CmpPrefix("VER", 1) == 0)
					XlsdfParam.Flags |= ExcelDbFile::fVerticalRec;
				break;
			case iefXlsHdrLinesCount: XlsdfParam.HdrLinesCount = val.ToLong(); break;
			case iefColumnsCount: XlsdfParam.ColumnsCount = val.ToLong(); break;
			// @vmiller {
			case iedllDllPath: ImpExpParamDll.DllPath = val; break;
			case iedllReceiptTag: ImpExpParamDll.RcptTagID = val.ToLong(); break;
			case iedllGlobalName:
				ImpExpParamDll.Login = val;
				// ������� ������
				{
					PPObjGlobalUserAcc obj_user_acc;
					PPGlobalUserAcc user_acc;
					SString pwd;
					PPID user_id = 0;
					if(obj_user_acc.SearchByName(ImpExpParamDll.Login, &user_id, &user_acc)) {
						if(!ImpExpParamDll.Login.CmpNC(user_acc.Name)) {
							Reference::Decrypt(Reference::crymRef2, user_acc.Password, strlen(user_acc.Password), pwd);
							ImpExpParamDll.Password.CopyFrom(pwd);
						}
					}
				}
				break;
			case iedllBeerGrpId: ImpExpParamDll.BeerGrpID = val.ToLong(); break;
			case iedllAlcoGrpId: ImpExpParamDll.AlcoGrpID = val.ToLong(); break;
			case iedllAlcoLicenseRegId: ImpExpParamDll.AlcoLicenseRegID = val.ToLong(); break;
			case iedllTTNTagId: ImpExpParamDll.TTNTagID = val.ToLong(); break;
			case iedllManufTagId: ImpExpParamDll.ManufTagID = val.ToLong(); break;
			case iedllManufKPPRegTagId: ImpExpParamDll.ManufKPPRegTagID = val.ToLong(); break;
			case iedllManufRegionCode: ImpExpParamDll.ManufRegionCode = val.ToLong(); break;
			case iedllIsManufTagId: ImpExpParamDll.IsManufTagID = val.ToLong(); break;
			case iedllGoodsKindSymb: ImpExpParamDll.GoodsKindSymb = val; break;
			case iedllXmlPrefix: ImpExpParamDll.XmlPrefix = val; break;
			case iedllGoodsKindTagID: ImpExpParamDll.GoodsKindTagID = val.ToLong(); break;
			case iedllOperType: ImpExpParamDll.OperType = val; break;
			case iedllGoodsVolSymb: ImpExpParamDll.GoodsVolSymb = val; break;
			case iedllManufINNID: ImpExpParamDll.ManufINNID = val.ToLong(); break;
			// } @vmiller
			// @v8.4.6 {
			case iefBaseFlags: BaseFlags = val.ToLong(); break;
			// } @v8.4.6
			// @v8.6.1 {
			case iefFtpAccSymb:
                {
                	PPObjInternetAccount ia_obj;
                	PPInternetAccount ia_pack;
                	PPID   ia_id = 0;
                	(msg_buf = val).Transf(CTRANSF_OUTER_TO_INNER);
                	if(ia_obj.SearchBySymb(msg_buf, &ia_id, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
                		FtpAccID = ia_pack.ID;
                	}
                }
				break;
			case iefFtpAccID:
				if(!FtpAccID) {
                	PPObjInternetAccount ia_obj;
                	PPInternetAccount ia_pack;
					if(ia_obj.Get(val.ToLong(), &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
						FtpAccID = ia_pack.ID;
					}
				}
				break;
			// } @v8.6.1
			default:
				{
					if(par.CmpPrefix("empty", 1) == 0) {
						THROW(ParseFormula(0, par, val));
					}
					else {
						if(InrRec.GetFieldByName(par, &fld) > 0) {
							if(pFile->GetFlags() & SIniFile::fWinCoding)
								val.Transf(CTRANSF_INNER_TO_OUTER);
							scan.Set(val, 0);
							outer_fld.Init();
							outer_fld.ID = fld.ID;
							THROW_SL(outer_fld.TranslateString(scan));
							//outer_fld.Typ = fld.Typ; // ??? ��������, ��� ���������� ���� ������ ����
							if(GETSTYPE(outer_fld.T.Typ) == S_ZSTRING && GETSSIZE(outer_fld.T.Typ) == 0) {
								size_t len = SFMTLEN(outer_fld.OuterFormat)+1;
								if(len & 0x3)
									len = ((len >> 2) << 2) + 4;
								SETMIN(len, 4096); // @v8.1.1 255-->4096
								outer_fld.T.Typ = MKSTYPE(S_ZSTRING, len);
							}
							THROW_SL(OtrRec.AddField(&outer_fld.ID, &outer_fld));
						}
						else if(HdrInrRec.GetFieldByName(par, &fld) > 0) {
							if(pFile->GetFlags() & SIniFile::fWinCoding)
								val.Transf(CTRANSF_INNER_TO_OUTER);
							scan.Set(val, 0);
							outer_fld.Init();
							outer_fld.ID = fld.ID;
							THROW_SL(outer_fld.TranslateString(scan));
							//outer_fld.Typ = fld.Typ; // ??? ��������, ��� ���������� ���� ������ ����
							if(GETSTYPE(outer_fld.T.Typ) == S_ZSTRING && GETSSIZE(outer_fld.T.Typ) == 0) {
								size_t len = SFMTLEN(outer_fld.OuterFormat)+1;
								if(len & 0x3)
									len = ((len >> 2) << 2) + 4;
								SETMIN(len, 4096); // @v8.1.1 255-->4096
								outer_fld.T.Typ = MKSTYPE(S_ZSTRING, len);
							}
							THROW_SL(HdrOtrRec.AddField(&outer_fld.ID, &outer_fld));
						}
						else {
							(msg_buf = 0).Cat(pSect).CatDiv(':', 1).Cat(ini_param);
							CALLEXCEPT_PP_S(PPERR_INVFLDCORR, msg_buf);
						}
					}
				}
				break;
		}
	}
	if(fld_div.NotEmptyS()) {
		if(TdfParam.Flags & TextDbFile::fVerticalRec)
			TdfParam.VertRecTerm = fld_div;
		else
			TdfParam.FldDiv = fld_div;
	}
	if(footer_line.NotEmptyS()) {
		if(TdfParam.Flags & TextDbFile::fVerticalRec)
			TdfParam.FooterLine = footer_line;
	}
	CATCHZOK
	pFile->SetFlag(SIniFile::fWinCoding, preserve_win_coding); // @v7.0.3
	return ok;
}

//
//
//
ImpExpParamDialog::ImpExpParamDialog(uint dlgID, long options) : TDialog(dlgID)
{
	FileBrowseCtrlGroup::Setup(this, CTLBRW_IMPEXP_FILENAME, CTL_IMPEXP_FILENAME, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
	Flags = options;
	if(Flags & fDisableName)
		disableCtrl(CTL_IMPEXP_NAME, 1);
	if(Flags & fDisableExport)
		DisableClusterItem(CTL_IMPEXP_DIR, 0, 1);
	if(Flags & fDisableImport)
		DisableClusterItem(CTL_IMPEXP_DIR, 1, 1);
	EnableExcelImpExp = 1;
	/*
	PPIniFile ini_file;
	if(ini_file.Valid())
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ENABLEEXELIMPEXP, &EnableExcelImpExp);
	*/
	DisableClusterItem(CTL_IMPEXP_FORMAT, 3, !EnableExcelImpExp);
}

IMPL_HANDLE_EVENT(ImpExpParamDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmImpExpTxtDbParam) {
			GetClusterData(CTL_IMPEXP_FORMAT, &Data.DataFormat);
			if(Data.DataFormat == PPImpExpParam::dfText) {
				getCtrlString(CTL_IMPEXP_FILENAME, Data.FileName);
				Data.TdfParam.DefFileName = Data.FileName;
				if(EditTextDbFileParam(/*&Data.TdfParam*/&Data) > 0) {
					Data.FileName = Data.TdfParam.DefFileName;
					setCtrlString(CTL_IMPEXP_FILENAME, Data.FileName);
				}
			}
			else if(Data.DataFormat == PPImpExpParam::dfXml) {
				EditXmlDbFileParam(/*&Data.XdfParam*/&Data);
			}
			else if(Data.DataFormat == PPImpExpParam::dfExcel)
				EditXlsDbFileParam(&Data.XlsdfParam);
		}
		else if(TVCMD == cmImpExpFldCorr) {
			EditFieldCorrList(&Data.InrRec, &Data.OtrRec);
		}
		else if(TVCMD == cmClusterClk) {
			if(event.isCtlEvent(CTL_IMPEXP_FORMAT)) {
				GetClusterData(CTL_IMPEXP_FORMAT, &Data.DataFormat);
				enableCommand(cmImpExpTxtDbParam, oneof3(Data.DataFormat, PPImpExpParam::dfText, PPImpExpParam::dfXml, PPImpExpParam::dfExcel)); // @v5.3.6 AHTOXA
			}
			else if(event.isCtlEvent(CTL_IMPEXP_DIR)) {
				GetClusterData(CTL_IMPEXP_DIR, &Data.Direction);
				DisableClusterItem(CTL_IMPEXP_FLAGS, 1, Data.Direction == 0);
				return; // �� ������� ����� ������� ������� ��������� ����������� ������ ���� ����� ��� ������������
			}
			else
				return;
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

int ImpExpParamDialog::setDTS(const PPImpExpParam * pData)
{
	SString name;
	Data = *pData;
	AddClusterAssocDef(CTL_IMPEXP_FORMAT,  0, PPImpExpParam::dfText);
	AddClusterAssoc(CTL_IMPEXP_FORMAT,  1, PPImpExpParam::dfDbf);
	AddClusterAssoc(CTL_IMPEXP_FORMAT,  2, PPImpExpParam::dfXml);
	AddClusterAssoc(CTL_IMPEXP_FORMAT,  3, PPImpExpParam::dfExcel);
	SetClusterData(CTL_IMPEXP_FORMAT, Data.DataFormat);
	if(Flags & fDisableImport && Data.Direction == 1)
		Data.Direction = 0;
	else if(Flags & fDisableExport && Data.Direction == 0)
		Data.Direction = 1;
	AddClusterAssocDef(CTL_IMPEXP_DIR, 0, 0);
	AddClusterAssoc(CTL_IMPEXP_DIR, 1, 1);
	SetClusterData(CTL_IMPEXP_DIR, Data.Direction);
	{
		long   _f = 0;
		SETFLAG(_f, 0x0001, Data.TdfParam.Flags & TextDbFile::fOemText);
		SETFLAG(_f, 0x0002, Data.BaseFlags & PPImpExpParam::bfDeleteSrcFiles);
		AddClusterAssoc(CTL_IMPEXP_FLAGS, 0, 0x0001);
		AddClusterAssoc(CTL_IMPEXP_FLAGS, 1, 0x0002);
		SetClusterData(CTL_IMPEXP_FLAGS, _f);

		DisableClusterItem(CTL_IMPEXP_FLAGS, 1, Data.Direction == 0);
	}
	setCtrlString(CTL_IMPEXP_FILENAME, Data.FileName);
	// @v8.6.1 {
	if(getCtrlView(CTLSEL_IMPEXP_FTPACC)) {
		SetupPPObjCombo(this, CTLSEL_IMPEXP_FTPACC, PPOBJ_INTERNETACCOUNT, Data.FtpAccID, OLW_CANINSERT, (void *)INETACCT_ONLYFTP);
	}
	// } @v8.6.1
	name = Data.Name;
	pData->ProcessName(2, name);
	setCtrlString(CTL_IMPEXP_NAME, name);
	enableCommand(cmImpExpTxtDbParam, oneof3(Data.DataFormat, PPImpExpParam::dfText, PPImpExpParam::dfXml, PPImpExpParam::dfExcel));
	return 1;
}

int ImpExpParamDialog::getDTS(PPImpExpParam * pData)
{
	int    ok = 1;
	SString name;
	GetClusterData(CTL_IMPEXP_FORMAT, &Data.DataFormat);
	GetClusterData(CTL_IMPEXP_DIR,    &Data.Direction);
	{
		long   _f = 0;
		GetClusterData(CTL_IMPEXP_FLAGS,  &_f);
		SETFLAG(Data.TdfParam.Flags, TextDbFile::fOemText, _f & 0x0001);
		SETFLAG(Data.BaseFlags, PPImpExpParam::bfDeleteSrcFiles, _f & 0x0002);
	}
	getCtrlString(CTL_IMPEXP_FILENAME, Data.FileName);
	// @v8.6.1 {
	if(getCtrlView(CTLSEL_IMPEXP_FTPACC)) {
		PPID   acc_id = 0;
		getCtrlData(CTLSEL_IMPEXP_FTPACC, &acc_id);
		if(acc_id) {
			PPObjInternetAccount ia_obj;
			PPInternetAccount ia_pack;
			if(ia_obj.Get(acc_id, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
				Data.FtpAccID = acc_id;
			}
		}
	}
	// } @v8.6.1
	getCtrlString(CTL_IMPEXP_NAME, name);
	THROW_PP(name.NotEmptyS(), PPERR_NAMENEEDED);
	pData->Direction = Data.Direction;
	pData->ProcessName(1, name);
	Data.Name = name;
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

int EditImpExpParam(PPImpExpParam * pData)
{
	DIALOG_PROC_BODY_P1(ImpExpParamDialog, DLG_IMPEXP, pData);
}
//
//
//
PPImpExp::StateBlock::StateBlock()
{
	Busy = 0;
	RecNo = 0;
}

PPImpExp::PPImpExp(const PPImpExpParam * pParam, const void * extraPtr)
{
	P_DbfT  = 0;
	P_TxtT  = 0;
	P_XmlT  = 0;
	P_SoapT = 0;
	P_XlsT  = 0;
	State = 0;
	R_RecNo = 0;
	W_RecNo = 0;
	P_ExprContext = 0;
	P_HdrData = 0; // @v7.4.6
	RVALUEPTR(P, pParam);
	R_SaveRecNo = 0;
	// @v9.3.10 SaveParam.Init();
	ExtractSubChild = 0;
	P.FileName.Transf(CTRANSF_INNER_TO_OUTER);
	PreserveOrgFileName = P.FileName; // @v9.3.10
	if(P.Direction == 0) {
		const SString preserve_file_name = P.FileName;
		SString result_file_name;
		int    mefn_r = 0;
		if(pParam) {
			mefn_r = pParam->MakeExportFileName(extraPtr, result_file_name);
		}
		else {
			mefn_r = P.MakeExportFileName(extraPtr, result_file_name);
		}
		if(mefn_r > 0) {
			P.FileName = result_file_name;
			if(CConfig.Flags & CCFLG_DEBUG) {
				SString fmt_buf, msg_buf;
				if(mefn_r == 100) {
					PPFormatT(PPTXT_LOG_EXPFILENAME_TMPL, &msg_buf, result_file_name.cptr(), preserve_file_name.cptr());
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
				}
				else {
					PPFormatT(PPTXT_LOG_EXPFILENAME, &msg_buf, result_file_name.cptr());
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
				}
			}
		}
		else if(mefn_r == 0)
			State |= sCtrError;
	}
	if(P.DataFormat == PPImpExpParam::dfXml) {
		// @v8.6.11 P.XdfParam.RootTag.Transf(CTRANSF_INNER_TO_OUTER);
		// @v8.6.11 P.XdfParam.RecTag.Transf(CTRANSF_INNER_TO_OUTER);
		// @v8.6.11 P.XdfParam.HdrTag.Transf(CTRANSF_INNER_TO_OUTER); // @v7.2.6
	}
}

PPImpExp::~PPImpExp()
{
	CloseFile();
	delete P_DbfT;
	delete P_TxtT;
	delete P_XmlT;
	delete P_SoapT;
	delete P_XlsT;
	delete P_HdrData; // @v7.4.6
}

int PPImpExp::IsOpened() const
{
	return BIN(State & sOpened);
}

int PPImpExp::IsCtrError() const
{
	return BIN(State & sCtrError);
}

int PPImpExp::SetExprContext(ExprEvalContext * pCtx)
{
	P_ExprContext = pCtx;
	return 1;
}

int PPImpExp::SetHeaderData(const Sdr_ImpExpHeader * pData)
{
	int    ok = 1;
	if(pData) {
		SETIFZ(P_HdrData, new Sdr_ImpExpHeader);
		if(P_HdrData)
			*P_HdrData = *pData;
		else
			ok = PPSetErrorNoMem();
	}
	else {
		ZDELETE(P_HdrData);
	}
	return ok;
}

const PPImpExpParam & PPImpExp::GetParam() const
{
	return P;
}

int PPImpExp::GetNumRecs(long * pNumRecs)
{
	int    ok = 1;
	ulong  numrecs = 0;
	if(P_TxtT)
		P_TxtT->GetNumRecords(&numrecs);
	else if(P_DbfT) {
		numrecs = P_DbfT->getNumRecs();
		ulong num_deleted_recs = 0;
		for(ulong i = 0; i < numrecs; i++) {
			THROW(P_DbfT->goToRec(i+1));
			if(P_DbfT->isDeletedRec())
				num_deleted_recs++;
		}
		numrecs -= num_deleted_recs;
	}
	else if(P_XmlT)
		P_XmlT->GetNumRecords(&numrecs);
	else if(P_SoapT)
		P_SoapT->GetNumRecords(&numrecs);
	else if(P_XlsT)
		P_XlsT->GetNumRecords(&numrecs);
	else
		ok = 0;
	CATCHZOK
	ASSIGN_PTR(pNumRecs, (long)numrecs);
	return ok;
}

int PPImpExp::OpenFileForReading(const char * pFileName)
{
	return Helper_OpenFile(pFileName, 1, 0, 0);
}

int PPImpExp::OpenFileForWriting(const char * pFileName, int truncOnWriting, StringSet * pResultFileList)
{
	return Helper_OpenFile(pFileName, 0, truncOnWriting, pResultFileList);
}

int FASTCALL PPImpExp::GetExportBuffer(SBuffer & rBuf)
{
	int    ok = -1;
	if(P.DataFormat == PPImpExpParam::dfXml) {
		ok = P_XmlT ? P_XmlT->GetExportBuffer(rBuf) : 0;
	}
	return ok;
}

int PPImpExp::Helper_OpenFile(const char * pFileName, int readOnly, int truncOnWriting, StringSet * pResultFileList)
{
	int    ok = 1;
	DBFCreateFld * p_dbf_flds = 0;
	DbfTable * p_dbf_tbl = 0;
	SString temp_buf;
	CloseFile();
	R_RecNo = 0;
	W_RecNo = 0;
	SString filename = isempty(pFileName) ? P.FileName : pFileName;
	THROW_PP(filename.NotEmpty(), PPERR_UNDEFIMPEXPFILENAME);
	const int is_buffer = BIN(filename.CmpNC(":buffer:") == 0);
	THROW_PP(!is_buffer || P.DataFormat == PPImpExpParam::dfXml, PPERR_IMPEXPUNSUPPBUFFORMAT);
	if(!is_buffer) {
		if(readOnly) {
			THROW_SL(fileExists(filename));
		}
		else {
			SString dir;
			SPathStruc sp;
			sp.Split(filename);
			sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, dir);
			if(!pathValid(dir.SetLastSlash(), 1))
				createDir(dir);
		}
	}
	if(P.DataFormat == PPImpExpParam::dfDbf) {
		if(!readOnly) {
			const uint fld_count = P.OtrRec.GetCount();
			SdbField sdb_fld;
			THROW_MEM(p_dbf_flds = new DBFCreateFld[fld_count]);
			for(uint i = 0; P.OtrRec.EnumFields(&i, &sdb_fld);)
				THROW(sdb_fld.ConvertToDbfField(p_dbf_flds+i-1));
			THROW_MEM(p_dbf_tbl = new DbfTable(filename));
			THROW_PP_S(p_dbf_tbl->create(fld_count, p_dbf_flds), PPERR_DBFCRFAULT, filename);
			ZDELETE(p_dbf_tbl);
		}
		THROW_MEM(P_DbfT = new DbfTable(filename));
		THROW_PP_S(P_DbfT->isOpened(), PPERR_DBFOPFAULT, filename);
	}
	else if(P.DataFormat == PPImpExpParam::dfText) {
		if(!readOnly && truncOnWriting)
			SFile::Remove(filename);
		THROW_MEM(P_TxtT = new TextDbFile);
		THROW_SL(P_TxtT->Open(filename, &P.TdfParam, readOnly));
		if(P.TdfParam.Flags & TextDbFile::fVerticalRec && P.HdrOtrRec.GetCount()) {
			Sdr_ImpExpHeader sdr_hdr;
			MEMSZERO(sdr_hdr);
			if(P_HdrData) {
				sdr_hdr.PeriodLow = P_HdrData->PeriodLow;
				sdr_hdr.PeriodUpp = P_HdrData->PeriodUpp;
			}
			getcurdatetime(&sdr_hdr.CurDate, &sdr_hdr.CurTime);
			PPVersionInfo vi = DS.GetVersionInfo();
			vi.GetProductName(temp_buf);
			temp_buf.CopyTo(sdr_hdr.SrcSystemName, sizeof(sdr_hdr.SrcSystemName));
			vi.GetVersionText(sdr_hdr.SrcSystemVer, sizeof(sdr_hdr.SrcSystemVer));
			P.HdrInrRec.SetDataBuf(&sdr_hdr, sizeof(sdr_hdr));
			ConvertInnerToOuter(1, &sdr_hdr, sizeof(sdr_hdr));
			THROW(P_TxtT->AppendHeader(P.HdrOtrRec, P.HdrOtrRec.GetDataC()));
		}
	}
	else if(P.DataFormat == PPImpExpParam::dfXml) {
		THROW_MEM(P_XmlT = new XmlDbFile);
		{
			temp_buf = DS.GetConstTLA().DL600XMLEntityParam;
			if(temp_buf.NotEmptyS())
				P_XmlT->SetEntitySpec(temp_buf);
		}
		THROW_SL(P_XmlT->Open(filename, &P.XdfParam, &P.OtrRec, readOnly));
		if(P.XdfParam.HdrTag.NotEmpty()) {
			Sdr_ImpExpHeader sdr_hdr;
			MEMSZERO(sdr_hdr);
			getcurdatetime(&sdr_hdr.CurDate, &sdr_hdr.CurTime);
			PPVersionInfo vi = DS.GetVersionInfo();
			vi.GetProductName(temp_buf);
			temp_buf.CopyTo(sdr_hdr.SrcSystemName, sizeof(sdr_hdr.SrcSystemName));
			vi.GetVersionText(sdr_hdr.SrcSystemVer, sizeof(sdr_hdr.SrcSystemVer));
			P.HdrInrRec.SetDataBuf(&sdr_hdr, sizeof(sdr_hdr));
			ConvertInnerToOuter(1, &sdr_hdr, sizeof(sdr_hdr));
			THROW(P_XmlT->AppendRecord(P.XdfParam.HdrTag, P.HdrOtrRec, P.HdrOtrRec.GetDataC()));
		}
	}
	else if(P.DataFormat == PPImpExpParam::dfSoap) {
		THROW_MEM(P_SoapT = new SoapDbFile);
		THROW_SL(P_SoapT->Open(filename, &P.SdfParam, readOnly));
	}
	else if(P.DataFormat == PPImpExpParam::dfExcel) {
		PPWaitMsg(PPSTR_TEXT, PPTXT_SCANEXCELFILE, filename);
		THROW_MEM(P_XlsT = new ExcelDbFile);
		THROW(P_XlsT->Open(filename, &P.XlsdfParam, readOnly));
	}
	else
		ok = PPSetError(PPERR_IMPEXPFMTUNSUPP, (const char *)0);
	if(ok) {
		State |= sOpened;
		SETFLAG(State, sBuffer, is_buffer); // @v8.6.8
		SETFLAG(State, sReadOnly, readOnly);
		if(pResultFileList && !is_buffer) {
            SPathStruc::NormalizePath(filename, 0, temp_buf);
            if(temp_buf.NotEmpty()) {
				if(!pResultFileList->search(temp_buf, 0, 1))
					pResultFileList->add(temp_buf);
            }
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_TxtT);
		ZDELETE(P_DbfT);
		ZDELETE(P_XmlT);
		ZDELETE(P_SoapT);
	ENDCATCH
	delete p_dbf_flds;
	delete p_dbf_tbl;
	return ok;
}

int PPImpExp::CloseFile()
{
	ZDELETE(P_TxtT);
	ZDELETE(P_DbfT);
	ZDELETE(P_XmlT);
	ZDELETE(P_XlsT);
	State &= ~(sOpened | sReadOnly);
	return 1;
}
//
// @vmiller comment
//int PPImpExp::Push(PPImpExpParam * pParam)
//{
//	if(pParam && P_XmlT && P.DataFormat == PPImpExpParam::dfXml && pParam->DataFormat == PPImpExpParam::dfXml) {
//		SString file_name_root;
//		PPImpExpParam p = *pParam;
//		file_name_root = P.FileName;
//		if(p.FileName.CmpNC(file_name_root.ToOem()) == 0) {
//			p.FileName.Transf(CTRANSF_INNER_TO_OUTER);
//			R_SaveRecNo = R_RecNo;
//			SaveParam   = P;
//			P = *pParam;
//			R_RecNo = 0;
//			ExtractSubChild = 1;
//			P_XmlT->Push(&P.XdfParam);
//		}
//	}
//	return 1;
//}

// @vmiller
int PPImpExp::Push(PPImpExpParam * pParam)
{
	int    ok = 1;
	if(pParam && P_XmlT && P.DataFormat == PPImpExpParam::dfXml && pParam->DataFormat == PPImpExpParam::dfXml) {
		SString file_name = P.FileName;
		file_name.Transf(CTRANSF_OUTER_TO_INNER);
		if(pParam->FileName.CmpNC(file_name) == 0) {
			int    pos = -1;
			StateBlock * p_state = 0;
			for(uint i = 0; i < StateColl.getCount(); i++) {
				if(!StateColl.at(i)->Busy) {
					pos = (int)i;
					p_state = StateColl.at(i);
					break;
				}
			}
			if(pos < 0) {
				uint   _pos = 0;
				THROW(p_state = StateColl.CreateNewItem(&_pos));
				pos = (int)_pos;
			}
			assert(pos >= 0 && pos < (int)StateColl.getCount());
			StateStack.push(pos);
			p_state->Busy = 1;
			p_state->RecNo = R_RecNo;
			p_state->FileNameRoot = P.FileName;
			p_state->Param = P;

			P = *pParam;
			R_RecNo = 0;
			ExtractSubChild++;
			P_XmlT->Push(&P.XdfParam);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPImpExp::Pop()
{
	if(ExtractSubChild) {
		int    pos = 0;
		// ����������� ����� ��������� �� �����
		int    r_debug = StateStack.pop(pos);
		assert(r_debug);
		assert(pos >= 0 && pos < (int)StateColl.getCount());
		StateBlock * p_state = StateColl.at(pos);
		assert(p_state->Busy != 0);

		R_RecNo = p_state->RecNo;
		P = p_state->Param;
		p_state->Busy = 0;
		ExtractSubChild--;
		P_XmlT->Pop();
	}
	return 1;
}

int FASTCALL PPImpExp::GetFileName(SString & rFileName) const
{
	switch(P.DataFormat) {
		case PPImpExpParam::dfDbf: rFileName = P_DbfT ? P_DbfT->getName() : 0; break;
		case PPImpExpParam::dfText: rFileName = P_TxtT ? P_TxtT->GetFileName() : 0; break;
		case PPImpExpParam::dfXml: rFileName = P_XmlT ? P_XmlT->GetFileName() : 0; break;
		case PPImpExpParam::dfExcel: rFileName = P_XlsT ? P_XlsT->GetFileName() : 0; break;
		case PPImpExpParam::dfSoap: rFileName = P_SoapT ? 0 : 0; break;
		default: rFileName = 0;
	}
	return 1;
}

const SString & SLAPI PPImpExp::GetPreservedOrgFileName() const
{
	return PreserveOrgFileName;
}

enum {
	iefrmEmpty=1,      // empty       ������ ������
	iefrmRecNo,        // recno       ����� ������� ������
	iefrmCurDate,      // curdate     ������� ����
	iefrmCurTime,      // curtime     ������� ����� //
	iefrmCurYear,      // curyear     ������� ���
	iefrmCurMonth,     // curmonth    ������� �����
	iefrmCurDay,       // curday      ������� ����
	iefrmPsnRegNum,    // personregnum(regtypesymb, person_id) ����� �������� �� �� ����������
	iefrmArRegNum,     // arregnum(regtypesymb, article_id)    ����� �������� �� �� ������������� ������
	iefrmObjTag,       // objtag(tagsymb, objtype, obj_id)     ��������� �������� ���� �������
	iefrmArRegDate,    // @v8.1.1 arregdate(regtypesymb, article_id)   ���� �������� �� �� ������������� ������
	iefrmCat,          // @v9.3.10 cat(...) ��������� ������������ ������ ���������� (��� ������� ��������)
	iefrmCats,         // @v9.3.10 cats(...) ��������� ������������ ������ ���������� (�� �������� �������� ����� ������ �����)
};

// static
int PPImpExp::ResolveVarName(const char * pSymb, const SdRecord & rRec, double * pVal)
{
	int    ok = 0;
	uint   pos = 0;
	double val = 0.0;
	if(pSymb && *pSymb == '@' && rRec.SearchName((pSymb+1), &pos) > 0) {
		SdbField inner_fld;
		if(rRec.GetFieldByPos(pos, &inner_fld)) {
			const void * p_fld_data = rRec.GetDataC(pos);
			if(p_fld_data) {
				char   buf[512];
				long   fmt = 0;
				int    t = GETSTYPE(inner_fld.T.Typ);
				if(oneof4(t, S_FLOAT, S_DEC, S_MONEY, S_INT)) {
					fmt = SFMT_MONEY;
					sttostr(inner_fld.T.Typ, p_fld_data, fmt, buf);
					val = atof(buf);
					ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

SLAPI ImpExpExprEvalContext::ImpExpExprEvalContext(const SdRecord & rRec) : ExprEvalContext(), R_Rec(rRec)
{
}

int SLAPI ImpExpExprEvalContext::Resolve(const char * pSymb, double * pVal)
{
	return PPImpExp::ResolveVarName(pSymb, R_Rec, pVal);
}

int PPImpExp::GetArgList(SStrScan & rScan, StringSet & rArgList)
{
	int    ok = 1;
	SString temp_buf, arg_buf;
	if(rScan.Skip()[0] == '(') {
		rScan.Incr();
		while(rScan.SearchChar(',')) {
			rScan.Get(temp_buf).Strip();
			rScan.IncrLen(1);
			ResolveFormula(temp_buf, 0, 0, arg_buf);
			rArgList.add(arg_buf);
		}
		if(rScan.SearchChar(')')) {
			rScan.Get(temp_buf).Strip();
			rScan.IncrLen(1);
			ResolveFormula(temp_buf, 0, 0, arg_buf);
			rArgList.add(arg_buf);
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

int PPImpExp::ResolveFormula(const char * pFormula, const void * pInnerBuf, size_t bufLen, SString & rResult)
{
	int    ok = 1;
	ExprEvalContext * p_expr_ctx = 0;
	//int    inner_expr_ctx = 0;
	double dbl_val = 0;
	StringSet arg_list;
	ImpExpExprEvalContext own_expr_ctx(P.InrRec);
	if(P_ExprContext) {
		p_expr_ctx = P_ExprContext;
		p_expr_ctx->SetInnerContext(&own_expr_ctx);
	}
	else
		p_expr_ctx = &own_expr_ctx;
	/*
	if(P_ExprContext) {
		p_expr_ctx = P_ExprContext;
	}
	else {
		THROW_MEM(p_expr_ctx = new ImpExpExprEvalContext(P.InrRec));
		inner_expr_ctx = 1;
	}
	*/
	rResult = 0;
	if(PPExprParser::CalcExpression(pFormula, &dbl_val, 0, p_expr_ctx) > 0) // ���������� ��������� � ������� ��������� - �����
		rResult.Cat(dbl_val);
	else {
		SString temp_buf, reg_type_symb;
		PPSymbTranslator st(PPSSYM_IMPEXPFORMULA);
		for(SStrScan scan(pFormula); *scan != 0;) {
			if(scan.GetQuotedString(temp_buf)) {
				rResult.Cat(temp_buf);
			}
			else {
				char cc = scan[0];
				if(cc == '@') {
					int    do_incr_len = 1;
					scan.Incr();
					long   sym  = st.Translate(scan);
					switch(sym) {
						case iefrmEmpty:
							break;
						case iefrmRecNo:
							rResult.Cat(W_RecNo);
							break;
						case iefrmCurDate:
							rResult.Cat(getcurdate_(), DATF_DMY|DATF_CENTURY);
							break;
						case iefrmCurYear:
							rResult.Cat(getcurdate_().year());
							break;
						case iefrmCurMonth:
							rResult.Cat(getcurdate_().month());
							break;
						case iefrmCurDay:
							rResult.Cat(getcurdate_().day());
							break;
						case iefrmCurTime:
							rResult.Cat(getcurtime_(), TIMF_HMS);
							break;
						case iefrmCat:
						case iefrmCats:
							scan.IncrLen();
							do_incr_len = 0;
							if(GetArgList(scan, arg_list)) {
								uint   arg_p = 0;
								PPID   reg_type_id = 0;
								uint   _count = 0;
								for(uint argp = 0; arg_list.get(&argp, temp_buf);) {
                                    if(sym == iefrmCats && _count)
										rResult.Space();
                                    rResult.Cat(temp_buf);
                                    _count++;
								}
							}
							break;
						case iefrmPsnRegNum:
							scan.IncrLen();
							do_incr_len = 0;
							if(GetArgList(scan, arg_list)) {
								uint   arg_p = 0;
								PPID   reg_type_id = 0;
								if(arg_list.get(&arg_p, reg_type_symb) && arg_list.get(&arg_p, temp_buf)) {
									PPID   person_id = temp_buf.ToLong();
									if(person_id && PPObjRegisterType::GetByCode(reg_type_symb, &reg_type_id) > 0) {
										PPObjPerson psn_obj;
										psn_obj.GetRegNumber(person_id, reg_type_id, temp_buf);
										rResult.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
									}
								}
							}
							break;
						case iefrmArRegNum:
							scan.IncrLen();
							do_incr_len = 0;
							if(GetArgList(scan, arg_list)) {
								uint arg_p = 0;
								if(arg_list.get(&arg_p, reg_type_symb) && arg_list.get(&arg_p, temp_buf)) {
									PPID ar_id = temp_buf.ToLong();
									PPID person_id = ObjectToPerson(ar_id, 0);
									PPID reg_type_id = 0;
									if(person_id && PPObjRegisterType::GetByCode(reg_type_symb, &reg_type_id) > 0) {
										PPObjPerson psn_obj;
										psn_obj.GetRegNumber(person_id, reg_type_id, temp_buf);
										rResult.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
									}
								}
							}
							break;
						// @v8.1.1 {
						case iefrmArRegDate:
							scan.IncrLen();
							do_incr_len = 0;
							if(GetArgList(scan, arg_list)) {
								uint arg_p = 0;
								if(arg_list.get(&arg_p, reg_type_symb) && arg_list.get(&arg_p, temp_buf)) {
									PPID ar_id = temp_buf.ToLong();
									PPID person_id = ObjectToPerson(ar_id, 0);
									PPID reg_type_id = 0;
									if(person_id && PPObjRegisterType::GetByCode(reg_type_symb, &reg_type_id) > 0) {
										PPObjPerson psn_obj;
										RegisterTbl::Rec reg_rec;
										if(psn_obj.GetRegister(person_id, reg_type_id, &reg_rec) > 0)
											rResult.Cat(reg_rec.Dt, DATF_DMY|DATF_CENTURY);
									}
								}
							}
							break;
						// } @v8.1.1
						case iefrmObjTag:
							scan.IncrLen();
							do_incr_len = 0;
							if(GetArgList(scan, arg_list)) {
								uint arg_p = 0;
								SString tag_symb, obj_type_symb;
								if(arg_list.get(&arg_p, tag_symb) && arg_list.get(&arg_p, obj_type_symb) && arg_list.get(&arg_p, temp_buf)) {
									PPID obj_id = temp_buf.ToLong();
									long obj_type_ext = 0;
									PPID obj_type = GetObjectTypeBySymb(obj_type_symb, &obj_type_ext);
									if(obj_type && oneof4(obj_type, PPOBJ_GOODS, PPOBJ_PERSON, PPOBJ_LOT, PPOBJ_GLOBALUSERACC)) {
										PPObjTag tag_obj;
										PPID   tag_id = 0;
										if(tag_obj.SearchBySymb(tag_symb, &tag_id, 0) > 0) {
											ObjTagItem tag_item;
											PPRef->Ot.GetTag(obj_type, obj_id, tag_id, &tag_item);
											tag_item.GetStr(temp_buf);
											rResult.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
										}
									}
								}
							}
							break;
						default:
							{
								uint pos = 0;
								if(P.InrRec.ScanName(scan, &pos)) {
									scan.IncrLen();
									scan.Len = 0;
									SdbField inner_fld;
									if(P.InrRec.GetFieldByPos(pos, &inner_fld)) {
										const void * p_fld_data = P.InrRec.GetDataC(pos);
										if(p_fld_data) {
											char   buf[512];
											long   fmt = 0;
											int    t = GETSTYPE(inner_fld.T.Typ);
											if(oneof3(t, S_FLOAT, S_DEC, S_MONEY))
												fmt = SFMT_MONEY;
											else if(t == S_DATE)
												fmt = MKSFMT(0, DATF_DMY);
											else if(t == S_TIME)
												fmt = MKSFMT(0, TIMF_HMS);
											sttostr(inner_fld.T.Typ, p_fld_data, fmt, buf);
											rResult.Cat(buf);
										}
									}
								}
							}
							break;
					}
					if(do_incr_len)
						scan.IncrLen();
				}
				else if(cc == '\\' && scan[1] == '\"') {
					rResult.CatChar('\"');
					scan.Incr(2);
				}
				else {
					rResult.CatChar(cc);
					scan.Incr();
				}
			}
		}
	}
	//CATCHZOK
	/*if(inner_expr_ctx)
		delete p_expr_ctx;*/
	if(P_ExprContext) {
		p_expr_ctx->SetInnerContext(&own_expr_ctx);
	}
	return ok;
}

int PPImpExp::ConvertInnerToOuter(int hdr, const void * pInnerBuf, size_t bufLen)
{
	int    ok = 1;
	SString formula_result;
	SdbField outer_fld, inner_fld;
	if(hdr) {
		if(P.HdrOtrRec.GetDataC() == 0)
			THROW_SL(P.HdrOtrRec.AllocDataBuf());
	}
	else {
		if(P.OtrRec.GetDataC() == 0)
			THROW_SL(P.OtrRec.AllocDataBuf());
	}
	for(uint i = 0; ;) {
		int    er = 0;
		if(hdr) {
			er = P.HdrOtrRec.EnumFields(&i, &outer_fld);
		}
		else {
			er = P.OtrRec.EnumFields(&i, &outer_fld);
		}
		if(er) {
			void * p_outer_fld_buf = hdr ? P.HdrOtrRec.GetData(i-1) : P.OtrRec.GetData(i-1);
			THROW(p_outer_fld_buf);
			if(outer_fld.T.Flags & STypEx::fFormula) {
				THROW(ResolveFormula(outer_fld.Formula, pInnerBuf, bufLen, formula_result));
				formula_result.Trim(255);
				stcast(MKSTYPE(S_ZSTRING, formula_result.Len()+1), outer_fld.T.Typ, (void *)(const char *)formula_result, p_outer_fld_buf, 0);
			}
			else {
				uint   inner_pos = 0;
				size_t len = 0;
				long   fmt = 0;
				if(hdr) {
					THROW(P.HdrInrRec.GetFieldByID(outer_fld.ID, &inner_pos, &inner_fld));
				}
				else {
					THROW(P.InrRec.GetFieldByID(outer_fld.ID, &inner_pos, &inner_fld));
				}
				if(outer_fld.T.IsZStr(&len)) {
					if(GETSTYPE(inner_fld.T.Typ) == S_DATE) {
						fmt = (len >= 11) ? MKSFMT(0, DATF_DMY|DATF_CENTURY) : MKSFMT(0, DATF_DMY);
					}
					else if(GETSTYPE(inner_fld.T.Typ) == S_FLOAT) {
						fmt = SFMT_MONEY;
					}
				}
				const void * p_data_ = hdr ? P.HdrInrRec.GetDataC(inner_pos) : P.InrRec.GetDataC(inner_pos);
				stcast(inner_fld.T.Typ, outer_fld.T.Typ, p_data_, p_outer_fld_buf, fmt);
			}
			if(P.TdfParam.Flags & TextDbFile::fOemText && stbase(outer_fld.T.Typ) == BTS_STRING) {
				char   temp_buf[512];
				sttostr(outer_fld.T.Typ, p_outer_fld_buf, 0, temp_buf);
				stfromstr(outer_fld.T.Typ, p_outer_fld_buf, 0, SCharToOem(temp_buf));
			}
		}
		else
			break;
	}
	CATCHZOK
	return ok;
}

int PPImpExp::InitDynRec(SdRecord * pDynRec) const
{
	assert(pDynRec != 0);
	int    ok = -1;
	long   c = 0; // ������� ������� ������������ �����
	SdbField outer_fld;
	SdbField dyn_fld; // ����������� ������� �� ������� ����� �� ��������� ������ ������������� ������
		// (������ ���� ��������� ��������� �������� SString)
	for(uint i = 0; P.OtrRec.EnumFields(&i, &outer_fld);) {
		if(outer_fld.T.Flags & STypEx::fFormula) {
			dyn_fld.Init();
			dyn_fld.ID = ++c;
			dyn_fld.Name.Cat("DYN").CatLongZ(dyn_fld.ID, 5);
			dyn_fld.T = outer_fld.T;
			dyn_fld.Formula = outer_fld.Formula;
			THROW_SL(pDynRec->AddField(0, &dyn_fld));
			ok = 1;
		}
	}
	if(ok > 0) {
		THROW_SL(pDynRec->AllocDataBuf());
	}
	CATCHZOK
	return ok;
}

int PPImpExp::ConvertOuterToInner(void * pInnerBuf, size_t bufLen, SdRecord * pDynRec)
{
	int    ok = 1;
	uint   dyn_fld_pos = 0;
	SdbField outer_fld;
	SString formula_result;
	SdbField inner_fld; // ����������� ������� �� ������� ����� �� ��������� ������ ������������� ������
		// (������ ���� ��������� ��������� �������� SString)
	if(P.OtrRec.GetDataC() == 0)
		THROW_SL(P.OtrRec.AllocDataBuf());
	for(uint i = 0; P.OtrRec.EnumFields(&i, &outer_fld);) {
		void * p_outer_fld_buf = P.OtrRec.GetData(i-1);
		THROW(p_outer_fld_buf);
		if(outer_fld.T.Flags & STypEx::fFormula) {
			/* @v9.3.10 @construction
			if(outer_fld.Formula.NotEmpty() && ResolveFormula(outer_fld.Formula, pInnerBuf, bufLen, formula_result)) {
			}
			*/
			if(pDynRec) {
				THROW_SL(pDynRec->GetFieldByPos(dyn_fld_pos, &inner_fld));
				if(P.TdfParam.Flags & TextDbFile::fOemText && stbase(outer_fld.T.Typ) == BTS_STRING) {
					char temp_buf[1024];
					sttostr(outer_fld.T.Typ, p_outer_fld_buf, 0, temp_buf);
					stfromstr(outer_fld.T.Typ, p_outer_fld_buf, 0, SOemToChar(temp_buf));
				}
				stcast(outer_fld.T.Typ, inner_fld.T.Typ, p_outer_fld_buf, pDynRec->GetData(dyn_fld_pos), 0);
			}
			dyn_fld_pos++;
		}
		else if(outer_fld.ID) {
			uint   inner_pos = 0;
			THROW_SL(P.InrRec.GetFieldByID(outer_fld.ID, &inner_pos, &inner_fld));
			if(P.TdfParam.Flags & TextDbFile::fOemText && stbase(outer_fld.T.Typ) == BTS_STRING) {
				char temp_buf[1024];
				sttostr(outer_fld.T.Typ, p_outer_fld_buf, 0, temp_buf);
				stfromstr(outer_fld.T.Typ, p_outer_fld_buf, 0, SOemToChar(temp_buf));
			}
			stcast(outer_fld.T.Typ, inner_fld.T.Typ, p_outer_fld_buf, P.InrRec.GetData(inner_pos), 0);
		}
	}
	CATCHZOK
	return ok;
}

int PPImpExp::AppendHdrRecord(void * pInnerBuf, size_t bufLen)
{
	int    ok = 1;
	THROW_PP(IsOpened(), PPERR_IMPEXNOPENED);
	THROW_PP((State & sReadOnly) == 0, PPERR_IMPEXPREADONLY);
	W_RecNo++;
	P.InrRec.SetDataBuf(pInnerBuf, bufLen);
	ConvertInnerToOuter(0, pInnerBuf, bufLen);
	if(P_TxtT) {
		THROW_SL(P_TxtT->AppendRecord(P.OtrRec, P.OtrRec.GetDataC()));
	}
	else if(P_DbfT) { // DbfTable
		DbfRecord dbfrec(P_DbfT);
		THROW(dbfrec.put(P.OtrRec));
		THROW(P_DbfT->appendRec(&dbfrec));
	}
	else if(P_XmlT) {
		THROW_SL(P_XmlT->AppendRecord(P.HdrOtrRec, P.HdrOtrRec.GetDataC()));
	}
	else if(P_SoapT) {
		THROW_SL(P_XmlT->AppendRecord(P.HdrOtrRec, P.HdrOtrRec.GetDataC()));
	}
	else if(P_XlsT) {
		THROW_SL(P_XlsT->AppendRecord(P.HdrOtrRec, P.HdrOtrRec.GetDataC()));
	}
	CATCHZOK
	return ok;
}

int PPImpExp::AppendRecord(void * pInnerBuf, size_t bufLen)
{
	int    ok = 1;
	THROW_PP(IsOpened(), PPERR_IMPEXNOPENED);
	THROW_PP((State & sReadOnly) == 0, PPERR_IMPEXPREADONLY);
	W_RecNo++;
	P.InrRec.SetDataBuf(pInnerBuf, bufLen);
	ConvertInnerToOuter(0, pInnerBuf, bufLen);
	if(P_TxtT) {
		THROW_SL(P_TxtT->AppendRecord(P.OtrRec, P.OtrRec.GetDataC()));
	}
	else if(P_DbfT) { // DbfTable
		DbfRecord dbfrec(P_DbfT);
		THROW(dbfrec.put(P.OtrRec));
		THROW_SL(P_DbfT->appendRec(&dbfrec));
	}
	else if(P_XmlT) {
		THROW_SL(P_XmlT->AppendRecord(P.OtrRec, P.OtrRec.GetDataC()));
	}
	else if(P_SoapT) {
		THROW_SL(P_XmlT->AppendRecord(P.OtrRec, P.OtrRec.GetDataC()));
	}
	else if(P_XlsT) {
		THROW_SL(P_XlsT->AppendRecord(P.OtrRec, P.OtrRec.GetDataC()));
	}
	CATCHZOK
	return ok;
}

int PPImpExp::ReadRecord(void * pInnerBuf, size_t bufLen, SdRecord * pDynRec)
{
	int    ok = -1;
	ulong  numrecs = 0;
	THROW_PP(IsOpened(), PPERR_IMPEXNOPENED);
	P.InrRec.SetDataBuf(pInnerBuf, bufLen);
	if(P.OtrRec.GetDataC() == 0)
		THROW_SL(P.OtrRec.AllocDataBuf());
	if(P_TxtT) {
		P_TxtT->GetNumRecords(&numrecs);
		if(R_RecNo < numrecs) {
			THROW_SL(P_TxtT->GoToRecord(R_RecNo));
			THROW_SL(P_TxtT->GetRecord(P.OtrRec, P.OtrRec.GetData()));
			THROW(ConvertOuterToInner(pInnerBuf, bufLen, pDynRec));
			R_RecNo++;
			ok = 1;
		}
		else
			ok = -1;
	}
	else if(P_DbfT) {
		numrecs = P_DbfT->getNumRecs();
		while(ok < 0 && R_RecNo < numrecs) {
			THROW(P_DbfT->goToRec(R_RecNo+1));
			if(!P_DbfT->isDeletedRec()) {
				DbfRecord dbfrec(P_DbfT);
				THROW(P_DbfT->getRec(&dbfrec));
				THROW(dbfrec.get(P.OtrRec));
				THROW(ConvertOuterToInner(pInnerBuf, bufLen, pDynRec));
				ok = 1;
			}
			R_RecNo++;
		}
	}
	else if(P_XmlT) {
		THROW((ok = P_XmlT->GetRecord(P.OtrRec, P.OtrRec.GetData())));
		if(ok > 0) {
			THROW(ConvertOuterToInner(pInnerBuf, bufLen, pDynRec));
			R_RecNo++;
		}
	}
	else if(P_SoapT) {
		THROW((ok = P_SoapT->GetRecord(P.OtrRec, P.OtrRec.GetData())));
		if(ok > 0) {
			THROW(ConvertOuterToInner(pInnerBuf, bufLen, pDynRec));
			R_RecNo++;
		}
	}
	else if(P_XlsT) {
		P_XlsT->GetNumRecords(&numrecs);
		if(R_RecNo < numrecs) {
			THROW_SL(P_XlsT->GoToRecord(R_RecNo));
			THROW_SL(P_XlsT->GetRecord(P.OtrRec, P.OtrRec.GetData()));
			THROW(ConvertOuterToInner(pInnerBuf, bufLen, pDynRec));
			R_RecNo++;
			ok = 1;
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI Test_ImpExpParamDialog()
{
	int    ok = -1;
	PPImpExpParam param;
	SString file_name;
	PPGetFilePath(PPPATH_BIN, "clibnk.ini", file_name);
	PPIniFile ini_file(file_name);
	THROW(LoadSdRecord(PPREC_CLIBNKDATA, &param.InrRec));
	if(!param.ReadIni(&ini_file, "baltbank-import"/*"test"*/, 0))
		PPError();
	ok = EditImpExpParam(&param);
	if(ok > 0) {
		THROW(param.WriteIni(&ini_file, "TEST"));
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
int SLAPI GetImpExpSections(uint fileNameId, uint sdRecID, PPImpExpParam * pParam, StrAssocArray * pList, int kind)
{
	int    ok = 1;
	uint   p = 0;
	long   id = 0;
	SString section, sect;
	StringSet ss;
	int direction = pParam->Direction; // @vmiller ���� ��������� � ������������ ���� ��������, ��� �� �������� � GetImpExpSections()
	THROW(GetImpExpSections(fileNameId, sdRecID, pParam, &ss, kind));
	for(p = 0, id = 1; ss.get(&p, section); id++) {
		pParam->ProcessName(2, sect = section);
		THROW_SL(pList->Add(id, sect));
	}
	CATCHZOK
	pParam->Direction = direction; // @vmiller
	return ok;
}

int SLAPI GetImpExpSections(uint fileNameId, uint sdRecID, PPImpExpParam * pParam,
	StringSet * pSectNames, int kind /*0 - all, 1 - export, 2 - import*/)
{
	int    ok = 1;
	SString ini_file_name, section;
	StringSet all_sections;
	THROW(PPGetFilePath(PPPATH_BIN, fileNameId, ini_file_name));
	THROW(LoadSdRecord(sdRecID, &pParam->InrRec));
	{
		int    is_exists = fileExists(ini_file_name);
		PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
		THROW(ini_file.IsValid());
		THROW(ini_file.GetSections(&all_sections));
		for(uint p = 0; all_sections.get(&p, section);) {
			int    r = 0;
			if(pParam->ProcessName(3, section)) {
				pParam->OtrRec.Clear();
				pParam->HdrOtrRec.Clear(); // @v7.5.3
				if(r = pParam->ReadIni(&ini_file, section, 0)) {
					if((kind == 1 && pParam->Direction == 0) || (kind == 2 && pParam->Direction == 1) || kind == 0)
						pSectNames->add(section, 0);
				}
				else if(r == 0)
				PPError();
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI GetImpExpSectionsCDb(PPConfigDatabase & rCDb, uint sdRecID, StrAssocArray & rList, int kind)
{
	rList.Clear();

	int    ok = 1;
	PPConfigDatabase::CObjHeader cobj_hdr;
	PPImpExpParam * p_param = PPImpExpParam::CreateInstance(sdRecID, 0);
	THROW(p_param);
	{
		const char * p_name = p_param->DataSymb;
		if(kind == 1)
			rCDb.GetObjList(PPConfigDatabase::tExport, p_name, 0, 0, rList);
		else if(kind == 2)
			rCDb.GetObjList(PPConfigDatabase::tImport, p_name, 0, 0, rList);
		else if(kind == 0) {
			rCDb.GetObjList(PPConfigDatabase::tExport, p_name, 0, 0, rList);
			rCDb.GetObjList(PPConfigDatabase::tImport, p_name, 0, 0, rList);
		}
	}
	CATCHZOK
	delete p_param;
	return ok;
}

int SLAPI Helper_ConvertImpExpConfig_IniToBdb(PPConfigDatabase & rCDb, SSerializeContext & rSCtx, uint iniFileNameId, uint sdRecID, /*PPImpExpParam * pParam,*/ PPLogger * pLogger)
{
	int    ok = 1;
	SString ini_file_name, section, db_dir;
	SString temp_buf, left_buf, right_buf, cobj_name, sub_symb;
	StringSet all_sections;
	PPImpExpParam * p_param = PPImpExpParam::CreateInstance(sdRecID, 0);
	THROW(p_param);
	THROW(PPGetFilePath(PPPATH_BIN, iniFileNameId, ini_file_name));
	if(fileExists(ini_file_name)) {
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(ini_file.IsValid());
		{
			PPConfigDatabase::CObjHeader cobj_hdr;
			SBuffer cobj_tail;
			int32 cobj_id = 0;

			THROW(LoadSdRecord(sdRecID, &p_param->InrRec));
			THROW(ini_file.GetSections(&all_sections));
			for(uint p = 0; all_sections.get(&p, section);) {
				int    r = 0;
				if(p_param->ProcessName(3, section)) {
					p_param->OtrRec.Clear();
					if(r = p_param->ReadIni(&ini_file, section, 0)) {
						p_param->ProcessName(2, section);
						p_param->Name = section;
						if(p_param->SerializeConfig(+1, cobj_hdr, cobj_tail, &rSCtx)) {
							cobj_id = 0;
							if(!rCDb.PutObj(&cobj_id, cobj_hdr, cobj_tail, 1)) {
								// @err
							}
						}
					}
					else if(r == 0) {
						// @err
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_param;
	return ok;
}

int SLAPI ConvertImpExpConfig_IniToBdb()
{
	int    ok = 1;
	SString db_dir;
	PPGetPath(PPPATH_BIN, db_dir);
	db_dir.SetLastSlash().Cat("CDB");
	PPConfigDatabase cfg_db(0);
	SSerializeContext sctx;
	PPLogger logger;
	THROW(cfg_db.Open(db_dir));
	{
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_BILL, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_BROW, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_GOODS2, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_INVENTORYITEM, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_PRICELIST, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_PHONELIST, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_SCARD, &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_LOT,   &logger));
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_WORKBOOK, &logger)); // @v8.1.1
		// @v9.1.3 THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_IMPEXP_INI, PPREC_CSESSCOMPLEX, &logger)); // @v8.2.7
		THROW(Helper_ConvertImpExpConfig_IniToBdb(cfg_db, sctx, PPFILNAM_CLIBNK_INI, PPREC_CLIBNKDATA, &logger));
	}
	{
		StrAssocArray test_list;
		THROW(GetImpExpSectionsCDb(cfg_db, PPREC_BILL, test_list, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
class ImpExpCfgsListDialog : public PPListDialog {
public:
	ImpExpCfgsListDialog();
private:
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	DECL_HANDLE_EVENT;

	int    EditParam(const char * pIniSection, long * pCDbID);
	ImpExpParamDialog * GetParamDlg(uint cfgPos);
	int    GetParamDlgDTS(ImpExpParamDialog * pDlg, uint cfgPos, PPImpExpParam * pParam);
	int    SetParamDlgDTS(ImpExpParamDialog * pDlg, uint cfgPos, PPImpExpParam * pParam);

	uint   CfgPos;
	PPConfigDatabase CDb;
	SSerializeContext SCtx;
	StrAssocArray CfgsList;
	StringSet Sections;
	PPImpExpParam * P_ParamList[32];
	PPGoodsImpExpParam GoodsParam;
	PPBillImpExpParam  BillParam;
	PPBillImpExpParam  BRowParam;
	PPInventoryImpExpParam  InvParam;
	PPPriceListImpExpParam  PriceListParam;
	// @v9.1.3 PPImpExpParam           CSessParam;
	// @v9.1.3 PPImpExpParam           CCheckParam;
	// @v9.1.3 PPImpExpParam           CCheckLineParam;
	// @v9.1.3 PPCSessComplexImpExpParam CscParam;     // @v8.2.7
	PPPhoneListImpExpParam  PhoneListParam; // @v7.3.3
	PPSCardImpExpParam      SCardListParam; // @v7.3.12
	PPLotImpExpParam        LotParam;       // @v7.4.7 @Muxa
	PPPersonImpExpParam     PersonParam;    // @v7.6.1
	PPWorkbookImpExpParam   WorkbookParam;  // @v8.1.1
	PPQuotImpExpParam       QuotParam;      // @v8.5.11
};

ImpExpCfgsListDialog::ImpExpCfgsListDialog() : PPListDialog(DLG_IMPEXPCFGS, CTL_IMPEXPCFGS_LIST), CDb(0)
{
	const long sdrecs_id[] = {
		PPREC_GOODS2,
		PPREC_BILL,
		PPREC_BROW,
		PPREC_INVENTORYITEM,
		PPREC_PRICELIST,
		// @v9.1.3 PPREC_CSESS,
		// @v9.1.3 PPREC_CCHECKS,
		// @v9.1.3 PPREC_CCLINES,
		// @v9.1.3 PPREC_CSESSCOMPLEX, // @v8.2.7
		PPREC_PHONELIST,
		PPREC_SCARD,
		PPREC_LOT,
		PPREC_PERSON,
		PPREC_WORKBOOK, // @v8.1.1
		PPREC_QUOTVAL   // @v8.5.11
	};
	SString str_cfgs, buf;
	PPLoadText(PPTXT_IMPEXPCFGNAMELIST, str_cfgs);
	StringSet ss(';', str_cfgs);
	for(uint i = 0, p = 0; ss.get(&i, buf) > 0; p++)
		CfgsList.Add(sdrecs_id[p], 0, buf);
	uint   pp = 0;
	P_ParamList[pp++] = &GoodsParam;
	P_ParamList[pp++] = &BillParam;
	P_ParamList[pp++] = &BRowParam;
	P_ParamList[pp++] = &InvParam;
	P_ParamList[pp++] = &PriceListParam;
	// @v9.1.3 P_ParamList[pp++] = &CSessParam;
	// @v9.1.3 P_ParamList[pp++] = &CCheckParam;
	// @v9.1.3 P_ParamList[pp++] = &CCheckLineParam;
	// @v9.1.3 P_ParamList[pp++] = &CscParam;       // @v8.2.7
	P_ParamList[pp++] = &PhoneListParam; // @v7.3.3
	P_ParamList[pp++] = &SCardListParam; // @v7.3.12
	P_ParamList[pp++] = &LotParam;
	P_ParamList[pp++] = &PersonParam;    // @v7.6.1
	P_ParamList[pp++] = &WorkbookParam;  // @v8.1.1
	P_ParamList[pp++] = &QuotParam;      // @v8.5.11
	{
		SmartListBox * p_box = (SmartListBox*)getCtrlView(CTL_IMPEXPCFGS_CFGS);
		if(p_box) {
			SetupStrListBox(p_box);
			for(uint i = 0; i < CfgsList.getCount(); i++)
				p_box->addItem(CfgsList.at(i).Id, CfgsList.at(i).Txt);
			if(p_box->def)
				p_box->def->top();
			p_box->Draw_();
		}
	}
	CfgPos = 0;
	if(DS.CheckExtFlag(ECF_USECDB)) {
		SString db_dir;
		PPGetPath(PPPATH_BIN, db_dir);
		db_dir.SetLastSlash().Cat("CDB");
		CDb.Open(db_dir);
	}
#ifdef NDEBUG
	showButton(cmImport, 0);
#endif
	updateList(-1);
}

IMPL_HANDLE_EVENT(ImpExpCfgsListDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmLBItemFocused) && event.isCtlEvent(CTL_IMPEXPCFGS_CFGS)) {
		SmartListBox * p_box = (SmartListBox*)getCtrlView(CTL_IMPEXPCFGS_CFGS);
		if(p_box && p_box->def) {
			CfgPos = p_box->def->_curItem();
			updateList(-1);
			clearEvent(event);
		}
	}
	else if(event.isCmd(cmImport)) {
		ConvertImpExpConfig_IniToBdb();
		clearEvent(event);
	}
}

int ImpExpCfgsListDialog::addItem(long * pPos, long * pID)
{
	if(EditParam(0, pID)) {
		updateList(-1);
		*pPos = *pID = 0;
		return 1;
	}
	else
		return 0;
}

int ImpExpCfgsListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	char   section[256];
	section[0] = 0;
	if(DS.CheckExtFlag(ECF_USECDB))
		ok = EditParam(section, &id);
	else
		ok = Sections.get((uint *)&id, section, sizeof(section)) ? EditParam(section, &id) : -1;
	return ok;
}

int ImpExpCfgsListDialog::delItem(long pos, long id)
{
	int    ok = 1;
	if(DS.CheckExtFlag(ECF_USECDB)) {
		THROW(CDb.DeleteObj(id, 1));
		updateList(-1);
	}
	else {
		SString ini_file_name, section;
		if(Sections.get((uint *)&id, section)) {
			int    is_exists = 0;
			THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
			is_exists = fileExists(ini_file_name);
			{
				PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
				THROW(ini_file.RemoveSection(section));
				THROW(ini_file.FlashIniBuf());
				updateList(-1);
			}
		}
		else
			ok = -1;
	}
	CATCHZOKPPERR
	return ok;
}

ImpExpParamDialog * ImpExpCfgsListDialog::GetParamDlg(uint cfgPos)
{
	ImpExpParamDialog * p_dlg = 0;
	if(cfgPos < CfgsList.getCount()) {
		switch(CfgsList.at(cfgPos).Id) {
			case PPREC_GOODS2:
				p_dlg = new GoodsImpExpDialog;
				break;
			case PPREC_PERSON: // @v7.6.1
				p_dlg = new PersonImpExpDialog;
				break;
			case PPREC_BILL:
				p_dlg = new BillHdrImpExpDialog;
				break;
			case PPREC_BROW:
			case PPREC_INVENTORYITEM:
			case PPREC_PRICELIST:
			case PPREC_CSESS:
			case PPREC_CCHECKS:
			case PPREC_CCLINES:
			case PPREC_WORKBOOK: // @v8.1.1
				p_dlg = new ImpExpParamDialog(DLG_IMPEXP, 0);
				break;
			/* @v9.1.3 case PPREC_CSESSCOMPLEX: // @v8.2.7
				p_dlg = new CSessComplexImpExpDialog;
				break; */
			case PPREC_LOT: // @Muxa
				p_dlg = new LotImpExpDialog;
				break;
			case PPREC_PHONELIST: // @v7.3.3
				p_dlg = new PhoneListImpExpDialog;
				break;
			case PPREC_SCARD: // @v7.3.12
				p_dlg = new SCardImpExpDialog;
				break;
			case PPREC_QUOTVAL: // @v8.5.11
				p_dlg = new QuotImpExpDialog;
				break;
		}
	}
	return p_dlg;
}

int ImpExpCfgsListDialog::SetParamDlgDTS(ImpExpParamDialog * pDlg, uint cfgPos, PPImpExpParam * pParam)
{
	int    ok = 0;
	if(pDlg && cfgPos < CfgsList.getCount()) {
		switch(CfgsList.at(cfgPos).Id) {
			case PPREC_GOODS2:
				ok = ((GoodsImpExpDialog*)pDlg)->setDTS((PPGoodsImpExpParam*)pParam);
				break;
			case PPREC_PERSON: // @v7.6.1
				ok = ((PersonImpExpDialog*)pDlg)->setDTS((PPPersonImpExpParam*)pParam);
				break;
			case PPREC_BILL:
				ok = ((BillHdrImpExpDialog*)pDlg)->setDTS((PPBillImpExpParam*)pParam);
				break;
			case PPREC_BROW:
			case PPREC_INVENTORYITEM:
			case PPREC_PRICELIST:
			case PPREC_CSESS:
			case PPREC_CCHECKS:
			case PPREC_CCLINES:
			case PPREC_WORKBOOK: // @v8.1.1
				ok = ((ImpExpParamDialog *)pDlg)->setDTS(pParam);
				break;
			/* @v9.1.3 case PPREC_CSESSCOMPLEX: // @v8.2.7
				ok = ((CSessComplexImpExpDialog *)pDlg)->setDTS((PPCSessComplexImpExpParam *)pParam);
				break; */
			case PPREC_LOT: // @Muxa
				ok = ((LotImpExpDialog *)pDlg)->setDTS((PPLotImpExpParam *)pParam);
				break;
			case PPREC_PHONELIST: // @v7.3.3
				ok = ((PhoneListImpExpDialog*)pDlg)->setDTS((PPPhoneListImpExpParam*)pParam);
				break;
			case PPREC_SCARD: // @v7.3.12
				ok = ((SCardImpExpDialog*)pDlg)->setDTS((PPSCardImpExpParam*)pParam);
				break;
			case PPREC_QUOTVAL: // @v8.5.11
				ok = ((QuotImpExpDialog*)pDlg)->setDTS((PPQuotImpExpParam*)pParam);
				break;
		}
	}
	return ok;
}

int ImpExpCfgsListDialog::GetParamDlgDTS(ImpExpParamDialog * pDlg, uint cfgPos, PPImpExpParam * pParam)
{
	int    ok = 0;
	if(pDlg && cfgPos < CfgsList.getCount()) {
		switch(CfgsList.at(cfgPos).Id) {
			case PPREC_GOODS2:
				ok = ((GoodsImpExpDialog*)pDlg)->getDTS((PPGoodsImpExpParam*)pParam);
				break;
			case PPREC_PERSON: // @v7.6.1
				ok = ((PersonImpExpDialog*)pDlg)->getDTS((PPPersonImpExpParam*)pParam);
				break;
			case PPREC_BILL:
				ok = ((BillHdrImpExpDialog*)pDlg)->getDTS((PPBillImpExpParam*)pParam);
				break;
			case PPREC_BROW:
			case PPREC_INVENTORYITEM:
			case PPREC_PRICELIST:
			case PPREC_CSESS:
			case PPREC_CCHECKS:
			case PPREC_CCLINES:
			case PPREC_WORKBOOK: // @v8.1.1
				ok = ((ImpExpParamDialog *)pDlg)->getDTS(pParam);
				break;
			/* @v9.1.3 case PPREC_CSESSCOMPLEX: // @v8.2.7
				ok = ((CSessComplexImpExpDialog *)pDlg)->getDTS((PPCSessComplexImpExpParam *)pParam);
				break; */
			case PPREC_LOT: // @Muxa
				ok = ((LotImpExpDialog *)pDlg)->getDTS((PPLotImpExpParam *)pParam);
				break;
			case PPREC_PHONELIST: // @v7.3.3
				ok = ((PhoneListImpExpDialog*)pDlg)->getDTS((PPPhoneListImpExpParam*)pParam);
				break;
			case PPREC_SCARD: // @v7.3.12
				ok = ((SCardImpExpDialog*)pDlg)->getDTS((PPSCardImpExpParam*)pParam);
				break;
			case PPREC_QUOTVAL: // @v8.5.11
				ok = ((QuotImpExpDialog*)pDlg)->getDTS((PPQuotImpExpParam*)pParam);
				break;
		}
	}
	return ok;
}

int ImpExpCfgsListDialog::EditParam(const char * pIniSection, long * pCDbID)
{
	MemLeakTracer mlt;
	int    ok = -1;
	int    is_exists = 0;
	long   cdb_id = pCDbID ? *pCDbID : 0;
	ImpExpParamDialog * p_param_dlg = 0;
	SString ini_file_name, section;
	PPImpExpParam * p_param = P_ParamList[CfgPos];
	THROW(CheckDialogPtr(&(p_param_dlg = GetParamDlg(CfgPos))));
	p_param->Init();
	THROW(LoadSdRecord(CfgsList.at(CfgPos).Id, &p_param->InrRec));
	if(DS.CheckExtFlag(ECF_USECDB)) {
		//PPConfigDatabase
		PPConfigDatabase::CObjHeader cobj_hdr;
		SBuffer cobj_tail;
		if(cdb_id) {
			THROW(CDb.GetObj(cdb_id, &cobj_hdr, &cobj_tail));
			THROW(p_param->SerializeConfig(-1, cobj_hdr, cobj_tail, &SCtx));
		}
		else {
		}
		SetParamDlgDTS(p_param_dlg, CfgPos, p_param);
		while(ok <= 0 && ExecView(p_param_dlg) == cmOK) {
			if(GetParamDlgDTS(p_param_dlg, CfgPos, p_param)) {
				int is_new = BIN(cdb_id == 0);
				p_param->ProcessName(2, section = p_param->Name);
				p_param->Name = section;
				if(p_param->SerializeConfig(+1, cobj_hdr, cobj_tail, &SCtx)) {
					if(!CDb.PutObj(&cdb_id, cobj_hdr, cobj_tail, 1))
						PPError();
				}
				else
					PPError();
			}
			else
				PPError();
			ok = 1;
		}
	}
	else {
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		is_exists = fileExists(ini_file_name);
		{
			int    direction = 0;
			long   rec_id = 0;
			PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
			if(!isempty(pIniSection)) {
				THROW(p_param->ReadIni(&ini_file, pIniSection, 0));
			}
			direction = p_param->Direction;
			SetParamDlgDTS(p_param_dlg, CfgPos, p_param);
			while(ok <= 0 && ExecView(p_param_dlg) == cmOK) {
				if(GetParamDlgDTS(p_param_dlg, CfgPos, p_param)) {
					int is_new = (pIniSection && *pIniSection && p_param->Direction == direction) ? 0 : 1;
					if(!isempty(pIniSection))
						if(is_new)
							ini_file.RemoveSection(pIniSection);
						else
							ini_file.ClearSection(pIniSection);
					PPErrCode = PPERR_DUPOBJNAME;
					if((!is_new || ini_file.IsSectExists(p_param->Name) == 0) && p_param->WriteIni(&ini_file, p_param->Name) && ini_file.FlashIniBuf())
						ok = 1;
					else
						PPError();
				}
				else
					PPError();
			}
		}
	}
	CATCHZOK
	p_param->InrRec.Clear();
	delete p_param_dlg;
	return ok;
}

int ImpExpCfgsListDialog::setupList()
{
	int    ok = 1, r;
	PROFILE_START
	SString sect, sub;
	StringSet ss(SLBColumnDelim);
	setStaticText(CTL_IMPEXPCFGS_CFGNAME, CfgsList.at(CfgPos).Txt);
	PPImpExpParam * p_param = PPImpExpParam::CreateInstance(CfgsList.at(CfgPos).Id, 0); //P_ParamList[CfgPos];
	if(p_param) {
		if(DS.CheckExtFlag(ECF_USECDB)) {
			StrAssocArray list;
			SBuffer cobj_tail;
			THROW(p_param->Init());
			THROW(GetImpExpSectionsCDb(CDb, CfgsList.at(CfgPos).Id, list, 0));
			for(uint i = 0; i < list.getCount(); i++) {
				StrAssocArray::Item item = list.at(i);
				PPConfigDatabase::CObjHeader cobj_hdr;
				p_param->OtrRec.Clear();
				p_param->FileName = 0;
				THROW(CDb.GetObj(item.Id, &cobj_hdr, &cobj_tail.Clear()));
				r = p_param->SerializeConfig(-1, cobj_hdr, cobj_tail, &SCtx);
				if(r) {
					PROFILE_START
					ss.clear();
					ss.add(cobj_hdr.Name);
					ss.add(p_param->Direction ? "Import" : "Export");
					switch(p_param->DataFormat) {
						case PPImpExpParam::dfText:  sub = "Text";  break;
						case PPImpExpParam::dfDbf:   sub = "Dbf";   break;
						case PPImpExpParam::dfXml:   sub = "Xml";   break;
						case PPImpExpParam::dfSoap:  sub = "SOAP";  break;
						case PPImpExpParam::dfExcel: sub = "Excel"; break;
						default: sub = "Unknown"; break;
					}
					ss.add(sub);
					ss.add(p_param->FileName);
					THROW(addStringToList(item.Id, ss.getBuf()));
					PROFILE_END
				}
				else if(r == 0)
					PPError();
			}
		}
		else {
			Sections.clear();
			{
				SString ini_file_name, section;
				StringSet all_sections;
				PROFILE(THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name)));
				{
					PROFILE_START
					int    is_exists = fileExists(ini_file_name);
					PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
					THROW(ini_file.IsValid());
					PROFILE(THROW(ini_file.GetSections(&all_sections)));
					for(uint p = 0; all_sections.get(&p, section);) {
						THROW(p_param->Init());
						PROFILE(THROW(LoadSdRecord(CfgsList.at(CfgPos).Id, &p_param->InrRec)));
						PROFILE_START
						if(p_param->ProcessName(3, section)) {
							p_param->OtrRec.Clear();
							p_param->FileName = 0;
							PROFILE(r = p_param->ReadIni(&ini_file, section, 0));
							if(r) {
								PROFILE_START
								uint sect_id = 0;
								Sections.add(section, &sect_id);
								p_param->ProcessName(2, sect = section);
								if(sect.CmpPrefix("DLL_", 1)) { // @vmiller
									ss.clear();
									ss.add(sect);
									ss.add(p_param->Direction ? "Import" : "Export");
									switch(p_param->DataFormat) {
										case PPImpExpParam::dfText:  sub = "Text";  break;
										case PPImpExpParam::dfDbf:   sub = "Dbf";   break;
										case PPImpExpParam::dfXml:   sub = "Xml";   break;
										case PPImpExpParam::dfSoap:  sub = "SOAP";  break;
										case PPImpExpParam::dfExcel: sub = "Excel"; break;
										default: sub = "Unknown"; break;
									}
									ss.add(sub);
									ss.add(p_param->FileName);
									THROW(addStringToList(sect_id, ss.getBuf()));
								} // @vmiller
								PROFILE_END
							}
							else if(r == 0)
								PPError();
						}
						PROFILE_END
					}
					PROFILE_END
				}
			}
		}
	}
	CATCHZOK
	delete p_param;
	PROFILE_END
	return ok;
}

int SLAPI EditImpExpConfigs()
{
	ImpExpCfgsListDialog * dlg = new ImpExpCfgsListDialog();
	return CheckDialogPtrErr(&dlg) ? ((ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1) : 0;
}
//
//
//
ImpExpCfgListDialog::ImpExpCfgListDialog() : PPListDialog(DLG_IMPEXPVIEW, CTL_OBJVIEW_LIST)
{
}

ImpExpCfgListDialog::ImpExpCfgListDialog(uint iniFileID, uint sdRecID, PPImpExpParam * pParam, ImpExpParamDialog * pDlg) :
	PPListDialog(DLG_IMPEXPVIEW, CTL_OBJVIEW_LIST)
{
	SetParams(iniFileID, sdRecID, pParam, pDlg);
}

int ImpExpCfgListDialog::SetParams(uint iniFileID, uint sdRecID, PPImpExpParam * pParam, ImpExpParamDialog * pDlg)
{
	IniFileID  = iniFileID;
	SDRecID    = sdRecID;
	P_Param    = pParam;
	P_ParamDlg = pDlg;
	Sections.clear();
	{
		SString ini_file_name;
		if(PPGetFilePath(PPPATH_BIN, IniFileID, ini_file_name) && fileExists(ini_file_name)) {
			PPIniFile ini_file(ini_file_name, 0, 1, 0);
			if(ini_file.IsValid())
				ini_file.Backup(5);
		}
	}
	updateList(-1);
	return 1;
}

int ImpExpCfgListDialog::setupList()
{
	int    ok = 1;
	SString sect, sub;
	StringSet ss(SLBColumnDelim);
	Sections.clear();
	{
		SString ini_file_name, section;
		StringSet all_sections;
		THROW(PPGetFilePath(PPPATH_BIN, IniFileID, ini_file_name));
		THROW(LoadSdRecord(SDRecID, &P_Param->InrRec));
		{
			int    is_exists = fileExists(ini_file_name);
			PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
			THROW(ini_file.IsValid());
			THROW(ini_file.GetSections(&all_sections));
			for(uint p = 0; all_sections.get(&p, section);) {
				int    r = 0;
				if(P_Param->ProcessName(3, section)) {
					P_Param->OtrRec.Clear();
					P_Param->FileName = 0;
					if(r = P_Param->ReadIni(&ini_file, section, 0)) {
						uint sect_id = 0;
						Sections.add(section, &sect_id);
						P_Param->ProcessName(2, sect = section);
						ss.clear();
						ss.add(sect);
						ss.add(P_Param->Direction ? "Import" : "Export");
						switch(P_Param->DataFormat) {
							case PPImpExpParam::dfText: sub = "Text"; break;
							case PPImpExpParam::dfDbf:  sub = "Dbf"; break;
							case PPImpExpParam::dfXml:  sub = "Xml"; break;
							case PPImpExpParam::dfSoap: sub = "SOAP"; break;
							default: sub = "Unknown"; break;
						}
						ss.add(sub);
						ss.add(P_Param->FileName);
						THROW(addStringToList(sect_id, ss.getBuf()));
					}
					else if(r == 0)
						PPError();
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int ImpExpCfgListDialog::addItem(long * pPos, long * pID)
{
	if(EditParam(0)) {
		updateList(-1);
		*pPos = *pID = 0;
		return 1;
	}
	else
		return 0;
}

int ImpExpCfgListDialog::editItem(long pos, long id)
{
	char   section[256];
	return Sections.get((uint *)&id, section, sizeof(section)) ? EditParam(section) : -1;
}

int ImpExpCfgListDialog::delItem(long pos, long id)
{
	int    ok = 1;
	SString ini_file_name, section;
	if(Sections.get((uint *)&id, section)) {
		int    is_exists = 0;
		THROW(PPGetFilePath(PPPATH_BIN, IniFileID, ini_file_name));
		is_exists = fileExists(ini_file_name);
		{
			PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
			THROW(ini_file.RemoveSection(section));
			THROW(ini_file.FlashIniBuf());
			updateList(-1);
		}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int ImpExpCfgListDialog::EditParam(const char * pIniSection)
{
	int    ok = -1;
	int    is_exists = 0;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, IniFileID, ini_file_name));
	is_exists = fileExists(ini_file_name);
	{
		int direction = 0;
		PPIniFile ini_file(ini_file_name, is_exists ? 0 : 1, 1, 1);
		P_Param->Init();
		THROW(LoadSdRecord(SDRecID, &P_Param->InrRec));
		if(!isempty(pIniSection))
			THROW(P_Param->ReadIni(&ini_file, pIniSection, 0));
		THROW(CheckDialogPtr(&P_ParamDlg));
		direction = P_Param->Direction;
		P_ParamDlg->setDTS(P_Param);
		while(ok <= 0 && ExecView(P_ParamDlg) == cmOK)
			if(P_ParamDlg->getDTS(P_Param)) {
				int is_new = (pIniSection && *pIniSection && P_Param->Direction == direction) ? 0 : 1;
				if(!isempty(pIniSection))
					if(is_new)
						ini_file.RemoveSection(pIniSection);
					else
						ini_file.ClearSection(pIniSection);
				PPErrCode = PPERR_DUPOBJNAME;
				if((!is_new || ini_file.IsSectExists(P_Param->Name) == 0) && P_Param->WriteIni(&ini_file, P_Param->Name) && ini_file.FlashIniBuf())
					ok = 1;
				else
					PPError();
			}
			else
				PPError();
	}
	CATCHZOK
	return ok;
}

int SLAPI EditImpExpParams(uint iniFileID, uint sdRecID, PPImpExpParam * pParam, ImpExpParamDialog * pParamDlg)
{
	ImpExpCfgListDialog * dlg = new ImpExpCfgListDialog(iniFileID, sdRecID, pParam, pParamDlg);
	return CheckDialogPtrErr(&dlg) ? ((ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1) : 0;
}

int InitImpExpParam(uint hdrRecType, uint recType, PPImpExpParam * pParam, const char * pFileName, int forExport, long dataFormat)
{
	int    ok = 1;
	THROW_INVARG(pParam && pFileName);
	pParam->Init();
	if(hdrRecType)
		THROW(LoadSdRecord(hdrRecType, &pParam->HdrInrRec));
	THROW(LoadSdRecord(recType, &pParam->InrRec));
	pParam->Direction  = forExport ? 0 : 1;
	pParam->DataFormat = dataFormat;
	pParam->FileName = pFileName;
	pParam->OtrRec = pParam->InrRec;
	pParam->HdrOtrRec = pParam->HdrInrRec;
	CATCHZOK
	return ok;
}

int InitImpExpDbfParam(uint recType, PPImpExpParam * pParam, const char * pFileName, int forExport)
{
	return InitImpExpParam(0, recType, pParam, pFileName, forExport, PPImpExpParam::dfDbf);
}

// @vmiller {
ImpExpDll::ImpExpDll()
{
	OpKind = 0;
	Inited = 0;
	P_Lib = 0;
	InitExport = 0;
	SetExportObj = 0;
	InitExportIter = 0;
	NextExportIter = 0;
	InitImport = 0;
	GetImportObj = 0;
	InitImportIter = 0;
	NextImportIter = 0;
	ReplyImportObjStatus = 0;
	EnumExpReceipt = 0;
	FinishImpExp = 0;
	GetErrorMessage = 0;
}

ImpExpDll::~ImpExpDll()
{
	if(Inited) {
		int size = 0;
		FinishImpExp();
		ReleaseLibrary();
	}
}

int ImpExpDll::InitLibrary(const char * pDllName, uint op)
{
	int    ok = 1;
	ReleaseLibrary();
	THROW_MEM(P_Lib = new SDynLibrary(0));
	THROW_SL(P_Lib->Load(pDllName));
	OpKind = op;
	if(op == 1) {
		InitExport = (InitExpProc)P_Lib->GetProcAddr("InitExport");
		SetExportObj = (SetExpObjProc)P_Lib->GetProcAddr("SetExportObj");
		InitExportIter = (InitExpIterProc)P_Lib->GetProcAddr("InitExportObjIter");
		NextExportIter = (NextExpIterProc)P_Lib->GetProcAddr("NextExportObjIter");
		EnumExpReceipt = (EnumExpReceiptProc)P_Lib->GetProcAddr("EnumExpReceipt");
	}
	else {
		InitImport = (InitImpProc)P_Lib->GetProcAddr("InitImport");
		GetImportObj = (GetImpObjProc)P_Lib->GetProcAddr("GetImportObj");
		InitImportIter = (InitImpIterProc)P_Lib->GetProcAddr("InitImportObjIter");
		NextImportIter = (NextImpIterProc)P_Lib->GetProcAddr("NextImportObjIter");
		ReplyImportObjStatus = (ReplyImpObjStatusProc)P_Lib->GetProcAddr("ReplyImportObjStatus");
	}
	FinishImpExp = (FinishImpExpProc)P_Lib->GetProcAddr("FinishImpExp");
	GetErrorMessage = (GetErrorMessageProc)P_Lib->GetProcAddr("GetErrorMessage");
	Inited = 1;
	CATCHZOK
	return ok;
}

void ImpExpDll::ReleaseLibrary()
{
	if(P_Lib)
		ZDELETE(P_Lib);
	InitExport = 0;
	SetExportObj = 0;
	InitExportIter = 0;
	NextExportIter = 0;
	InitImport = 0;
	GetImportObj = 0;
	InitImportIter = 0;
	NextImportIter = 0;
	ReplyImportObjStatus = 0;
	EnumExpReceipt = 0;
	FinishImpExp = 0;
	GetErrorMessage = 0;
	Inited = 0;
}
// } @vmiller
