expiry// PPPOSPROTOCOL.CPP
// Copyright (c) A.Sobolev 2016, 2017
//
#include <pp.h>
#pragma hdrstop

/*

pap

source:
	system:
	version:
	name:
	uuid:
	code:

destination:
	system:
	version:
	name:
	uuid:
	code:
*/

/*
	query-csession [id] [date] [daterange] [last] [current]
		tokId, tokCode, tokPeriod, tokTime
	query-refs [objtype]
		tokObjType
*/
class ACS_PAPYRUS_APN : public PPAsyncCashSession {
public:
	SLAPI  ACS_PAPYRUS_APN(PPID n, PPID parent) : PPAsyncCashSession(n), ParentNodeID(parent)
	{
		P_Pib = 0;
		PPAsyncCashNode acn;
		if(GetNodeData(&acn) > 0) {
			acn.GetLogNumList(LogNumList);
			ExpPath = acn.ExpPaths;
			ImpPath = acn.ImpFiles;
		}
		StatID = 0;
	}
	SLAPI ~ACS_PAPYRUS_APN()
	{
		delete P_Pib;
	}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0)
	{
		int    ok = -1;
		int    is_forward = 0;
		ZDELETE(P_Pib);
		THROW_MEM(P_Pib = new PPPosProtocol::ProcessInputBlock(this));
		P_Pib->PosNodeID = NodeID;
		P_Pib->Flags |= (P_Pib->fStoreReadBlocks | P_Pib->fBackupProcessed | P_Pib->fRemoveProcessed);
		int    pir = Pp.ProcessInput(*P_Pib);
		THROW(pir);
		if(P_Pib->SessionCount) {
			THROW(CreateTables());
			ok = 1;
		}
		CATCH
			ZDELETE(P_Pib);
			ok = 0;
		ENDCATCH
		ASSIGN_PTR(pIsForwardSess, is_forward);
		if(P_Pib) {
			ASSIGN_PTR(pSessCount, P_Pib->SessionCount);
			ASSIGN_PTR(pPrd, P_Pib->SessionPeriod);
		}
		else {
			ASSIGN_PTR(pSessCount, 0);
			CALLPTRMEMB(pPrd, SetZero());
		}
		return ok;
	}
	virtual int SLAPI ImportSession(int sessN)
	{
		int    ok = -1;
        int    local_n = 0;
		SString temp_buf;
		const TSCollection <PPPosProtocol::ReadBlock> * p_rb_list = P_Pib ? (const TSCollection <PPPosProtocol::ReadBlock> *)P_Pib->GetStoredReadBlocks() : 0;
		if(p_rb_list) {
			for(uint i = 0; ok < 0 && i < p_rb_list->getCount(); i++) {
				const PPPosProtocol::ReadBlock * p_ib = p_rb_list->at(i);
				if(p_ib) {
					for(uint _csidx = 0; ok < 0 && _csidx < p_ib->RefList.getCount(); _csidx++) {
						const PPPosProtocol::ObjBlockRef & r_ref = p_ib->RefList.at(_csidx);
						if(r_ref.Type == PPPosProtocol::obCSession) {
							if(local_n != sessN) {
								local_n++;
							}
							else {
								int    type = 0;
								PPID   src_ar_id = 0;
								const PPPosProtocol::CSessionBlock * p_cb = (const PPPosProtocol::CSessionBlock *)p_ib->GetItem(_csidx, &type);
								const uint rc = p_ib->RefList.getCount();
								THROW(type == PPPosProtocol::obCSession);
								{
        							PPTransaction tra(1);
        							THROW(tra);
									for(uint cc_refi = 0; cc_refi < rc; cc_refi++) {
										const PPPosProtocol::ObjBlockRef & r_cc_ref = p_ib->RefList.at(cc_refi);
										const long pos_n = 1; // @stub // ������������� ����� �����
										if(r_cc_ref.Type == PPPosProtocol::obCCheck) {
											int    cc_type = 0;
											const PPPosProtocol::CCheckBlock * p_ccb = (const PPPosProtocol::CCheckBlock *)p_ib->GetItem(cc_refi, &cc_type);
											THROW(cc_type == PPPosProtocol::obCCheck);
											if(p_ccb->CSessionBlkP == _csidx) {
												int    ccr = 0;
												PPID   cc_id = 0;
												PPID   cashier_id = 0;
												PPID   sc_id = 0; // �� ������������ �����, ����������� � ����
												LDATETIME cc_dtm = p_ccb->Dtm;
												double cc_amount = p_ccb->Amount;
												double cc_discount = p_ccb->Discount;
												long   cc_flags = 0;
												if(p_ccb->SaCcFlags & CCHKF_RETURN)
													cc_flags |= CCHKF_RETURN;
												if(p_ccb->SCardBlkP) {
													int    sc_type = 0;
													SCardTbl::Rec sc_rec;
													const PPPosProtocol::SCardBlock * p_scb = (const PPPosProtocol::SCardBlock *)p_ib->GetItem(p_ccb->SCardBlkP, &sc_type);
													assert(sc_type == PPPosProtocol::obSCard);
													if(p_scb->NativeID) {
														sc_id = p_scb->NativeID;
													}
													else {
														p_ib->GetS(p_scb->CodeP, temp_buf);
														temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
														if(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0) {
															sc_id = sc_rec.ID;
														}
														else {
															; // @log
														}
													}
												}
												THROW(ccr = AddTempCheck(&cc_id, p_cb->Code, cc_flags, pos_n, p_ccb->Code, cashier_id, sc_id, &cc_dtm, cc_amount, cc_discount));
												//
												for(uint cl_refi = 0; cl_refi < rc; cl_refi++) {
													const PPPosProtocol::ObjBlockRef & r_cl_ref = p_ib->RefList.at(cl_refi);
													if(r_cl_ref.Type == PPPosProtocol::obCcLine) {
														int    cl_type = 0;
														const PPPosProtocol::CcLineBlock * p_clb = (const PPPosProtocol::CcLineBlock *)p_ib->GetItem(cl_refi, &cl_type);
														THROW(cl_type == PPPosProtocol::obCcLine);
														if(p_clb->CCheckBlkP == cc_refi) {
															short  div_n = (short)p_clb->DivN;
															double qtty = fabs(p_clb->Qtty);
															double price = p_clb->Price;
															double dscnt = (p_clb->SumDiscount != 0.0 && qtty != 0.0) ? (p_clb->SumDiscount / qtty) : p_clb->Discount;
															PPID   goods_id = 0;
															if(p_clb->GoodsBlkP) {
																int    g_type = 0;
																const PPPosProtocol::GoodsBlock * p_gb = (const PPPosProtocol::GoodsBlock *)p_ib->GetItem(p_clb->GoodsBlkP, &g_type);
																assert(g_type == PPPosProtocol::obGoods);
																if(p_gb->NativeID) {
																	goods_id = p_gb->NativeID;
																}
																else {
																	THROW(Pp.ResolveGoodsBlock(*p_gb, p_clb->GoodsBlkP, 1, 0, 0, src_ar_id, 0, &goods_id));
																}
															}
															SetupTempCcLineRec(0, cc_id, p_cb->Code, cc_dtm.d, div_n, goods_id);
															SetTempCcLineValues(0, qtty, price, dscnt, 0 /*serial*/);
															THROW_DB(P_TmpCclTbl->insertRec());
														}
													}
												}
											}
										}
									}
									THROW(tra.Commit());
								}
								ok = 1;
							}
						}
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int SLAPI FinishImportSession(PPIDArray * pList)
	{
		return -1;
	}
	virtual int SLAPI InteractiveQuery()
	{
		int    ok = -1;
		TSArray <PPPosProtocol::QueryBlock> qb_list;
		if(PPPosProtocol::EditPosQuery(qb_list) > 0) {
			if(qb_list.getCount()) {
				for(uint i = 0; i < qb_list.getCount(); i++) {
					THROW(Pp.SendQuery(NodeID, qb_list.at(i)));
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
private:
	PPID   StatID;
	PPID   ParentNodeID;
	PPIDArray  LogNumList;
	PPIDArray  SessAry;
	SString    ExpPath;
	SString    ImpPath;
	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjSCard  ScObj;

	PPPosProtocol Pp; // ��������� PPPosProtocol ����������� ��� ������� ������
	PPPosProtocol::ProcessInputBlock * P_Pib;
};

class CM_PAPYRUS : public PPCashMachine {
public:
	SLAPI  CM_PAPYRUS(PPID posNodeID) : PPCashMachine(posNodeID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface()
	{
		return new ACS_PAPYRUS_APN(NodeID, ParentNodeID);
	}
};

REGISTER_CMT(PAPYRUS,0,1);

//virtual
int SLAPI ACS_PAPYRUS_APN::ExportData(int updOnly)
{
	return Pp.ExportDataForPosNode(NodeID, updOnly, SinceDlsID);
}

int SLAPI PPPosProtocol::InitSrcRootInfo(PPID posNodeID, PPPosProtocol::RouteBlock & rInfo)
{
	int    ok = 1;
	PPCashNode cn_rec;
	SString temp_buf;
	rInfo.Destroy();
	if(posNodeID && CnObj.Search(posNodeID, &cn_rec) > 0) {
		ObjTagItem tag_item;
		S_GUID uuid;
		if(PPRef->Ot.GetTag(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_UUID, &tag_item) > 0 && tag_item.GetGuid(&uuid) > 0) {
			rInfo.Uuid = uuid;
		}
		{
			(temp_buf = cn_rec.Symb).Strip();
			if(temp_buf.IsDigit()) {
				long cn_n = temp_buf.ToLong();
				if(cn_n > 0)
					rInfo.Code = temp_buf;
			}
		}
	}
	{
		DbProvider * p_dict = CurDict;
		if(p_dict) {
			if(rInfo.Uuid.IsZero())
				p_dict->GetDbUUID(&rInfo.Uuid);
			if(rInfo.Code.Empty())
				p_dict->GetDbSymb(rInfo.Code);
		}
	}
	{
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(rInfo.System);
		vi.GetVersion().ToStr(rInfo.Version);
	}
	return ok;
}

int SLAPI PPPosProtocol::SendQuery(PPID posNodeID, const PPPosProtocol::QueryBlock & rQ)
{
	int    ok = 1;
	StringSet ss_out_files;
	SString out_file_name;
	SString temp_buf;
	PPAsyncCashNode acn_pack;
	THROW(oneof3(rQ.Q, QueryBlock::qTest, QueryBlock::qRefs, QueryBlock::qCSession));
	//PPMakeTempFileName("pppp", "xml", 0, out_file_name);
	if(!posNodeID || CnObj.GetAsync(posNodeID, &acn_pack) <= 0) {
		acn_pack.ID = 0;
	}
	THROW(SelectOutFileName(acn_pack.ID ? &acn_pack : 0, "query", ss_out_files));
	for(uint ssp = 0; ss_out_files.get(&ssp, out_file_name);) {
		PPPosProtocol::WriteBlock wb;
		THROW(StartWriting(out_file_name, wb));
		{
			PPPosProtocol::RouteBlock rb_src;
			InitSrcRootInfo(posNodeID, rb_src);
			THROW(WriteRouteInfo(wb, "source", rb_src));
			//THROW(WriteSourceRoute(wb));
			/*
			{
				PPPosProtocol::RouteBlock rb_dest;
				THROW(WriteRouteInfo(wb, "destination", rb_dest));
			}
			*/
			{
				PPPosProtocol::RouteBlock rb_dest;
				int    dest_list_written = 0;
				if(acn_pack.ID) {
					for(uint i = 0; i < acn_pack.ApnCorrList.getCount(); i++) {
						const PPGenCashNode::PosIdentEntry * p_dest_entry = acn_pack.ApnCorrList.at(i);
						if(p_dest_entry && p_dest_entry->N > 0) {
							rb_dest.Destroy();
							rb_dest.Code.Cat(p_dest_entry->N);
							rb_dest.Uuid = p_dest_entry->Uuid;
							THROW(WriteRouteInfo(wb, "destination", rb_dest));
							dest_list_written = 1;
						}
					}
					if(!dest_list_written) {
						LongArray n_list;
						acn_pack.GetLogNumList(n_list);
						for(uint i = 0; i < n_list.getCount(); i++) {
							const long n_item = n_list.get(i);
							if(n_item > 0) {
								rb_dest.Destroy();
								rb_dest.Code.Cat(n_item);
								THROW(WriteRouteInfo(wb, "destination", rb_dest));
								dest_list_written = 2;
							}
						}
					}
					// THROW(dest_list_written); // @todo error
				}
			}
		}
		{
			if(rQ.Q == QueryBlock::qCSession) {
				SXml::WNode w_s(wb.P_Xw, "query-csession");
				if(rQ.Flags & QueryBlock::fCSessCurrent) {
					w_s.PutInner("current", 0);
				}
				else if(rQ.Flags & QueryBlock::fCSessLast) {
					w_s.PutInner("last", 0);
				}
				else if(!rQ.Period.IsZero()) {
					THROW(checkdate(rQ.Period.low, 1) && checkdate(rQ.Period.upp, 1));
					temp_buf = 0;
					if(rQ.Period.low)
						temp_buf.Cat(rQ.Period.low, DATF_ISO8601|DATF_CENTURY);
					temp_buf.Dot().Dot();
					if(rQ.Period.upp)
						temp_buf.Cat(rQ.Period.upp, DATF_ISO8601|DATF_CENTURY);
					w_s.PutInner("period", temp_buf);
				}
				else if(rQ.CSess) {
					(temp_buf = 0).Cat(rQ.CSess);
					w_s.PutInner((rQ.Flags & QueryBlock::fCSessN) ? "code" : "id", temp_buf);
				}
			}
			else if(rQ.Q == QueryBlock::qRefs) {
				SXml::WNode w_s(wb.P_Xw, "query-refs");
			}
			else if(rQ.Q == QueryBlock::qTest) {
				SXml::WNode w_s(wb.P_Xw, "query-test");
			}
		}
		FinishWriting(wb);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ExportDataForPosNode(PPID nodeID, int updOnly, PPID sinceDlsID)
{
	int    ok = 1;
	PPIDArray qk_list; // ������ ��������������� ����� ���������, ������� ������ �����������
	PPIDArray used_qk_list; // ������ ��������������� ����� ���������, ������� �����������
	SString out_file_name;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	PPPosProtocol::WriteBlock wb;
	PPObjQuotKind qk_obj;
	PPObjGoods goods_obj;

	PPAsyncCashNode cn_data;
	THROW(CnObj.GetAsync(nodeID, &cn_data) > 0);
	wb.LocID = cn_data.LocID; // @v9.6.7
	PPWait(1);
	PPMakeTempFileName("pppp", "xml", 0, out_file_name);
	{
		DeviceLoadingStat dls;
		PPID   stat_id = 0;
		dls.StartLoading(&stat_id, dvctCashs, nodeID, 1);
		THROW(StartWriting(out_file_name, wb));
		{
			PPQuotKind qk_rec;
			for(SEnum en = qk_obj.Enum(0); en.Next(&qk_rec) > 0;) {
				if(qk_rec.ID == PPQUOTK_BASE || (qk_rec.Flags & QUOTKF_RETAILED)) {
					qk_list.add(qk_rec.ID);
				}
			}
			qk_list.sortAndUndup();
		}
		{
			PPPosProtocol::RouteBlock rb_src;
			InitSrcRootInfo(nodeID, rb_src);
			THROW(WriteRouteInfo(wb, "source", rb_src));
			//THROW(WriteSourceRoute(wb));
		}
		{
			/*
			{
				PPPosProtocol::RouteBlock rb;
				rb.System = "Papyrus";
				THROW(WriteRouteInfo(wb, "destination", rb));
			}
			*/
			PPPosProtocol::RouteBlock rb;
			int    dest_list_written = 0;
			{
				for(uint i = 0; i < cn_data.ApnCorrList.getCount(); i++) {
					const PPGenCashNode::PosIdentEntry * p_dest_entry = cn_data.ApnCorrList.at(i);
					if(p_dest_entry && p_dest_entry->N > 0) {
						rb.Destroy();
						rb.Code.Cat(p_dest_entry->N);
						rb.Uuid = p_dest_entry->Uuid;
						THROW(WriteRouteInfo(wb, "destination", rb));
						dest_list_written = 1;
					}
				}
			}
			if(!dest_list_written) {
                LongArray n_list;
                cn_data.GetLogNumList(n_list);
                for(uint i = 0; i < n_list.getCount(); i++) {
                    const long n_item = n_list.get(i);
                    if(n_item > 0) {
						rb.Destroy();
						rb.Code.Cat(n_item);
						THROW(WriteRouteInfo(wb, "destination", rb));
						dest_list_written = 2;
                    }
                }
			}
			THROW(dest_list_written); // @todo error
		}
		/*
		{
			SXml::WNode n_scope(wb.P_Xw, "destination");
		}
		*/
		{
			SXml::WNode n_scope(wb.P_Xw, "refs");
			{
				long   acgi_flags = ACGIF_ALLCODESPERITER;
				PPQuotArray qlist;
				if(updOnly)
					acgi_flags |= ACGIF_UPDATEDONLY;
				AsyncCashGoodsIterator acgi(nodeID, acgi_flags, sinceDlsID, &dls);
				if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
					AsyncCashGoodsGroupInfo acggi_info;
					AsyncCashGoodsGroupIterator * p_group_iter = acgi.GetGroupIterator();
					if(p_group_iter) {
						while(p_group_iter->Next(&acggi_info) > 0) {
							qlist.clear();
							goods_obj.GetQuotList(acggi_info.ID, cn_data.LocID, qlist);
							{
								uint qp = qlist.getCount();
								if(qp) do {
									PPQuot & r_q = qlist.at(--qp);
									if(!qk_list.bsearch(r_q.Kind))
										qlist.atFree(qp);
									else
										used_qk_list.addUnique(r_q.Kind);
								} while(qp);
							}
							THROW(WriteGoodsGroupInfo(wb, "goodsgroup", acggi_info, &qlist));
						}
					}
				}
				{
					// ������
					AsyncCashGoodsInfo acgi_item;
					while(acgi.Next(&acgi_item) > 0) {
						qlist.clear();
						goods_obj.GetQuotList(acgi_item.ID, cn_data.LocID, qlist);
						{
							uint qp = qlist.getCount();
							if(qp) do {
								PPQuot & r_q = qlist.at(--qp);
								if(!qk_list.bsearch(r_q.Kind))
									qlist.atFree(qp);
								else
									used_qk_list.addUnique(r_q.Kind);
							} while(qp);
						}
						THROW(WriteGoodsInfo(wb, "ware", acgi_item, &qlist));
						PPWaitPercent(acgi.GetIterCounter());
					}
				}
			}
			{
				//
				// �����
				//
				LAssocArray scard_quot_list;
				PPObjSCardSeries scs_obj;
				PPSCardSeries scs_rec;
				AsyncCashSCardsIterator acci(nodeID, updOnly, &dls, sinceDlsID);
				for(PPID ser_id = 0, idx = 1; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
					const int scs_type = scs_rec.GetType();
					//if(scs_type == scstDiscount || (scs_type == scstCredit && CrdCardAsDsc)) {
					{
						AsyncCashSCardInfo acci_item;
						PPSCardSerPacket scs_pack;
						if(scs_obj.GetPacket(ser_id, &scs_pack) > 0) {
							if(scs_rec.QuotKindID_s)
								THROW_SL(scard_quot_list.Add(scs_rec.ID, scs_rec.QuotKindID_s, 0));
							(msg_buf = fmt_buf).CatDiv(':', 2).Cat(scs_rec.Name);
							for(acci.Init(&scs_pack); acci.Next(&acci_item) > 0;) {
								THROW(WriteSCardInfo(wb, "card", acci_item));
								PPWaitPercent(acci.GetCounter(), msg_buf);
							}
						}
					}
				}
			}
			{
				//
				// ���� ���������
				//
				if(used_qk_list.getCount()) {
					PPQuotKind qk_rec;
					for(uint i = 0; i < used_qk_list.getCount(); i++) {
						if(qk_obj.Search(used_qk_list.get(i), &qk_rec) > 0) {
							THROW(WriteQuotKindInfo(wb, "quotekind", qk_rec));
						}
					}
				}
			}
		}
		FinishWriting(wb);
		if(stat_id)
			dls.FinishLoading(stat_id, 1, 1);
	}
	{
		int   copy_result = 0;
		StringSet ss_out_files;
		THROW(SelectOutFileName(&cn_data, "refs", ss_out_files));
		for(uint ssp = 0; ss_out_files.get(&ssp, temp_buf);) {
			if(SCopyFile(out_file_name, temp_buf, 0, FILE_SHARE_READ, 0))
				copy_result = 1;
		}
		if(copy_result)
			SFile::Remove(out_file_name);
	}
	CATCHZOK
	PPWait(0);
	return ok;
}
//
//
//
SLAPI PPPosProtocol::WriteBlock::WriteBlock()
{
	P_Xw = 0;
	P_Xd = 0;
	P_Root = 0;
	LocID = 0;
}

SLAPI PPPosProtocol::WriteBlock::~WriteBlock()
{
	Destroy();
}

void PPPosProtocol::WriteBlock::Destroy()
{
	ZDELETE(P_Root);
	ZDELETE(P_Xd);
	xmlFreeTextWriter(P_Xw);
	P_Xw = 0;
	UsedQkList.freeAll();
}

SLAPI PPPosProtocol::RouteBlock::RouteBlock()
{
	Uuid.SetZero();
}

void SLAPI PPPosProtocol::RouteBlock::Destroy()
{
	Uuid.SetZero();
	System = 0;
	Version = 0;
	Code = 0;
}

int SLAPI PPPosProtocol::RouteBlock::IsEmpty() const
{
	return BIN(Uuid.IsZero() && System.Empty() && Version.Empty() && Code.Empty());
}

int FASTCALL PPPosProtocol::RouteBlock::IsEqual(const RouteBlock & rS) const
{
	int    yes = 1;
	if(!Uuid.IsZero()) {
		if(!rS.Uuid.IsZero())
			yes = (Uuid == rS.Uuid);
		else
			yes = 0;
	}
	else if(!rS.Uuid.IsZero())
		yes = 0;
	else if(Code.NotEmpty()) {
		if(rS.Code.NotEmpty())
			yes = (Code == rS.Code);
		else
			yes = 0;
	}
	else if(rS.Code.NotEmpty())
		yes = 0;
	return yes;
}
//
//
//
SLAPI PPPosProtocol::ObjectBlock::ObjectBlock()
{
	Flags = 0;
	ID = 0;
	NativeID = 0;
	NameP = 0;
}

SLAPI PPPosProtocol::PosNodeBlock::PosNodeBlock() : ObjectBlock()
{
	CodeP = 0;
	CodeI = 0;
}

SLAPI PPPosProtocol::QuotKindBlock::QuotKindBlock() : ObjectBlock()
{
    CodeP = 0;
	Rank = 0;
	Reserve = 0;
    Period.SetZero();
	TimeRestriction.SetZero();
	AmountRestriction.Clear();
}

SLAPI PPPosProtocol::GoodsBlock::GoodsBlock() : ObjectBlock()
{
	ParentBlkP = 0;
	InnerId = 0;
	Price = 0.0;
	Rest = 0.0;
}

SLAPI PPPosProtocol::GoodsGroupBlock::GoodsGroupBlock() : ObjectBlock()
{
	CodeP = 0;
	ParentBlkP = 0;
}

SLAPI PPPosProtocol::LotBlock::LotBlock() : ObjectBlock()
{
	GoodsBlkP = 0;
	Dt = ZERODATE;
	Expiry = ZERODATE;
	Cost = 0.0;
	Price = 0.0;
	Rest = 0.0;
	SerailP = 0;
}

SLAPI PPPosProtocol::PersonBlock::PersonBlock() : ObjectBlock()
{
	CodeP = 0;
}

SLAPI PPPosProtocol::SCardBlock::SCardBlock() : ObjectBlock()
{
	CodeP = 0;
	OwnerBlkP = 0;
	Discount = 0.0;
}

SLAPI PPPosProtocol::CSessionBlock::CSessionBlock() : ObjectBlock()
{
	ID = 0;
	Code = 0;
	PosBlkP = 0;
	Dtm.SetZero();
}

SLAPI PPPosProtocol::CCheckBlock::CCheckBlock() : ObjectBlock()
{
	Code = 0;
	CcFlags = 0;
	SaCcFlags = 0;
	CTableN = 0;
	GuestCount = 0;
	CSessionBlkP = 0;
	AddrBlkP = 0;
	AgentBlkP = 0;
	Amount = 0.0;
	Discount = 0.0;
	Dtm.SetZero();
	CreationDtm.SetZero();
	SCardBlkP = 0;
	MemoP = 0;
}

SLAPI PPPosProtocol::CcLineBlock::CcLineBlock() : ObjectBlock()
{
	CcID = 0;
	RByCheck = 0; // (id)
	CclFlags = 0;
	DivN = 0;
	Queue = 0;
	GoodsBlkP = 0;
	Qtty = 0.0;
	Price = 0.0;
	Discount = 0.0;
	SumDiscount = 0.0;
	Amount = 0.0;
	CCheckBlkP = 0;
	SerialP = 0;
	EgaisMarkP = 0;
}

SLAPI PPPosProtocol::QueryBlock::QueryBlock()
{
	Init(qUnkn);
}

void FASTCALL PPPosProtocol::QueryBlock::Init(int q)
{
	THISZERO();
	Q = oneof3(q, qTest, qCSession, qRefs) ? q : qUnkn;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionLast()
{
	Init(qCSession);
	Flags = fCSessLast;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionCurrent()
{
	Init(qCSession);
	Flags = fCSessCurrent;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByID(PPID sessID)
{
	Init(qCSession);
	CSess = sessID;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByNo(long sessN)
{
	Init(qCSession);
	Flags = fCSessN;
	CSess = sessN;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByDate(const DateRange & rPeriod)
{
	Init(qCSession);
	Period = rPeriod;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryRefs()
{
	Init(qRefs);
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryTest()
{
	Init(qTest);
}

SLAPI PPPosProtocol::QuotBlock::QuotBlock()
{
	THISZERO();
}

SLAPI PPPosProtocol::ParentBlock::ParentBlock()
{
	THISZERO();
}

SLAPI PPPosProtocol::GoodsCode::GoodsCode()
{
	THISZERO();
}

SLAPI PPPosProtocol::RouteObjectBlock::RouteObjectBlock() : ObjectBlock()
{
	Direction = 0;
	SystemP = 0;
	VersionP = 0;
	CodeP = 0;
	Uuid.SetZero();
}

SLAPI PPPosProtocol::ObjBlockRef::ObjBlockRef(int t, uint pos)
{
	Type = t;
	P = pos;
}

int  SLAPI PPPosProtocol::ReadBlock::CreateItem(int type, uint * pRefPos)
{
	int    ok = 1;
	switch(type) {
		case obGoods:       THROW(Helper_CreateItem(GoodsBlkList, type, pRefPos)); break;
		case obGoodsGroup:  THROW(Helper_CreateItem(GoodsGroupBlkList, type, pRefPos)); break;
		case obPerson:      THROW(Helper_CreateItem(PersonBlkList, type, pRefPos)); break;
		case obGoodsCode:   THROW(Helper_CreateItem(GoodsCodeList, type, pRefPos)); break;
		case obLot:         THROW(Helper_CreateItem(LotBlkList, type, pRefPos)); break;
		case obSCard:       THROW(Helper_CreateItem(SCardBlkList, type, pRefPos)); break;
		case obParent:      THROW(Helper_CreateItem(ParentBlkList, type, pRefPos)); break;
		case obQuotKind:    THROW(Helper_CreateItem(QkBlkList, type, pRefPos)); break;
		case obQuot:        THROW(Helper_CreateItem(QuotBlkList, type, pRefPos)); break;
		case obSource:      THROW(Helper_CreateItem(SrcBlkList, type, pRefPos)); break;
		case obDestination: THROW(Helper_CreateItem(DestBlkList, type, pRefPos)); break;
		case obCSession:    THROW(Helper_CreateItem(CSessBlkList, type, pRefPos)); break;
		case obCCheck:      THROW(Helper_CreateItem(CcBlkList, type, pRefPos)); break;
		case obCcLine:      THROW(Helper_CreateItem(CclBlkList, type, pRefPos)); break;
		case obPosNode:     THROW(Helper_CreateItem(PosBlkList, type, pRefPos)); break;
		case obQuery:       THROW(Helper_CreateItem(QueryList, type, pRefPos)); break;
		default:
			assert(0);
			CALLEXCEPT();
			break;
	}
	CATCHZOK
	return ok;
}

PPPosProtocol::ReadBlock & FASTCALL PPPosProtocol::ReadBlock::Copy(const PPPosProtocol::ReadBlock & rS)
{
	Destroy();
	SStrGroup::CopyS(rS);
	#define CPY_FLD(f) f = rS.f
	CPY_FLD(SrcFileName);
	CPY_FLD(SrcBlkList);
	CPY_FLD(DestBlkList);
	CPY_FLD(GoodsBlkList);
	CPY_FLD(GoodsGroupBlkList);
	CPY_FLD(GoodsCodeList);
	CPY_FLD(LotBlkList);
	CPY_FLD(QkBlkList);
	CPY_FLD(QuotBlkList);
	CPY_FLD(PersonBlkList);
	CPY_FLD(SCardBlkList);
	CPY_FLD(ParentBlkList);
	CPY_FLD(PosBlkList);
	CPY_FLD(CSessBlkList);
	CPY_FLD(CcBlkList);
	CPY_FLD(CclBlkList);
	CPY_FLD(QueryList);
	CPY_FLD(RefList);
	#undef CPY_FLD
	return *this;
}

int SLAPI PPPosProtocol::ReadBlock::GetRouteItem(const RouteObjectBlock & rO, RouteBlock & rR) const
{
	int    ok = 1;
	rR.Destroy();
	rR.Uuid = rO.Uuid;
	GetS(rO.CodeP, rR.Code);
	GetS(rO.SystemP, rR.System);
	GetS(rO.VersionP, rR.Version);
	return ok;
}

void * SLAPI PPPosProtocol::ReadBlock::GetItem(uint refPos, int * pType) const
{
	void * p_ret = 0;
	int    type = 0;
	if(refPos < RefList.getCount()) {
		const ObjBlockRef & r_ref = RefList.at(refPos);
		type = r_ref.Type;
		switch(r_ref.Type) {
			case obGoods:       p_ret = &GoodsBlkList.at(r_ref.P); break;
			case obGoodsGroup:  p_ret = &GoodsGroupBlkList.at(r_ref.P); break;
			case obPerson:      p_ret = &PersonBlkList.at(r_ref.P); break;
			case obGoodsCode:   p_ret = &GoodsCodeList.at(r_ref.P); break;
			case obSCard:       p_ret = &SCardBlkList.at(r_ref.P); break;
			case obParent:      p_ret = &ParentBlkList.at(r_ref.P); break;
			case obQuotKind:    p_ret = &QkBlkList.at(r_ref.P); break;
			case obQuot:        p_ret = &QuotBlkList.at(r_ref.P); break;
			case obSource:      p_ret = &SrcBlkList.at(r_ref.P); break;
			case obDestination: p_ret = &DestBlkList.at(r_ref.P); break;
			case obCSession:    p_ret = &CSessBlkList.at(r_ref.P); break;
			case obCCheck:      p_ret = &CcBlkList.at(r_ref.P); break;
			case obCcLine:      p_ret = &CclBlkList.at(r_ref.P); break;
			case obPosNode:     p_ret = &PosBlkList.at(r_ref.P); break;
			case obQuery:       p_ret = &QueryList.at(r_ref.P); break;
			case obLot:         p_ret = &LotBlkList.at(r_ref.P); break;
			default:
				assert(0);
				CALLEXCEPT();
				break;
		}
	}
	CATCH
		p_ret = 0;
	ENDCATCH
	ASSIGN_PTR(pType, type);
	return p_ret;
}

int SLAPI PPPosProtocol::ReadBlock::SearchRef(int type, uint pos, uint * pRefPos) const
{
	int    ok = 0;
	uint   ref_pos = 0;
	for(uint i = 0; !ok && i < RefList.getCount(); i++) {
		const ObjBlockRef & r_ref = RefList.at(i);
		if(r_ref.Type == type && r_ref.P == pos) {
			ref_pos = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pRefPos, ref_pos);
	return ok;
}

int SLAPI PPPosProtocol::ReadBlock::SearchAnalogRef_QuotKind(const PPPosProtocol::QuotKindBlock & rBlk, uint exclPos, uint * pRefPos) const
{
	int    ok = 0;
	uint   ref_pos = 0;
	SString code_buf;
	GetS(rBlk.CodeP, code_buf);
	if(rBlk.ID || code_buf.NotEmpty()) {
		SString temp_buf;
		for(uint i = 0; !ok && i < RefList.getCount(); i++) {
			if((!exclPos || i != exclPos) && RefList.at(i).Type == obQuotKind) {
				int   test_type = 0;
				const QuotKindBlock * p_item = (const QuotKindBlock *)GetItem(i, &test_type);
				assert(test_type == obQuotKind);
				if(rBlk.ID) {
					if(p_item->ID == rBlk.ID) {
						if(code_buf.NotEmpty()) {
							if(GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf) {
								ref_pos = i;
								ok = 1;
							}
						}
						else if(p_item->CodeP == 0) {
							ref_pos = i;
							ok = 1;
						}
					}
				}
				else if(code_buf.NotEmpty() && GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf) {
					ref_pos = i;
					ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pRefPos, ref_pos);
	return ok;
}

const PPPosProtocol::QuotKindBlock * FASTCALL PPPosProtocol::ReadBlock::SearchAnalog_QuotKind(const PPPosProtocol::QuotKindBlock & rBlk) const
{
	const QuotKindBlock * p_ret = 0;
	if(rBlk.ID) {
		for(uint i = 0; !p_ret && i < QkBlkList.getCount(); i++) {
			const QuotKindBlock & r_item = QkBlkList.at(i);
			if(r_item.NativeID && r_item.ID == rBlk.ID) {
				p_ret = &r_item;
			}
		}
	}
	return p_ret;
}
//
// Descr: ������� �������� ������ ����� ���������� rBlk � ������������������
//   NativeID.
// Returns:
//   ��������� �� ��������� ����-������
//   ���� ����� �������� �����������, �� ���������� 0
//
const PPPosProtocol::PersonBlock * FASTCALL PPPosProtocol::ReadBlock::SearchAnalog_Person(const PPPosProtocol::PersonBlock & rBlk) const
{
	const PersonBlock * p_ret = 0;
	if(rBlk.ID) {
		for(uint i = 0; !p_ret && i < PersonBlkList.getCount(); i++) {
			const PersonBlock & r_item = PersonBlkList.at(i);
			if(r_item.NativeID && r_item.ID == rBlk.ID) {
				p_ret = &r_item;
			}
		}
	}
	return p_ret;
}

SLAPI PPPosProtocol::ProcessInputBlock::ProcessInputBlock()
{
	Helper_Construct();
}

SLAPI PPPosProtocol::ProcessInputBlock::ProcessInputBlock(PPAsyncCashSession * pAcs)
{
	Helper_Construct();
	P_ACS = pAcs;
}

void SLAPI PPPosProtocol::ProcessInputBlock::Helper_Construct()
{
	P_ACS = 0;
	P_RbList = 0;
	Flags = 0;
	PosNodeID = 0;
	SessionCount = 0;
	SessionPeriod.SetZero();
}

SLAPI PPPosProtocol::ProcessInputBlock::~ProcessInputBlock()
{
	if(P_RbList) {
		delete (TSCollection <PPPosProtocol::ReadBlock> *)P_RbList;
		P_RbList = 0;
	}
}

const void * SLAPI PPPosProtocol::ProcessInputBlock::GetStoredReadBlocks() const
{
	return P_RbList;
}

SLAPI PPPosProtocol::PPPosProtocol()
{
	P_BObj = BillObj;
}

SLAPI PPPosProtocol::~PPPosProtocol()
{
}

const SString & FASTCALL PPPosProtocol::EncText(const char * pS)
{
	EncBuf = pS;
	PROFILE(XMLReplaceSpecSymb(EncBuf, "&<>\'"));
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

int SLAPI PPPosProtocol::WritePosNode(WriteBlock & rB, const char * pScopeXmlTag, PPCashNode & rInfo)
{
    int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
        w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.ID));
        w_s.PutInnerSkipEmpty("code", EncText(rInfo.Symb));
        w_s.PutInnerSkipEmpty("name", EncText(rInfo.Name));
	}
    return ok;
}

int SLAPI PPPosProtocol::WriteCSession(WriteBlock & rB, const char * pScopeXmlTag, const CSessionTbl::Rec & rInfo)
{
	int    ok = 1;
	PPID   src_ar_id = 0; // ������ �������������� �����, ��������������� ��������� ������
	LDATETIME dtm;
	SString temp_buf;
	{
		/*
	struct Rec {
		int32  ID;
		int32  SuperSessID;
		int32  CashNodeID;
		int32  CashNumber;
		int32  SessNumber;
		LDATE  Dt;
		LTIME  Tm;
		int16  Incomplete;
		int16  Temporary;
		double Amount;
		double Discount;
		double AggrAmount;
		double AggrRest;
		double WrOffAmount;
		double WrOffCost;
		double Income;
		double BnkAmount;
		double CSCardAmount;
	} data;
		*/
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.ID));
		w_s.PutInner("code", (temp_buf = 0).Cat(rInfo.SessNumber));
		if(rInfo.CashNodeID) {
			//PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(CnObj.Search(rInfo.CashNodeID, &cn_rec) > 0) {
				THROW(WritePosNode(rB, "pos", cn_rec));
			}
		}
		{
			dtm.Set(rInfo.Dt, rInfo.Tm);
			(temp_buf = 0).Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
			w_s.PutInner("time", EncText(temp_buf));
		}
		{
			PPIDArray cc_list;
			ArGoodsCodeArray ar_code_list;
            CCheckCore * p_cc = ScObj.P_CcTbl;
            if(p_cc) {
				CCheckPacket cc_pack;
                THROW(p_cc->GetListBySess(rInfo.ID, 0, cc_list));
				for(uint i = 0; i < cc_list.getCount(); i++) {
					const PPID cc_id = cc_list.get(i);
					cc_pack.Init();
					if(p_cc->LoadPacket(cc_id, 0, &cc_pack) > 0) {
						const double cc_amount = MONEYTOLDBL(cc_pack.Rec.Amount);
						const double cc_discount = MONEYTOLDBL(cc_pack.Rec.Discount);
						SXml::WNode w_cc(rB.P_Xw, "cc");
                        w_cc.PutInner("id",   (temp_buf = 0).Cat(cc_pack.Rec.ID));
                        w_cc.PutInner("code", (temp_buf = 0).Cat(cc_pack.Rec.Code));
						{
							dtm.Set(cc_pack.Rec.Dt, cc_pack.Rec.Tm);
							(temp_buf = 0).Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
							w_cc.PutInner("time", EncText(temp_buf));
						}
						if(cc_pack.Rec.Flags) {
							(temp_buf = 0).CatHex(cc_pack.Rec.Flags);
							w_cc.PutInner("flags", EncText(temp_buf));
						}
						if(cc_pack.Rec.Flags & CCHKF_RETURN) {
							w_cc.PutInner("return", "");
						}
						(temp_buf = 0).Cat(cc_amount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
						w_cc.PutInner("amount", temp_buf);
						if(cc_discount != 0.0) {
							(temp_buf = 0).Cat(cc_discount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
							w_cc.PutInner("discount", temp_buf);
						}
						if(cc_pack.Rec.SCardID) {
							SCardTbl::Rec sc_rec;
							if(ScObj.Search(cc_pack.Rec.SCardID, &sc_rec) > 0) {
								SXml::WNode w_sc(rB.P_Xw, "card");
								w_sc.PutInner("code", EncText(temp_buf = sc_rec.Code));
							}
						}
						for(uint ln_idx = 0; ln_idx < cc_pack.GetCount(); ln_idx++) {
							const CCheckLineTbl::Rec & r_item = cc_pack.GetLine(ln_idx);

							const double item_qtty  = fabs(r_item.Quantity);
							const double item_price = intmnytodbl(r_item.Price);
							const double item_discount = r_item.Dscnt;
							const double item_amount   = item_price * item_qtty;
							const double item_sum_discount = item_discount * item_qtty;

							SXml::WNode w_ccl(rB.P_Xw, "ccl");
                            w_ccl.PutInner("id", (temp_buf = 0).Cat(r_item.RByCheck));
                            if(r_item.GoodsID) {
								Goods2Tbl::Rec goods_rec;
								if(GObj.Search(r_item.GoodsID, &goods_rec) > 0) {
									SXml::WNode w_w(rB.P_Xw, "ware");
									GObj.P_Tbl->ReadArCodesByAr(goods_rec.ID, src_ar_id, &ar_code_list);
									if(ar_code_list.getCount()) {
										w_w.PutInner("id", EncText((temp_buf = 0).Cat(ar_code_list.at(0).Code)));
									}
									w_w.PutInner("innerid", (temp_buf = 0).Cat(goods_rec.ID));
                                    GObj.GetSingleBarcode(r_item.GoodsID, temp_buf);
                                    if(temp_buf.NotEmptyS()) {
										w_w.PutInner("code", EncText(temp_buf));
                                    }
								}
                            }
                            w_ccl.PutInner("qtty", (temp_buf = 0).Cat(item_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
                            w_ccl.PutInner("price", (temp_buf = 0).Cat(item_price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
                            if(item_discount != 0.0)
								w_ccl.PutInner("discount", (temp_buf = 0).Cat(item_discount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
							w_ccl.PutInner("amount", (temp_buf = 0).Cat(item_amount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
							if(item_sum_discount != 0.0)
								w_ccl.PutInner("sumdiscount", (temp_buf = 0).Cat(item_sum_discount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
						}
					}
				}
            }
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::WriteGoodsGroupInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashGoodsGroupInfo & rInfo, const PPQuotArray * pQList)
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		if(rInfo.Code[0]) {
			temp_buf = rInfo.Code;
			w_s.PutInner("code", EncText(temp_buf));
		}
		if(rInfo.ParentID) {
			SXml::WNode w_p(rB.P_Xw, "parent");
			w_p.PutInner("id", (temp_buf = 0).Cat(rInfo.ParentID));
		}
		if(pQList && pQList->getCount()) {
			for(uint i = 0; i < pQList->getCount(); i++) {
				WriteQuotInfo(rB, "quote", PPOBJ_GOODSGROUP, pQList->at(i));
			}
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::WriteRouteInfo(WriteBlock & rB, const char * pScopeXmlTag, const PPPosProtocol::RouteBlock & rInfo)
{
	int    ok = 1;
	if(!rInfo.IsEmpty()) {
		SString temp_buf;
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(temp_buf);
		w_s.PutInnerSkipEmpty("system", EncText(temp_buf = rInfo.System));
		w_s.PutInnerSkipEmpty("version", EncText(temp_buf = rInfo.Version));
		if(!rInfo.Uuid.IsZero()) {
			rInfo.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
			w_s.PutInner("uuid", EncText(temp_buf));
		}
		w_s.PutInnerSkipEmpty("code", EncText(temp_buf = rInfo.Code));
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPPosProtocol::WriteGoodsInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashGoodsInfo & rInfo, const PPQuotArray * pQList)
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		if(rInfo.P_CodeList && rInfo.P_CodeList->getCount()) {
			for(uint i = 0; i < rInfo.P_CodeList->getCount(); i++) {
				const BarcodeTbl::Rec & r_bc_rec = rInfo.P_CodeList->at(i);
				w_s.PutInner("code", EncText(r_bc_rec.Code));
			}
		}
		if(rInfo.ParentID) {
			SXml::WNode w_p(rB.P_Xw, "parent");
			w_p.PutInner("id", (temp_buf = 0).Cat(rInfo.ParentID));
		}
		if(rInfo.Price > 0.0)
			w_s.PutInner("price", (temp_buf = 0).Cat(rInfo.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
		if(rInfo.Rest > 0.0)
			w_s.PutInner("rest", (temp_buf = 0).Cat(rInfo.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
        {
        	//
        	// ����
        	//
			LotArray lot_list;
			if(P_BObj->trfr->Rcpt.GetListOfOpenedLots(+1, rInfo.ID, rB.LocID, MAXDATE, &lot_list) > 0) {
                for(uint i = 0; i < lot_list.getCount(); i++) {
					const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
					SXml::WNode w_l(rB.P_Xw, "lot");
					if(checkdate(r_lot_rec.Dt, 0))
						w_l.PutInner("date", (temp_buf = 0).Cat(r_lot_rec.Dt, DATF_ISO8601|DATF_CENTURY));
					if(checkdate(r_lot_rec.Expiry, 0))
						w_l.PutInner("expiry", (temp_buf = 0).Cat(r_lot_rec.Expiry, DATF_ISO8601|DATF_CENTURY));
					if(r_lot_rec.Cost > 0.0)
						w_l.PutInner("cost", (temp_buf = 0).Cat(r_lot_rec.Cost, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
					if(r_lot_rec.Price > 0.0)
						w_l.PutInner("price", (temp_buf = 0).Cat(r_lot_rec.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
					if(r_lot_rec.Rest > 0.0)
						w_l.PutInner("rest", (temp_buf = 0).Cat(r_lot_rec.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
					if(P_BObj->GetSerialNumberByLot(r_lot_rec.ID, temp_buf, 1) > 0) {
						w_l.PutInnerSkipEmpty("serial", temp_buf);
					}
                }
			}
        }
		if(pQList && pQList->getCount()) {
			for(uint i = 0; i < pQList->getCount(); i++) {
				WriteQuotInfo(rB, "quote", PPOBJ_GOODS, pQList->at(i));
			}
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::WriteQuotKindInfo(WriteBlock & rB, const char * pScopeXmlTag, const PPQuotKind & rInfo)
{
    int    ok = 1;
    SString temp_buf;
	TimeRange tr;
    {
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		w_s.PutInnerSkipEmpty("code", EncText(rInfo.Symb));
		w_s.PutInner("rank", (temp_buf = 0).Cat(rInfo.Rank));
		if(!rInfo.Period.IsZero() || rInfo.HasWeekDayRestriction() || rInfo.GetTimeRange(tr) > 0 || !rInfo.AmtRestr.IsZero()) {
			SXml::WNode w_r(rB.P_Xw, "restriction");
			if(!rInfo.Period.IsZero()) {
				w_r.PutInner("period", EncText((temp_buf = 0).Cat(rInfo.Period, 0)));
			}
			if(rInfo.HasWeekDayRestriction()) {
				temp_buf = 0;
				for(uint d = 0; d < 7; d++) {
					temp_buf.CatChar((rInfo.DaysOfWeek & (1 << d)) ? '1' : '0');
				}
				w_r.PutInner("weekday", EncText(temp_buf));
			}
			if(rInfo.GetTimeRange(tr) > 0) {
				SXml::WNode w_t(rB.P_Xw, "timerange");
				w_t.PutInner("low", (temp_buf = 0).Cat(tr.low, TIMF_HMS));
				w_t.PutInner("upp", (temp_buf = 0).Cat(tr.upp, TIMF_HMS));
			}
			if(!rInfo.AmtRestr.IsZero()) {
				SXml::WNode w_a(rB.P_Xw, "amountrange");
				if(rInfo.AmtRestr.low)
					w_a.PutInner("low", (temp_buf = 0).Cat(rInfo.AmtRestr.low));
				if(rInfo.AmtRestr.upp)
					w_a.PutInner("upp", (temp_buf = 0).Cat(rInfo.AmtRestr.upp));
			}
		}
    }
    return ok;
}

int SLAPI PPPosProtocol::WriteQuotInfo(WriteBlock & rB, const char * pScopeXmlTag, PPID parentObj, const PPQuot & rInfo)
{
    int    ok = -1;
	SString temp_buf;
	if(rInfo.Kind && rInfo.GoodsID) {
		PPQuotKind qk_rec;
		Goods2Tbl::Rec goods_rec;
		if(QkObj.Fetch(rInfo.Kind, &qk_rec) > 0 && GObj.Fetch(rInfo.GoodsID, &goods_rec) > 0) {
			if(goods_rec.Kind == PPGDSK_GOODS || (goods_rec.Kind == PPGDSK_GROUP && !(goods_rec.Flags & GF_ALTGROUP))) {
				SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
				if(parentObj != PPOBJ_QUOTKIND) {
					SXml::WNode w_k(rB.P_Xw, "kind");
					w_k.PutInner("id", (temp_buf = 0).Cat(rInfo.Kind));
				}
				if(goods_rec.Kind == PPGDSK_GOODS && parentObj != PPOBJ_GOODS) {
					SXml::WNode w_g(rB.P_Xw, "ware");
					w_g.PutInner("id", (temp_buf = 0).Cat(rInfo.GoodsID));
				}
				else if(goods_rec.Kind == PPGDSK_GROUP && parentObj != PPOBJ_GOODSGROUP) {
					SXml::WNode w_g(rB.P_Xw, "goodsgroup");
					w_g.PutInner("id", (temp_buf = 0).Cat(rInfo.GoodsID));
				}
				if(rInfo.MinQtty > 0) {
					w_s.PutInner("minqtty", (temp_buf = 0).Cat(rInfo.MinQtty));
				}
				if(!rInfo.Period.IsZero()) {
					w_s.PutInner("period", EncText((temp_buf = 0).Cat(rInfo.Period, 0)));
				}
				w_s.PutInner("value", EncText(rInfo.PutValToStr(temp_buf)));
				ok = 1;
			}
		}
	}
    return ok;
}

int SLAPI PPPosProtocol::WritePersonInfo(WriteBlock & rB, const char * pScopeXmlTag, PPID codeRegTypeID, const PPPersonPacket & rPack)
{
    int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", (temp_buf = 0).Cat(rPack.Rec.ID));
		w_s.PutInner("name", EncText(rPack.Rec.Name));
		if(codeRegTypeID) {
            rPack.GetRegNumber(codeRegTypeID, temp_buf);
            if(temp_buf.NotEmptyS()) {
				w_s.PutInner("code", EncText(temp_buf));
            }
		}
	}
    return ok;
}

int SLAPI PPPosProtocol::WriteSCardInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashSCardInfo & rInfo)
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		const int sc_type = 0; // @todo
		w_s.PutInner("id", (temp_buf = 0).Cat(rInfo.Rec.ID));
		w_s.PutInner("code", EncText(rInfo.Rec.Code));
		if(rInfo.Rec.PersonID) {
			PPPersonPacket psn_pack;
			if(PsnObj.GetPacket(rInfo.Rec.PersonID, &psn_pack, 0) > 0) {
				PPID   reg_typ_id = 0;
				PPSCardConfig sc_cfg;
				PPObjSCard::FetchConfig(&sc_cfg);
				if(sc_cfg.PersonKindID) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(sc_cfg.PersonKindID, &pk_rec) > 0)
						reg_typ_id = pk_rec.CodeRegTypeID;
				}
				THROW(WritePersonInfo(rB, "owner", reg_typ_id, psn_pack));
			}
		}
		if(checkdate(rInfo.Rec.Expiry, 0))
			w_s.PutInner("expiry", (temp_buf = 0).Cat(rInfo.Rec.Expiry, DATF_ISO8601|DATF_CENTURY));
		if(rInfo.Rec.PDis > 0)
			w_s.PutInner("discount", (temp_buf = 0).Cat(fdiv100i(rInfo.Rec.PDis), MKSFMTD(0, 2, NMBF_NOTRAILZ)));
		//if(rInfo.P_QuotByQttyList)
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::StartWriting(const char * pFileName, PPPosProtocol::WriteBlock & rB)
{
	int    ok = 1;
	THROW_LXML(rB.P_Xw = xmlNewTextWriterFilename(pFileName, 0), 0);
	xmlTextWriterSetIndent(rB.P_Xw, 1);
	xmlTextWriterSetIndentString(rB.P_Xw, (const xmlChar*)"\t");
	THROW_MEM(rB.P_Xd = new SXml::WDoc(rB.P_Xw, cpUTF8));
	rB.P_Root = new SXml::WNode(rB.P_Xw, "PapyrusAsyncPosInterchange");
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::FinishWriting(PPPosProtocol::WriteBlock & rB)
{
	rB.Destroy();
	return 1;
}

//static
void PPPosProtocol::Scb_StartDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, StartDocument());
}

//static
void PPPosProtocol::Scb_EndDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, EndDocument());
}

//static
void PPPosProtocol::Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, StartElement((const char *)pName, (const char **)ppAttrList));
}

//static
void PPPosProtocol::Scb_EndElement(void * ptr, const xmlChar * pName)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, EndElement((const char *)pName));
}

//static
void PPPosProtocol::Scb_Characters(void * ptr, const uchar * pC, int len)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, Characters((const char *)pC, len));
}

int PPPosProtocol::StartDocument()
{
	RdB.TagValue = 0;
	return 1;
}

int PPPosProtocol::EndDocument()
{
	return 1;
}

int FASTCALL PPPosProtocol::Helper_PushQuery(int queryType)
{
	int    ok = 1;
	uint   ref_pos = 0;
	assert(oneof2(queryType, QueryBlock::qCSession, QueryBlock::qRefs));
	THROW(RdB.CreateItem(obQuery, &ref_pos));
	RdB.RefPosStack.push(ref_pos);
	{
		int    test_type = 0;
		QueryBlock * p_item = (QueryBlock *)RdB.GetItem(ref_pos, &test_type);
		assert(test_type == obQuery);
		p_item->Q = queryType;
	}
	CATCHZOK
	return ok;
}

PPPosProtocol::QueryBlock * PPPosProtocol::Helper_RenewQuery(uint & rRefPos, int queryType)
{
	QueryBlock * p_blk = 0;
	int    type = 0;
	RdB.RefPosStack.pop(rRefPos);
	THROW(Helper_PushQuery(queryType));
	rRefPos = PeekRefPos();
	p_blk = (QueryBlock *)RdB.GetItem(rRefPos, &type);
	assert(type == obQuery && p_blk && p_blk->Q == queryType);
	CATCH
		p_blk = 0;
	ENDCATCH
	return p_blk;
}

int PPPosProtocol::StartElement(const char * pName, const char ** ppAttrList)
{
	/*
			stRefsOccured   = 0x0004,
			stQueryOccured  = 0x0008,
			stCSessOccured  = 0x0010
	*/
	int    ok = 1;
	uint   ref_pos = 0;
    (RdB.TempBuf = pName).ToLower();
    int    tok = 0;
    if(RdB.P_ShT) {
		uint _ut = 0;
		RdB.P_ShT->Search(RdB.TempBuf, &_ut, 0);
		tok = _ut;
    }
    if(tok == PPHS_PPPP_START) {
		RdB.State |= RdB.stHeaderOccured;
	}
	else {
		THROW_PP(RdB.State & RdB.stHeaderOccured, PPERR_FILEISNTPPAPI);
		if(RdB.Phase == RdB.phPreprocess) {
			switch(tok) {
				case PPHS_SOURCE:
					THROW(RdB.CreateItem(obSource, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_DESTINATION:
					THROW(RdB.CreateItem(obDestination, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_QUERYCSESS: RdB.State |= RdB.stQueryOccured; break;
				case PPHS_QUERYREFS:  RdB.State |= RdB.stQueryOccured; break;
				case PPHS_REFS:       RdB.State |= RdB.stRefsOccured;  break;
				case PPHS_CSESSION:   RdB.State |= RdB.stCSessOccured; break;
			}
		}
		else if(RdB.Phase == RdB.phProcess) {
			switch(tok) {
				case PPHS_SOURCE:
					THROW(RdB.CreateItem(obSource, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_DESTINATION:
					THROW(RdB.CreateItem(obDestination, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_QUERYCSESS:
					RdB.State |= RdB.stQueryOccured;
					THROW(Helper_PushQuery(QueryBlock::qCSession));
					break;
				case PPHS_QUERYREFS:
					RdB.State |= RdB.stQueryOccured;
					THROW(Helper_PushQuery(QueryBlock::qRefs));
					break;
				case PPHS_POS:
					THROW(RdB.CreateItem(obPosNode, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_WARE:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);

						THROW(RdB.CreateItem(obGoods, &ref_pos));
						RdB.RefPosStack.push(ref_pos);

						if(link_type == obCcLine) {
							int    test_type = 0;
							GoodsBlock * p_item = (GoodsBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obGoods);
							p_item->Flags |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_GOODSGROUP:
					THROW(RdB.CreateItem(obGoodsGroup, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_LOT:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obLot, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						{
							int    test_type = 0;
							LotBlock * p_item = (LotBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obLot);
							if(link_type == obGoods) {
								p_item->GoodsBlkP = link_ref_pos; // ��� ��������� �� ������� ������, �������� �����������
							}
						}
					}
					break;
				case PPHS_CARD:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obSCard, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCCheck) {
							int    test_type = 0;
							SCardBlock * p_item = (SCardBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obSCard);
							p_item->Flags |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_QUOTE:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obQuot, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						{
							int    test_type = 0;
							QuotBlock * p_item = (QuotBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obQuot);
							if(link_type == obGoods) {
								p_item->GoodsBlkP = link_ref_pos; // ��������� ��������� �� ������� ������, �������� �����������
							}
							else if(link_type == obGoodsGroup) {
								p_item->BlkFlags |= p_item->fGroup;
								p_item->GoodsGroupBlkP = link_ref_pos; // ��������� ��������� �� ������� �������� ������, ������� �����������
							}
						}
					}
					break;
				case PPHS_KIND:
					{
						int  type = 0;
						void * p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuot) {
							uint   qk_ref_pos = 0;
							THROW(RdB.CreateItem(obQuotKind, &qk_ref_pos));
							{
								int    test_type = 0;
								QuotKindBlock * p_qk_blk = (QuotKindBlock *)RdB.GetItem(qk_ref_pos, &test_type);
								assert(test_type == obQuotKind);
								p_qk_blk->Flags |= ObjectBlock::fRefItem;
							}
							RdB.RefPosStack.push(qk_ref_pos);
						}
					}
					break;
				case PPHS_QUOTEKIND:
					THROW(RdB.CreateItem(obQuotKind, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_CSESSION:
					RdB.State |= RdB.stCSessOccured;
					THROW(RdB.CreateItem(obCSession, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_CC:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obCCheck, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCSession) {
							int    test_type = 0;
							CCheckBlock * p_item = (CCheckBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obCCheck);
							p_item->CSessionBlkP = link_ref_pos; // ��� ��������� �� ������� ������, ������� �����������
						}
					}
					break;
				case PPHS_CCL:
					{
						uint   link_ref_pos = PeekRefPos();
						int    link_type = 0;
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obCcLine, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCCheck) {
							int    test_type = 0;
							CcLineBlock * p_item = (CcLineBlock *)RdB.GetItem(ref_pos, &test_type);
							assert(test_type == obCcLine);
							p_item->CCheckBlkP = link_ref_pos; // ������ ��������� �� ������� ����, �������� �����������
						}
					}
					break;
				case PPHS_OWNER:
					THROW(RdB.CreateItem(obPerson, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_PARENT:
					THROW(RdB.CreateItem(obParent, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_CODE:
					{
						int  type = 0;
						void * p_item = PeekRefItem(&ref_pos, &type);
						if(type == obGoods) {
							uint   goods_code_ref_pos = 0;
							THROW(RdB.CreateItem(obGoodsCode, &goods_code_ref_pos));
							RdB.RefPosStack.push(goods_code_ref_pos);
							{
								int    test_type = 0;
								GoodsCode * p_item = (GoodsCode *)RdB.GetItem(goods_code_ref_pos, &test_type);
								assert(test_type == obGoodsCode);
								p_item->GoodsBlkP = ref_pos; // ��� ��������� �� ������� ������, �������� �����������
							}
						}
					}
					break;
				case PPHS_REFS:
					RdB.State |= RdB.stRefsOccured;
					break;
				case PPHS_SYSTEM:
				case PPHS_VERSION:
				case PPHS_UUID:
				case PPHS_TIME:
				case PPHS_ID:
				case PPHS_NAME:
				case PPHS_RANK:
				case PPHS_PRICE:
				case PPHS_DISCOUNT:
				case PPHS_RETURN:
				case PPHS_RESTRICTION:
				case PPHS_PERIOD:
				case PPHS_WEEKDAY:
				case PPHS_TIMERANGE:
				case PPHS_AMOUNTRANGE:
				case PPHS_LOW:
				case PPHS_UPP:
				case PPHS_VALUE:
				case PPHS_EXPIRY: // lot
				case PPHS_FLAGS:
				case PPHS_AMOUNT:
				case PPHS_QTTY:
				case PPHS_SUMDISCOUNT:
				case PPHS_INNERID:
				case PPHS_OBJ:
				case PPHS_LAST:
				case PPHS_CURRENT:
				case PPHS_DATE: // log
				case PPHS_COST: // lot
				case PPHS_SERIAL: // lot
					break;
			}
		}
	}
	RdB.TokPath.push(tok);
	RdB.TagValue = 0;
	CATCH
		SaxStop();
		RdB.State |= RdB.stError;
		ok = 0;
	ENDCATCH
    return ok;
}

void FASTCALL PPPosProtocol::Helper_AddStringToPool(uint * pPos)
{
	if(RdB.TagValue.NotEmptyS()) {
		RdB.AddS(RdB.TagValue, pPos);
	}
}

uint SLAPI PPPosProtocol::PeekRefPos() const
{
	void * p_test = RdB.RefPosStack.SStack::peek();
	return p_test ? *(uint *)p_test : UINT_MAX;
}

void * SLAPI PPPosProtocol::PeekRefItem(uint * pRefPos, int * pType) const
{
	void * p_result = 0;
	void * p_test = RdB.RefPosStack.SStack::peek();
	if(p_test) {
		const uint ref_pos = RdB.RefPosStack.peek();
		ASSIGN_PTR(pRefPos, ref_pos);
		p_result = RdB.GetItem(ref_pos, pType);
	}
	else {
		ASSIGN_PTR(pRefPos, UINT_MAX);
	}
	return p_result;
}

int PPPosProtocol::EndElement(const char * pName)
{
	int    tok = 0;
	int    ok = RdB.TokPath.pop(tok);
	uint   ref_pos = 0;
	uint   parent_ref_pos = 0;
	void * p_item = 0;
	int    type = 0;
	int    is_processed = 1;
	if(RdB.Phase == RdB.phPreprocess) {
		switch(tok) {
			case PPHS_PPPP_START:
				RdB.State &= ~RdB.stHeaderOccured;
				break;
			case PPHS_CODE:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obSource:
					case obDestination: Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->CodeP); break;
						break;
				}
				break;
			case PPHS_SYSTEM:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->SystemP);
				}
				break;
			case PPHS_VERSION:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->VersionP);
				}
				break;
			case PPHS_UUID:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					if(RdB.TagValue.NotEmptyS()) {
						S_GUID uuid;
						if(uuid.FromStr(RdB.TagValue)) {
							((RouteObjectBlock *)p_item)->Uuid = uuid;
						}
						else
							; // @error
					}
				}
				break;
			case PPHS_SOURCE:
			case PPHS_DESTINATION:
				RdB.RefPosStack.pop(ref_pos);
				break;
			default:
				is_processed = 0;
				break;
		}
	}
	else if(RdB.Phase == RdB.phProcess) {
		switch(tok) {
			case PPHS_PPPP_START:
				RdB.State &= ~RdB.stHeaderOccured;
				break;
			case PPHS_OBJ:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qRefs) {
						if(RdB.TagValue.NotEmptyS()) {
							long   obj_type_ext = 0;
							PPID   obj_type = DS.GetObjectTypeBySymb(RdB.TagValue, &obj_type_ext);
							if(obj_type) {
								if(p_blk->ObjType) {
									THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qRefs));
								}
								p_blk->ObjType = obj_type;
							}
						}
					}
				}
				break;
			case PPHS_LAST:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qCSession) {
						if(p_blk->CSess || !p_blk->Period.IsZero()) {
							THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
						}
						p_blk->Flags |= QueryBlock::fCSessLast;
					}
				}
				break;
			case PPHS_CURRENT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qCSession) {
						if(p_blk->CSess || !p_blk->Period.IsZero()) {
							THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
						}
						p_blk->Flags |= QueryBlock::fCSessCurrent;
					}
				}
				break;
			case PPHS_POS:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCSession) {
					((CSessionBlock *)p_item)->PosBlkP = ref_pos;
				}
				break;
			case PPHS_WARE:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCcLine) {
					((CcLineBlock *)p_item)->GoodsBlkP = ref_pos;
				}
				break;
			case PPHS_QUOTE:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obGoods) {
					((SCardBlock *)p_item)->OwnerBlkP = ref_pos;
				}
				else if(type == obGoodsGroup) {

				}
				break;
			case PPHS_CARD:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCCheck) {
					((CCheckBlock *)p_item)->SCardBlkP = ref_pos;
				}
				break;
			case PPHS_KIND:
				{
					RdB.RefPosStack.pop(ref_pos);
					int    this_type = 0;
					void * p_this_item = RdB.GetItem(ref_pos, &this_type);
					if(this_type == obQuotKind) {
						//
						// �������� ����� ����������� ��� ���������. ���� �������, �� �������� ���� �������
						// � ���������� ��������� ������
						//
						if(ref_pos == (RdB.RefList.getCount()-1)) {
							const ObjBlockRef obr = RdB.RefList.at(ref_pos); // not ��� obr ������ ��������� ������ - ������� ����� ���� ������ ����
							assert(obr.Type == obQuotKind);
							if(obr.P == (RdB.QkBlkList.getCount()-1)) {
								uint   other_ref_pos = 0;
								if(RdB.SearchAnalogRef_QuotKind(*(QuotKindBlock *)p_this_item, ref_pos, &other_ref_pos)) {
									RdB.RefList.atFree(ref_pos);
									RdB.QkBlkList.atFree(obr.P);
									ref_pos = other_ref_pos;
								}
							}
						}
						parent_ref_pos = PeekRefPos();
						p_item = RdB.GetItem(parent_ref_pos, &type);
						if(type == obQuot) {
							((QuotBlock *)p_item)->QuotKindBlkP = ref_pos;
						}
					}
				}
				break;
			case PPHS_OWNER:
				RdB.RefPosStack.pop(ref_pos);
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obSCard) {
					((SCardBlock *)p_item)->OwnerBlkP = ref_pos;
				}
				break;
			case PPHS_PARENT:
				RdB.RefPosStack.pop(ref_pos);
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obGoods) {
					((GoodsBlock *)p_item)->ParentBlkP = ref_pos;
				}
				else if(type == obGoodsGroup) {
					((GoodsGroupBlock *)p_item)->ParentBlkP = ref_pos;
				}
				break;
			case PPHS_PERIOD:
				{
					DateRange period;
					if(strtoperiod(RdB.TagValue, &period, 0)) {
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind) {
							((QuotKindBlock *)p_item)->Period = period;
						}
						else if(type == obQuery) {
							QueryBlock * p_blk = (QueryBlock *)p_item;
							if(p_blk->Q == QueryBlock::qCSession) {
								if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
									THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
								}
								p_blk->Period = period;
							}
						}
					}
				}
				break;
			case PPHS_LOW:
				{
					int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_TIMERANGE) {
						LTIME   t = ZEROTIME;
						if(strtotime(RdB.TagValue, TIMF_HMS, &t)) {
							p_item = PeekRefItem(&ref_pos, &type);
							if(type == obQuotKind) {
								((QuotKindBlock *)p_item)->TimeRestriction.low = t;
							}
						}
					}
					else if(prev_tok == PPHS_AMOUNTRANGE) {
						double v = RdB.TagValue.ToReal();
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind) {
							((QuotKindBlock *)p_item)->AmountRestriction.low = v;
						}
					}
				}
				break;
			case PPHS_UPP:
				{
					int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_TIMERANGE) {
						LTIME   t = ZEROTIME;
						if(strtotime(RdB.TagValue, TIMF_HMS, &t)) {
							p_item = PeekRefItem(&ref_pos, &type);
							if(type == obQuotKind) {
								((QuotKindBlock *)p_item)->TimeRestriction.upp = t;
							}
						}
					}
					else if(prev_tok == PPHS_AMOUNTRANGE) {
						double v = RdB.TagValue.ToReal();
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind) {
							((QuotKindBlock *)p_item)->AmountRestriction.upp = v;
						}
					}
				}
				break;
			case PPHS_ID:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obPosNode: ((PosNodeBlock *)p_item)->ID = _val_id; break;
						case obGoods: ((GoodsBlock *)p_item)->ID = _val_id; break;
						case obGoodsGroup: ((GoodsGroupBlock *)p_item)->ID = _val_id; break;
						case obPerson: ((PersonBlock *)p_item)->ID = _val_id; break;
						case obSCard: ((SCardBlock *)p_item)->ID = _val_id; break;
						case obParent: ((ParentBlock *)p_item)->ID = _val_id; break;
						case obQuotKind: ((QuotKindBlock *)p_item)->ID = _val_id; break;
						case obCSession: ((CSessionBlock *)p_item)->ID = _val_id; break;
						case obCCheck: ((CCheckBlock *)p_item)->ID = _val_id; break;
						case obCcLine: ((CcLineBlock *)p_item)->RByCheck = _val_id; break;
						case obQuery:
							{
								QueryBlock * p_blk = (QueryBlock *)p_item;
								if(p_blk->Q == QueryBlock::qCSession) {
									if(_val_id) {
										if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
											THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
										}
										p_blk->CSess = _val_id;
										p_blk->Flags &= ~QueryBlock::fCSessN;
									}
								}
							}
							break;
					}
				}
				break;
			case PPHS_INNERID:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obGoods) {
						((GoodsBlock *)p_item)->InnerId = _val_id;
					}
				}
				break;
			case PPHS_VALUE:
				{
					const double _value = RdB.TagValue.ToReal();
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obQuot:
							((QuotBlock *)p_item)->Value = _value;
							break;
					}
				}
				break;
			case PPHS_RANK:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obQuotKind:
							((QuotKindBlock *)p_item)->Rank = (int16)_val_id;
							break;
					}
				}
				break;
			case PPHS_NAME:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obPosNode: Helper_AddStringToPool(&((PosNodeBlock *)p_item)->NameP); break;
					case obGoods:   Helper_AddStringToPool(&((GoodsBlock *)p_item)->NameP); break;
					case obGoodsGroup: Helper_AddStringToPool(&((GoodsGroupBlock *)p_item)->NameP); break;
					case obPerson: Helper_AddStringToPool(&((PersonBlock *)p_item)->NameP); break;
					case obSCard: break;
					case obQuotKind: Helper_AddStringToPool(&((QuotKindBlock *)p_item)->NameP); break;
				}
				break;
			case PPHS_SERIAL:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obLot)
					Helper_AddStringToPool(&((LotBlock *)p_item)->SerailP);
				break;
			case PPHS_CODE:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obPosNode:     Helper_AddStringToPool(&((PosNodeBlock *)p_item)->CodeP); break;
					case obGoodsGroup:  Helper_AddStringToPool(&((GoodsGroupBlock *)p_item)->CodeP); break;
					case obPerson:      Helper_AddStringToPool(&((PersonBlock *)p_item)->CodeP); break;
					case obSCard:       Helper_AddStringToPool(&((SCardBlock *)p_item)->CodeP); break;
					case obParent:      Helper_AddStringToPool(&((ParentBlock *)p_item)->CodeP); break;
					case obQuotKind:    Helper_AddStringToPool(&((QuotKindBlock *)p_item)->CodeP); break;
					case obSource:
					case obDestination: Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->CodeP); break;
					case obGoods:
						break;
					case obGoodsCode:
						Helper_AddStringToPool(&((GoodsCode *)p_item)->CodeP);
						RdB.RefPosStack.pop(ref_pos);
						break;
					case obCSession:
						if(RdB.TagValue.NotEmptyS()) {
							long   icode = RdB.TagValue.ToLong();
							((CSessionBlock *)p_item)->Code = icode;
						}
						break;
					case obCCheck:
						if(RdB.TagValue.NotEmptyS()) {
							long   icode = RdB.TagValue.ToLong();
							((CCheckBlock *)p_item)->Code = icode;
						}
						break;
					case obQuery:
						{
							QueryBlock * p_blk = (QueryBlock *)p_item;
							if(p_blk->Q == QueryBlock::qCSession) {
								const long csess_n = RdB.TagValue.ToLong();
								if(csess_n) {
									if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
										THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
									}
									p_blk->CSess = csess_n;
									p_blk->Flags |= QueryBlock::fCSessN;
								}
							}
						}
						break;
				}
				break;
			case PPHS_REST:
				{
					const double _value = RdB.TagValue.ToReal();
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						if(_value >= 0.0)
							((LotBlock *)p_item)->Rest = _value;
					}
					else if(type == obGoods) {
						if(_value >= 0.0)
							((GoodsBlock *)p_item)->Rest = _value;
					}
				}
				break;
			case PPHS_COST:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obLot)
					((LotBlock *)p_item)->Cost = RdB.TagValue.ToReal();
				break;
			case PPHS_PRICE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods)
					((GoodsBlock *)p_item)->Price = RdB.TagValue.ToReal();
				else if(type == obCcLine)
					((CcLineBlock *)p_item)->Price = RdB.TagValue.ToReal();
				else if(type == obLot)
					((LotBlock *)p_item)->Price = RdB.TagValue.ToReal();
				break;
			case PPHS_DISCOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obSCard)
					((SCardBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				else if(type == obCCheck)
					((CCheckBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				else if(type == obCcLine)
					((CcLineBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				break;
			case PPHS_RETURN:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obCCheck) {
					((CCheckBlock *)p_item)->SaCcFlags |= CCHKF_RETURN;
				}
				else {
					; // @error
				}
				break;
			case PPHS_SUMDISCOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obCcLine)
					((CcLineBlock *)p_item)->SumDiscount = RdB.TagValue.ToReal();
				break;
			case PPHS_QTTY:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obCcLine)
					((CcLineBlock *)p_item)->Qtty = RdB.TagValue.ToReal();
				break;
			case PPHS_SYSTEM:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->SystemP);
				}
				break;
			case PPHS_VERSION:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->VersionP);
				}
				break;
			case PPHS_UUID:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					if(RdB.TagValue.NotEmptyS()) {
						S_GUID uuid;
						if(uuid.FromStr(RdB.TagValue)) {
							((RouteObjectBlock *)p_item)->Uuid = uuid;
						}
						else
							; // @error
					}
				}
				break;
			case PPHS_DATE:
				{
					LDATE dt = strtodate_(RdB.TagValue, DATF_ISO8601);
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						((LotBlock *)p_item)->Dt = dt;
					}
				}
				break;
			case PPHS_EXPIRY:
				{
					LDATE dt = strtodate_(RdB.TagValue, DATF_ISO8601);
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						((LotBlock *)p_item)->Expiry = dt;
					}
				}
				break;
			case PPHS_TIME:
				{
					LDATETIME dtm;
					strtodatetime(RdB.TagValue, &dtm, DATF_ISO8601, 0);
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obCSession: ((CSessionBlock *)p_item)->Dtm = dtm; break;
						case obCCheck:   ((CCheckBlock *)p_item)->Dtm = dtm; break;
					}
				}
				break;
			case PPHS_FLAGS:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obCCheck:
						{
							uint32 _f = 0;
							size_t real_len = 0;
							RdB.TagValue.DecodeHex(0, &_f, sizeof(_f), &real_len);
							if(real_len == sizeof(_f)) {
								((CCheckBlock *)p_item)->CcFlags = _f;
							}
						}
						break;
				}
				break;
			case PPHS_AMOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obCCheck:
						{
							const double _value = RdB.TagValue.ToReal();
							((CCheckBlock *)p_item)->Amount = _value;
						}
						break;
					case obCcLine:
						{
							const double _value = RdB.TagValue.ToReal();
							((CcLineBlock *)p_item)->Amount = _value;
						}
						break;
				}
				break;
			case PPHS_SOURCE:
			case PPHS_DESTINATION:
			case PPHS_GOODSGROUP:
			case PPHS_LOT:
			case PPHS_QUOTEKIND:
			case PPHS_CSESSION:
			case PPHS_CC:
			case PPHS_CCL:
				RdB.RefPosStack.pop(ref_pos);
				break;
			case PPHS_RESTRICTION:
			case PPHS_REFS:
			case PPHS_QUERYCSESS:
			case PPHS_QUERYREFS:
			case PPHS_TIMERANGE:
			case PPHS_AMOUNTRANGE:
				break;
			default:
				is_processed = 0;
				break;
		}
	}
	if(is_processed) {
		(RdB.TempBuf = pName).ToLower();
		if(RdB.P_ShT) {
			uint _ut = 0;
			RdB.P_ShT->Search(RdB.TempBuf, &_ut, 0);
			assert((int)_ut == tok);
		}
	}
	assert(ok);
	CATCH
		SaxStop();
		RdB.State |= RdB.stError;
		ok = 0;
	ENDCATCH
    return ok;
}

int PPPosProtocol::Characters(const char * pS, size_t len)
{
	//
	// ���� ������ ����� ���� �������� ����������� ��������. �� ����� StartElement ��������
	// ����� RdB.TagValue, � ����� ������ ����� ��������� ������������ ������ ��������� ���������
	//
	RdB.TagValue.CatN(pS, len);
	return 1;
}

extern "C" xmlParserCtxtPtr xmlCreateURLParserCtxt(const char * filename, int options);
void xmlDetectSAX2(xmlParserCtxtPtr ctxt); // @prototype

SLAPI PPPosProtocol::ReadBlock::ReadBlock()
{
	P_SaxCtx = 0;
	State = 0;
	Phase = phUnkn;
	P_ShT = PPGetStringHash(PPSTR_HASHTOKEN);
}

SLAPI PPPosProtocol::ReadBlock::~ReadBlock()
{
	Destroy();
	P_ShT = 0;
}

void SLAPI PPPosProtocol::ReadBlock::Destroy()
{
	if(P_SaxCtx) {
		P_SaxCtx->sax = 0;
		xmlFreeDoc(P_SaxCtx->myDoc);
		P_SaxCtx->myDoc = 0;
		xmlFreeParserCtxt(P_SaxCtx);
		P_SaxCtx = 0;
	}
	State = 0;
	Phase = phUnkn;

	TempBuf = 0;
	TagValue = 0;
	SrcFileName = 0;
	TokPath.freeAll();
	RefPosStack.clear();
	SrcBlkList.freeAll();
	DestBlkList.freeAll();
	GoodsBlkList.freeAll();
	GoodsGroupBlkList.freeAll();
	GoodsCodeList.freeAll();
	LotBlkList.freeAll();
	QkBlkList.freeAll();
	QuotBlkList.freeAll();
	PersonBlkList.freeAll();
	SCardBlkList.freeAll();
	ParentBlkList.freeAll();
	PosBlkList.freeAll();
	CSessBlkList.freeAll();
	CcBlkList.freeAll();
	CclBlkList.freeAll();
	QueryList.freeAll();
	RefList.freeAll();
}

int SLAPI PPPosProtocol::CreateGoodsGroup(const GoodsGroupBlock & rBlk, int isFolder, PPID * pID)
{
	int    ok = -1;
	SString name_buf, code_buf;
	PPID   native_id = 0;
	if(rBlk.NativeID) {
		native_id = rBlk.NativeID;
		ok = 2;
	}
	else {
		Goods2Tbl::Rec gg_rec;
		BarcodeTbl::Rec bc_rec;
		PPID   inner_parent_id = 0;
		if(rBlk.ParentBlkP) {
			int   inner_type = 0;
			ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(rBlk.ParentBlkP, &inner_type);
			assert(p_inner_blk);
			if(p_inner_blk) {
				assert(inner_type == obParent);
				THROW(CreateParentGoodsGroup(*p_inner_blk, 1, &inner_parent_id)); // @recursion
			}
		}
		RdB.GetS(rBlk.NameP, name_buf);
		name_buf.Transf(CTRANSF_UTF8_TO_INNER);
		RdB.GetS(rBlk.CodeP, code_buf);
		code_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(code_buf.NotEmptyS() && GgObj.SearchCode(code_buf, &bc_rec) > 0 && GgObj.Search(bc_rec.GoodsID, &gg_rec) > 0) {
			if(gg_rec.Kind == PPGDSK_GROUP && !(gg_rec.Flags & GF_ALTGROUP)) {
				//if((isFolder && gg_rec.Flags & GF_FOLDER) || (!isFolder && !(gg_rec.Flags & GF_FOLDER))) {
				{
					native_id = gg_rec.ID;
					ok = 3;
				}
			}
		}
		if(!native_id) {
			PPID   inner_id = 0;
			if(name_buf.NotEmptyS() && GgObj.SearchByName(name_buf, &inner_id, &gg_rec) > 0) {
				if(gg_rec.Kind == PPGDSK_GROUP && !(gg_rec.Flags & GF_ALTGROUP)) {
					//if((isFolder && gg_rec.Flags & GF_FOLDER) || (!isFolder && !(gg_rec.Flags & GF_FOLDER))) {
					{
						native_id = gg_rec.ID;
						ok = 3;
					}
				}
			}
		}
		if(!native_id) {
			PPGoodsPacket gg_pack;
			THROW(GgObj.InitPacket(&gg_pack, isFolder ? gpkndFolderGroup : gpkndOrdinaryGroup, inner_parent_id, 0, code_buf));
			STRNSCPY(gg_pack.Rec.Name, name_buf);
			if(gg_pack.Rec.ParentID) {
				Goods2Tbl::Rec parent_rec;
				if(GgObj.Search(gg_pack.Rec.ParentID, &parent_rec) > 0) {
					if(parent_rec.Kind != PPGDSK_GROUP || !(parent_rec.Flags & GF_FOLDER) || (parent_rec.Flags & (GF_ALTGROUP|GF_EXCLALTFOLD))) {
						gg_pack.Rec.ParentID = 0;
						// @todo message
					}
				}
				else {
					gg_pack.Rec.ParentID = 0;
					// @todo message
				}
			}
			THROW(GgObj.PutPacket(&native_id, &gg_pack, 1));
			ok = 1;
		}
	}
	ASSIGN_PTR(pID, native_id);
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::CreateParentGoodsGroup(const ParentBlock & rBlk, int isFolder, PPID * pID)
{
	int    ok = -1;
	PPID   native_id = 0;
	SString code_buf, temp_buf;
	SString name_buf;
	RdB.GetS(rBlk.CodeP, code_buf);
	code_buf.Transf(CTRANSF_UTF8_TO_INNER);
	if(rBlk.ID || code_buf.NotEmptyS()) {
		for(uint i = 0; i < RdB.GoodsGroupBlkList.getCount(); i++) {
			GoodsGroupBlock & r_grp_blk = RdB.GoodsGroupBlkList.at(i);
			int   this_block = 0;
			if(rBlk.ID) {
				if(r_grp_blk.ID == rBlk.ID) {
					this_block = 1;
				}
			}
			else if(r_grp_blk.CodeP) {
				RdB.GetS(r_grp_blk.CodeP, temp_buf);
				if(code_buf == temp_buf) {
					this_block = 1;
				}
			}
			if(this_block) {
				THROW(ok = CreateGoodsGroup(r_grp_blk, isFolder, &native_id));
				break;
			}
		}
	}
	ASSIGN_PTR(pID, native_id);
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ResolveGoodsBlock(const GoodsBlock & rBlk, uint refPos, int asRefOnly, PPID defParentID, PPID defUnitID, PPID srcArID, PPID locID, PPID * pNativeID)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   native_id = rBlk.NativeID;
	SString temp_buf;
	Goods2Tbl::Rec ex_goods_rec;
	Goods2Tbl::Rec parent_rec;
	BarcodeTbl::Rec ex_bc_rec;
	PPIDArray pretend_obj_list;
	if(rBlk.Flags & ObjectBlock::fRefItem) {
		//
		// ��� ��������, ���������� ��� ������ �� ������ ����� ������� � ����� ��, �� ��������� �� �����
		//
		PPID   pretend_id = 0;
		if(rBlk.ID > 0 && GObj.Search(rBlk.ID, &ex_goods_rec) > 0) {
			pretend_id = ex_goods_rec.ID;
		}
		for(uint j = 0; j < RdB.GoodsCodeList.getCount(); j++) {
			const GoodsCode & r_c = RdB.GoodsCodeList.at(j);
			if(r_c.GoodsBlkP == refPos) {
				RdB.GetS(r_c.CodeP, temp_buf);
				if(temp_buf.NotEmptyS()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.Len() < sizeof(ex_bc_rec.Code)) {
						if(GObj.SearchByBarcode(temp_buf, &ex_bc_rec, &ex_goods_rec, 0 /* no adopt */) > 0)
							pretend_obj_list.add(ex_goods_rec.ID);
					}
					else {
						; // @error
					}
				}
			}
		}
		if(pretend_obj_list.getCount()) {
			pretend_obj_list.sortAndUndup();
			if(pretend_id) {
				if(pretend_obj_list.lsearch(pretend_id)) {
					native_id = pretend_id; // ��� - ��� �������� ��� ������������� (������������� � �� id � �� ����)
				}
				else {
					// @todo ���� ��������. ������������� ������, �� �� ������������� �����, ���������� ��� ������ � ���
					if(pretend_obj_list.getCount() == 1) {
						native_id = pretend_obj_list.get(0); // ���������� ��������, ��������� �� ����. �� ������, ��
							// ���, ��-�����, ��������, ��� �� ��������������.
					}
					else {
						native_id = pretend_id; // ���� ������������ �� ����� ���������, �� ������������� �������� ��������
					}
				}
			}
			else if(pretend_obj_list.getCount() == 1) {
				native_id = pretend_obj_list.get(0); // ������������ ��� ��� �������������� - ������ �������� ��������
			}
			else {
				// @todo ���� ��������: �������� ��������� ����� � ������� �������������� ���������������
				native_id = pretend_obj_list.getLast(); // ���� ���������� ��������� - �� ������ ����� ����� �����
			}
		}
		else if(pretend_id)
			native_id = pretend_id;
		else {
			; // @err ���������� ����������� ���������� ������ �� �����
		}
	}
	else if(!asRefOnly) {
		PPGoodsPacket goods_pack;
		PPQuotArray quot_list;
		GObj.InitPacket(&goods_pack, gpkndGoods, 0, 0, 0);
		quot_list.GoodsID = 0;
		int    use_ar_code = 0;
		PPID   goods_by_ar_id = 0;
		if(rBlk.ID > 0) {
			(temp_buf = 0).Cat(rBlk.ID);
			if(GObj.P_Tbl->SearchByArCode(srcArID, temp_buf, 0, &ex_goods_rec) > 0) {
				goods_by_ar_id = ex_goods_rec.ID;
			}
			else {
				ArGoodsCodeTbl::Rec new_ar_code;
				MEMSZERO(new_ar_code);
				new_ar_code.ArID = srcArID;
				new_ar_code.Pack = 1000; // =1
				STRNSCPY(new_ar_code.Code, temp_buf);
				THROW_SL(goods_pack.ArCodes.insert(&new_ar_code));
			}
			use_ar_code = 1;
		}
		RdB.GetS(rBlk.NameP, temp_buf);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		STRNSCPY(goods_pack.Rec.Name, temp_buf);
		STRNSCPY(goods_pack.Rec.Abbr, temp_buf);
		if(GObj.SearchByName(temp_buf, 0, &ex_goods_rec) > 0) {
			if(use_ar_code && (/*goods_by_ar_id && */ex_goods_rec.ID != goods_by_ar_id)) {
				THROW(GObj.ForceUndupName(goods_by_ar_id, temp_buf));
				THROW(GObj.UpdateName(ex_goods_rec.ID, temp_buf, 1));
			}
			else if(!use_ar_code) { // ��� ������ � ������� ��������������� ������ �� ������������ �� ��������� � ������
				pretend_obj_list.add(ex_goods_rec.ID);
			}
		}
		{
			PPID   parent_id = 0;
			if(rBlk.ParentBlkP) {
				int   inner_type = 0;
				ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(rBlk.ParentBlkP, &inner_type);
				assert(p_inner_blk);
				if(p_inner_blk) {
					assert(inner_type == obParent);
					THROW(CreateParentGoodsGroup(*p_inner_blk, 0, &parent_id));
				}
			}
			{
				if(GgObj.Fetch(parent_id, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_FOLDER|GF_EXCLALTFOLD)))
					goods_pack.Rec.ParentID = parent_id;
				else
					goods_pack.Rec.ParentID = defParentID;
			}
		}
		for(uint j = 0; j < RdB.GoodsCodeList.getCount(); j++) {
			const GoodsCode & r_c = RdB.GoodsCodeList.at(j);
			if(r_c.GoodsBlkP == refPos) {
				RdB.GetS(r_c.CodeP, temp_buf);
				if(temp_buf.NotEmptyS()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					if(temp_buf.Len() < sizeof(ex_bc_rec.Code)) {
						THROW(goods_pack.Codes.Add(temp_buf, 0, 1.0));
						if(GObj.SearchByBarcode(temp_buf, &ex_bc_rec, &ex_goods_rec, 0 /* no adopt */) > 0) {
							if(use_ar_code && (goods_by_ar_id && ex_goods_rec.ID != goods_by_ar_id)) {
								THROW(GObj.P_Tbl->RemoveDupBarcode(goods_by_ar_id, temp_buf, 1));
							}
							else
								pretend_obj_list.add(ex_goods_rec.ID);
						}
					}
					else {
						; // @error
					}
				}
			}
		}
		pretend_obj_list.sortAndUndup();
		if(use_ar_code) {
			if(goods_by_ar_id) {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = goods_by_ar_id;
				if(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0) {
					STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
					STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
					if(goods_pack.Rec.ParentID)
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					ex_goods_pack.Codes = goods_pack.Codes;
					//
					// ����������� ������ ����� �� �������:
					// ���� (use_ar_code && goods_by_ar_id), �� ������ ��� ��� ��������� � ex_goods_pack
					// ����� ���� ��� ����� ���� � ���� ���� �� �������, �� �������� � �����������
					// ������� �������������� ������.
					// ����� �������, ������� ����������� �� ������� ����� �� ������� ����� �� ����������.
					//
					if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
						parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					}
					THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
					native_id = ex_goods_id;
				}
				else {
					; // @error
				}
			}
			else if(pretend_obj_list.getCount() == 0) {
				PPID   new_goods_id = 0;
				SETIFZ(goods_pack.Rec.UnitID, defUnitID);
				THROW(GObj.PutPacket(&new_goods_id, &goods_pack, 1));
				native_id = new_goods_id;
			}
			else {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = pretend_obj_list.get(0);
				if(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0) {
					STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
					STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
					if(goods_pack.Rec.ParentID)
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					ex_goods_pack.Codes = goods_pack.Codes;
					for(uint bci = 0; bci < goods_pack.Codes.getCount(); bci++) {
						const BarcodeTbl::Rec & r_bc_rec = goods_pack.Codes.at(bci);
						uint   bcp = 0;
						if(ex_goods_pack.Codes.SearchCode(r_bc_rec.Code, &bcp)) {
							ex_goods_pack.Codes.atFree(bcp);
						}
						ex_goods_pack.Codes.insert(&r_bc_rec);
						THROW(GObj.P_Tbl->RemoveDupBarcode(ex_goods_id, r_bc_rec.Code, 1));
					}
					for(uint aci = 0; aci < goods_pack.ArCodes.getCount(); aci++) {
						THROW_SL(ex_goods_pack.ArCodes.insert(&goods_pack.ArCodes.at(aci)));
					}
					if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
						parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					}
					THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
					native_id = ex_goods_id;
				}
				else {
					; // @error
				}
			}
		}
		else {
			if(pretend_obj_list.getCount() == 0) {
				PPID   new_goods_id = 0;
				SETIFZ(goods_pack.Rec.UnitID, defUnitID);
				THROW(GObj.PutPacket(&new_goods_id, &goods_pack, 1));
				native_id = new_goods_id;
			}
			else if(pretend_obj_list.getCount() == 1) {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = pretend_obj_list.get(0);
				if(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0) {
					STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
					STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
					if(goods_pack.Rec.ParentID)
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					ex_goods_pack.Codes = goods_pack.Codes;
					if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
						parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
						ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
					}
					THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
					native_id = ex_goods_id;
				}
				else {
					; // @error
				}
			}
			else {
				; // @error ������������� ����� ����� ���� ����������� ����� ��� ������ ������ � ��.
			}
		}
		if(native_id) {
			{
				//
				// ������ ���������
				//
				quot_list.GoodsID = native_id;
				for(uint k = 0; k < RdB.QuotBlkList.getCount(); k++) {
					const QuotBlock & r_qb = RdB.QuotBlkList.at(k);
					if(r_qb.GoodsBlkP == refPos) {
						assert(!(r_qb.BlkFlags & r_qb.fGroup));
						int    type_qk = 0;
						const QuotKindBlock * p_qk_item = (const QuotKindBlock *)RdB.GetItem(r_qb.QuotKindBlkP, &type_qk);
						if(p_qk_item) {
							assert(type_qk == obQuotKind);
							if(p_qk_item->NativeID) {
								QuotIdent qi(0 /*locID*/, p_qk_item->NativeID, 0 /*curID*/, 0);
								quot_list.SetQuot(qi, r_qb.Value, r_qb.Flags, r_qb.MinQtty, r_qb.Period.IsZero() ? 0 : &r_qb.Period);
							}
						}
					}
				}
				//
				// ������� ���� ��������� � ������ quot_list ��������� �� ������, ���� � ���������� ������ ���������
				// ���� ������� � ������� (���� �������� ��������������� � ������ Price).
				//
				if(rBlk.Price > 0.0) {
					QuotIdent qi(0 /*locID*/, PPQUOTK_BASE, 0 /*curID*/, 0);
					quot_list.SetQuot(qi, rBlk.Price, 0 /*flags*/, 0, 0 /* period */);
				}
				THROW(GObj.PutQuotList(native_id, &quot_list, 1));
			}
			if(locID) {
				//
				// ������ �����
				//
				ReceiptCore & r_rcpt = P_BObj->trfr->Rcpt;
				LotArray lot_list;
				StrAssocArray serial_list;
				for(uint k = 0; k < RdB.LotBlkList.getCount(); k++) {
					const LotBlock & r_blk = RdB.LotBlkList.at(k);
					if(r_blk.GoodsBlkP == refPos) {
						ReceiptTbl::Rec lot_rec;
						MEMSZERO(lot_rec);
						lot_rec.GoodsID = native_id;
						lot_rec.Dt = r_blk.Dt;
						lot_rec.Expiry = r_blk.Expiry;
						lot_rec.Cost = r_blk.Cost;
						lot_rec.Price = r_blk.Price;
						lot_rec.Quantity = r_blk.Rest;
						lot_rec.Rest = r_blk.Rest;
						if(lot_rec.Rest <= 0.0) {
							lot_rec.Closed = 1;
							lot_rec.Flags |= LOTF_CLOSED;
						}
						lot_rec.Flags |= LOTF_SURROGATE;
						lot_rec.LocID = locID;
						THROW_SL(lot_list.insert(&lot_rec));
                        if(RdB.GetS(r_blk.SerailP, temp_buf) && temp_buf.NotEmptyS()) {
                            serial_list.Add((long)lot_list.getCount(), temp_buf);
                        }
					}
				}
				if(lot_list.getCount() == 0 && rBlk.Rest > 0.0) {
					ReceiptTbl::Rec lot_rec;
					MEMSZERO(lot_rec);
					lot_rec.GoodsID = native_id;
					lot_rec.Dt = getcurdate_();
					lot_rec.Quantity = rBlk.Rest;
					lot_rec.Rest = rBlk.Rest;
					if(lot_rec.Rest <= 0.0) {
						lot_rec.Closed = 1;
						lot_rec.Flags |= LOTF_CLOSED;
					}
					lot_rec.Flags |= LOTF_SURROGATE;
					lot_rec.LocID = locID;
					THROW_SL(lot_list.insert(&lot_rec));
				}
				{
					PPTransaction tra(1);
					THROW(tra);
					//
					// ������� ��� ������������ SURROGATE-����
					//
					{
						PPIDArray lot_id_list_to_remove;
						ReceiptTbl::Key2 rk2;
						MEMSZERO(rk2);
						rk2.GoodsID = native_id;
						if(r_rcpt.search(2, &rk2, spGe) && r_rcpt.data.GoodsID == native_id) do {
							if(r_rcpt.data.Flags & LOTF_SURROGATE && r_rcpt.data.BillID == 0) {
								THROW_SL(lot_id_list_to_remove.add(r_rcpt.data.ID));
                                THROW_DB(r_rcpt.deleteRec());
							}
						} while(r_rcpt.search(2, &rk2, spNext) && r_rcpt.data.GoodsID == native_id);
						for(uint _lidx = 0; _lidx < lot_id_list_to_remove.getCount(); _lidx++) {
							THROW(p_ref->Ot.RemoveTag(PPOBJ_LOT, lot_id_list_to_remove.get(_lidx), 0, 0));
						}
					}
					//
					// ��������� ����� SURROGATE-����
					//
					{
                        for(uint _lidx = 0; _lidx < lot_list.getCount(); _lidx++) {
							ReceiptTbl::Rec lot_rec = lot_list.at(_lidx);
							long   oprno = 1;
							ReceiptTbl::Key1 rk1;
							MEMSZERO(rk1);
							rk1.Dt = lot_rec.Dt;
							rk1.OprNo = oprno;
							while(r_rcpt.search(1, &rk1, spEq)) {
								rk1.OprNo = ++oprno;
							}
							lot_rec.OprNo = oprno;
							{
                        		ReceiptTbl::Key0 rk0;
								MEMSZERO(rk0);
								THROW_DB(r_rcpt.insertRecBuf(&lot_rec, 0, &rk0));
								if(serial_list.Get(_lidx+1, temp_buf)) {
									THROW(P_BObj->SetSerialNumberByLot(rk0.ID, temp_buf, 0));
								}
							}
                        }
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pNativeID, native_id);
	return ok;
}

int SLAPI PPPosProtocol::AcceptData(PPID posNodeID, int silent)
{
	int    ok = 1;
	PPID   loc_id = 0;
	uint   qpb_list_idx = 0;
	SString fmt_buf, msg_buf;
	PPID   src_ar_id = 0; // ������ �������������� �����, ��������������� ��������� ������
	SString temp_buf;
	SString code_buf;
	SString name_buf;
	SString wait_msg_buf;
	PPIDArray temp_id_list;
	PPIDArray pretend_obj_list; // ������ �� ��������, ������� ������������� ��������������
	PPObjUnit u_obj;
	PPObjSCardSeries scs_obj;
	PPObjPersonKind pk_obj;
	if(!silent)
		PPWait(1);
	if(posNodeID) {
		PPCashNode cn_rec;
		if(CnObj.Search(posNodeID, &cn_rec) > 0) {
			loc_id = cn_rec.LocID;
		}
	}
	{
		//
		// ������ ����� �������� ������ �� ��������� � ����������� ������
		//
		if(RdB.SrcBlkList.getCount() == 1) {
			RouteObjectBlock & r_blk = RdB.SrcBlkList.at(0);
			if(!r_blk.Uuid.IsZero()) {
				// @todo ������� ��� ����� ���������� ������� ������ � �������������� �� ������������� ������ src_ar_id
			}
		}
		else {
			if(RdB.SrcBlkList.getCount() > 1) {
				; // @error
			}
			else { // no src info
				; // @log
			}
		}
		//

		//
		if(RdB.DestBlkList.getCount()) {
			for(uint i = 0; i < RdB.DestBlkList.getCount(); i++) {
			}
		}
	}
	{
		//
		// ������ ����� ���������.
		// 2 ����
		//   1-� ���� - ���� ��������� ��� ����� ObjectBlock::fRefItem
		//   2-� ���� - ���� ��������� �� �������� �� ����� ObjectBlock::fRefItem (��� NativeID)
		//
		PPLoadText(PPTXT_IMPQUOTKIND, wait_msg_buf);
		const uint __count = RdB.QkBlkList.getCount();
		for(uint phase = 0; phase < 2; phase++) {
			for(uint i = 0; i < __count; i++) {
				QuotKindBlock & r_blk = RdB.QkBlkList.at(i);
				if(((phase > 0) || !(r_blk.Flags & r_blk.fRefItem)) && !r_blk.NativeID) {
					uint   ref_pos = 0;
					PPID   native_id = 0;
					PPQuotKind qk_rec;
					if(RdB.SearchRef(obQuotKind, i, &ref_pos)) {
						const QuotKindBlock * p_analog = RdB.SearchAnalog_QuotKind(r_blk);
						if(p_analog) {
							r_blk.NativeID = p_analog->NativeID;
						}
						else {
							PPQuotKindPacket pack;
							RdB.GetS(r_blk.CodeP, code_buf);
							code_buf.Transf(CTRANSF_UTF8_TO_INNER);
							RdB.GetS(r_blk.NameP, name_buf);
							name_buf.Transf(CTRANSF_UTF8_TO_INNER);
							if(code_buf.NotEmptyS()) {
								if(QkObj.SearchBySymb(code_buf, &native_id, &qk_rec) > 0) {
									pack.Rec = qk_rec;
								}
								else
									native_id = 0;
							}
							else if(name_buf.NotEmpty()) {
								if(QkObj.SearchByName(name_buf, &native_id, &qk_rec) > 0) {
									pack.Rec = qk_rec;
								}
								else
									native_id = 0;
							}
							if(name_buf.NotEmpty())
								STRNSCPY(pack.Rec.Name, name_buf);
							if(pack.Rec.Name[0] == 0)
								STRNSCPY(pack.Rec.Name, code_buf);
							if(pack.Rec.Name[0] == 0) {
								(temp_buf = 0).CatChar('#').Cat(r_blk.ID);
								STRNSCPY(pack.Rec.Name, temp_buf);
							}
							STRNSCPY(pack.Rec.Symb, code_buf);
							pack.Rec.Rank = r_blk.Rank;
							pack.Rec.SetAmtRange(&r_blk.AmountRestriction);
							pack.Rec.SetTimeRange(r_blk.TimeRestriction);
							pack.Rec.Period = r_blk.Period;
							THROW(QkObj.PutPacket(&native_id, &pack, 1));
							r_blk.NativeID = native_id;
						}
					}
				}
				PPWaitPercent((phase * __count) + i+1, __count * 2, wait_msg_buf);
			}
		}
	}
	{
		//
		// ������ �������� �����
		//
		{
			//
			// ������� �������� ���� �� ������� � ����� ������� ��� ���������������� ��� ������-�����
			//
			PPLoadText(PPTXT_IMPGOODSGRP, wait_msg_buf);
			const uint __count = RdB.GoodsGroupBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				GoodsGroupBlock & r_blk = RdB.GoodsGroupBlkList.at(i);
				uint   ref_pos = 0;
				if(RdB.SearchRef(obGoodsGroup, i, &ref_pos)) {
					PPID   parent_id = 0;
					if(r_blk.ParentBlkP) {
						int   inner_type = 0;
						ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(r_blk.ParentBlkP, &inner_type);
						assert(p_inner_blk);
						if(p_inner_blk) {
							assert(inner_type == obParent);
							THROW(CreateParentGoodsGroup(*p_inner_blk, 1, &parent_id));
						}
					}
				}
				else {
					; // @error
				}
				PPWaitPercent(i+1, __count * 2, wait_msg_buf);
			}
		}
		{
			//
			// ������ �������� ��� ������ ������� ������ (����� ��� ���������������� �� ���������� �������)
			//
			PPLoadText(PPTXT_IMPGOODSGRP, wait_msg_buf);
			const uint __count = RdB.GoodsGroupBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				GoodsGroupBlock & r_blk = RdB.GoodsGroupBlkList.at(i);
				if(r_blk.NativeID == 0) {
					uint   ref_pos = 0;
					if(RdB.SearchRef(obGoodsGroup, i, &ref_pos)) {
						THROW(CreateGoodsGroup(r_blk, 0, &r_blk.NativeID));
					}
					else {
						; // @error
					}
				}
				PPWaitPercent(__count+i+1, __count * 2, wait_msg_buf);
			}
		}
	}
	{
		//
		// ������ �������
		//
		PPGoodsPacket goods_pack;
		PPQuotArray quot_list;
		Goods2Tbl::Rec parent_rec;
		PPUnit u_rec;
		SString def_parent_name;
		SString def_unit_name;
		PPID   def_parent_id = GObj.GetConfig().DefGroupID;
		PPID   def_unit_id = GObj.GetConfig().DefUnitID;
		if(def_unit_id && u_obj.Search(def_unit_id, &u_rec) > 0) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			def_unit_id = 0;
			def_unit_name = "default";
			while(!def_parent_id && u_obj.SearchByName(def_unit_name, 0, &u_rec) > 0) {
				if(u_rec.Flags & PPUnit::Trade) {
					def_unit_id = u_rec.ID;
				}
				else
					(def_unit_name = "default").CatChar('-').CatLongZ(++def_counter, 3);
			}
		}
		if(def_parent_id && GgObj.Search(def_parent_id, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			def_parent_id = 0;
			def_parent_name = "default";
			while(!def_parent_id && GgObj.SearchByName(def_parent_name, 0, &parent_rec) > 0) {
				if(parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
					def_parent_id = parent_rec.ID;
				}
				else
					(def_parent_name = "default").CatChar('-').CatLongZ(++def_counter, 3);
			}
		}
		if(!def_parent_id || def_parent_id != GObj.GetConfig().DefGroupID || !def_unit_id || def_unit_id != GObj.GetConfig().DefUnitID) {
			PPTransaction tra(1);
			THROW(tra);
			if(!def_parent_id) {
				THROW(GgObj.AddSimple(&def_parent_id, gpkndOrdinaryGroup, 0, def_parent_name, 0, 0, 0));
			}
			if(!def_unit_id) {
				THROW(u_obj.AddSimple(&def_unit_id, def_unit_name, PPUnit::Trade, 0));
			}
			if(def_parent_id != GObj.GetConfig().DefGroupID || def_unit_id != GObj.GetConfig().DefUnitID) {
				PPGoodsConfig new_cfg;
				GObj.ReadConfig(&new_cfg);
				new_cfg.DefGroupID = def_parent_id;
				new_cfg.DefUnitID = def_unit_id;
				THROW(GObj.WriteConfig(&new_cfg, 0, 0));
			}
			THROW(tra.Commit());
		}
		assert(def_parent_id > 0);
		assert(def_unit_id > 0);
		{
			PPLoadText(PPTXT_IMPGOODS, wait_msg_buf);
			const uint __count = RdB.GoodsBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				GoodsBlock & r_blk = RdB.GoodsBlkList.at(i);
				uint   ref_pos = 0;
				if(RdB.SearchRef(obGoods, i, &ref_pos)) {
					PPID   native_id = 0;
					THROW(ResolveGoodsBlock(r_blk, ref_pos, 0, def_parent_id, def_unit_id, src_ar_id, loc_id, &native_id));
					r_blk.NativeID = native_id;
				}
				else {
					; // @error
				}
				PPWaitPercent(i+1, __count, wait_msg_buf);
			}
		}
	}
	{
		//
		// ������ ������������ ����
		//
		SString def_dcard_ser_name;
		SString def_ccard_ser_name;
		PPSCardConfig sc_cfg;
		SCardTbl::Rec _ex_sc_rec;
		PPSCardSeries scs_rec;
		PPPersonPacket psn_pack;
		ScObj.FetchConfig(&sc_cfg);
		PPID   def_dcard_ser_id = sc_cfg.DefSerID; // ����� ���������� ���� �� ���������
		PPID   def_ccard_ser_id = sc_cfg.DefCreditSerID; // ����� ��������� ���� �� ���������
		if(def_dcard_ser_id && scs_obj.Search(def_dcard_ser_id, &scs_rec) > 0) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			def_dcard_ser_id = 0;
			def_dcard_ser_name = "default";
			while(!def_dcard_ser_id && scs_obj.SearchByName(def_dcard_ser_name, 0, &scs_rec) > 0) {
				def_dcard_ser_id = scs_rec.ID;
			}
		}
		if(def_ccard_ser_id && scs_obj.Search(def_ccard_ser_id, &scs_rec) > 0) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			def_ccard_ser_id = 0;
			def_ccard_ser_name = "default-credit";
			while(!def_ccard_ser_id && scs_obj.SearchByName(def_ccard_ser_name, 0, &scs_rec) > 0) {
				if(scs_rec.GetType() == scstCredit)
					def_ccard_ser_id = scs_rec.ID;
				else
					(def_ccard_ser_name = "default-credit").CatChar('-').CatLongZ(++def_counter, 3);
			}
		}
		if(!def_dcard_ser_id || !def_ccard_ser_id) {
			PPTransaction tra(1);
			THROW(tra);
			{
				ScObj.ReadConfig(&sc_cfg);
				if(!def_dcard_ser_id) {
					PPSCardSerPacket scs_pack;
					STRNSCPY(scs_pack.Rec.Name, def_dcard_ser_name);
					scs_pack.Rec.SetType(scstDiscount);
					scs_pack.Rec.PersonKindID = PPPRK_CLIENT;
					THROW(scs_obj.PutPacket(&def_dcard_ser_id, &scs_pack, 0));
					sc_cfg.DefSerID = def_dcard_ser_id;
				}
				if(!def_ccard_ser_id) {
					PPSCardSerPacket scs_pack;
					STRNSCPY(scs_pack.Rec.Name, def_ccard_ser_name);
					scs_pack.Rec.SetType(scstCredit);
					scs_pack.Rec.PersonKindID = PPPRK_CLIENT;
					THROW(scs_obj.PutPacket(&def_ccard_ser_id, &scs_pack, 0));
					sc_cfg.DefCreditSerID = def_ccard_ser_id;
				}
				THROW(ScObj.WriteConfig(&sc_cfg, 0));
			}
			THROW(tra.Commit());
		}
		{
			PPLoadText(PPTXT_IMPSCARD, wait_msg_buf);
			const uint __count = RdB.SCardBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				SCardBlock & r_blk = RdB.SCardBlkList.at(i);
				uint   ref_pos = 0;
				if(RdB.SearchRef(obSCard, i, &ref_pos)) {
					if(r_blk.Flags & ObjectBlock::fRefItem) {
						//
						// ��� ��������, ���������� ��� ������ �� ������ ����� ������� � ����� ��, �� ��������� �� �����
						//
						RdB.GetS(r_blk.CodeP, temp_buf);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(ScObj.SearchCode(0, temp_buf, &_ex_sc_rec) > 0) {
							//
							// ����� ��� ������ - ����� �� ����, ������ ���� �����
							// Note: ������, �� �������������� �� ��������� �������� �� ������������ ������� ����.
							//
							r_blk.NativeID = _ex_sc_rec.ID;
						}
					}
					else {
						//SCardTbl::Rec sc_pack;
						//MEMSZERO(sc_pack);
						PPSCardPacket sc_pack;
						pretend_obj_list.clear();
						RdB.GetS(r_blk.CodeP, temp_buf);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						STRNSCPY(sc_pack.Rec.Code, temp_buf);
						if(r_blk.Discount != 0.0) {
							sc_pack.Rec.PDis = R0i(r_blk.Discount * 100);
						}
						assert(def_dcard_ser_id);
						sc_pack.Rec.SeriesID = def_dcard_ser_id;
						if(r_blk.OwnerBlkP) {
							PPPersonKind pk_rec;
							PPID   owner_status_id = PPPRS_PRIVATE;
							int    ref_type = 0;
							PersonBlock * p_psn_blk = (PersonBlock *)RdB.GetItem(r_blk.OwnerBlkP, &ref_type);
							assert(p_psn_blk);
							assert(ref_type == obPerson);
							if(p_psn_blk->NativeID) {
								sc_pack.Rec.PersonID = p_psn_blk->NativeID;
							}
							else {
								const PersonBlock * p_analog = RdB.SearchAnalog_Person(*p_psn_blk);
								if(p_analog) {
									sc_pack.Rec.PersonID = p_analog->NativeID;
									p_psn_blk->NativeID = p_analog->NativeID;
								}
							}
							if(!sc_pack.Rec.PersonID) {
								psn_pack.destroy();
								psn_pack.Rec.Status = owner_status_id;
								RdB.GetS(p_psn_blk->CodeP, code_buf);
								code_buf.Transf(CTRANSF_UTF8_TO_INNER);
								RdB.GetS(p_psn_blk->NameP, temp_buf);
								temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								STRNSCPY(psn_pack.Rec.Name, temp_buf);
								PPID   owner_kind_id = (sc_pack.Rec.SeriesID && scs_obj.Fetch(sc_pack.Rec.SeriesID, &scs_rec) > 0) ? scs_rec.PersonKindID : 0;
								SETIFZ(owner_kind_id, ScObj.GetConfig().PersonKindID);
								SETIFZ(owner_kind_id, PPPRK_CLIENT);
								if(owner_kind_id && pk_obj.Search(owner_kind_id, &pk_rec) > 0) {
									PersonTbl::Rec psn_rec;
									PPID   reg_type_id = pk_rec.CodeRegTypeID;
									temp_id_list.clear();
									if(code_buf.NotEmptyS() && reg_type_id && PsnObj.GetListByRegNumber(reg_type_id, owner_kind_id, code_buf, temp_id_list) > 0) {
										if(temp_id_list.getCount() == 1)
											sc_pack.Rec.PersonID = temp_id_list.getSingle();
										else if(temp_id_list.getCount() > 1) {
											for(uint k = 0; !sc_pack.Rec.PersonID && k < temp_id_list.getCount(); k++) {
												if(PsnObj.Search(temp_id_list.get(k), &psn_rec) > 0 && stricmp866(psn_rec.Name, psn_pack.Rec.Name) == 0)
													sc_pack.Rec.PersonID = psn_rec.ID;
											}
										}
									}
									if(!sc_pack.Rec.PersonID && psn_pack.Rec.Name[0]) {
										temp_id_list.clear();
										temp_id_list.add(owner_kind_id);
										if(PsnObj.SearchFirstByName(psn_pack.Rec.Name, &temp_id_list, 0, &psn_rec) > 0)
											sc_pack.Rec.PersonID = psn_rec.ID;
									}
									if(sc_pack.Rec.PersonID) {
										PPPersonPacket ex_psn_pack;
										THROW(PsnObj.GetPacket(sc_pack.Rec.PersonID, &ex_psn_pack, 0) > 0);
										STRNSCPY(ex_psn_pack.Rec.Name, psn_pack.Rec.Name);
										if(reg_type_id && code_buf.NotEmptyS()) {
											THROW(ex_psn_pack.AddRegister(reg_type_id, code_buf, 1));
										}
										psn_pack.Kinds.addUnique(owner_kind_id);
										THROW(PsnObj.PutPacket(&sc_pack.Rec.PersonID, &ex_psn_pack, 1));
									}
									else {
										if(reg_type_id && code_buf.NotEmptyS()) {
											RegisterTbl::Rec reg_rec;
											MEMSZERO(reg_rec);
											reg_rec.RegTypeID = reg_type_id;
											STRNSCPY(reg_rec.Num, code_buf);
										}
										psn_pack.Kinds.addUnique(owner_kind_id);
										THROW(PsnObj.PutPacket(&sc_pack.Rec.PersonID, &psn_pack, 1));
									}
									p_psn_blk->NativeID = sc_pack.Rec.PersonID;
								}
								else {
									; // @error �� ������� ������� ����������-��������� ����� ��-�� �� �������������� ����
								}
							}
						}
						if(ScObj.SearchCode(0, sc_pack.Rec.Code, &_ex_sc_rec) > 0) {
							pretend_obj_list.add(_ex_sc_rec.ID);
						}
						//
						if(pretend_obj_list.getCount() == 0) {
							PPID   new_sc_id = 0;
							if(ScObj.PutPacket(&new_sc_id, &sc_pack, 1)) {
								r_blk.NativeID = new_sc_id;
							}
							else {
								; // @error
							}
						}
						else if(pretend_obj_list.getCount() == 1) {
							PPID   sc_id = pretend_obj_list.get(0);
							PPSCardPacket ex_sc_pack;
							THROW(ScObj.GetPacket(sc_id, &ex_sc_pack) > 0);
							STRNSCPY(ex_sc_pack.Rec.Code, sc_pack.Rec.Code);
							ex_sc_pack.Rec.PDis = sc_pack.Rec.PDis;
							ex_sc_pack.Rec.PersonID = sc_pack.Rec.PersonID;
							if(ScObj.PutPacket(&sc_id, &ex_sc_pack, 1)) {
								r_blk.NativeID = sc_id;
							}
							else {
								; // @error
							}
						}
						else {
							; // @error ������������� ����� ����� ���� ������������ ����� ��� ����� ����� � ��.
						}
					}
				}
				else {
					; // @error
				}
				PPWaitPercent(i+1, __count, wait_msg_buf);
			}
		}
	}
	{
		//
		// ������ �������� ������
		//
		for(uint i = 0; i < RdB.CSessBlkList.getCount(); i++) {
			CSessionBlock & r_blk = RdB.CSessBlkList.at(i);
			uint   ref_pos = 0;
			if(RdB.SearchRef(obCSession, i, &ref_pos)) {
				for(uint ccidx = 0; ccidx < RdB.CcBlkList.getCount(); ccidx++) {
					CCheckBlock & r_cc_blk = RdB.CcBlkList.at(ccidx);
					if(r_cc_blk.CSessionBlkP == ref_pos) {
						uint   cc_ref_pos = 0;
						if(RdB.SearchRef(obCCheck, i, &cc_ref_pos)) {
							for(uint clidx = 0; clidx < RdB.CclBlkList.getCount(); clidx++) {
								CcLineBlock & r_cl_blk = RdB.CclBlkList.at(clidx);
								if(r_cl_blk.CCheckBlkP == cc_ref_pos) {

								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	RdB.Destroy();
	if(!silent)
		PPWait(0);
    return ok;
}

void PPPosProtocol::SaxStop()
{
	xmlStopParser(RdB.P_SaxCtx);
}

int SLAPI PPPosProtocol::SaxParseFile(const char * pFileName, int preprocess)
{
    int    ok = 1;
    SString msg_buf;
    PPWait(1);
	xmlSAXHandler saxh;
	MEMSZERO(saxh);
	saxh.startDocument = Scb_StartDocument;
	saxh.endDocument = Scb_EndDocument;
	saxh.startElement = Scb_StartElement;
	saxh.endElement = Scb_EndElement;
	saxh.characters = Scb_Characters;

	PPFormatT(PPTXT_POSPROT_PARSING, &msg_buf, pFileName);
	PPWaitMsg(msg_buf);
	xmlFreeParserCtxt(RdB.P_SaxCtx);
	THROW(RdB.P_SaxCtx = xmlCreateURLParserCtxt(pFileName, 0));
	if(RdB.P_SaxCtx->sax != (xmlSAXHandlerPtr)&xmlDefaultSAXHandler)
		SAlloc::F(RdB.P_SaxCtx->sax);
	RdB.P_SaxCtx->sax = &saxh;
	xmlDetectSAX2(RdB.P_SaxCtx);
	RdB.P_SaxCtx->userData = this;
	RdB.Phase = preprocess ? RdB.phPreprocess : RdB.phProcess;
	RdB.SrcFileName = pFileName;
	xmlParseDocument(RdB.P_SaxCtx);
	THROW_LXML(RdB.P_SaxCtx->wellFormed, RdB.P_SaxCtx);
	THROW(RdB.P_SaxCtx->errNo == 0);
	THROW(!(RdB.State & RdB.stError));
	CATCHZOK
	PPWait(0);
    return ok;
}

void SLAPI PPPosProtocol::DestroyReadBlock()
{
    RdB.Destroy();
}

int SLAPI PPPosProtocol::Helper_GetPosNodeInfo_ForInputProcessing(const PPCashNode * pCnRec, TSArray <PosNodeISymbEntry> & rISymbList, TSArray <PosNodeUuidEntry> & rUuidList)
{
	int    ok = -1;
	SString temp_buf;
	(temp_buf = pCnRec->Symb).Strip();
	if(temp_buf.IsDigit()) {
		long   isymb = temp_buf.ToLong();
		if(isymb > 0) {
			PosNodeISymbEntry new_entry;
			new_entry.PosNodeID = pCnRec->ID;
			new_entry.ISymb = isymb;
			rISymbList.insert(&new_entry);
			ok = 1;
		}
	}
	{
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(PPOBJ_CASHNODE, pCnRec->ID, PPTAG_POSNODE_UUID, &tag_item) > 0) {
			S_GUID cn_uuid;
			if(tag_item.GetGuid(&cn_uuid) && !cn_uuid.IsZero()) {
				PosNodeUuidEntry new_entry;
				new_entry.PosNodeID = pCnRec->ID;
				new_entry.Uuid = cn_uuid;
				THROW_SL(rUuidList.insert(&new_entry));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::BackupInputFile(const char * pFileName)
{
	int    ok = 1;
	long   n = 0;
	SString arc_file_name;
	SString src_file_name;
	SString src_file_ext;
	SPathStruc ps;
	ps.Split(pFileName);
	src_file_name = ps.Nam;
	src_file_ext = ps.Ext;
    ps.Nam = "pppp-backup";
    ps.Ext = "zip";
    ps.Merge(arc_file_name);
    {
		SArchive arc;
		SString temp_buf;
		SString to_arc_name;
		THROW_SL(arc.Open(SArchive::tZip, arc_file_name, SFile::mReadWrite));
		{
			const int64 zec = arc.GetEntriesCount();
			int   _found = 0;
			do {
				_found = 0;
				to_arc_name = src_file_name;
				if(n)
					to_arc_name.CatChar('-').CatLongZ(n, 4);
				if(src_file_ext.NotEmpty()) {
					if(src_file_ext.C(0) != '.')
						to_arc_name.Dot();
					to_arc_name.Cat(src_file_ext);
				}
				for(int64 i = 0; !_found && i < zec; i++) {
					arc.GetEntryName(i, temp_buf);
					if(temp_buf.CmpNC(to_arc_name) == 0) {
						n++;
						_found = 1;
					}
				}
			} while(_found);
		}
		THROW_SL(arc.AddEntry(pFileName, to_arc_name, 0));
    }
    CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ProcessInput(PPPosProtocol::ProcessInputBlock & rPib)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const char * p_base_name = "pppp";
	const char * p_done_suffix = "-done";
	SString in_file_name;
	SString temp_buf;
	StringSet to_remove_file_list;
	SPathStruc ps;
	if(!(rPib.Flags & rPib.fSilent))
		PPWait(1);
	{
		SFileEntryPool fep;
		SString in_path;
		SDirEntry de;
		SString done_plus_xml_suffix;
		TSArray <PosNodeUuidEntry> pos_node_uuid_list;
		TSArray <PosNodeISymbEntry> pos_node_isymb_list;
		StringSet ss_paths;

		(done_plus_xml_suffix = p_done_suffix).Dot().Cat("xml");
		{
			PPCashNode cn_rec;
			if(rPib.PosNodeID) {
				THROW(CnObj.Search(rPib.PosNodeID, &cn_rec) > 0);
				THROW(Helper_GetPosNodeInfo_ForInputProcessing(&cn_rec, pos_node_isymb_list, pos_node_uuid_list));
				{
					PPAsyncCashNode acn_pack;
					if(CnObj.GetAsync(rPib.PosNodeID, &acn_pack) > 0) {
						if(acn_pack.ImpFiles.NotEmptyS()) {
							StringSet ss_row_paths(';', acn_pack.ImpFiles);
							for(uint ssrp_pos = 0; ss_row_paths.get(&ssrp_pos, temp_buf);) {
								if(isDir(temp_buf.RmvLastSlash())) {
									ss_paths.add(temp_buf);
								}
							}
						}
					}
				}
			}
			else {
				for(SEnum en = CnObj.Enum(0); en.Next(&cn_rec) > 0;) {
					THROW(Helper_GetPosNodeInfo_ForInputProcessing(&cn_rec, pos_node_isymb_list, pos_node_uuid_list));
				}
			}
			if(!ss_paths.getCount()) {
				THROW(PPGetPath(PPPATH_IN, temp_buf));
				ss_paths.add(temp_buf.Strip().RmvLastSlash());
			}
		}
		for(uint ssp_pos = 0; ss_paths.get(&ssp_pos, in_path);) {
			(temp_buf = in_path).SetLastSlash().Cat(p_base_name).Cat("*.xml");
			for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
				if(de.IsFile()) {
					const size_t fnl = strlen(de.FileName);
					if(fnl <= done_plus_xml_suffix.Len() || !sstreqi_ascii(de.FileName+fnl-done_plus_xml_suffix.Len(), done_plus_xml_suffix)) {
						THROW_SL(fep.Add(in_path, de));
					}
				}
			}
		}
		fep.Sort(SFileEntryPool::scByWrTime/*|SFileEntryPool::scDesc*/);
		{
			S_GUID this_db_uuid;
			SString this_db_symb;
			DbProvider * p_dict = CurDict;
			if(p_dict) {
				p_dict->GetDbUUID(&this_db_uuid);
				p_dict->GetDbSymb(this_db_symb);
			}
			else
				this_db_uuid.SetZero();
			SFileEntryPool::Entry fe;
			for(uint i = 0; i < fep.GetCount(); i++) {
				if(fep.Get(i, fe)) {
					(in_file_name = fe.Path).SetLastSlash().Cat(fe.Name);
					if(fileExists(in_file_name) && SFile::WaitForWriteSharingRelease(in_file_name, 60000)) {
						DestroyReadBlock();
						PPID   cn_id = 0;
						int    pr = SaxParseFile(in_file_name, 1 /* preprocess */);
						THROW(pr);
						if(pr > 0) {
							int    is_my_file = 0;
							RouteBlock root_blk;
							for(uint didx = 0; !is_my_file && didx < RdB.DestBlkList.getCount(); didx++) {
								const RouteObjectBlock & r_dest = RdB.DestBlkList.at(didx);
								RdB.GetRouteItem(r_dest, root_blk);
								//
								// ���� ����������� ������ �� ������ UUID ��� ��������, ��� �� ����� �������
								// ������ ������ � �� ����, ������� ������������� ����� UUID (������������� �� ������� ����������).
								//
								if(!root_blk.Uuid.IsZero()) {
									if(root_blk.Uuid == this_db_uuid) {
										is_my_file = 1;
									}
									else {
										for(uint cnuidx = 0; !is_my_file && cnuidx < pos_node_uuid_list.getCount(); cnuidx++) {
											if(root_blk.Uuid == pos_node_uuid_list.at(cnuidx).Uuid) {
												cn_id = pos_node_uuid_list.at(cnuidx).PosNodeID;
												is_my_file = 2;
											}
										}
									}
								}
								else if(!is_my_file) {
									if(root_blk.Code.IsDigit()) {
										long   isymb = root_blk.Code.ToLong();
										uint   isymb_pos = 0;
										if(isymb > 0 && pos_node_isymb_list.lsearch(&isymb, &isymb_pos, CMPF_LONG)) {
											cn_id = pos_node_isymb_list.at(isymb_pos).PosNodeID;
											is_my_file = 3;
										}
									}
								}
							}
							if(is_my_file) {
								SETIFZ(cn_id, rPib.PosNodeID);
								DestroyReadBlock();
								pr = SaxParseFile(in_file_name, 0 /* !preprocess */);
								THROW(pr);
								if(rPib.Flags & rPib.fStoreReadBlocks) {
									if(!rPib.P_RbList) {
										THROW_MEM(rPib.P_RbList = new TSCollection <ReadBlock>);
									}
									TSCollection <ReadBlock> * p_rb_list = (TSCollection <ReadBlock> *)rPib.P_RbList;
									ReadBlock * p_persistent_rb = p_rb_list->CreateNewItem();
									THROW_SL(p_persistent_rb);
									p_persistent_rb->Copy(RdB);
								}
								{
									for(uint csidx = 0; csidx < RdB.CSessBlkList.getCount(); csidx++) {
										const CSessionBlock & r_cs_blk = RdB.CSessBlkList.at(csidx);
										rPib.SessionCount++;
										rPib.SessionPeriod.AdjustToDate(r_cs_blk.Dtm.d);
									}
								}
								{
									uint   qpb_list_idx = 0;
									{
										RouteBlock src_;
										if(RdB.SrcBlkList.getCount() == 1) {
											RouteObjectBlock & r_blk = RdB.SrcBlkList.at(0);
											RdB.GetRouteItem(r_blk, src_);
											for(uint j = 0; !qpb_list_idx && j < rPib.QpBlkList.getCount(); j++) {
												QueryProcessBlock * p_qpb = rPib.QpBlkList.at(j);
												if(p_qpb->PosNodeID == cn_id && p_qpb->R.IsEqual(src_))
													qpb_list_idx = j+1;
											}
											if(!qpb_list_idx) {
												uint _pos = 0;
												QueryProcessBlock * p_new_qpb = rPib.QpBlkList.CreateNewItem(&_pos);
												THROW_SL(p_new_qpb);
												p_new_qpb->PosNodeID = cn_id;
												p_new_qpb->R = src_;
												qpb_list_idx = _pos+1;
											}
										}
									}
									if(qpb_list_idx) {
										QueryProcessBlock * p_qpb = rPib.QpBlkList.at(qpb_list_idx-1);
										for(uint qidx = 0; qidx < RdB.QueryList.getCount(); qidx++) {
											const QueryBlock & r_qb = RdB.QueryList.at(qidx);
											THROW_SL(p_qpb->QL.insert(&r_qb));
										}
									}
								}
								{
									int    do_backup_file = 0;
									if(rPib.Flags & rPib.fProcessRefs) {
										if(AcceptData(cn_id, BIN(rPib.Flags & rPib.fSilent)))
											do_backup_file = 1;
										else
											PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
									}
									if(rPib.Flags & rPib.fProcessQueries) {
										do_backup_file = 1;
									}
									if(do_backup_file) {
										int    backup_ok = 1;
										if(rPib.Flags & rPib.fBackupProcessed) {
											backup_ok = BackupInputFile(in_file_name);
										}
										if(rPib.Flags & rPib.fRemoveProcessed && backup_ok)
											to_remove_file_list.add(in_file_name);
									}
								}
							}
						}
					}
				}
			}
			if(rPib.Flags & rPib.fProcessQueries) {
				for(uint bidx = 0; bidx < rPib.QpBlkList.getCount(); bidx++) {
					const QueryProcessBlock * p_qpb = rPib.QpBlkList.at(bidx);
					if(p_qpb) {
						PPIDArray csess_list;
						const PPID cn_id = p_qpb->PosNodeID;
						PPID  sync_cn_id = 0;
						PPID  async_cn_id = 0;
						PPCashNode cn_rec;
						CSessionTbl::Rec cs_rec;
						if(CnObj.Search(cn_id, &cn_rec) > 0) {
							if(cn_rec.Flags & CASHF_SYNC) {
								sync_cn_id = cn_id;
							}
							else if(cn_rec.Flags & CASHF_ASYNC) {
								async_cn_id = cn_id;
							}
						}
						for(uint qidx = 0; qidx < p_qpb->QL.getCount(); qidx++) {
							const QueryBlock & r_q = p_qpb->QL.at(qidx);
							/*
								qTest = 1, // �������� ������ ��� �������� ������ �������
        						qCSession, // ������ �������� ������
        						qRefs,     // ������ ������������
							*/
							if(r_q.Q == QueryBlock::qTest) {
							}
							else if(r_q.Q == QueryBlock::qCSession) {
								if(cn_id) {
									if(r_q.Flags & QueryBlock::fCSessLast) {
										if(CsObj.P_Tbl->SearchLast(cn_id, 1010, 0, &cs_rec) > 0) {
											csess_list.add(cs_rec.ID);
										}
									}
									if(r_q.Flags & QueryBlock::fCSessCurrent) {
										if(sync_cn_id) {
											if(cn_rec.CurSessID && CsObj.Search(cn_rec.CurSessID, &cs_rec) > 0) {
												csess_list.add(cs_rec.ID);
											}
										}
										else if(async_cn_id) {
											if(CsObj.P_Tbl->SearchLast(async_cn_id, 10, 0, &cs_rec) > 0) {
												csess_list.add(cs_rec.ID);
											}
										}
									}
									if(r_q.CSess) {
										if(r_q.Flags & QueryBlock::fCSessN) {
											// ������ �� ������
											PPID   cs_id = 0;
											if(CsObj.P_Tbl->SearchByNumber(&cs_id, cn_id, cn_id, r_q.CSess, ZERODATE) > 0) {
												csess_list.add(cs_id);
											}
										}
										else {
											// ������ �� ��������������
											if(CsObj.P_Tbl->Search(r_q.CSess, &cs_rec) > 0) {
												if(cs_rec.CashNodeID == cn_id) {
													csess_list.add(cs_rec.ID);
												}
											}
										}
									}
								}
							}
							else if(r_q.Q == QueryBlock::qRefs) {
							}
						}
						if(csess_list.getCount()) {
							csess_list.sortAndUndup();
							RouteBlock rb_src;
							InitSrcRootInfo(cn_id, rb_src);
							ExportPosSession(csess_list, cn_id, &rb_src, &p_qpb->R);
						}
					}
				}
			}
		}
	}
	if(!(rPib.Flags & rPib.fSilent))
		PPWait(0);
	CATCH
		if(rPib.Flags & rPib.fSilent) {
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
		}
		else
			PPErrorZ();
		ok = 0;
	ENDCATCH
	{
		for(uint ssp = 0; to_remove_file_list.get(&ssp, temp_buf);) {
			SFile::Remove(temp_buf);
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::SelectOutFileName(const PPAsyncCashNode * pPosNode, const char * pInfix, StringSet & rResultSs)
{
	rResultSs.clear(1);

	int    ok = -1;
	const char * p_base_name = "pppp";
	SString temp_buf;
	SString temp_result_buf;
	SString path;
	StringSet ss_paths(";");
	if(pPosNode && pPosNode->ExpPaths.NotEmpty()) {
		ss_paths.setBuf(pPosNode->ExpPaths);
	}
	else {
		THROW(PPGetPath(PPPATH_OUT, temp_buf));
		ss_paths.setBuf(temp_buf);
	}
	for(uint ssp = 0; ss_paths.get(&ssp, path);) {
		path.RmvLastSlash();
		if(isDir(path) || createDir(path)) {
			long   _seq = 0;
			do {
				temp_buf = p_base_name;
				if(!isempty(pInfix))
					temp_buf.CatChar('-').Cat(pInfix);
				if(_seq)
					temp_buf.CatChar('-').Cat(_seq);
				temp_buf.Dot().Cat("xml");
				(temp_result_buf = path).SetLastSlash().Cat(temp_buf);
				_seq++;
			} while(::fileExists(temp_result_buf));
			rResultSs.add(temp_result_buf);
			ok = 1;
		}
		else {
			// @todo log message
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ExportPosSession(const PPIDArray & rSessList, PPID posNodeID, const PPPosProtocol::RouteBlock * pSrc, const PPPosProtocol::RouteBlock * pDestination)
{
	int    ok = -1;
	SString out_file_name;
	SString temp_buf;
	PPPosProtocol::WriteBlock wb;
	PPMakeTempFileName("pppp", "xml", 0, out_file_name);
	{
		THROW(StartWriting(out_file_name, wb));
		{
			if(pSrc) {
				THROW(WriteRouteInfo(wb, "source", *pSrc));
			}
			else {
				PPPosProtocol::RouteBlock rb_src;
				InitSrcRootInfo(0, rb_src);
				THROW(WriteRouteInfo(wb, "source", rb_src));
			}
			//THROW(WriteSourceRoute(wb));
			if(pDestination) {
				THROW(WriteRouteInfo(wb, "destination", *pDestination));
			}
		}
		{
			PPObjCSession cs_obj;
			CSessionTbl::Rec cs_rec;
			for(uint i = 0; i < rSessList.getCount(); i++) {
				const PPID sess_id = rSessList.get(i);
				if(cs_obj.Search(sess_id, &cs_rec) > 0) {
					if(!WriteCSession(wb, "csession", cs_rec)) {
						; // @todo logerror
					}
				}
			}
		}
		FinishWriting(wb);
	}
	{
		int   copy_result = 0;
		StringSet ss_out_files;
		THROW(SelectOutFileName(0, "csess", ss_out_files));
		for(uint ssp = 0; ss_out_files.get(&ssp, temp_buf);) {
			if(SCopyFile(out_file_name, temp_buf, 0, FILE_SHARE_READ, 0))
				copy_result = 1;
		}
		if(copy_result)
			SFile::Remove(out_file_name);
	}
	ok = 1;
	CATCHZOK
	return ok;
}

//static
int SLAPI PPPosProtocol::EditPosQuery(TSArray <PPPosProtocol::QueryBlock> & rQList)
{
	enum {
		qvLastSession = 1,
		qvCurrSession,
		qvSessByPeriod,
		qvSessByNumber,
		qvSessByID,
		qvRefs,
		qvTest
	};
	class PosQueryDialog : public TDialog {
	public:
        SLAPI  PosQueryDialog() : TDialog(DLG_POSNQUERY)
        {
        	SetupCalPeriod(CTLCAL_POSNQUERY_PERIOD, CTL_POSNQUERY_PERIOD);
        }
        int    SLAPI setDTS(const TSArray <PPPosProtocol::QueryBlock> * pData)
        {
        	int    ok = 1;
        	//RVALUEPTR(Data, pData);
        	AddClusterAssocDef(CTL_POSNQUERY_Q, 0, qvLastSession);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 1, qvCurrSession);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 2, qvSessByPeriod);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 3, qvSessByNumber);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 4, qvSessByID);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 5, qvRefs);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 6, qvTest);
        	SetClusterData(CTL_POSNQUERY_Q, qvLastSession);
			SetupCtrls();
        	return ok;
        }
        int    SLAPI getDTS(TSArray <PPPosProtocol::QueryBlock> * pData)
        {
        	int    ok = 1;
			uint   sel = 0;
			long   qv = 0;
			SString input_buf;
			SString temp_buf;
			Data.freeAll();
			{
				getCtrlString(CTL_POSNQUERY_N, input_buf);
				StringSet ss;
				LongArray n_list;
				input_buf.Tokenize(";,", ss);
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					const long n = temp_buf.ToLong();
					if(n > 0)
						n_list.add(n);
				}
				if(n_list.getCount()) {
					n_list.sortAndUndup();
					// @todo
				}
			}
			GetClusterData(sel = CTL_POSNQUERY_Q, &qv);
			switch(qv) {
				case qvLastSession:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qCSession;
						qb.Flags |= qb.fCSessLast;
						Data.insert(&qb);
					}
					break;
				case qvCurrSession:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qCSession;
						qb.Flags |= qb.fCSessCurrent;
						Data.insert(&qb);
					}
					break;
				case qvSessByPeriod:
					{
						PPPosProtocol::QueryBlock qb;
						THROW(GetPeriodInput(this, sel = CTL_POSNQUERY_PERIOD, &qb.Period));
						THROW_PP(qb.Period.low && qb.Period.upp, PPERR_INVPERIOD);
						qb.Q = qb.qCSession;
						Data.insert(&qb);
					}
					break;
				case qvSessByNumber:
				case qvSessByID:
					{
						LongArray n_list;
						getCtrlString(sel = CTL_POSNQUERY_SESSL, input_buf);
						StringSet ss;
						input_buf.Tokenize(";,", ss);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							const long n = temp_buf.ToLong();
							if(n > 0)
								n_list.add(n);
						}
						THROW_PP(n_list.getCount(), PPERR_USERINPUT);
						n_list.sortAndUndup();
						for(uint i = 0; i < n_list.getCount(); i++) {
							PPPosProtocol::QueryBlock qb;
							qb.Q = qb.qCSession;
							qb.CSess = n_list.get(i);
							if(qv == qvSessByNumber)
								qb.Flags |= qb.fCSessN;
							Data.insert(&qb);
						}
					}
					break;
				case qvRefs:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qRefs;
						Data.insert(&qb);
					}
					break;
				case qvTest:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qTest;
						Data.insert(&qb);
					}
					break;
				default:
					CALLEXCEPT_PP(PPERR_USERINPUT);
					break;
			}
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
			if(event.isClusterClk(CTL_POSNQUERY_Q)) {
				SetupCtrls();
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			long   qv = 0;
            GetClusterData(CTL_POSNQUERY_Q, &qv);
			disableCtrl(CTL_POSNQUERY_PERIOD, (qv != qvSessByPeriod));
			disableCtrl(CTL_POSNQUERY_SESSL, !oneof2(qv, qvSessByNumber, qvSessByID));
		}
		TSArray <PPPosProtocol::QueryBlock> Data;
	};
	DIALOG_PROC_BODY(PosQueryDialog, &rQList);
}

int SLAPI RunInputProcessThread(PPID posNodeID)
{
	class PosInputProcessThread : public PPThread {
	public:
		struct InitBlock {
			InitBlock(PPID posNodeID)
			{
				PosNodeID = posNodeID;
				ForcePeriodMs = 0;
			}
			SString DbSymb;
			SString UserName;
			SString Password;
			PPID   PosNodeID;
			uint   ForcePeriodMs; // ������ ������������� �������� ��������� �������� (ms)
		};
		SLAPI PosInputProcessThread(const InitBlock & rB) : PPThread(PPThread::kPpppProcessor, 0, 0), IB(rB)
		{
			InitStartupSignal();
		}
	private:
		void SLAPI Startup()
		{
			PPThread::Startup();
			SignalStartup();
		}
		void   FASTCALL DoProcess(PPPosProtocol & rPppp)
		{
			PPPosProtocol::ProcessInputBlock pib;
			pib.PosNodeID = IB.PosNodeID;
			pib.Flags = (pib.fProcessRefs|pib.fProcessQueries|pib.fBackupProcessed|pib.fRemoveProcessed|pib.fSilent);
			rPppp.ProcessInput(pib);
		}
		virtual void Run()
		{
			DirChangeNotification * p_dcn = 0;
			SString msg_buf, temp_buf;
			STimer timer;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			THROW(DS.Login(IB.DbSymb, IB.UserName, IB.Password));
			IB.Password.Obfuscate();
			{
        		//
        		// Login has been done
        		//
        		SString in_path;
        		PPObjCashNode cn_obj;
				PPSyncCashNode cn_pack;
				THROW(cn_obj.GetSync(IB.PosNodeID, &cn_pack) > 0);
				PPGetPath(PPPATH_IN, in_path);
				in_path.RmvLastSlash();
				if(!isDir(in_path)) {

				}
				else {
					PPPosProtocol pppp;
					{
						//
						// �������������� ��������� �������� ������
						//
						DoProcess(pppp);
					}
					DirChangeNotification * p_dcn = new DirChangeNotification(in_path, 0, FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME);
					THROW(p_dcn);
					for(int stop = 0; !stop;) {
						uint   h_count = 0;
						HANDLE h_list[32];
						h_list[h_count++] = stop_event;
						h_list[h_count++] = *p_dcn;
						if(IB.ForcePeriodMs) {
							LDATETIME dtm = getcurdatetime_();
							dtm.addsec(IB.ForcePeriodMs / 1000);
							timer.Set(dtm, 0);
							h_list[h_count++] = timer;
						}
						uint   r = WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
						if(r == WAIT_OBJECT_0 + 0) { // stop event
							stop = 1; // quit loop
						}
						else if(r == WAIT_OBJECT_0 + 1) { // file created
							DoProcess(pppp);
							p_dcn->Next();
						}
						else if(r == WAIT_OBJECT_0 + 2) { // timer
							DoProcess(pppp);
						}
						else if(r == WAIT_FAILED) {
							// error
						}
					}
				}
			}
			CATCH
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_LASTERR);
			ENDCATCH
			delete p_dcn;
			DS.Logout();
		}
		InitBlock IB;
	};
	int    ok = -1;

	TSCollection <PPThread::Info> thread_info_list;
	DS.GetThreadInfoList(PPThread::kPpppProcessor, thread_info_list);
	if(!thread_info_list.getCount()) {
		Reference * p_ref = PPRef;
		PPID   user_id = 0;
		PPSecur usr_rec;
		char    pw[128];
		SString db_symb;
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		THROW(cn_obj.Search(posNodeID, &cn_rec) > 0);
		{
			DbProvider * p_dict = CurDict;
			CALLPTRMEMB(p_dict, GetDbSymb(db_symb));
		}
		{
			ObjTagItem tag_item;
			S_GUID host_uuid;
			if(p_ref->Ot.GetTag(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_HOSTUUID, &tag_item) > 0 && tag_item.GetGuid(&host_uuid) > 0) {
				const PPThreadLocalArea & r_tla = DS.GetConstTLA();
				PosInputProcessThread::InitBlock ib(posNodeID);
				ib.DbSymb = db_symb;
				ib.UserName = r_tla.UserName;
				THROW(p_ref->SearchName(PPOBJ_USR, &user_id, ib.UserName, &usr_rec) > 0);
				Reference::GetPassword(&usr_rec, pw, sizeof(pw));
				ib.Password = pw;
				memzero(pw, sizeof(pw));
				{
					PosInputProcessThread * p_sess = new PosInputProcessThread(ib);
					p_sess->Start(1);
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
