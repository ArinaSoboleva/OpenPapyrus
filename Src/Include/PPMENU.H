// PPMENU.H
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
// @codepage windows-1251
// ��������� ������ ���� ������� Papyrus
//
#ifndef __PPMENU_H
#define __PPMENU_H

#ifndef cmQuit
	#define cmQuit                 1
#endif
#define cmCountry                      101 // �����������
#define cmAnotherPerson                106 // Obsolete
#define cmPerson                       106 //
#define cmNewConfig                    107 //
#define cmEditConfig                   108 // ������� ������������ //
#define cmGroups_n_Users               109 // ������ � ������������
#define cmUnit                         110 // ������� ��������� //
#define cmCityStatus                   111 // ������� ���������� �������
#define cmPersonKind                   112 // ���� ����������
#define cmPersonStatus                 113 // ����������� ������� ����������
#define cmBnkAccType                   114 // ���� ���������� ������
#define cmAccSheet                     115 // ������� �������������� �����
#define cmAccArticle                   116 // ������ �������������� �����
#define cmAccPlan                      117 // ���� ������
#define cmGoodsGroup                   118 // ������ �������
#define cmGoods                        119 // ������
#define cmLocation                     120 // ������
#define cmAccTurnList                  121 // ����� ���. ��������
#define cmAccTurn                      122 // ����� ���. ��������
#define cmAccAnalyze                   123 // ������ �����
#define cmBalance                      124 // ������� (��������� ���������)
#define cmAmountType                   125 // ���� ���� ����������
#define cmOprType                      126 // ���� ��������
#define cmOprKind                      127 // ���� ��������
#define cmScale                        128 // ����
#define cmRegisterType                 129 // ��� ��������������� ����������
#define cmELinkKind                    130 // ���� ����������� �������
#define cmQuotKind                     131 // ���� ��������� �������
#define cmObjTag                       132 // ���� ��������
#define cmSelectLocation               135 // ����� �������
#define cmGoodsBillList                136 // �������� �������� ����������
#define cmViewLots                     137 // �������� �����
#define cmViewStatus                   138 // �������� �������� �������
#define cmViewGoodsCard                139 // �������� ������ (Obsolete from @v2.5.2 instead use cmTrfrAnalyze)
#define cmGoodsTurnover                140 // ����� � �������������
#define cmGoodsRest                    142 // ������� �������
#define cmCorrectTrfrRvrs              143 // ������������� ���������� ��������
#define cmConvert1                     144 //
#define cmConvert2                     145 //
#define cmConvert3                     146 //
#define cmCorrectBillPaym              147 // ������������� ���� ����� �� ���������
#define cmSupplTrnovr                  148 // ������� �� �����������
#define cmSellTrnovr                   149 // ������� �� �����������
#define cmGoodsGrpRlz                  150 // (obsolete since @v2.9.11) ���������� �� ������� �������
#define cmGoodsTaxAnalyze              150 // ��������� ������ �������������
#define cmGoodsMov                     151 // �������� �������
#define cmGoodsOprGrouping             152 // ����������� �������� ��������
#define cmCorrectAccTurnRest           153 // ������������� �������� ���. ��������
#define cmCorrectLot                   154 // ������������� �����
#define cmCashNode                     157 // PPOBJ_CASHNODE
#define cmRecalcBillTurns              158 // �������� �������� �� ����������
#define cmGoodsOper                    159 // �������� ��������� �� ��������
#define cmGoodsOrder                   160 // ������
#define cmAction                       161 // PPOBJ_ACTION
#define cmViewSysJ                     162 // �������� ���������� �������
#define cmCalc                         164 // �����������
#define cmUnifyGoodsPrice              165 // ���������� ��� �������
#define cmPriceList                    166 // �����-����
#define cmCommConfig                   167 // ����� ���������
#define cmViewQCert                    168 // �������� ������������
#define cmViewCntGrpList               169 // ����������� �������� �� ��������
#define cmCashBook                     170 // �������� �����
#if 0 // {
#define cmExportGoodsRest       171 // ������� �������� �������� �� �����-������
#define cmOpenCashDay           172 // ������� �������� ������
#define cmCloseCashDay          156 // ������� �������� ������
#define cmCashNodePanel         173 // ������ �������� � �������� �����
#define cmOperateCashDay        174 // �������� � �������� ������
	#define cmCashBillList          155 // ��������� �� �������� ����
	#define cmUpdateCashDay         175 // ���������� �������� ������
#define cmACSViewExcess         180
#endif // 0 }
#define cmCashNodePanel                171
//
// ������� �������� ������
// (CS - CashSession, ACS - Async Cash Session, SCS - Sync Cash Session)
//
#define cmCSOpen                       172
#define cmCSClose                      173
#define cmACSUpdate                    174
#define cmCSViewCheckList              175
#define cmACSViewExcess                176 // @obsolete use cmViewCSessDeficit
#define cmViewCSessDeficit             176 // == cmACSViewExcess
#define cmSCSLock                      177
#define cmSCSUnlock                    178
#define cmSCSXReport                   179 // X-����� (SyncCashSess)
#define cmACSGetChecks                 180 // �������� ���� (Async Cash Session)
#define cmCashBillList                 175 // Obsolete: use cmCSViewCheckList
#define cmCloseCashDay                 173 // Obsolete: use cmCSClose
#define cmBarCodeStruc                 181 // ��������� ���������� �����-�����
#define cmCorrectBalance               182 // ������������� ��������
#define cmCorrectCurRest               183 // ������������� ������� CurRest
#define cmRemoveEmptyAcctRels          184 // �������� �������������� AcctRel
#define cmDBDiv                        185 // ���������� �������� ��
#define cmSelfSync                     186 // ������������� �� ��� ��������� �� ��� �������
#define cmSendBills                    187 // ������� ����� ��� ������� ����������
#define cmReceivePackets               188 // ������� �������� ������
#define cmReplacePerson                189 // ���������� ����������
#define cmReplaceGoods                 190 // ���������� ������
#define cmGoodsCfg                     191 // ������������ ����������� �������
#define cmCorrectLotsCloseTags         192 // ������������� ��������� �������� �����
#define cmVATBook                      193 // ����� ������/�������
#define cmVATBookSellCfg               194 // ������������ ���� ������
#define cmVATBookBuyCfg                195 // ������������ ���� �������
#define cmExpMonarchCfg                196 // ������������ �������� � ���������� Monarch
#define cmCorrectLotSuppl              197 // ������������� ����������� �� �����
#define cmCreateReservedObjs           198 // �������� ����������������� ��������
#define cmGoodsType                    199 // ���� �������
#define cmConvert197                   200 // ����������� ���� ������� � ����� @v1.9.7
#define cmPrinterCfg                   202 // ������������ �������� �� ���������
#define cmTransmitQCert                220 // ��������� ������� @v1.9.10
#define cmCorrectZeroQCertRefs         221 // ��������� ������� ������ �� ����������� � ReceiptTbl
#define cmResetOpCounters              222 // ����� ��������� ���� ��������
#define cmCheckBillAmounts             223 // �������� ���� �� ����������
#define cmFormula                      224 // �������
#define cmConvert200                   225 // ����������� @v2.0.0
#define cmDefaultCliAgt                226 // ���������� � ��������� �� ���������
#define cmAccturnBillList              227 // �������� ������������� ����������
#define cmCheckPane                    228 // ������ ����� ������� ��������
#define cmViewCCheck                   229 // ������� �����
#define cmConvert2110                  230 // ����������� @v2.1.10
#define cmScalePrepareData             231 // ���������� ������ ��� �����
#define cmScaleTransmitData            232 // �������� ������ �� ����
#define cmConvert221                   233 // ����������� @v2.2.1 (BarcodeTbl)
#define cmConvert224                   234 // ����������� @v2.2.4 (Location, Person)
#define cmInventory                    235 // �������������
#define cmConvert229                   236 // ����������� @v2.2.9 (Receipt - Expiry)
#define cmUpdateQuots                  237 // �������� ��������� ���������
#define cmPersonOpKind                 238 // ���� ������������ �������� @v2.3.4
#define cmAbout                        239 // � ���������...
#define cmPersonEvent                  240 // ������������ ������� //
#define cmDebtSupplTrnovr              244 // ����� �� �����������
#define cmDBXchgCfg                    245 // ������������ ������ ������� ����� ���������
#define cmRecover_v242                 246 // ������������� �������� @v2.4.2
#define cmDivision                     247 // ������������
#define cmAutoCharge                   248 // �������������� ���������� �����
#define cmTrfrAnalyze                  249 // ������ �������� ��������
#define cmStaffList                    250 // ������� ����������
#define cmConvert253                   251 // ����������� @v2.5.3 (VATBook - 2 indexes added)
#define cmStaff                        252 // �����
#define cmNamedGoodsStruc              253 // ����������� ��������� �������
#define cmMakeCRptFiles                254 //
#define cmViewOrdLots                  255 //
#define cmConvert270                   256 //
#define cmGoodsTax                     257 // �������� ��������� ������
#define cmCorrectAbsenceLots           258 // �������������� ������������� (��-�� ����) �����
#define cmBarcodePrinter               259 // �������� ����������
#define cmShipmAnalyze                 260 // ������ ��������
#define cmCurrency                     261 // ������
#define cmCurRateType                  262 // ���� �������� ������
#define cmViewCurRates                 263 // �������� �����
#define cmConvert300                   265 // @v3.0.0 ����������� //
#define cmUpdatePassword               266 // ��������� ������������ ������ //
#define cmPoolBillList                 267 // ���� ����������
#define cmCorrectAccrelArRefs          268 // ������������� ��������������
	// ����� �������� ������������� ������ � �������� ���� �� ������ � ������� AcctRel. ��� �������� ����������
	// ��� ��� ������, ������������������� �������� ���� 2.9.11. � ���� ������� ������������� ���������
	// �������������� ������� ������������ ��� ������ ��������� ������ ������������� ������.
	//
#define cmTransmitGoods                269 // �������� ������� (� �����������)
#define cmShowTree                     270 // ���������� ����
#define cmOpCounter                    271 // �������� ��������
#define cmConvert301                   272 // ����������� //
#define cmReplaceGoodsTaxGrp           273 // ����������� ��������� �����
#define cmDatabaseCutting              274 // �������� ���� ������
#define cmCurReval                     275 // �������� ����������
#define cmSaveBalance                  276 // ��������� ������
#define cmRestoreBalance               277 // ������������ ������
#define cmShowToolbar                  278 // ���������
#define cmViewCSess                    279 // �������� ������
#define cmConvert329                   280 // ����������� (sj.btr)
#define cmViewObjSync                  281 // �������� ������� ������������� ��������
#define cmTransmitModifications        282 // �������� ���������
#define cmGoodsClass                   283 // ���������� ������ �������
#define cmCorrectLotTaxGrp             284 // ������������� ��������� ����� � ����������� �����
#define cmWoodBreed                    285 // ������ ��������� (���������� ����������)
#define cmPckgType                     286 // ���� �������� �������
#define cmTransportCar                 287 // �������������
#define cmTransportShip                288 // �������� ����
#define cmCorrectPckgCloseTags         291 // ������������� ��������� �������� �������
#define cmGoodsBasket                  292 // ������� �������
#define cmPriceListCfg                 293 // ������������ �����-�����
#define cmImportGoodsGrp               301 // ������ ����������� ����� �������
#define cmImportGoods                  302 // ������ ����������� �������
#define cmImportPerson                 303 // ������ ����������� ���������� (������ ������)
#define cmImportQuot                   304 // ������ ��������� (������ ������)
#define cmChainDatabase                305 // ������ ���� ������
#define cmConvert3512                  306 // ����������� (Register)
#define cmBillCfg                      307 // ������������ ����������
#define cmDraftBillList                308 // �����-���������
#define cmDeleteTmpFiles               309 // �������� ��������� ������
#define cmMainOrg2                     310 // ����� "������� �����������"
#define cmCreateNewFiles               311 // �������� ���� ������
#define cmBht                          312 // ��������� ����� ������ (BHT)
#define cmBhtConfig                    313 // ������������ ������ ������� � BHT
#define cmBhtTransmit                  314 // �������� ������ � BHT
#define cmBhtReceive                   315 // ����� ������ �� BHT
#define cmBhtTransmitProgram           316 // @TEMP �������� ��������� �� BHT
#define cmUpdateLicense                318 // ���������� �������� �� ������������� Papyrus'a
#define cmRegistration                 319 // ����������
#define cmSCardSeries                  320 // ����� ����������� ����
#define cmSCard                        321 // ����������� �����
#define cmConvert372                   322 // ����������� �������� �����
#define cmStyloPalmExport              323 // �������� ������ ��� �������� � StyloPalm
#define cmStyloPalmImpExp              323 // ������/������� ��� StyloPalm
#define cmStyloPalm                    324 // Palm
#define cmDraftWrOff                   325 // ���� �������� �����-����������
#define cmWriteOffDrafts               326 // �������� �����-���������� �� ����
#define cmConvert380                   327 // ����������� �����-�����
#define cmAssetWrOffGrp                328 // ������ �������� �������
#define cmRevalAssets                  329 // �������� �������
#define cmPredictSales                 330 // ������� �������
#define cmFillPredictSalesTbl          331 // ���������� ������� ������
#define cmPredictCfg                   332 // ������������ �������� ������
#define cmGetPrognosis                 333 // ���������� �������� ������ �� ���������� �������
#define cmPaymPlan                     334 // ���� ��������
#define cmAdvBillKind                  335 // ���� �������� ����������
#define cmActiveUsersList              336 // ������ �������� �� ������ ������ ������������
#define cmImportOrders                 337 // ����� ������� Albatros // @obsolete
#define cmAlbatrosCfg                  338 // ������������ ������� ���������
#define cmProcessDL200                 339 // ��������� �������� ������ DL200
#define cmAccounts                     340 // ������������� ����� (����� �� �������)
#define cmAccOutbal                    341 // ������ ������������ ������
#define cmAccRegister                  342 // ������ ������������� ���������
#define cmAccAggr                      343 // ������ ������������ ������
#define cmConvert400                   344 // ����������� ������� Account
#define cmCorrectAccturn               345 // �������� � �������������� ������������� ��������
#define cmReceiveCharry                346 // ����� �������� �� ���������� Charry
#define cmConvert31102                 347 // ����������� ������ VATBook, ObjTag (�������� ������ �������)
#define cmCorrectAbsenceGoods          348 // �������������� ������������� (��-�� ����) �������
#define cmAnalyzeObjSyncCmp            349 // ������ ������������� ��������
#define cmConvert31110                 350 // ����������� ������� CCheck (�������� ������ �������)
#define cmRecoverAbsenceBills          351 // �������������� ���������� ����� ����������
#define cmViewAsset                    352 // ����� �� �������� ���������
#define cmSStat                        353 // ���������� �� ��������
#define cmCalcGoodsSaldo               354 // ������ ������ �� �������
#define cmImportCurrList               356 // �������� ������ ����� �� ���������
#define cmInetConnCfg                  357 // ������������ ���������� � ����������
#define cmUserInterfaceCfg             358 // ��������� ����������
#define cmFreight                      359 // ���������� � ������ �� ����������
#define cmProject                      360 // �������
#define cmViewQuots                    361 // ��������� �������
#define cmSimpleLedgerCfg              362 // ��������� ����� �������/��������
#define cmMailAccount                  363 // ������� ������ ����������� �����
#define cmAddressBook                  364 // �������� �����
#define cmMailCharryFiles              365 // �������� ������ charry �� ����������� �����
#define cmImportBanks                  366 // ������ ������
#define cmConvert4402                  367 // ����������� ������� VATBook
#define cmConvert4405                  369 // ����������� ������ Goods2, PriceLine, QualityCert
#define cmBrand                        370 // �������� �����
#define cmAsset                        371 // ���������� �������� �������
#define cmFastGrp_n_Users              372 // ������� �������������� ����� � ������������� (��. ������)
#define cmEditHolidays                 373 // �������������� ������ �������� ��� ��������������� //
#define cmPersonRelType                374 // ���� ��������� ����� ������������
#define cmImportSpecSeries             375 // ������ ����������� �����
#define cmRemoveBadReckons             376 // �������� ������� � ������������ ������������ �����
#define cmEquipCfg                     377 // ������������ �����������
#define cmCliBnkFmtCfg			       378 // ��������� �������� �������/�������� � ������-����
#define cmCliBnkAsCfg                  379 // ��������� ���������� ������� �� ������-�����
#define cmCliBnkImport                 380 // ������ ���������� ������� �� ������-�����
#define cmSStatSupplOrder              381 // ���������� ������ � ������� ��� ������ ����������
#define cmPersonCategory               382 // ��������� ����������
#define cmAccAlias                     383 // ������ ������
#define cmMrpTab                       384 // MRP-�������
#define cmPrjTask                      385 // ������
#define cmStyloPalmCfg                 386 // ������������ ������ �� StyloPalm
#define cmProcessor                    387 // ����������
#define cmTech                         388 // ����������
#define cmProjectCfg                   389 // ������������ ��������
#define cmTSession                     390 // ��������������� ������
#define cmConvert4707                  391 // ����������� ������� Receipt
#define cmTSessAnlz                    392 // ������ ��������������� ������
#define cmGoods2LocAssoc               393 // ������ ���������� �����-�����
#define cmDraftCreateRules             394 // ������� �������� �������
#define cmImportCountry                395 // ������ �����������
#define cmImportSCard                  396 // ������ ���������� ���� (������ ������� �� v7.3.12)
#define cmDefaultSupplAgt              397 // ���������� � ������������ �� ���������
#define cmImportSuppl                  398 // ������ �����������
#define cmTSessWrOffOrder              399 // ������� �������� ��������������� ������
#define cmCorrectGoodsGroups           400 // ������������� �������� �����
#define cmViewDLStat                   401 // ������ �������� �����������
#define cmConvert4805                  402 // ����������� ������� DlsObj
#define cmTSessionCfg                  403 // ������������ ��������������� ������
#define cmSetSCardsByRule              404 // ��������� ������ �� ����� �� ��������
#define cmGoods2SupplAssoc             405 // ���������� ����� (�������� ������) - ���������
#define cmGoodsRestTest                406 // ���� �� �������� �������
#define cmMaintainPrjTask              407 // ������������ �����
#define cmPriceAnlz                    408 // ������������� ������ ���
#define cmDynamicObjs                  409 // ������ ������������ ��������
#define cmJobPool                      410 // ��������� ������
#define cmCheckGStrucRecursion         411 // �������� ��������������� �������� ��������
#define cmBillStatus                   412 // ������� ����������
#define cmDBMaintain                   413 // ������������ ���� ������
#define cmPrcPane                      414 // ������ ����� ��������������� ������ �� ����������
#define cmManufPlan                    415 // ���������������� �����
#define cmFTPAcct                      416 // FTP ��������
#define cmReport                       417 // ��������� �������
#define cmPrcBusy                      418 // @v5.0.5  ��������� �����������
#define cmPaymWOLinkBill               419 // @v5.0.10 ����� ����� ��� ���������� ���������
#define cmRemoveZeroLinkCCLines        420 // @v5.1.0  �������� ������� ����� �����
#define cmTouchScreen                  421 // @v5.1.2  TouchScreen
#define cmEditDesktops                 422 // @v5.1.x  �������������� ������� ������
#define cmSelDesktop                   423 // @v5.1.x  ����� �������� �����
#define cmEditMenus                    424 // @v5.1.x  �������������� ����
#define cmEditMenusFromFile            425 // @v5.1.x  �������������� ���� �� �����
#define cmDateTimeRep                  426 // @v5.2.0  �������������
#define cmDutySched                    427 // @v5.2.0  ������ ��������
#define cmLocPrinter                   428 // @v5.2.0  ��������� ��������
#define cmCfgImpSpecSeries             429 // @v5.2.1  ������������ ������� ����������� �����
#define cmEditWorld                    430 // @v5.2.6  �������������� �������������� ��������
#define cmBillExpImpParams             431 // @v5.2.7  �������������� �������� ��������/������� ����������
#define cmBRowExpImpParams             432 // @v5.2.7  �������������� �������� ��������/������� ����� ����������
#define cmImportBills                  433 // @v5.2.7  ������ ����������
#define cmViewGoodsInfo                434 // @v5.2.9  �������������� �����
#define cmEditGoodsInfo                435 // @v5.2.9  �������������� �������������� �������
#define cmAddPerson                    436 // @v5.2.9  �������� ����������
#define cmAddTask                      437 // @v5.2.9  �������� ������
#define cmPrjTask_ByStatus             438 // @v5.2.9  �������� ��������������� ������ (���������� �� �������� ������ ���� �� ���� �������)
#define cmLoadFonts                    439 // @v5.2.12 �������� ������� �� ������� ����������
#define cmCashSessExpImpParams         440 // @v5.3.0  �������������� �������� ��������/������� �������� ������
#define cmGenerateGoodsBills           441	// @v5.3.1  ��������� �������� ����������
#define cmPersonRel                    442 // @v5.3.2  �������������� ������������ ���������
#define cmViewLogs                     443 // @v5.3.3  �������� �����
#define cmMailAcct                     444 // @v5.3.4  �������������� �������� ��������
#define cmImportEmailAccts             445 // @v5.3.4  ������ �������� ��������
#define cmKeyboardWKeyCfg              446 // @v5.3.5  ������������ ���������� � ������
#define cmViewGoodsStruc               447 // @v5.3.6  �������� ���������
#define cmCorrectTransfer              448 // @v5.3.7  �������������� ������� transfer
#define cmSalCharge                    449 // @v5.3.8  ���������� ��������
// @v5.5.1 #define cmSalChargeGrp      450 // @v5.3.8  ������ ���������� ��������
#define cmStaffCalendar                451 // @v5.3.9  ���������
#define cmSCSIncasso                   452 // @v5.3.11 ���������� (SyncCashSess)
#define cmSupplGoodsImport             453 // @v5.4.0  ������ ������� �� ����������
#define cmSupplInterchange             454 // @v5.4.0  ������� ������ ��� ���������� // @v9.2.1 cmExportDataForSuppl-->cmSupplInterchange
#define cmExportDataForSuppl           cmSupplInterchange // @obsolete
#define cmSCardCfg                     455 // @v5.4.0  ������������ ���������� ����
#define cmRecoverAbsenceAccounts       456 // @v5.4.5 �������������� ������������� ������
#define cmAddPersonEvent               457 // @v5.4.7  �������� ������������ �������
#define cmRecoverAbsencePrjTaskPerson  458 // @v5.4.9  �������������� ������������� ���������� � �������
#define cmPersonCfg                    459 // @v5.4.10 ������������ ����������
#define cmReplaceUnit                  460 // @v5.4.11 ����������� ������ ��������
#define cmReplaceWorld                 461 // @v5.4.11 ����������� �������������� ��������
#define cmRegisterComServer            462 // @v5.4.11 ����������� COM-�������
#define cmUnregisterComServer          463 // @v5.4.11 ������������� COM-�������
#define cmDBMaintainCfg                464 // @v5.4.14 ������������ ������������ ���� ������
#define cmViewSalary                   465 // @v5.5.1  �������� ���������� ��������
#define cmChargeSalary                 466 // @v5.5.1  ���������� ��������
#define cmGoods2CashNodeAssoc          467 // @v5.5.6  ���������� ����� (�������� ������) - �������� ����
#define cmViewStaffCal                 468 // @v5.5.7  ������ ������� ����������
#define cmImportKLADR                  469 // @v5.5.12 ������ �������������� ������� (�����)
#define cmGoods2WareplaceAssoc         470 // @v5.6.3  ���������� ����� (�������� ������) - ����� �������� //
#define cmViewObjSyncQueue             471 // @v5.6.8  �������� ������� ������������� ��������
#define cmPriceListExpImpParams        472 // @v5.6.8  ������������ �������� ��������/������� �����-������
#define cmCreateObjLikeness            473 // @v5.6.14 �������� ������� ��������� ��������
#define cmObjLikeness                  474 // @v5.6.14 ��������� ��������
#define cmNamedObjAssoc                475 // @v5.7.3  ����������� ���������� �������
#define cmAddBill                      476 // @v5.7.5  �������� ��������
#define cmExpImpParamsInventory        477 // @v5.8.4  ������������ ������� ��������������
#define cmImportInventory              478 // @v5.8.4  ������ ��������������
#define cmEditPaintCloths              479 // @v5.8.x  �������������� �������
#define cmEditWhatman                  479 // @v6.6.9
#define cmViewLogsMon		           480 // @v5.8.4  ������� ��������
#define cmDebtRate                     481 // @v5.9.9  ���������� ���������
#define cmBizScore                     482 // @v5.9.10 ������-����������
#define cmTransportCfg                 483 // @v5.9.10 ������������ ����������
#define cmBizScoreCalc                 484 // @v5.9.10 ������ ������-�����������
#define cmBizScoreVal                  485 // @v5.9.10 �������� ������-�����������
#define cmShowBizScoreOnDesktop        486 // @v5.9.11 �������� ������-���������� �� ������� �����
#define cmGlobalUserAcc                487 // @v5.9.12 ���������� ������� ������
#define cmCreateGlobalUserAcc          488 // @v5.9.14 ����������� ������������ � ���������� ���� ������
#define cmDebtorStat                   489 // @v6.0.0  ���������� �� ���������
#define cmDebtorStatCfg                490 // @v6.0.1  ������������ ���������� ���������
#define cmUpdatePpIni                  491 // @v6.0.2  ��������� ���������� ����� bin\pp.ini ����������� �������������
									       // ����� pp_upd.ini
#define cmGotoSite                     492 // @v6.0.7  ������� �� ���� ����� ���������
#define cmGenerateCliBnkImport         493 // @v6.0.9 ��������� �������� ��� ������� �� ������-����
#define cmLocTransf                    494 // @v6.1.0 ��������� ��������
#define cmLocationCfg                  495 // @v6.1.0 ������������ �������
#define cmWmsBillList                  496 // @v6.1.1 ��������� ��������� ��������
#define cmViewBBoard                   497 // @v6.1.x ���������� � ���������� ������� �� Job �������
#define cmSumByDivision                498 // @v6.1.x ������� �������������� ����� ���� � ���������� � �������� � ������� ��������������.
#define cmDivisionBySum                499 // @v6.1.x ������� �������� ������� ������������� � �������� � ����� ���� � ����������.
#define cmLogin                        500 // @v6.1.10 ���� � �������
#define cmDBMaintenance                501 // @v6.1.10 ������������ ���� ������
#define cmDBExit                       502 // @v6.1.10 ����� �� �������
#define cmGoodsExpImpParams            503 // @v6.1.12 ������������ �������/�������� �������
#define cmImportGoodsOld               504 // @v6.1.12 ������ ������� (�� ���������� ����������)
#define cmImportTodoFromVCal           505 // @v6.2.x  ������ ����� �� vCalendar/iCalendar
#define cmVerifyAllCRpt                506 // @v6.2.8  ����������� ���� �������
//#define cmEditImpExpCfgs               507 // @v6.3.0  ������������ �������/��������
#define cmImpExpListConfig             507 // @v6.3.0  ��������� ������������ �������/��������
#define cmTransport                    508 // @v6.3.x  ������������ �������� (PPView)
#define cmLotTag                       509 // @v6.3.x  ���� �����
#define cmPrivateBasket                510 // @v6.4.0  �������� ��������� �������
#define cmReplaceLikeWorld             511 // @v6.3.x  ����������� ������� �������������� ��������
#define cmSendBillsByFilt              512 // @v6.4.2  �������� ��������� � �����������
#define cmSearchDlvrAddr               514 // @v6.4.8  ����� ������ �������� �� ���� ��� ��.
#define cmGoodsValRestr                515 // @v6.4.10 ����������� �������� ������� PPOBJ_GOODSVALRESTR
#define cmPrjTask_ByReminder           516 // @v6.4.15 �������� ������������ ������ �� ������������ ���������� ���� (����������� � ���������) (���������� �� �������� ������ ���� �� ���� �������)
#define cmCreateTechSess               517 // @v6.5.1  �������� ��������������� ������
#define cmConstructionTest             518 // @v6.5.2  ����������� ������� ��� ������� ��������������� �������
#define cmSCardPane                    519 // @v6.5.4  ������ ���������� � ���������� ������
#define cmCreateDraftBySupplOrder      520 // @v6.5.6  �������� ����� ���������� �� ������ ������ ����������
#define cmAccBudget                    521 // @v6.5.14 ��������� �����
#define cmBudgeting                    522 // @v6.5.14 ��������������
#define cmViewBizScTempl               523 // @v6.6.8  ������� ������-�����������
#define cmBizScValByTempl              524 // @v6.6.8  �������� ������-����������� �� �������
#define cmRecalcLotCurRest             525 // @v6.6.9  �������� �������� �� �����
#define cmViewCheckOpJ                 526 // @v6.6.10 ������ ������� ��������
#define cmPallet                       527 // @v6.7.0  ���������� ������
#define cmCTblOrdCreate                528 // @v6.7.7  �������� ������ ����� � ���������
#define cmReplaceDynamicObj            529 // @v6.7.12 ����������� ������������ ��������
#define cmBillTag                      530 // @v6.8.4  ���� ����������
#define cmCTblOrdTimeGraph             531 // @v6.8.5  ��������� ��������� ������� ������
#define cmSearchBillByCtx              532 // @v6.8.7  ����� ��������� �� ���������
#define cmSetupPreferredBarcodeTags    533 // @v6.8.8  ������������� ������������� �������� ���������������� ����������
#define cmStockOpt                     534 // @v6.8.11
#define cmStockOptCfg                  535 // @v6.8.11
#define cmRFIDDevice                   536 // @v6.8.x  RFID-����������
#define cmReplacePersonCat             537 // @v6.9.0  ����������� ��������� ����������
#define cmViewDialog                   538 // @v6.9.9  �������
#define cmDebtDim                      539 // @v6.x.x
#define cmGoodsTag                     540 // @v7.0.x  ���� �������
#define cmFiltPool                     541 // @paul pentaho �������
#define cmConvertQuot720               542 // @v7.1.12 ����������� ���������
#define cmEventToken                   543 // @v7.2.1 ���������� �����
#define cmGlobalUserAccTag             544 // @v7.2.2 ���� ���������� ������� �������
#define cmReplaceBrand                 545 // @v7.2.4 ����������� �������
#define cmSmsAccount                   546 // ������� ������ ���-��������
#define cmSpecSeries                   547 // @v7.3.0 ����������� �����
#define cmPhoneService                 548 // @v7.3.1 ���������� ���������� ��������
#define cmImportPhoneList              549 // @v7.3.3 ������ ����������� �����������
#define cmEditSmsAcc                   550 // @v7.3.3
#define cmIndexPhones                  551 // @v7.3.3 ���������� ��������� �� ���������� � �� �������.
#define cmCvtQCert                     552 // @v7.x.x ����������� ������������ �������� � ���� ����� (��� �����)
#define cmGtaJournal                   553 // @v7.3.9 �������� ������� �������������� �������� ���������� ���������
#define cmSCardOp                      554 // @v7.3.9 �������� �������� �� ���������� ������
#define cmImportSCard2                 555 // ������ ���������� ����
#define cmGlobalAccCfg                 556 // @v7.4.7 ������������ ���������� ������� �������
#define cmCorrectTrfrIntrUnite         557 // @v7.4.10 ������������� ��������� ����������� ���������� ���������� ��������
#define cmTestPredictSalesTbl          558 // @v7.5.1 ������������ ������� ������
#define cmUhttStore                    559 // @v7.6.1 �������� �������� Universe-HTT
#define cmUhttStoreTag                 560 // @v7.6.1 ���� �������� ��������� Universe-HTT
#define cmConvertBillUUID7601          561 // @v7.6.1 ����������� ��������� ���������� UUID
#define cmImportPerson2                562 // @v7.6.1 ������ ����������� ����������
#define cmExportGoodsRestUhtt          563 // @v7.6.3 "������ ��� � �������� �������� � Universe-HTT
#define cmLotPacking                   564 // @v7.6.4 �������� �����
#define cmCorrectCCheckLine01          565 // @v7.6.6 ������������� �������� ����� ����� 01
#define cmTextBrowserTest              566 // @v7.6.x ��������� �������� ����
#define cmOpenTextFile                 567 // ������� ��������� ���� ��� �������������� //
#define cmEditIniFiles                 567 // @obsolete @v7.6.x �������������� ini ������
#define cmViewNewVersionList           568 // @v7.6.x �������� ����� ������ Papyrus'a
#define cmGenericDevice                569 // @v7.8.0 ���������� ����������
#define cmIndexTextObjects             570 // @v7.8.0 ���������� ��������� �������� // @construction
#define cmImportSR25                   571 // ������ ���� ������ SR25
#define cmImportSuprWare               572 // ������ ������� � �� �������������
#define cmSuprWare                     573 // ������ � �� ������
#define cmProcessObjText               574 // ��������� ��������� ��������
#define cmPersonEventByReader          575 // @v7.9.6 �������� ������������ ������� �� ������ � ��������� �����
#define cmWorkbook                     576 // �������
#define cmTestReconnect                577 // @v8.0.6 ���� ���������� ��������� ������
#define cmUserProfile                  578 // @v8.x.x ��������� �� ������������� � ��������
#define cmTSessAutoSms                 579 // �������� ��� �� ������ ��������������� ������
#define cmImportWorkbook               580 // @v8.1.2 ������ ������� ����
#define cmConvertWorkbook813           581 // @v8.1.3 ����������� ������� ����
#define cmWorkbookCfg                  582 // @v8.1.7 ������������ ������� ����
#define cmConvertLatexToHtml           583 // @v8.1.12 ������� ����������� ������������ (������ ��� ����������� �������������)
#define cmRecoverLostUnifiedLinks      584 // @v8.2.8 ������������� ���������� � ���������� ����������� ��������, ������ � �'���
#define cmGenerateKeywordSeq           585 // @v8.2.10 ��������� ������������������� �������� ���� (����)
#define cmTestUhttGetLocByPhone        586 // @v8.3.2 ���� SOAP-������� ��������� ������ ������� �� ������ ��������
#define cmTestUhttGetQuotByLoc         587 // @v8.3.6 ���� SOAP-������� ��������� ������ ��������� �� ���� �������
#define cmLocationTag                  588 // @v8.3.6 ���� �������
#define cmCorrectGoodsGroupsNIA        589 // @v8.3.6 ������������� �������� ����� (�� �������������)
#define cmConvertVersionToHtml         590 // @v8.3.12 ������� ����������� ���������� � ������� (������ ��� ����������� �������������)
#define cmCafeTable                    591 // @v8.4.1 ���������� ������ � ����������� ����
#define cmAlcReportCfg                 592 // @v8.4.4 ������������ ����������� ����������
#define cmObjTagAllObj                 593 // @v8.5.3 ���� ����� �� ���� �������� ������
#define cmEdiProvider                  594 // @v8.5.5 EDI-����������
#define cmImportQuot2                  595 // @v8.5.11 ������ ���������
#define cmImportFias                   596 // @v8.6.2 ������ ����������� ����
#define cmExportDbTblTransfer          597 // @v8.6.7 ������� � XML ������ ������� Transfer
#define cmExportDbTblBill              598 // @v8.6.8 ������� � XML ������ ������� Bill
#define cmTestFias                     599 // @v8.6.10 ������������ ����������� ����
#define cmViewGeoTracking              600 // @v8.6.11 �������� ������� ���-��������
#define cmImportTSessUHTT              601 // @v8.7.10 Universe-HTT ������ ��������������� ������
#define cmTestCURL                     602 // @v8.8.0 ������������ ���������� � ����������� cURL
#define cmTestEGAIS                    603 // @v8.8.0 ������������ ���������� � �������� �����
#define cmQueryEGAIS                   604 // @v8.8.1 ������������� ������ � ������� �����
#define cmTestSearchEmail              605 // @v8.8.9 ������������ ������� ������ email ������� � ����������� � ��������
#define cmImportPosRefs                606 // @v9.0.8 ������ POS-������������
#define cmBuildLocalDL600              607 // @v9.1.7 ���������� ��������� �������� DL600
#define cmWorkbookInterchange          608 // @v9.3.10 ������������� ������� ���� � ������� Universe-HTT
#define cmProcessOsm                   609 // @v9.5.9 ���������� ������ OpenStreetMap
#define cmPosNodeQuery                 610 // @v9.6.5 ������������� ������ � ��������� ����
#define cmTestLogWindow                611 // @v9.7.0 ������������ ���� ������ ���������

#endif /* __PPMENU_H */
