// PSNOPK.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016
//
// ���� ������������ ��������
//
#include <pp.h>
#pragma hdrstop
//
//
//
struct PoClause_Pre780 { // @persistent
	PoClause_Pre780();
	PPID   GetDirObjType() const;

	long   Num;     // Counter
	PPID   VerbID;  // POVERB_XXX
	PPID   Subj;    // POCOBJ_PRIMARY | POCOBJ_SECONDARY
	PPID   DirObj;  // ������, ��������� � ���������. ��� ������� ����� ���� ��������� � ������� ������� PoClause::GetDirObjType()
	long   Flags;
};

typedef TSArray <PoClause_Pre780> PoClauseArray_Pre780;
//
//
//
PoClause_Pre780::PoClause_Pre780()
{
	THISZERO();
}

struct VerbObjAssoc {
	enum {
		fAllowCmdText = 0x0001
	};
	int16  Verb;
	int16  ObjType;
	uint16 Flags;
};
//
//
//
SLAPI PoClauseArray_::PoClauseArray_()
{
	Pool.add("$", 0); // zero index - is empty string
}

PoClauseArray_ & FASTCALL PoClauseArray_::operator = (const PoClauseArray_ & rS)
{
	L = rS.L;
	Pool = rS.Pool;
	return *this;
}

int SLAPI PoClauseArray_::IsEqual(const PoClauseArray_ & rS, int options) const
{
	if(L.getCount() != rS.L.getCount())
		return 0;
	else {
		PoClause_ c, sc;
		for(uint i = 0; i < L.getCount(); i++) {
			Get(i, c);
			rS.Get(i, sc);
			if(!c.IsEqual(sc))
				return 0;
		}
		return 1;
	}
}

PoClauseArray_ & SLAPI PoClauseArray_::Clear()
{
	L.clear();
	Pool.clear(1);
	Pool.add("$", 0); // zero index - is empty string
	return *this;
}

uint SLAPI PoClauseArray_::GetCount() const
{
	return L.getCount();
}

int SLAPI PoClauseArray_::Get(uint pos, PoClause_ & rItem) const
{
	int    ok = 1;
	if(pos < L.getCount()) {
		const Item & r_item = L.at(pos);
		rItem.Num = r_item.Num;
		rItem.VerbID = r_item.VerbID;
		rItem.Subj = r_item.Subj;
		rItem.DirObj = r_item.DirObj;
		rItem.Flags = r_item.Flags;
		Pool.getnz(r_item.CmdTextP, rItem.CmdText);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PoClauseArray_::Add(const PoClause_ & rItem)
{
	int    ok = 1;
	Item   item;
	item.Num = rItem.Num;
	item.VerbID = rItem.VerbID;
	item.Subj = rItem.Subj;
	item.DirObj = rItem.DirObj;
	item.Flags = rItem.Flags;
	if(rItem.CmdText.NotEmpty()) {
		Pool.add(rItem.CmdText, &item.CmdTextP);
	}
	else
		item.CmdTextP = 0;
	L.insert(&item);
	return ok;
}

int SLAPI PoClauseArray_::Set(uint pos, const PoClause_ * pItem)
{
	int    ok = 1;
	if(pos < L.getCount()) {
		if(pItem) {
			Item   item;
			item.Num = pItem->Num;
			item.VerbID = pItem->VerbID;
			item.Subj = pItem->Subj;
			item.DirObj = pItem->DirObj;
			item.Flags = pItem->Flags;
			if(pItem->CmdText.NotEmpty()) {
				Pool.add(pItem->CmdText, &item.CmdTextP);
			}
			else
				item.CmdTextP = 0;
			L.at(pos) = item;
		}
		else {
			L.atFree(pos);
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PoClauseArray_::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	PoClauseArray_Pre780 list_pre780;
	if(dir > 0) {
		PoClause_ clause;
		for(uint i = 0; i < L.getCount(); i++) {
			Get(i, clause);
			PoClause_Pre780 item_pre780;
			item_pre780.Num = clause.Num;
			item_pre780.VerbID = clause.VerbID;
			item_pre780.Subj = clause.Subj;
			item_pre780.DirObj = clause.DirObj;
			item_pre780.Flags = clause.Flags;
			THROW_SL(list_pre780.insert(&item_pre780));
		}
		THROW_SL(pCtx->Serialize(dir, &list_pre780, rBuf));
	}
	else if(dir < 0) {
		THROW_SL(pCtx->Serialize(dir, &list_pre780, rBuf));
		Clear();
		PoClause_ clause;
		for(uint i = 0; i < list_pre780.getCount(); i++) {
			const PoClause_Pre780 & r_item_pre780 = list_pre780.at(i);
			clause.Num = r_item_pre780.Num;
			clause.VerbID = r_item_pre780.VerbID;
			clause.Subj = r_item_pre780.Subj;
			clause.DirObj = r_item_pre780.DirObj;
			clause.Flags = r_item_pre780.Flags;
			clause.CmdText = 0;
			THROW(Add(clause));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
static VerbObjAssoc __VerbObjAssocList[] = {
	{ POVERB_ASSIGNKIND,       PPOBJ_PRSNKIND, 0},
	{ POVERB_REVOKEKIND,       PPOBJ_PRSNKIND, 0},
	{ POVERB_SETTAG,           PPOBJ_TAG, 0},
	{ POVERB_REMOVETAG,        PPOBJ_TAG, 0},
	{ POVERB_ASSIGNPOST,       0, 0},
	{ POVERB_REVOKEPOST,       0, 0},
	{ POVERB_ASSIGNREG,        0, 0 },
	{ POVERB_REVOKEREG,        PPOBJ_REGISTERTYPE, 0},
	{ POVERB_SETCALENDAR,      PPOBJ_STAFFCAL, 0},
	{ POVERB_COMPLETECAL,      PPOBJ_STAFFCAL, 0},
	{ POVERB_INCTAG,           PPOBJ_TAG, 0},
	{ POVERB_DECTAG,           PPOBJ_TAG, 0},
	{ POVERB_SETCALCONT,       PPOBJ_STAFFCAL, 0},
	{ POVERB_SETCALCONT_SKIP,  PPOBJ_STAFFCAL, 0},
	{ POVERB_RESETCALCONT,     PPOBJ_STAFFCAL, 0},
	{ POVERB_SETCALENDAR_SKIP, PPOBJ_STAFFCAL, 0},
	{ POVERB_COMPLETECAL_SKIP, PPOBJ_STAFFCAL, 0},
	{ POVERB_ADDRELATION,      PPOBJ_PERSONRELTYPE, 0},
	{ POVERB_REVOKERELATION,   PPOBJ_PERSONRELTYPE, 0},
	{ POVERB_INCSCARDOP,       PPOBJ_GOODS, VerbObjAssoc::fAllowCmdText },
	{ POVERB_DECSCARDOP,       PPOBJ_GOODS, VerbObjAssoc::fAllowCmdText },
	{ POVERB_DEVICECMD,        0, VerbObjAssoc::fAllowCmdText },
	{ POVERB_STYLODISPLAY,     PPOBJ_STYLOPALM, 0 },
	{ POVERB_BEEP,             0, VerbObjAssoc::fAllowCmdText },
	{ POVERB_CHKSCARDBILLDEBT, 0, 0 }
};
//
//
//
SLAPI PoClause_::PoClause_()
{
	Num = 0;
	VerbID = 0;
	Subj = 0;
	DirObj = 0;
	Flags = 0;
}

int FASTCALL PoClause_::IsEqual(const PoClause_ & rS) const
{
	if(Num != rS.Num)
		return 0;
	else if(VerbID != rS.VerbID)
		return 0;
	else if(Subj != rS.Subj)
		return 0;
	else if(DirObj != rS.DirObj)
		return 0;
	else if(Flags != rS.Flags)
		return 0;
	else if(CmdText != rS.CmdText)
		return 0;
	else
		return 1;
}

PPID SLAPI PoClause_::GetDirObjType() const
{
	PPID   obj_type = 0;
	if(VerbID)
		for(uint i = 0; !obj_type && i < SIZEOFARRAY(__VerbObjAssocList); i++) {
			if(__VerbObjAssocList[i].Verb == (int16)VerbID)
				obj_type = __VerbObjAssocList[i].ObjType;
		}
	return obj_type;
}

long SLAPI PoClause_::GetDirFlags() const
{
	long   flags = 0;
	if(VerbID)
		for(uint i = 0; i < SIZEOFARRAY(__VerbObjAssocList); i++) {
			if(__VerbObjAssocList[i].Verb == (int16)VerbID) {
				flags = (long)__VerbObjAssocList[i].Flags;
				break;
			}
		}
	return flags;
}
//
//
//
int FASTCALL PPPsnOpKind2::IsEqual(const PPPsnOpKind2 & rS) const
{
#define TEST_FLD(fld) if(fld != rS.fld) return 0
	if(stricmp(Name, rS.Name) != 0)
		return 0;
	if(stricmp(Symb, rS.Symb) != 0)
		return 0;
	TEST_FLD(ParentID);
	TEST_FLD(RestrStaffCalID); // @v8.0.10
	TEST_FLD(RscMaxTimes);     // @v8.0.10 ������������ ���������� �������, ������� ����� ��������� � ������� ������ ������� ��������� RestrStaffCalID.
	TEST_FLD(RegTypeID);
	TEST_FLD(ExValGrp);
	TEST_FLD(PairType);
	TEST_FLD(ExValSrc);
	TEST_FLD(Flags);
	TEST_FLD(LinkBillOpID);
	TEST_FLD(PairOp);
	return 1;
#undef TEST_FLD
}

int FASTCALL PPPsnOpKindPacket::IsEqual(const PPPsnOpKindPacket & rS) const
{
#define TEST_FLD(fld) if(fld != rS.fld) return 0
	if(!Rec.IsEqual(rS.Rec))
		return 0;
	TEST_FLD(PCPrmr.PersonKindID);
	TEST_FLD(PCPrmr.StatusType);
	TEST_FLD(PCPrmr.DefaultID);
	TEST_FLD(PCPrmr.RestrictTagID);
	TEST_FLD(PCPrmr.RestrictScSerList); // @v7.8.9
	TEST_FLD(PCScnd.PersonKindID);
	TEST_FLD(PCScnd.StatusType);
	TEST_FLD(PCScnd.DefaultID);
	TEST_FLD(PCScnd.RestrictTagID);
	TEST_FLD(PCScnd.RestrictScSerList); // @v7.8.9
	if(!ClauseList.IsEqual(rS.ClauseList))
		return 0;
	if(!AllowedTags.IsEqual(rS.AllowedTags))
		return 0;
	return 1;
#undef TEST_FLD
}

PPPsnOpKindPacket::PsnConstr::PsnConstr()
{
	Clear();
}

PPPsnOpKindPacket::PsnConstr & PPPsnOpKindPacket::PsnConstr::Clear()
{
	PersonKindID = 0;
	StatusType = 0;
	Reserve1 = 0;
	DefaultID = 0;
	RestrictTagID = 0;
	RestrictScSerList.freeAll();
	return *this;
}

SLAPI PPPsnOpKindPacket::PPPsnOpKindPacket()
{
	destroy();
}

void SLAPI PPPsnOpKindPacket::destroy()
{
	MEMSZERO(Rec);
	PCPrmr.Clear();
	PCScnd.Clear();
	ClauseList.Clear();
	AllowedTags.FreeAll();
}

PPPsnOpKindPacket & FASTCALL PPPsnOpKindPacket::operator = (const PPPsnOpKindPacket & src)
{
	Rec = src.Rec;
	PCPrmr = src.PCPrmr;
	PCScnd = src.PCScnd;
	ClauseList = src.ClauseList;
	AllowedTags = src.AllowedTags;
	return *this;
}

int SLAPI PPPsnOpKindPacket::CheckExVal()
{
	int    ok = 1;
	if(Rec.ExValGrp == POKEVG_TAG) {
		PPIDArray tags_list;
		if(AllowedTags.IsExists())
			tags_list = AllowedTags.Get();
		uint tags_count = tags_list.getCount();
		THROW_PP(tags_count, PPERR_UNDEFPOKEVTAG);
		{
			PPObjTag tagobj;
			PPObjectTag tag;
			for(uint i = 0; i < tags_count; i++) {
				THROW_PP(tagobj.Fetch(tags_list.at(i), &tag) > 0, PPERR_UNDEFPOKEVTAG);
			}
		}
	}
	else {
		Rec.ExValSrc = 0;
		AllowedTags.FreeAll();
		THROW_PP(Rec.ExValGrp == POKEVG_POST || Rec.ExValGrp == POKEVG_NONE, PPERR_INVPOKEVG);
	}
	CATCHZOK
	return ok;
}
//
//
//
PsnOpKindView::PsnOpKindView(PPObjPsnOpKind * pObj) : PPListDialog(DLG_PSNOPKINDVIEW, CTL_OBJVIEW_LIST)
{
	P_Items   = 0;
	IterNo    = 0;
	updateList(-1);
}

PsnOpKindView::~PsnOpKindView()
{
	delete P_Items;
}

PPID PsnOpKindView::getCurrID()
{
	PPID   id = 0;
	return (P_Box && P_Box->getCurID(&id)) ? id : 0;
}

int PsnOpKindView::addItem(long * pPos, long * pID)
{
	PPID   obj_id = 0;
	int    r = Obj.Edit(&obj_id, 0);
	if(r == cmOK) {
		ASSIGN_PTR(pID, obj_id);
		return 2;
	}
	else if(r == 0)
		return 0;
	else
		return -1;
}

int PsnOpKindView::editItem(long, long id)
{
	int    r = id ? Obj.Edit(&id, 0) : -1;
	return (r == cmOK) ? 1 : ((r == 0) ? 0 : -1);
}

int PsnOpKindView::delItem(long, long id)
{
	int    ok = -1;
	if(id && PPMessage(mfConf|mfYesNo, PPCFM_DELETE, 0) == cmYes) {
		ok = Obj.PutPacket(&id, 0, 1);
		if(!ok)
			PPError();

	}
	return ok;
}

// virtual
int PsnOpKindView::setupList()
{
	int    ok = -1;
	if(P_Box) {
		P_Box->setDef(Obj.Selector(0));
		ok = 1;
	}
	return ok;
}

IMPL_HANDLE_EVENT(PsnOpKindView)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		PPID   id = getCurrID();
		switch(TVCMD) {
			case cmSysJournalByObj:
				if(id)
					ViewSysJournal(PPOBJ_PERSONOPKIND, id, 1);
				break;
			case cmPrint:
				{
					PView  pv(this);
					PPAlddPrint(REPORT_PSNOPKINDVIEW, &pv, 0);
				}
				break;
			case cmTransmit:
				{
					PPIDArray id_list;
					ReferenceTbl::Rec rec;
					for(PPID id = 0; Obj.EnumItems(&id, &rec) > 0;)
						id_list.add(rec.ObjID);
					if(id_list.getCount()) {
						ObjTransmitParam param;
						if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
							const PPIDArray & rary = param.DestDBDivList.Get();
							PPObjIDArray objid_ary;
							PPWait(1);
							if(!objid_ary.Add(Obj.Obj, id_list) || (!PPObjectTransmit::Transmit(&rary, &objid_ary, &param))) {
								PPError();
							}
							PPWait(0);
						}
					}
				}
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

int PsnOpKindView::InitIteration()
{
	StrAssocArray * p_list = Obj.MakeList(0);
	ZDELETE(P_Items);
	IterNo  = 0;
	P_Items = new StrAssocArray;
	CopyList(0, p_list, P_Items);
	ZDELETE(p_list);
	return (P_Items && P_Items->getCount()) ? 1 : -1;
}

int PsnOpKindView::NextIteration(PPPsnOpKind * pItem)
{
	int    ok = -1;
	if(P_Items && IterNo < P_Items->getCount())
		ok = Obj.Search(P_Items->at(IterNo++).Id, pItem);
	return ok;
}

long PsnOpKindView::GetLevel(PPID opKindID)
{
	return Obj.GetLevel(opKindID);
}

int PsnOpKindView::CopyList(PPID parentID, StrAssocArray * pSrc, StrAssocArray * pDest)
{
	if(pSrc && pDest && pSrc->getCount()) {
		LongArray list;
		pSrc->GetListByParent(parentID, 0, list);
		for(uint i = 0; i < list.getCount(); i++) {
			uint pos = 0;
			PPID id = list.at(i);
			if(pSrc->Search(id, &pos) > 0) {
				StrAssocArray::Item item = pSrc->at(pos);
				if(pSrc->HasChild(id)) {
					pDest->Add(item.Id, item.ParentId, item.Txt);
					CopyList(item.Id, pSrc, pDest); // @recursion
				}
				else if(parentID != 0 || item.ParentId == 0)
					pDest->Add(item.Id, item.ParentId, item.Txt);
			}
		}
	}
	return 1;
}

// static
PPID SLAPI PPObjPsnOpKind::Select(long)
{
	PPID   id = 0;
	int    r = PPSelectObject(PPOBJ_PERSONOPKIND, &id, PPTXT_SELECTPSNOP, 0);
	return (r > 0) ? id : ((r < 0) ? -1 : 0);
}

// static
int SLAPI PPObjPsnOpKind::CheckRecursion(PPID id, PPID parentID)
{
	int    ok = 1;
	if(id) {
		PPObjPsnOpKind obj;
		do {
			 if(parentID) {
				PPPsnOpKind rec;
				THROW_PP(id != parentID, PPERR_RECURSIONFOUND);
				parentID = (obj.Search(parentID, &rec) > 0) ? rec.ParentID : 0;
			 }
		 } while(parentID);
	}
	CATCHZOK
	return ok;
}

SLAPI PPObjPsnOpKind::PPObjPsnOpKind(void * extraPtr) : PPObjReference(PPOBJ_PERSONOPKIND, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	if(extraPtr)
		CurrFilt = *(PsnOpKindFilt*)extraPtr;
}

long SLAPI PPObjPsnOpKind::GetLevel(PPID id)
{
	long   level = 0;
	PPPsnOpKind item;
	if(Search(id, &item) > 0) {
		PPID   parent_id = item.ParentID;
		PPID   id = item.ParentID;
		do {
			MEMSZERO(item);
			if(id && Search(id, &item) > 0)
				level++;
			id = item.ParentID;
		} while(id && id != parent_id);
	}
	return level;
}

// virtual
void * SLAPI PPObjPsnOpKind::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjPsnOpKindListWindow : public PPObjListWindow {
	public:
		PPObjPsnOpKindListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_PSNOPKIND);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				getResult(&id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmPrint:
							{
								PView  pv(this);
								PPAlddPrint(REPORT_PSNOPKINDVIEW, &pv, 0);
							}
							break;
						case cmaMore:
							if(id) {
								PersonEventFilt filt;
								filt.PsnOpList.Add(id);
								((PPApp*)APPL)->LastCmd = TVCMD;
								PPView::Execute(PPVIEW_PERSONEVENT, &filt, 1, 0);
							}
							break;
						case cmTransmit:
							{
								PPIDArray id_list;
								ReferenceTbl::Rec rec;
								PPObjPsnOpKind pk_obj;
								for(PPID id = 0; pk_obj.EnumItems(&id, &rec) > 0;)
									id_list.add(rec.ObjID);
								if(id_list.getCount()) {
									ObjTransmitParam param;
									if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
										const PPIDArray & rary = param.DestDBDivList.Get();
										PPObjIDArray objid_ary;
										PPWait(1);
										if(!objid_ary.Add(pk_obj.Obj, id_list) || (!PPObjectTransmit::Transmit(&rary, &objid_ary, &param))) {
											PPError();
										}
										PPWait(0);
									}
								}
							}
							break;
					}
				}
				PostProcessHandleEvent(update, id);
			}
		}
	};
	return new PPObjPsnOpKindListWindow(this, flags, extraPtr);
}

// virtual
int SLAPI PPObjPsnOpKind::Browse(void * extraPtr)
{
	int    ok = 0;
	if(CheckRights(PPR_READ)) {
		int    ok = 1;
		TDialog * p_dlg = new PsnOpKindView(this);
		if(CheckDialogPtr(&p_dlg, 1)) {
			ExecViewAndDestroy(p_dlg);
			ok = 1;
		}
	}
	else
		PPError();
	return ok;
}

// virtual
int  SLAPI PPObjPsnOpKind::ValidateSelection(PPID id, uint olwFlags, void * extraPtr)
{
	int    ok = 0;
	PPPsnOpKind rec;
	if(Search(id, &rec) > 0) {
		if((olwFlags & OLW_CANSELUPLEVEL) || !(rec.Flags & POKF_GROUP))
			ok = 1;
	}
	return ok;
}

struct _POKExtra {         // @persistent @store(PropertyTbl)
	PPID   Tag;            // Const=PPOBJ_PERSONOPKIND
	PPID   ID;             // @id
	PPID   Prop;           // Const=POKPRP_EXTRA
	PPID   PrmrKindID;
	PPID   ScndKindID;
	short  PrmrStatusType;
	short  ScndStatusType;
	PPID   ScndDefaultID;
	PPID   PrmrRestrictTagID;  // @v6.2.0
	PPID   ScndRestrictTagID;  // @v6.2.0
	SVerT Ver;                // @v7.8.9 ������ �������, ��������� ������
	uint32 Size;               // @v7.8.9 ������ ������ ��������� ������� ��������� ����������
	long   Reserve[10];
	// AllowedTags[]
	// PrmrRestrictScSerList[]
	// ScndRestrictScSerList[]
};

struct _POKClause {        // @persistent @store(PropertyTbl)
	enum {
		extstrCmdText = 1
	};
	PPID   Tag;            // Const=PPOBJ_PERSONOPKIND
	PPID   ID;             // @id
	PPID   Prop;           // POKPRP_FIRSTCLAUSE..
	long   Num;            // == Prop
	PPID   VerbID;         //
	PPID   Subj;           //
	PPID   DirObj;         //
	long   Flags;          //
	SVerT Ver;            // ������ �������, ��������� ������
	uint32 Size;           // ������ ������ ��������� ������� ��������� ������
	long   Reserve[11];    // @reserve
	// .. ExtString
};

int SLAPI PoClause_::PutToPropBuf(STempBuffer & rBuf) const
{
	int    ok = 1;
	SString ext_string;
	PPPutExtStrData(_POKClause::extstrCmdText, ext_string, CmdText);
	const size_t es_len = ext_string.Len();
	const size_t sz = sizeof(_POKClause) + (es_len ? (es_len+1) : 0);
	THROW_SL(rBuf.Alloc(sz));
	memzero(rBuf, rBuf.GetSize());
	_POKClause * p_sbuf = (_POKClause *)(char *)rBuf;
	p_sbuf->Num = Num;
	p_sbuf->VerbID = VerbID;
	p_sbuf->Subj = Subj;
	p_sbuf->DirObj = DirObj;
	p_sbuf->Flags = Flags;
	p_sbuf->Ver = DS.GetVersion();
	p_sbuf->Size = sz;
	if(sz > sizeof(*p_sbuf))
		ext_string.CopyTo((char *)(p_sbuf+1), sz-sizeof(*p_sbuf));
	CATCHZOK
	return ok;
}

int SLAPI PoClause_::GetFromPropBuf(STempBuffer & rBuf, long exValSrc)
{
	int    ok = 1;
	const _POKClause * p_sbuf = (_POKClause *)(char *)rBuf;
	if(rBuf.GetSize() >= sizeof(*p_sbuf)) {
		size_t sz = sizeof(*p_sbuf);
		if(p_sbuf->Ver.IsGt(7, 7, 12)) {
			assert(rBuf.GetSize() >= p_sbuf->Size);
			sz = p_sbuf->Size;
		}
		assert(p_sbuf->Tag == PPOBJ_PERSONOPKIND);
		Num    = p_sbuf->Num;
		VerbID = p_sbuf->VerbID;
		Subj   = p_sbuf->Subj;
		DirObj = p_sbuf->DirObj;
		DirObj = (p_sbuf->VerbID == POVERB_SETTAG && exValSrc) ? exValSrc : p_sbuf->DirObj;
		Flags  = p_sbuf->Flags;
		CmdText = 0;
		if(sz > sizeof(*p_sbuf)) {
			SString ext_string = (const char *)(p_sbuf+1);
			PPGetExtStrData(_POKClause::extstrCmdText, ext_string, CmdText);
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPObjPsnOpKind::GetPacket(PPID id, PPPsnOpKindPacket * pack)
{
	int    ok = 1, r = -1;
	int    allowed_tags_processed = 0;
	_POKExtra * p_ex = 0;
	pack->destroy();
	if((r = Search(id, &pack->Rec)) > 0) {
		size_t extra_sz = 0;
		if(PPRef->GetPropActualSize(Obj, id, POKPRP_EXTRA, &extra_sz) > 0) {
			p_ex = (_POKExtra *)malloc(extra_sz);
			THROW(PPRef->GetProp(Obj, id, POKPRP_EXTRA, p_ex, extra_sz) > 0);
			if(extra_sz >= sizeof(_POKExtra)) {
				pack->PCPrmr.PersonKindID = p_ex->PrmrKindID;
				pack->PCScnd.PersonKindID = p_ex->ScndKindID;
				pack->PCPrmr.StatusType   = p_ex->PrmrStatusType;
				pack->PCScnd.StatusType   = p_ex->ScndStatusType;
				pack->PCScnd.DefaultID    = p_ex->ScndDefaultID;
				pack->PCPrmr.RestrictTagID = p_ex->PrmrRestrictTagID;
				pack->PCScnd.RestrictTagID = p_ex->ScndRestrictTagID;
				if(p_ex->Ver.IsGt(7, 8, 8)) {
					allowed_tags_processed = 1;
					if(extra_sz > sizeof(_POKExtra)) {
						size_t p4 = 0;
						uint   i;
						uint   c = 0;
						c = PTR32((p_ex+1))[p4++];
						for(i = 0; i < c; i++) {
							pack->AllowedTags.Add((PPID)PTR32((p_ex+1))[p4++]);
						}
						c = PTR32((p_ex+1))[p4++];
						for(i = 0; i < c; i++) {
							pack->PCPrmr.RestrictScSerList.add((PPID)PTR32((p_ex+1))[p4++]);
						}
						c = PTR32((p_ex+1))[p4++];
						for(i = 0; i < c; i++) {
							pack->PCScnd.RestrictScSerList.add((PPID)PTR32((p_ex+1))[p4++]);
						}
						//assert((p4*sizeof(uint32) + sizeof(_POKExtra)) == sz);
					}
				}
			}
		}
		long   exvalsrc = pack->Rec.ExValSrc;
		STempBuffer prop_buf(2048);
		if(!allowed_tags_processed) {
			PPIDArray tags_list;
			THROW(PPRef->GetPropArray(Obj, id, POKPRP_ALLOWEDTAGS, &tags_list));
			pack->AllowedTags.Set(&tags_list);
		}
		if(pack->Rec.ExValGrp == POKEVG_TAG) {
			pack->AllowedTags.Add(pack->Rec.ExValSrc);
			pack->Rec.ExValSrc = 0;
		}
		for(PPID prop_id = 0; (r = ref->EnumProps(Obj, id, &prop_id, (char *)prop_buf, prop_buf.GetSize())) > 0;) {
			/* @v7.8.9 if(prop_id == POKPRP_EXTRA) {
				_POKExtra & ex = *(_POKExtra*)(char *)prop_buf;
				pack->PCPrmr.PersonKindID = ex.PrmrKindID;
				pack->PCScnd.PersonKindID = ex.ScndKindID;
				pack->PCPrmr.StatusType   = ex.PrmrStatusType;
				pack->PCScnd.StatusType   = ex.ScndStatusType;
				pack->PCScnd.DefaultID    = ex.ScndDefaultID;
				pack->PCPrmr.RestrictTagID = ex.PrmrRestrictTagID;
				pack->PCScnd.RestrictTagID = ex.ScndRestrictTagID;
			}
			else*/
				if(prop_id >= POKPRP_FIRSTCLAUSE && prop_id <= POKPRP_LASTCLAUSE) {
					PoClause_ clause;
					if(clause.GetFromPropBuf(prop_buf, exvalsrc)) {
						if(!pack->ClauseList.Add(clause)) {
							ok = 0;
							break;
						}
					}
				}
		}
		if(r == 0)
			ok = 0;
	}
	else
		ok = r;
	CATCHZOK
	free(p_ex);
	return ok;
}

void * SLAPI PPPsnOpKindPacket::AllocExtraProp(size_t * pSz) const
{
	_POKExtra * p_ex = 0;
	size_t sz = 0;
	const uint allt_c  = AllowedTags.GetCount();
	const uint pscsl_c = PCPrmr.RestrictScSerList.getCount();
	const uint sscsl_c = PCScnd.RestrictScSerList.getCount();
	if(PCPrmr.PersonKindID || PCScnd.PersonKindID || PCPrmr.StatusType || PCScnd.StatusType || PCScnd.DefaultID ||
		PCPrmr.RestrictTagID || PCScnd.RestrictTagID || pscsl_c || sscsl_c || allt_c) {
		sz = sizeof(_POKExtra);
		if(pscsl_c || sscsl_c || allt_c) {
			sz += sizeof(uint32); // AllowedTags.GetCount()
			sz += allt_c * sizeof(uint32);
			sz += sizeof(uint32); // PCPrmr.RestrictScSerList.getCount()
			sz += pscsl_c * sizeof(uint32);
			sz += sizeof(uint32); // PCScnd.RestrictScSerList.getCount()
			sz += sscsl_c * sizeof(uint32);
		}
		p_ex = (_POKExtra *)malloc(sz);
		THROW_MEM(p_ex);
		memzero(p_ex, sz);
		p_ex->PrmrKindID     = PCPrmr.PersonKindID;
		p_ex->ScndKindID     = PCScnd.PersonKindID;
		p_ex->PrmrStatusType = PCPrmr.StatusType;
		p_ex->ScndStatusType = PCScnd.StatusType;
		p_ex->ScndDefaultID  = PCScnd.DefaultID;
		p_ex->PrmrRestrictTagID = PCPrmr.RestrictTagID;
		p_ex->ScndRestrictTagID = PCScnd.RestrictTagID;
		p_ex->Ver = DS.GetVersion();
		p_ex->Size = sz;
		if(pscsl_c || sscsl_c || allt_c) {
			uint   i;
			size_t p4 = 0;
			PTR32((p_ex+1))[p4++] = allt_c;
			for(i = 0; i < allt_c; i++) {
				PTR32((p_ex+1))[p4++] = (uint32)AllowedTags.Get(i);
			}
			PTR32((p_ex+1))[p4++] = pscsl_c;
			for(i = 0; i < pscsl_c; i++) {
				PTR32((p_ex+1))[p4++] = (uint32)PCPrmr.RestrictScSerList.get(i);
			}
			PTR32((p_ex+1))[p4++] = sscsl_c;
			for(i = 0; i < sscsl_c; i++) {
				PTR32((p_ex+1))[p4++] = (uint32)PCScnd.RestrictScSerList.get(i);
			}
			assert((p4*sizeof(uint32) + sizeof(_POKExtra)) == sz);
		}
	}
	CATCH
		ZDELETE(p_ex);
	ENDCATCH
	ASSIGN_PTR(pSz, sz);
	return p_ex;
}

int SLAPI PPObjPsnOpKind::PutPacket(PPID * pID, PPPsnOpKindPacket * pPack, int use_ta)
{
	int    ok = 1;
	uint   i;
	PPID   prop_id;
	_POKExtra * p_ex = 0;
	PoClause_ clause;
	PPIDArray tags_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			SString ext_string;
			size_t extra_sz = 0;
			THROW(CheckRights(*pID ? PPR_MOD : PPR_INS));
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(ref->CheckUniqueSymb(Obj, *pID, pPack->Rec.Symb, offsetof(PPPsnOpKind, Symb)));
			pPack->Rec.ExValSrc = (pPack->Rec.ExValGrp == POKEVG_TAG) ? 0 : pPack->Rec.ExValSrc;
			THROW(EditItem(Obj, *pID, &pPack->Rec, 0));
			*pID = pPack->Rec.ID;
			p_ex = (_POKExtra *)pPack->AllocExtraProp(&extra_sz);
			THROW(extra_sz == 0 || p_ex);
			THROW(ref->PutProp(Obj, *pID, POKPRP_EXTRA, p_ex, extra_sz, 0));
			THROW_DB(deleteFrom(&ref->Prop, 0, (ref->Prop.ObjType == Obj && ref->Prop.ObjID == *pID &&
				ref->Prop.Prop >= POKPRP_FIRSTCLAUSE && ref->Prop.Prop <= POKPRP_LASTCLAUSE)));
			for(i = 0; i < pPack->ClauseList.GetCount(); i++) {
				pPack->ClauseList.Get(i, clause);
				if((prop_id = POKPRP_FIRSTCLAUSE + i) <= POKPRP_LASTCLAUSE) {
					STempBuffer sbuf(0);
					THROW(clause.PutToPropBuf(sbuf));
					THROW(ref->PutProp(Obj, *pID, prop_id, (char *)sbuf, sbuf.GetSize(), 0));
				}
			}
			/* @v7.8.9 {
			if(pPack->AllowedTags.IsExists())
				tags_list = pPack->AllowedTags.Get();
			THROW(PPRef->PutPropArray(Obj, *pID, POKPRP_ALLOWEDTAGS, &tags_list, 0));
			*/
		}
		else if(*pID) {
			THROW(CheckRights(PPR_DEL));
			THROW(ref->RemoveItem(Obj, *pID, 0));
			DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	free(p_ex);
	return ok;
}

StrAssocArray * SLAPI PPObjPsnOpKind::MakeList(const PsnOpKindFilt * pFilt)
{
	int    r;
	PPID   id = 0;
	PPPsnOpKind rec;
	StrAssocArray * p_list = new StrAssocArray();
	const  PsnOpKindFilt * p_filt = NZOR(pFilt, &CurrFilt);
	THROW_MEM(p_list);
	while((r = EnumItems(&id, &rec)) > 0) {
		int apply = 1;
		if(p_filt->ParentID && p_filt->ParentID != rec.ParentID)
			apply = 0;
		else if(p_filt->Show == PsnOpKindFilt::tShowItems && (rec.Flags & POKF_GROUP))
			apply = 0;
		else if(p_filt->Show == PsnOpKindFilt::tShowGroups && !(rec.Flags & POKF_GROUP))
			apply = 0;
		if(apply) {
			if(*strip(rec.Name) == 0)
				ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
			THROW_SL(p_list->Add(rec.ID, rec.ParentID, rec.Name));
		}
	}
	THROW(r);
	p_list->RemoveRecursion(0); // @v8.3.0
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

StrAssocArray * SLAPI PPObjPsnOpKind::MakeStrAssocList(void * extraPtr)
{
	return MakeList((const PsnOpKindFilt*)extraPtr);
}

int SLAPI PPObjPsnOpKind::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPPsnOpKindPacket);
	PPPsnOpKindPacket * p_pack = (PPPsnOpKindPacket *)p->Data;
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPsnOpKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1, ta = 0;
	if(p && p->Data) {
		PPPsnOpKindPacket * p_pack = (PPPsnOpKindPacket *)p->Data;
		SString added_buf;
		if(stream == 0) {
			PPID   same_id = 0;
			PPPsnOpKindPacket same_pack;
			if(*pID == 0 && p_pack->Rec.Symb[0] && SearchBySymb(p_pack->Rec.Symb, &same_id, &same_pack.Rec) > 0) {
				*pID = same_id;
			}
			if(*pID) {
				if(GetPacket(*pID, &same_pack) > 0 && p_pack->IsEqual(same_pack)) {
					ok = 1; // ����� � �� �� ���������� �� ��������� ������
				}
				else {
					if(!PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPSNOPKIND, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
				}
			}
			else {
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPSNOPKIND, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else
					ok = 101; // @ObjectCreated
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPsnOpKind::SerializePacket(int dir, PPPsnOpKindPacket * pPack, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, pPack->PCPrmr.PersonKindID, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCPrmr.StatusType, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCPrmr.Reserve1, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCPrmr.DefaultID, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCPrmr.RestrictTagID, rBuf));
	THROW_SL(pCtx->Serialize(dir, &pPack->PCPrmr.RestrictScSerList, rBuf)); // @v8.2.3

	THROW_SL(pCtx->Serialize(dir, pPack->PCScnd.PersonKindID, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCScnd.StatusType, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCScnd.Reserve1, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCScnd.DefaultID, rBuf));
	THROW_SL(pCtx->Serialize(dir, pPack->PCScnd.RestrictTagID, rBuf));
	THROW_SL(pCtx->Serialize(dir, &pPack->PCScnd.RestrictScSerList, rBuf)); // @v8.2.3

	THROW_SL(pPack->ClauseList.Serialize(dir, rBuf, pCtx));
	THROW(pPack->AllowedTags.Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjPsnOpKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		uint   i;
		PPPsnOpKindPacket * p_pack = (PPPsnOpKindPacket *)p->Data;
		ProcessObjRefInArray(PPOBJ_PERSONOPKIND,  &p_pack->Rec.ParentID,     ary, replace);
		ProcessObjRefInArray(PPOBJ_REGISTERTYPE,  &p_pack->Rec.RegTypeID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_OPRKIND,       &p_pack->Rec.LinkBillOpID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSONOPKIND,  &p_pack->Rec.PairOp,       ary, replace);
		//
		ProcessObjRefInArray(PPOBJ_PRSNKIND,      &p_pack->PCPrmr.PersonKindID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,        &p_pack->PCPrmr.DefaultID,     ary, replace);
		ProcessObjRefInArray(PPOBJ_TAG,           &p_pack->PCPrmr.RestrictTagID, ary, replace);
		// @v8.2.3 {
		for(i = 0; i < p_pack->PCPrmr.RestrictScSerList.getCount(); i++) {
			ProcessObjRefInArray(PPOBJ_SCARDSERIES, &p_pack->PCPrmr.RestrictScSerList.at(i), ary, replace);
		}
		// } @v8.2.3
		//
		ProcessObjRefInArray(PPOBJ_PRSNKIND,      &p_pack->PCScnd.PersonKindID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,        &p_pack->PCScnd.DefaultID,     ary, replace);
		ProcessObjRefInArray(PPOBJ_TAG,           &p_pack->PCScnd.RestrictTagID, ary, replace);
		// @v8.2.3 {
		for(i = 0; i < p_pack->PCScnd.RestrictScSerList.getCount(); i++) {
			ProcessObjRefInArray(PPOBJ_SCARDSERIES, &p_pack->PCScnd.RestrictScSerList.at(i), ary, replace);
		}
		// } @v8.2.3
		//
		if(p_pack->AllowedTags.IsExists()) {
			PPIDArray temp_list = p_pack->AllowedTags.Get();
			for(i = 0; i < temp_list.getCount(); i++) {
				ProcessObjRefInArray(PPOBJ_TAG, &temp_list.at(i), ary, replace);
			}
			if(replace)
				p_pack->AllowedTags.Set(&temp_list);
		}
		//
		{
			PoClause_ clause;
			for(i = 0; i < p_pack->ClauseList.GetCount(); i++) {
				p_pack->ClauseList.Get(i, clause);
				PPID   obj_type = clause.GetDirObjType();
				ProcessObjRefInArray(obj_type, &clause.DirObj, ary, replace);
				if(replace) {
					p_pack->ClauseList.Set(i, &clause);
				}
			}
		}
	}
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjPsnOpKind, PPPsnOpKindPacket);
//
//
//
class PsnOpExVDialog : public PPListDialog {
public:
	PsnOpExVDialog() : PPListDialog(DLG_POKEXV, CTL_POKEXV_ALLOWEDTAGS)
	{
		Data.destroy();
	}
	int    setDTS(PPPsnOpKindPacket * pPack)
	{
		if(!RVALUEPTR(Data, pPack))
			Data.destroy();
		ushort v = Data.Rec.ExValGrp;
		setCtrlData(CTL_POKEXV_GRP, &v);
		disableTagsList(Data.Rec.ExValGrp != POKEVG_TAG);
		updateList(-1);
		return 1;
	}
	int    getDTS(PPPsnOpKindPacket * pPack)
	{
		int    ok = 1;
		ushort v;
		getCtrlData(CTL_POKEXV_GRP, &v);
		Data.Rec.ExValGrp = v;
		if(Data.Rec.ExValGrp != POKEVG_TAG)
			Data.AllowedTags.FreeAll();
		if(!Data.CheckExVal())
			ok = 0;
		else
			ASSIGN_PTR(pPack, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isClusterClk(CTL_POKEXV_GRP)) {
			disableTagsList(getCtrlUInt16(CTL_POKEXV_GRP) != 1);
			clearEvent(event);
		}
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);
	int    disableTagsList(int disable);

	PPPsnOpKindPacket Data;
};

int PsnOpExVDialog::setupList()
{
	PPObjTag    objtag;
	PPObjectTag tag;
	if(Data.AllowedTags.IsExists()) {
		SString buf;
		const PPIDArray & r_ary = Data.AllowedTags.Get();
		for(uint i = 0; i < r_ary.getCount(); i++) {
			PPID   tag_id = r_ary.at(i);
			if(objtag.Fetch(tag_id, &tag) > 0)
				buf = tag.Name;
			else
				(buf = 0).Cat(tag_id);
			if(!addStringToList(tag_id, buf))
				return 0;
		}
	}
	return 1;
}

int PsnOpExVDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long   tag_id = 0;
	if(SelectObjTag(&tag_id, 0, 0) > 0) {
		Data.AllowedTags.Add(tag_id);
		ok = 1;
	}
	ASSIGN_PTR(pID, tag_id);
	ASSIGN_PTR(pPos, (long)Data.AllowedTags.GetCount() - 1);
	return ok;
}

int PsnOpExVDialog::delItem(long pos, long id)
{
	int    ok = 1;
	PoClause_ clause;
	for(uint i = 0; ok && i < Data.ClauseList.GetCount(); i++) {
		Data.ClauseList.Get(i, clause);
		if(oneof2(clause.VerbID, POVERB_SETTAG, POVERB_REMOVETAG) && clause.DirObj == id)
			ok = 0;
	}
	if(ok)
		Data.AllowedTags.Remove(id);
	else
		PPError(PPERR_TAGBLOCKED);
	return ok;
}

int PsnOpExVDialog::disableTagsList(int disable)
{
	disableCtrl(CTL_POKEXV_ALLOWEDTAGS, disable);
	enableCommand(cmaInsert, !disable);
	enableCommand(cmaEdit,   !disable);
	enableCommand(cmaDelete, !disable);
	return 1;
}
//
//
//
class PsnOpDialog : public TDialog {
public:
	PsnOpDialog() : TDialog(DLG_PSNOPK)
	{
	}
	int    setDTS(PPPsnOpKindPacket * pData)
	{
		int    ok = 1;
		ushort v = 0;
		PsnOpKindFilt psnopk_filt(PsnOpKindFilt::tShowGroups);
		Data = *pData;
		setCtrlData(CTL_PSNOPK_NAME, Data.Rec.Name);
		SetupPPObjCombo(this, CTLSEL_PSNOPK_PAIR, PPOBJ_PERSONOPKIND, Data.Rec.PairOp, 0);
		SetupPPObjCombo(this, CTLSEL_PSNOPK_REGTYP, PPOBJ_REGISTERTYPE, Data.Rec.RegTypeID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_PSNOPK_PARENT,  PPOBJ_PERSONOPKIND, Data.Rec.ParentID, OLW_CANSELUPLEVEL, &psnopk_filt);
		SetupPPObjCombo(this, CTLSEL_PSNOPK_RSC, PPOBJ_STAFFCAL, Data.Rec.RestrStaffCalID, 0); // @v8.0.10
		setCtrlData(CTL_PSNOPK_RSCMAXTIMES, &Data.Rec.RscMaxTimes); // @v8.0.10
		AddClusterAssoc(CTL_PSNOPK_FLAGS, 0, POKF_UNIQUE);
		AddClusterAssoc(CTL_PSNOPK_FLAGS, 1, POKF_BINDING);
		SetClusterData(CTL_PSNOPK_FLAGS, Data.Rec.Flags);
		setCtrlData(CTL_PSNOPK_SYMB, Data.Rec.Symb);
		v = Data.Rec.PairType;
		setCtrlData(CTL_PSNOPK_PAIRTYPE, &v);
		if(Data.Rec.PairOp == 0)
			disableCtrl(CTL_PSNOPK_PAIRTYPE, 1);
		if(Data.Rec.Flags & POKF_GROUP) {
			disableCtrls(1, CTLSEL_PSNOPK_PAIR, CTLSEL_PSNOPK_REGTYP, CTL_PSNOPK_FLAGS, CTL_PSNOPK_PAIRTYPE, 0L);
			enableCommand(cmPsnOpkPrmr,     0);
			enableCommand(cmPsnOpkScnd,     0);
			enableCommand(cmPsnOpkExtraObj, 0);
			enableCommand(cmPsnOpkActl,     0);
		}
		setCtrlLong(CTL_PSNOPK_REDOTIMEOUT, Data.Rec.RedoTimeout); // @v7.9.0
		{
			SString temp_buf;
			setStaticText(CTL_PSNOPK_ID, temp_buf.Cat(Data.Rec.ID));
		}
		return ok;
	}
	int    getDTS(PPPsnOpKindPacket * pData)
	{
		int    ok = 1;
		ushort v = 0;
		getCtrlData(CTL_PSNOPK_NAME,      Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTLSEL_PSNOPK_PAIR,   &Data.Rec.PairOp);
		getCtrlData(CTLSEL_PSNOPK_PARENT,  &Data.Rec.ParentID);
		THROW_PP((!Data.Rec.ID || Data.Rec.ID != Data.Rec.PairOp), PPERR_PAIREQTHIS);
		THROW(PPObjPsnOpKind::CheckRecursion(Data.Rec.ID, Data.Rec.ParentID));
		getCtrlData(CTLSEL_PSNOPK_REGTYP, &Data.Rec.RegTypeID);
		GetClusterData(CTL_PSNOPK_FLAGS, &Data.Rec.Flags);
		getCtrlData(CTL_PSNOPK_PAIRTYPE,  &(v = 0));
		getCtrlData(CTL_PSNOPK_SYMB, Data.Rec.Symb);
		getCtrlData(CTL_PSNOPK_REDOTIMEOUT, &Data.Rec.RedoTimeout); // @v7.9.0
		getCtrlData(CTLSEL_PSNOPK_RSC, &Data.Rec.RestrStaffCalID); // @v8.0.10
		getCtrlData(CTL_PSNOPK_RSCMAXTIMES, &Data.Rec.RscMaxTimes); // @v8.0.10
		if(v <= POKPT_NULLCLOSE)
			Data.Rec.PairType = v;
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    editPsnConstr(int scnd /* !scnd == prmr */);
	int    editExtraVal();

	PPPsnOpKindPacket Data;
};

int PsnOpDialog::editPsnConstr(int scnd)
{
	class PsnConstrDialog : public TDialog {
	public:
		PsnConstrDialog(int scnd) : TDialog(scnd ? DLG_PSNOPKSC : DLG_PSNOPKPC)
		{
			Scnd = scnd;
		}
		int    setDTS(PPPsnOpKindPacket::PsnConstr * pc)
		{
			data = *pc;
			SetupPPObjCombo(this, CTLSEL_PSNOPKPC_KIND, PPOBJ_PRSNKIND, data.PersonKindID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_PSNOPKPC_RTAG, PPOBJ_TAG, data.RestrictTagID, 0);
			ushort v = data.StatusType;
			setCtrlData(CTL_PSNOPKPC_STATUSTYP, &v);
			if(Scnd)
	   			SetupPPObjCombo(this, CTLSEL_PSNOPKPC_DEFAULT, PPOBJ_PERSON, data.DefaultID, 0, (void *)data.PersonKindID);
			{
				SmartListBox * p_box = (SmartListBox *)getCtrlView(CTL_PSNOPKPC_SCSLIST);
				if(p_box) {
					p_box->setDef(CreateScsListDef());
					p_box->drawView();
				}
			}
			return 1;
		}
		int    getDTS(PPPsnOpKindPacket::PsnConstr * pc)
		{
			ushort v = 0;
			getCtrlData(CTLSEL_PSNOPKPC_KIND,   &data.PersonKindID);
			getCtrlData(CTLSEL_PSNOPKPC_RTAG,   &data.RestrictTagID);
			getCtrlData(CTL_PSNOPKPC_STATUSTYP, &v);
			data.StatusType = v;
			if(Scnd)
				getCtrlData(CTLSEL_PSNOPKPC_DEFAULT, &data.DefaultID);
			else
				data.DefaultID = 0;
			*pc = data;
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(Scnd && event.isCbSelected(CTLSEL_PSNOPKPC_KIND)) {
				SetupPPObjCombo(this, CTLSEL_PSNOPKPC_DEFAULT, PPOBJ_PERSON, 0, 0, (void *)getCtrlLong(CTLSEL_PSNOPKPC_KIND));
			}
			else if(event.isCmd(cmaInsert)) {
				SmartListBox * p_box = (SmartListBox*)getCtrlView(CTL_PSNOPKPC_SCSLIST);
				if(p_box) {
					PPIDArray scs_list;
					ListToListData l2l(PPOBJ_SCARDSERIES, 0, &scs_list);
					if(ListToListDialog(&l2l) > 0) {
						data.RestrictScSerList.addUnique(&scs_list);
						p_box->setDef(CreateScsListDef());
						p_box->drawView();
					}
				}
			}
			else if(event.isCmd(cmaDelete)) {
				PPID   scs_id;
				SmartListBox * p_box = (SmartListBox*)getCtrlView(CTL_PSNOPKPC_SCSLIST);
				if(p_box && p_box->getCurID(&scs_id) && scs_id) {
					data.RestrictScSerList.freeByKey(scs_id, 0);
					p_box->setDef(CreateScsListDef());
					p_box->drawView();
				}
			}
			else
				return;
			clearEvent(event);
		}
		ListBoxDef * CreateScsListDef()
		{
			StrAssocListBoxDef * p_def = 0;
			StrAssocArray * p_list = new StrAssocArray;
			if(p_list) {
				PPObjSCardSeries scs_obj;
				PPSCardSeries scs_rec;
				for(uint i = 0; i < data.RestrictScSerList.getCount(); i++) {
					if(scs_obj.Fetch(data.RestrictScSerList.get(i), &scs_rec) > 0) {
						p_list->Add(scs_rec.ID, scs_rec.Name);
					}
				}
				p_def = new StrAssocListBoxDef(p_list, lbtDisposeData);
			}
			return p_def;
		}
		int    Scnd;
		PPPsnOpKindPacket::PsnConstr data;
	};
	PPPsnOpKindPacket::PsnConstr * pc = scnd ? & Data.PCScnd : &Data.PCPrmr;
	DIALOG_PROC_BODY_P1(PsnConstrDialog, scnd, pc);
}

int PsnOpDialog::editExtraVal()
{
	DIALOG_PROC_BODYERR(PsnOpExVDialog, &Data);
}

//
//
//
class PoVerbListDialog : public PPListDialog {
public:
	PoVerbListDialog(PPPsnOpKindPacket * p) : PPListDialog(DLG_POKACTL, CTL_POKACTL_LIST)
	{
		pack = p;
		Data = pack->ClauseList;
		updateList(-1);
	}
	int    getDTS(PPPsnOpKindPacket * p)
	{
		p->ClauseList = Data;
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pos, long * id);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	long    getIncNum();

	PPPsnOpKindPacket * pack;
	PoClauseArray_ Data;
};

int PoVerbListDialog::setupList()
{
	SString verb_name;
	PoClause_ clause;
	for(uint i = 0; i < Data.GetCount(); i++) {
		Data.Get(i, clause);
		if(!PPGetSubStrById(PPTXT_POVERB, clause.VerbID, verb_name))
			verb_name.Cat(clause.VerbID);
		if(!addStringToList(i, verb_name))
			return 0;
	}
	return 1;
}

long PoVerbListDialog::getIncNum()
{
	long   max_num = (POKPRP_FIRSTCLAUSE-1);
	PoClause_ clause;
	for(uint i = 0; i < Data.GetCount(); i++) {
		Data.Get(i, clause);
		if(clause.Num > max_num)
			max_num = clause.Num;
	}
	return (max_num+1);
}

int SLAPI EditPoClause(PPPsnOpKindPacket * pPokPack, PoClause_ * pClause)
{
	class PoClauseDialog : public TDialog {
	public:
		PoClauseDialog(PPPsnOpKindPacket * pokPack) : TDialog(DLG_POVERB)
		{
			PokPack = pokPack;
			disableCtrl(CTLSEL_POVERB_LINK, 1);
		}
		int    setDTS(PoClause_ * pClause)
		{
			int    ok = 1;
			ushort v;
			SString tag_name;
			PPIDArray allowed_tags;
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			data = *pClause;
			SetupStringCombo(this, CTLSEL_POVERB_VERB, PPTXT_POVERB, data.VerbID);
			if(data.Subj == POCOBJ_PRIMARY)
				v = 0;
			else if(data.Subj == POCOBJ_SECONDARY)
				v = 1;
			else if(data.Num == 0)
				v = 0;
			else
				ok = PPSetError(PPERR_INVPOCLAUSESUBJ);
			setCtrlData(CTL_POVERB_SUBJ, &v);
			if(PokPack && PokPack->AllowedTags.IsExists())
				allowed_tags = PokPack->AllowedTags.Get();
			for(uint i = 0; i < allowed_tags.getCount(); i++) {
				PPID   tag_id = allowed_tags.at(i);
				if(tag_obj.Fetch(tag_id, &tag_rec) > 0)
					tag_name = tag_rec.Name;
				else
					ideqvalstr(tag_id, tag_name = 0);
				TagList.Add(tag_id, tag_name);
			}
			setCtrlString(CTL_POVERB_CMDTXT, data.CmdText); // @v7.8.0
			// @v7.9.0 {
			AddClusterAssoc(CTL_POVERB_FLAGS, 0, PoClause_::fPassive);
			AddClusterAssoc(CTL_POVERB_FLAGS, 1, PoClause_::fOnRedo);
			SetClusterData(CTL_POVERB_FLAGS, data.Flags);
			// } @v7.9.0
			replyVerbSelected(1);
			disableCtrl(CTLSEL_POVERB_VERB, BIN(data.Num));
			return ok;
		}
		int    getDTS(PoClause_ * pClause)
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v = 0;
			getCtrlData(CTL_POVERB_SUBJ, &v);
			if(v == 0)
				data.Subj = POCOBJ_PRIMARY;
			else
				data.Subj = POCOBJ_SECONDARY;
			getCtrlData(CTLSEL_POVERB_VERB, &data.VerbID);
			getCtrlData(CTLSEL_POVERB_LINK, &data.DirObj);
			sel = CTL_POVERB_LINK;
			switch(data.VerbID) {
				case POVERB_ASSIGNKIND:
				case POVERB_REVOKEKIND:
					THROW_PP(data.DirObj, PPERR_PSNKINDNEEDED);
					break;
				case POVERB_REMOVETAG:
				case POVERB_SETTAG:
					THROW_PP(data.DirObj, PPERR_PSNTAGNEEDED);
					break;
				case POVERB_SETCALENDAR:
				case POVERB_SETCALENDAR_SKIP:
				case POVERB_COMPLETECAL:
				case POVERB_COMPLETECAL_SKIP:
				case POVERB_SETCALCONT:
				case POVERB_SETCALCONT_SKIP:
				case POVERB_RESETCALCONT:
				case POVERB_ADDRELATION:
				case POVERB_REVOKERELATION:
					THROW_PP(data.DirObj, PPERR_STAFFCALNEEDED);
					break;
				case POVERB_INCSCARDOP:
				case POVERB_DECSCARDOP:
					break;
				case POVERB_DEVICECMD:
					THROW_PP(data.DirObj, PPERR_GENDVCNEEDED);
					break;
				case POVERB_STYLODISPLAY:
					THROW_PP(data.DirObj, PPERR_STYLOPALMNEEDED);
					break;
			}
			getCtrlString(CTL_POVERB_CMDTXT, data.CmdText); // @v7.8.0
			GetClusterData(CTL_POVERB_FLAGS, &data.Flags);  // @v7.9.0
			ASSIGN_PTR(pClause, data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_POVERB_VERB)) {
				replyVerbSelected(0);
				clearEvent(event);
			}
		}
		int    replyVerbSelected(int init)
		{
			int    ok = 1;
			const  PPID verb_id = init ? data.VerbID : getCtrlLong(CTLSEL_POVERB_VERB);
			switch(verb_id) {
				case POVERB_ASSIGNKIND:
				case POVERB_REVOKEKIND:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_PRSNKIND, data.DirObj, 0, 0);
					break;
				case POVERB_SETTAG:
					if(PokPack->Rec.ExValGrp != POKEVG_TAG)
						ok = (PPErrCode = PPERR_INADMISSPOVERB, 0);
					else {
						disableCtrl(CTLSEL_POVERB_LINK, 0);
						SetupStrAssocCombo(this, CTLSEL_POVERB_LINK, &TagList, data.DirObj, 0);
					}
					break;
				case POVERB_REMOVETAG:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_TAG, data.DirObj, 0, 0);
					//SetupObjTagCombo(this, CTLSEL_POVERB_LINK, data.DirObj, 0, 0);
					break;
				case POVERB_INCTAG:
				case POVERB_DECTAG:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_TAG, data.DirObj, 0, 0);
					break;
				case POVERB_ASSIGNPOST:
					disableCtrl(CTLSEL_POVERB_LINK, 1);
					if(PokPack->Rec.ExValGrp != POKEVG_POST)
						ok = (PPErrCode = PPERR_INADMISSPOVERB, 0);
					break;
				case POVERB_ADDRELATION:
				case POVERB_REVOKERELATION:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_PERSONRELTYPE, data.DirObj, 0, 0);
					break;
				case POVERB_REVOKEPOST:
				case POVERB_ASSIGNREG:
					disableCtrl(CTLSEL_POVERB_LINK, 1);
					break;
				case POVERB_REVOKEREG:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_REGISTERTYPE, data.DirObj, 0, 0);
					break;
				case POVERB_SETCALENDAR:
				case POVERB_SETCALENDAR_SKIP:
				case POVERB_SETCALCONT:
				case POVERB_SETCALCONT_SKIP:
				case POVERB_RESETCALCONT:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_STAFFCAL, data.DirObj, 0, 0);
					break;
				case POVERB_COMPLETECAL:
				case POVERB_COMPLETECAL_SKIP:
					if(oneof2(PokPack->Rec.PairType, POKPT_CLOSE, POKPT_NULLCLOSE)) {
						disableCtrl(CTLSEL_POVERB_LINK, 0);
						SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_STAFFCAL, data.DirObj, 0, 0);
					}
					else
						ok = (PPErrCode = PPERR_INADMISSPOVERB, 0);
					break;
				case POVERB_INCSCARDOP:
				case POVERB_DECSCARDOP:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_GOODS, data.DirObj, OLW_LOADDEFONOPEN, 0);
					break;
				case POVERB_DEVICECMD:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_GENERICDEVICE, data.DirObj, 0, 0);
					break;
				case POVERB_STYLODISPLAY:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					SetupPPObjCombo(this, CTLSEL_POVERB_LINK, PPOBJ_STYLOPALM, data.DirObj, 0, 0);
					break;
				case POVERB_BEEP:
					disableCtrl(CTLSEL_POVERB_LINK, 0);
					break;
			}
			if(ok) {
				data.VerbID = verb_id;
				disableCtrl(CTL_POVERB_CMDTXT, BIN(!(data.GetDirFlags() & VerbObjAssoc::fAllowCmdText)));
			}
			else {
				PPError();
				setCtrlData(CTLSEL_POVERB_VERB, &(data.VerbID = 0));
				messageToCtrl(CTLSEL_POVERB_VERB, evCommand, cmCBActivate, 0);
			}
			return ok;
		}
		StrAssocArray TagList;
		PPPsnOpKindPacket * PokPack;
		PoClause_ data;
	};
	DIALOG_PROC_BODY_P1(PoClauseDialog, pPokPack, pClause);
}

int PoVerbListDialog::addItem(long * pPos, long * pID)
{
	PoClause_ clause;
	if(EditPoClause(pack, &clause) > 0) {
		clause.Num = getIncNum();
		if(!Data.Add(clause))
			return PPSetErrorSLib();
		else {
			ASSIGN_PTR(pPos, Data.GetCount() - 1);
			ASSIGN_PTR(pID, clause.Num);
		}
		return 1;
	}
	else
		return -1;
}

int PoVerbListDialog::editItem(long pos, long)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.GetCount()) {
		PoClause_ clause;
		Data.Get(pos, clause);
		ok = EditPoClause(pack, &clause);
		if(ok > 0) {
			Data.Set(pos, &clause);
		}
	}
	return ok;
}

int PoVerbListDialog::delItem(long pos, long)
{
	if(pos >= 0 && pos < (long)Data.GetCount()) {
 		if(PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE, 0) == cmYes) {
			Data.Set((uint)pos, 0);
			return 1;
		}
	}
	return -1;
}

IMPL_HANDLE_EVENT(PsnOpDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_PSNOPK_PAIR) && !(Data.Rec.Flags & POKF_GROUP)) {
		PPID   pair = getCtrlLong(CTLSEL_PSNOPK_PAIR);
		disableCtrl(CTL_PSNOPK_PAIRTYPE, pair == 0);
		if(pair == 0)
			setCtrlUInt16(CTL_PSNOPK_PAIRTYPE, 0);
	}
	else if(event.isCmd(cmPsnOpkPrmr))
		editPsnConstr(0);
	else if(event.isCmd(cmPsnOpkScnd))
		editPsnConstr(1);
	else if(event.isCmd(cmPsnOpkExtraObj))
		editExtraVal();
	else if(event.isCmd(cmPsnOpkActl)) {
		PoVerbListDialog * dlg = new PoVerbListDialog(&Data);
		if(ExecView(dlg) == cmOK)
			dlg->getDTS(&Data);
		delete dlg;
	}
	else
		return;
	clearEvent(event);
}
//
//
//
int SLAPI PPObjPsnOpKind::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0, apply = 1;
	uint   what = 0;
	PsnOpDialog * dlg = 0;
	PPPsnOpKindPacket pack;
	THROW(CheckDialogPtr(&(dlg = new PsnOpDialog)));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else if(SelectorDialog(DLG_SELNEWPSNOPK, CTL_SELNEWPSNOPK_WHAT, &what) > 0) {
		SETFLAG(pack.Rec.Flags, POKF_GROUP, what > 0);
	}
	else
		apply = 0;
	if(apply > 0) {
		THROW(dlg->setDTS(&pack));
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&pack)) {
				valid_data = 1;
				THROW(PutPacket(pID, &pack, 1));
				ok = cmOK;
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
class PsnOpKindCache : public ObjCache {
public:
	SLAPI  PsnOpKindCache() : ObjCache(PPOBJ_PERSONOPKIND, sizeof(Data)) {}
private:
	virtual int SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual int SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   RegTypeID;
		short  ExValGrp;
		short  PairType;
		PPID   ExValSrc;
		long   Flags;
		PPID   LinkBillOpID;
		PPID   PairOp;
	};
};

int SLAPI PsnOpKindCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPObjPsnOpKind pok_obj;
	PPPsnOpKind2 rec;
	if(pok_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(RegTypeID);
		CPY_FLD(ExValGrp);
		CPY_FLD(PairType);
		CPY_FLD(ExValSrc);
		CPY_FLD(Flags);
		CPY_FLD(LinkBillOpID);
		CPY_FLD(PairOp);
#undef CPY_FLD
		StringSet ss("/&");
		ss.add(rec.Name);
		ss.add(rec.Symb);
		ok = PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PsnOpKindCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPPsnOpKind * p_data_rec = (PPPsnOpKind *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_PERSONOPKIND;
	CPY_FLD(ID);
	CPY_FLD(RegTypeID);
	CPY_FLD(ExValGrp);
	CPY_FLD(PairType);
	CPY_FLD(ExValSrc);
	CPY_FLD(Flags);
	CPY_FLD(LinkBillOpID);
	CPY_FLD(PairOp);
#undef CPY_FLD
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	StringSet ss("/&");
	ss.setBuf(temp_buf, strlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_rec->Name, sizeof(p_data_rec->Name));
	ss.get(&p, p_data_rec->Symb, sizeof(p_data_rec->Symb));
	return 1;
}

IMPL_OBJ_FETCH(PPObjPsnOpKind, PPPsnOpKind, PsnOpKindCache);