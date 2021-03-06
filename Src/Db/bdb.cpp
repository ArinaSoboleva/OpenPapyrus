// BDB.CPP
// Copyright (c) A.Sobolev 2011, 2012, 2015, 2016, 2017
//
#include <db.h>
#pragma hdrstop
#include <errno.h>
#include <BerkeleyDB.h>

BDbDatabase::Txn::Txn() : TblList(16, aryPtrContainer /* not aryEachItem */)
{
	T = 0;
}

// static
SString & BDbDatabase::MakeFileName(const char * pFile, const char * pTblName, SString & rFileName)
{
	rFileName = 0;
	if(!isempty(pFile)) {
		(rFileName = pFile).Strip();
		if(!isempty(pTblName))
			rFileName.Cat("->").Cat(pTblName);
	}
	return rFileName;
}

// static
int BDbDatabase::SplitFileName(const char * pFileName, SString & rFile, SString & rTbl)
{
	int    ok = 0;
	rFile = 0;
	rTbl = 0;
	SString file_name = pFileName;
	size_t pos = 0;
	if(file_name.Strip().Search("->", 0, 0, &pos)) {
		file_name.Sub(0, pos, rFile);
		file_name.Sub(pos+2, file_name.Len()-pos-2, rTbl);
		ok = 2;
	}
	else if(file_name.NotEmpty()) {
		rFile = file_name;
		ok = 1;
	}
	return ok;
}

int BDbDatabase::SetupErrLog(const char * pFileName)
{
	int    ok = 0;
	ErrF.Close();
	if(isempty(pFileName)) {
		if(E) {
			E->set_errfile(E, 0);
			ok = 1;
		}
	}
	else if(ErrF.Open(pFileName, SFile::mAppend)) {
		if(E) {
			E->set_errfile(E, ErrF);
			ok = 1;
		}
	}
	return ok;
}

BDbDatabase::Config::Config()
{
	Flags = 0;
	CacheSize = 0;
	CacheCount = 0;
	PageSize = 0; // @v9.6.4
	MaxLockers = 0;
	MaxLocks = 0;    // @v9.6.4
	MaxLockObjs = 0;  // @v9.6.4
	LogBufSize = 0;
	LogFileSize = 0;
	// @v8.3.5 {
	MutexCountInit = 0;
	MutexCountMax = 0;
	MutexCountIncr = 0;
	// } @v8.3.5
}

int BDbDatabase::GetCurrentConfig(Config & rCfg)
{
	int    ok = 1;
	if(E) {
		{
			uint32 gb = 0, b = 0;
			int    n = 0;
			THROW(ProcessError(E->get_cachesize(E, &gb, &b, &n)));
			rCfg.CacheSize = ((int64)gb * 1024i64 * 1024i64 * 1024i64) + b;
			rCfg.CacheCount = (uint)n;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->get_memory_init(E, DB_MEM_LOCK, &v)));
			rCfg.MaxLocks = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->get_memory_init(E, DB_MEM_LOCKOBJECT, &v)));
			rCfg.MaxLockObjs = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->get_memory_init(E, DB_MEM_LOCKER, &v)));
			rCfg.MaxLockers = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->mutex_get_init(E, &v)));
			rCfg.MutexCountInit = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->mutex_get_max(E, &v)));
			rCfg.MutexCountMax = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->mutex_get_increment(E, &v)));
			rCfg.MutexCountIncr = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->get_lg_bsize(E, &v)));
			rCfg.LogBufSize = v;
		}
		{
			uint32 v = 0;
			THROW(ProcessError(E->get_lg_max(E, &v)));
			rCfg.LogFileSize = v;
		}
		{
			const char * p_log_dir = 0;
			THROW(ProcessError(E->get_lg_dir(E, &p_log_dir)));
			rCfg.LogSubDir = p_log_dir;
		}
		{
			int    on = 0;
			E->log_get_config(E, DB_LOG_NOSYNC, &on);
			SETFLAG(rCfg.Flags, rCfg.fLogNoSync, on);
			E->log_get_config(E, DB_LOG_AUTO_REMOVE, &on);
			SETFLAG(rCfg.Flags, rCfg.fLogAutoRemove, on);
			E->log_get_config(E, DB_LOG_IN_MEMORY, &on);
			SETFLAG(rCfg.Flags, rCfg.fLogInMemory, on);
		}
	}
	CATCHZOK
	return ok;
}

int BDbDatabase::Helper_SetConfig(const char * pHomeDir, Config & rCfg)
{
	int    ok = 1;
	SString temp_buf;
	if(E) {
		if(rCfg.CacheCount || rCfg.CacheSize) {
			int    n_ = 0, n = 0;
			uint32 gb_ = 0, gb = 0;
			uint32 b_ = 0, b = 0;
			THROW(ProcessError(E->get_cachesize(E, &gb_, &b_, &n_)));
			if(rCfg.CacheSize != 0) {
				gb = (uint32)(rCfg.CacheSize / (1024i64 * 1024i64 * 1024i64));
				b = (uint32)(rCfg.CacheSize % (1024i64 * 1024i64 * 1024i64));
			}
			else {
				gb = gb_;
				b = b_;
			}
			n = (int)NZOR(rCfg.CacheCount, n_);
			THROW(ProcessError(E->set_cachesize(E, gb, b, n)));
		}
		if(rCfg.MaxLockers) {
			THROW(ProcessError(E->set_memory_init(E, DB_MEM_LOCKER, rCfg.MaxLockers)));
		}
		// @v9.6.4 {
		if(rCfg.MaxLocks) {
			THROW(ProcessError(E->set_memory_init(E, DB_MEM_LOCK, rCfg.MaxLocks)));
		}
		if(rCfg.MaxLockObjs) {
			THROW(ProcessError(E->set_memory_init(E, DB_MEM_LOCKOBJECT, rCfg.MaxLockObjs)));
		}
		// } @v9.6.4
		if(rCfg.MutexCountInit) {
			THROW(ProcessError(E->mutex_set_init(E, rCfg.MutexCountInit)));
		}
		if(rCfg.MutexCountMax) {
			THROW(ProcessError(E->mutex_set_max(E, rCfg.MutexCountMax)));
		}
		if(rCfg.MutexCountIncr) {
			THROW(ProcessError(E->mutex_set_increment(E, rCfg.MutexCountIncr)));
		}
		if(rCfg.LogBufSize) {
			THROW(ProcessError(E->set_lg_bsize(E, (uint32)rCfg.LogBufSize)));
		}
		if(rCfg.LogFileSize) {
			THROW(ProcessError(E->set_lg_max(E, rCfg.LogFileSize)));
		}
		if(rCfg.LogSubDir.NotEmpty()) {
			const char * p_log_dir = 0;
			(temp_buf = pHomeDir).SetLastSlash().Cat(rCfg.LogSubDir);
			THROW(::createDir(temp_buf));
			THROW(ProcessError(E->set_lg_dir(E, temp_buf)));
		}
		if(rCfg.Flags & (rCfg.fLogNoSync | rCfg.fLogAutoRemove | rCfg.fLogInMemory)) {
			E->log_set_config(E, DB_LOG_NOSYNC,      BIN(rCfg.Flags & rCfg.fLogNoSync));
			E->log_set_config(E, DB_LOG_AUTO_REMOVE, BIN(rCfg.Flags & rCfg.fLogAutoRemove));
			E->log_set_config(E, DB_LOG_IN_MEMORY,   BIN(rCfg.Flags & rCfg.fLogInMemory));
		}
	}
	CATCHZOK
	return ok;
}

BDbDatabase::BDbDatabase(const char * pHomeDir, Config * pCfg, long options)
{
	int    r = 0;
	SString temp_buf;
	State = 0;
	E = 0;
	P_SeqT = 0;
	P_SCtx = new SSerializeContext;
	HomePathPos = 0;
	StrPool.add("$", 0); // zero index - is empty string
	temp_buf = pHomeDir;
	if(temp_buf.NotEmptyS())
		StrPool.add(temp_buf, &HomePathPos);
	THROW(ProcessError(db_env_create(&E, 0), pHomeDir));
	THROW(::createDir(pHomeDir));
	if(pCfg) {
		THROW(Helper_SetConfig(pHomeDir, *pCfg));
	}
	E->set_timeout(E, 1000000, DB_SET_LOCK_TIMEOUT);
	{
		int    opf = DB_CREATE|DB_INIT_MPOOL|DB_INIT_LOCK|DB_USE_ENVIRON|DB_INIT_TXN|DB_INIT_LOG;
		if(options & oRecover)
			opf |= DB_RECOVER;
		// @v9.6.4 {
		if(options & oPrivate)
			opf |= DB_PRIVATE;
		// } @v9.6.4
		// @todo ������������ �������������� ���������� (DB_RECOVER) ��� ������� ����������.
#ifdef _MT
		opf |= DB_THREAD;
#endif
		r = E->open(E, pHomeDir, opf, /* Open flags */0);
	}
	{
		Config temp_cfg;
		THROW(GetCurrentConfig(temp_cfg));
	}
	THROW(ProcessError(r, pHomeDir));
	LockDetect();
	CATCH
		State |= stError;
		E = 0;
	ENDCATCH
}

BDbDatabase::~BDbDatabase()
{
	RollbackWork();
	uint   c = SeqList.getCount();
	if(c) do {
		Helper_CloseSequence(--c);
	} while(c);
	ZDELETE(P_SeqT);
	if(E) {
		E->log_flush(E, 0); // @v9.6.7
		E->close(E, 0);
		E = 0;
	}
	ZDELETE(P_SCtx);
	ErrF.Close();
}

int BDbDatabase::operator ! () const
{
	return BIN(State & stError);
}

BDbDatabase::operator DB_ENV * ()
{
	return E;
}

BDbDatabase::operator DB_TXN * ()
{
	return T.T;
}

// static
int FASTCALL BDbDatabase::ProcessError(int bdbErrCode, const char * pAddedMsg)
{
	int    ok = 0;
	if(bdbErrCode) {
		switch(bdbErrCode) {
			case DB_LOCK_DEADLOCK:     DBS.SetError(BE_BDB_LOCKDEADLOCK, pAddedMsg); break;
			case DB_LOCK_NOTGRANTED:   DBS.SetError(BE_BDB_LOCKNOTGRANTED, pAddedMsg); break;
			case DB_OLD_VERSION:       DBS.SetError(BE_BDB_OLDVERSION, pAddedMsg); break;
			case ENOENT:               DBS.SetError(BE_FNFOUND, pAddedMsg); break;
			case ENOMEM:               DBS.SetError(BE_BDB_NOMEM); break;
			case EEXIST:               DBS.SetError(BE_FEXISTS, pAddedMsg); break;
			case EINVAL:               DBS.SetError(BE_BDB_INVAL, pAddedMsg); break;
			case DB_REP_HANDLE_DEAD:   DBS.SetError(BE_BDB_REPHANDLEDEAD, pAddedMsg); break;
			case DB_REP_LOCKOUT:       DBS.SetError(BE_BDB_REPLOCKOUT, pAddedMsg); break;
			case DB_NOTFOUND:          DBS.SetError(BE_KEYNFOUND); break;
			case DB_BUFFER_SMALL:      DBS.SetError(BE_UBUFLEN); break;
			case DB_REP_LEASE_EXPIRED: DBS.SetError(BE_DBD_REPLEASEEXPIRED); break;
			default:                   DBS.SetError(BE_BDB_UNKN, pAddedMsg); break;
		}
	}
	else
		ok = 1;
	return ok;
}

int BDbDatabase::Helper_Close(void * pH)
{
	DB * p_db = (DB *)pH;
	CALLPTRMEMB(p_db, close(p_db, 0));
	return 1;
}

int BDbDatabase::RemoveUnusedLogs()
{
	int    ok = 1;
	THROW(E);
	THROW(ProcessError(E->log_archive(E, 0, DB_ARCH_REMOVE)));
	CATCHZOK
	return ok;
}

int BDbDatabase::StartTransaction()
{
	int    ok = 1;
	THROW(E);
	THROW_D(!T.T, BE_DBD_INNERTXN);
	{
		const uint32 ta_flags = DB_TXN_SNAPSHOT | DB_TXN_BULK;
		int r = E->txn_begin(E, 0, &T.T, ta_flags);
		THROW(ProcessError(r));
	}
	CATCHZOK
	return ok;
}

int BDbDatabase::RollbackWork()
{
	int    ok = 1;
	if(T.T) {
		int r = T.T->abort(T.T);
		T.T = 0;
		//
		for(uint i = 0; i < T.TblList.getCount(); i++) {
			BDbTable * p_tbl = (BDbTable *)T.TblList.at(i);
			if(p_tbl && p_tbl->IsConsistent())
				p_tbl->Helper_EndTransaction();
		}
		T.TblList.freeAll();
		//
		THROW(ProcessError(r));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int BDbDatabase::CommitWork()
{
	int    ok = 1;
	if(T.T) {
		uint32 commit_flags = 0;
		int r = T.T->commit(T.T, commit_flags);
		T.T = 0;
		//
		for(uint i = 0; i < T.TblList.getCount(); i++) {
			BDbTable * p_tbl = (BDbTable *)T.TblList.at(i);
			if(p_tbl && p_tbl->IsConsistent())
				p_tbl->Helper_EndTransaction();
		}
		T.TblList.freeAll();
		//
		THROW(ProcessError(r));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int BDbDatabase::TransactionCheckPoint()
{
	int    ok = 1;
	THROW(ProcessError(E->txn_checkpoint(E, 0/*kbyte*/, 0/*min*/, 0/*flags*/)));  
	CATCHZOK
	return ok;
}

int BDbDatabase::LockDetect()
{
	int    ok = -1;
	int    rejected = 0, rej_ = 0;
	THROW(E);
	THROW(ProcessError(E->lock_detect(E, 0, DB_LOCK_DEFAULT, &rej_)));
	rejected += rej_;
	THROW(ProcessError(E->lock_detect(E, 0, DB_LOCK_EXPIRE, &(rej_ = 0))));
	rejected += rej_;
	ok = (rejected > 0) ? 1 : -1;
	CATCHZOK
	return ok;
}

void * BDbDatabase::Helper_Open(const char * pFileName, BDbTable * pTbl)
{
	int    r = 0;
	DB   * p_db = 0;
	SString file_name, tbl_name;
	int    r2 = BDbDatabase::SplitFileName(pFileName, file_name, tbl_name);
	THROW(ProcessError(db_create(&p_db, E, 0)));
	if(pTbl) {
		if(pTbl->Implement_Cmp(0, 0) == 1) {
			DBTYPE db_type = (DBTYPE)0;
			if(p_db->get_type(p_db, &db_type) == 0) {
				if(db_type == DB_BTREE) {
					p_db->set_bt_compare(p_db, BDbTable::CmpCallback);
				}
				else if(db_type == DB_HASH) {
					p_db->set_h_compare(p_db, BDbTable::CmpCallback);
				}
			}
		}
		/*
		if(pTbl->Cfg.CacheSize) {
			const uint32 _gb = (pTbl->Cfg.CacheSize / (1024 * 1024));
			const uint32 _b  = (pTbl->Cfg.CacheSize % (1024 * 1024)) * 1024;
			THROW(ProcessError(p_db->set_cachesize(p_db, _gb, _b, 1)));
		}
		*/
	}
	{
		int    opf = DB_AUTO_COMMIT | DB_MULTIVERSION | DB_READ_UNCOMMITTED;
#ifdef _MT
		opf |= DB_THREAD;
#endif
		r = p_db->open(p_db, T.T, (r2 > 0) ? file_name.cptr() : 0, (r2 == 2) ? tbl_name.cptr() : 0, DB_UNKNOWN, opf, 0 /*mode*/);
	}
	THROW(ProcessError(r));
	CATCH
		if(r != 0) {
			p_db->close(p_db, 0);
			p_db = 0;
		}
	ENDCATCH
	return p_db;
}

int BDbDatabase::IsFileExists(const char * pFileName)
{
	int    yes = 0;
	DB * p_db = (DB *)Helper_Open(pFileName, 0);
	if(p_db) {
		Helper_Close(p_db);
		yes = 1;
	}
	return yes;
}

int BDbDatabase::Helper_Create(const char * pFileName, int createMode, BDbTable::Config * pCfg)
{
	int    ok = 1, r = 0;
	DB   * p_db = 0;
	DBTYPE dbtype = DB_UNKNOWN;
	SString file_name, tbl_name, temp_buf;
	int    r2 = BDbDatabase::SplitFileName(pFileName, file_name, tbl_name);
	switch(pCfg->IdxType) {
		case BDbTable::idxtypBTree: dbtype = DB_BTREE; break;
		case BDbTable::idxtypHash:  dbtype = DB_HASH; break;
		case BDbTable::idxtypHeap:  dbtype = DB_HEAP; break;
		case BDbTable::idxtypQueue: dbtype = DB_QUEUE; break;
		case BDbTable::idxtypDefault: dbtype = DB_BTREE; break;
		default:
			DBS.SetError(BE_BDB_UNKNDBTYPE, (temp_buf = 0).Cat(pCfg->IdxType));
			CALLEXCEPT();
	}
	THROW(ProcessError(db_create(&p_db, E, 0)));
	if(pCfg->Flags & (BDbTable::cfDup/*|BDbTable::cfEncrypt*/)) {
		THROW(ProcessError(p_db->set_flags(p_db, DB_DUP)));
	}
	if(pCfg->PageSize) {
		THROW(ProcessError(p_db->set_pagesize(p_db, pCfg->PageSize)));
	}
	{
		int    opf = (DB_CREATE|DB_AUTO_COMMIT);
#ifdef _MT
		opf |= DB_THREAD;
#endif
		r = p_db->open(p_db, T.T, (r2 > 0) ? file_name.cptr() : 0, (r2 == 2) ? tbl_name.cptr() : 0, dbtype, opf, 0 /*mode*/);
	}
	THROW(ProcessError(r));
	CATCHZOK
	Helper_Close(p_db);
	return ok;
}

int BDbDatabase::CreateDataFile(const char * pFileName, int createMode, BDbTable::Config * pCfg)
{
	return Helper_Create(pFileName, createMode, pCfg);
}

int BDbDatabase::Implement_Open(BDbTable * pTbl, const char * pFileName, int openMode, char * pPassword)
{
	int    ok = 1;
	pTbl->H = (DB *)Helper_Open(pFileName, pTbl);
	if(pTbl->H) {
		CheckInTxnTable(pTbl);
		pTbl->State |= BDbTable::stOpened;
	}
	else {
		pTbl->State &= ~BDbTable::stOpened;
		pTbl->State |= BDbTable::stError;
	}
	return ok;
}

int BDbDatabase::Implement_Close(BDbTable * pTbl)
{
	int    ok = 1;
	if(Helper_Close(pTbl->H)) {
		pTbl->H = 0;
		pTbl->State &= ~BDbTable::stOpened;
	}
	else {
		ok = 0;
	}
	return ok;
}

int FASTCALL BDbDatabase::CheckInTxnTable(BDbTable * pTbl)
{
	if(T.T && pTbl && pTbl->IsConsistent()) {
		T.TblList.insert(pTbl);
		return 1;
	}
	else
		return 0;
}

uint FASTCALL BDbDatabase::SearchSequence(const char * pSeqName) const
{
	uint   pos_plus_one = 0;
	SString temp_buf;
	for(uint i = 0; !pos_plus_one && i < SeqList.getCount(); i++) {
		const Seq & r_item = SeqList.at(i);
		if(StrPool.getnz(r_item.NamePos, temp_buf) && temp_buf == pSeqName)
			pos_plus_one = (i+1);
	}
	return pos_plus_one;
}

long BDbDatabase::CreateSequence(const char * pName, int64 initVal)
{
	long   h = 0;
	if(!P_SeqT) {
		SString file_name;
		(file_name = "system.db").Cat("->").Cat("sequence");
		BDbTable::Config cfg(file_name, BDbTable::idxtypHash, 0, 0, 0);
		THROW_D(P_SeqT = new BDbTable(cfg, this), BE_NOMEM);
		THROW(P_SeqT->GetState(BDbTable::stOpened));
	}
	{
		uint   pos_plus_one = SearchSequence(pName);
		if(pos_plus_one) {
			h = SeqList.at(pos_plus_one-1).Id;
		}
		else {
			DB_SEQUENCE * p_seq = 0;
			BDbTable::Buffer key;
			THROW(ProcessError(db_sequence_create(&p_seq, P_SeqT->H, 0), pName));
			SETIFZ(initVal, 1);
			THROW(ProcessError(p_seq->initial_value(p_seq, initVal)));
			key = pName;
			{
				int    opf = DB_CREATE;
#ifdef _MT
				opf |= DB_THREAD;
#endif
				THROW(ProcessError(p_seq->open(p_seq, 0/*TXN*/, key, opf)));
			}
			{
				Seq seq_item;
				seq_item.Id = 1;
				seq_item.H = p_seq;
				StrPool.add(pName, &seq_item.NamePos);
				for(uint i = 0; i < SeqList.getCount(); i++) {
					const long item_id = SeqList.at(i).Id;
					if(item_id >= seq_item.Id)
						seq_item.Id = item_id+1;
				}
				SeqList.insert(&seq_item);
				h = seq_item.Id;
			}
		}
	}
	CATCH
		h = 0;
	ENDCATCH
	return h;
}

int FASTCALL BDbDatabase::Helper_CloseSequence(uint pos)
{
	int    ok = 1;
	if(pos < SeqList.getCount()) {
		Seq  & r_item = SeqList.at(pos);
		if(r_item.H) {
			THROW(ProcessError(r_item.H->close(r_item.H, 0)));
		}
		SeqList.atFree(pos);
	}
	CATCHZOK
	return ok;
}

int BDbDatabase::CloseSequence(long seqId)
{
	int    ok = -1;
	uint   c = SeqList.getCount();
	if(c) do {
		if(SeqList.at(--c).Id == seqId) {
			THROW(Helper_CloseSequence(c));
			ok = 1;
		}
	} while(c);
	CATCHZOK
	return ok;
}

int BDbDatabase::GetSequence(long seqId, int64 * pVal)
{
	int    ok = 0;
	int64  val = 0;
	for(uint i = 0; i < SeqList.getCount(); i++) {
		if(SeqList.at(i).Id == seqId) {
			DB_SEQUENCE * p_seq = SeqList.at(i).H;
			if(p_seq) {
				db_seq_t _v = 0;
				THROW(ProcessError(p_seq->get(p_seq, T.T, 1, &_v, DB_TXN_NOSYNC)));
				val = (int64)_v;
				ok = 1;
			}
			break;
		}
	}
	if(!ok) {
		SString msg_buf;
		msg_buf.Cat(seqId);
		DBS.SetError(BE_BDB_SEQIDNFOUND, msg_buf);
	}
	CATCHZOK
	ASSIGN_PTR(pVal, val);
	return ok;
}

//static
int BDbTable::VerifyStatic()
{
	assert(BDbTable::Buffer::fMalloc == DB_DBT_MALLOC);
	assert(BDbTable::Buffer::fRealloc == DB_DBT_REALLOC);
	assert(BDbTable::Buffer::fAppMalloc == DB_DBT_APPMALLOC);
	assert(BDbTable::Buffer::fUserMem == DB_DBT_USERMEM);
	assert(BDbTable::Buffer::fReadOnly == DB_DBT_READONLY);
	assert(BDbTable::Buffer::fPartial == DB_DBT_PARTIAL);
	assert(BDbTable::Buffer::fMultiple == DB_DBT_MULTIPLE);
	assert(sizeof(BDbTable::Buffer) >= sizeof(DBT));
	return 1;
}
//
//
//
IMPL_INVARIANT_C(BDbTable::Buffer)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(!(Flags & fUserMem) || Size <= ULen, pInvP);
	S_ASSERT_P(!(Flags & fUserMem) || B.GetSize() >= ULen, pInvP);
	S_ASSERT_P(!(Flags & fUserMem) || P_Data == 0 || (P_Data == (const char *)B && Flags & fUserMem), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

BDbTable::Buffer::Buffer(size_t sz /*=128*/) : B(sz)
{
	Reset();
}

BDbTable::Buffer::Buffer(const DBT * pB) : B(0)
{
	Reset();
	if(pB) {
		P_Data = pB->data;
		Size = pB->size;
		ULen = pB->ulen;
		DLen = pB->dlen;
		DOff = pB->doff;
		Flags = (pB->flags & ~fUserMem);
		P_AppData = pB->app_data;
	}
	assert(InvariantC(0));
}

DBT * FASTCALL BDbTable::Buffer::Get(DBT * pB) const
{
	if(pB) {
		pB->data = P_Data;
		pB->size = Size;
		pB->ulen = ULen;
		pB->dlen = DLen;
		pB->doff = DOff;
		pB->flags = Flags;
		pB->app_data = P_AppData;
	}
	assert(InvariantC(0));
	return pB;
}

BDbTable::Buffer::operator DBT * () const
{
	return (DBT *)this;
}

void BDbTable::Buffer::Reset()
{
	P_Data = 0;
	P_AppData = 0;
	Size = ULen = DLen = DOff = Flags = 0;
}

int BDbTable::Buffer::Realloc()
{
	int    ok = -1;
	if(Size > ULen) {
		if(Flags & fUserMem)
			ok = Alloc(Size);
		else
			ok = 0;
	}
	return ok;
}

int FASTCALL BDbTable::Buffer::Alloc(size_t sz)
{
	int    ok = 1, r = 1;
	Reset();
	if(sz < B.GetSize() || (r = B.Alloc(sz))) {
		P_Data = B;
		ULen = B.GetSize();
		Flags |= fUserMem;
	}
	if(!r)
		ok = 0;
	return ok;
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const BDbTable::Buffer & rS)
{
	B = rS.B;
	Size = rS.Size;
	ULen = rS.ULen;
	DLen = rS.DLen;
	DOff = rS.DOff;
	P_AppData = rS.P_AppData;
	Flags = rS.Flags;
	P_Data = (rS.P_Data == (const void *)(const char *)rS.B) ? (void *)(const char *)B : rS.P_Data;
	return *this;
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const SBuffer & rBuf)
{
	Reset();
	const size_t src_size = rBuf.GetAvailableSize();
	if(src_size && Alloc(ALIGNSIZE(src_size, 6))) {
		rBuf.ReadStatic(B, src_size);
		Size = src_size;
	}
	return *this;
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const char * pStr)
{
	Reset();
	const size_t src_size = sstrlen(pStr) + 1;
	if(src_size && Alloc(ALIGNSIZE(src_size, 6))) {
		if(pStr)
			memcpy(B, pStr, src_size);
		else
			B[0] = 0;
		Size = src_size;
	}
	return *this;
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const wchar_t * pUStr)
{
	Reset();
	const size_t src_size = (sstrlen(pUStr) + 1) << 1;
	if(src_size && Alloc(ALIGNSIZE(src_size, 6))) {
		if(pUStr)
			memcpy(B, pUStr, src_size);
		else
			((wchar_t *)((char *)B))[0] = 0;
		Size = src_size;
	}
	return *this;
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const int32 & rVal)
{
	return Set(&rVal, sizeof(rVal));
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const uint32 & rVal)
{
	return Set(&rVal, sizeof(rVal));
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const int64 & rVal)
{
	return Set(&rVal, sizeof(rVal));
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::operator = (const uint64 & rVal)
{
	return Set(&rVal, sizeof(rVal));
}

BDbTable::Buffer & FASTCALL BDbTable::Buffer::Set(const void * pData, size_t sz)
{
	const size_t src_size = sz;
	if(src_size) {
		// Alloc �������� Reset ������ ����� ��� �������� �� ���� (�������������� ��������)
		if(Alloc(ALIGNSIZE(src_size, 6))) {
			if(pData)
				memcpy(B, pData, src_size);
			else
				memzero(B, src_size);
			Size = src_size;
		}
	}
	else
		Reset();
	return *this;
}

size_t BDbTable::Buffer::GetSize() const
{
	return Size;
}

const void * FASTCALL BDbTable::Buffer::GetPtr(size_t * pSize) const
{
	ASSIGN_PTR(pSize, Size);
	return P_Data;
}

int FASTCALL BDbTable::Buffer::Get(SBuffer & rBuf) const
{
	int    ok = 1;
	assert(InvariantC(0));
	if(Size) {
		if(!rBuf.Write(P_Data, Size))
			ok = 0;
	}
	return ok;
}

int FASTCALL BDbTable::Buffer::Get(SString & rBuf) const
{
	int    ok = 1;
	assert(InvariantC(0));
	if(Size) {
		(rBuf = 0).CatN((const char *)P_Data, Size);
	}
	return ok;
}

int FASTCALL BDbTable::Buffer::Get(SStringU & rBuf) const
{
	int    ok = 1;
	assert(InvariantC(0));
	if(Size) {
		(rBuf = 0).CatN((const wchar_t *)(const char *)P_Data, Size/2);
	}
	return ok;
}

int BDbTable::Buffer::Get(void * pBuf, size_t bufSize) const
{
	int    ok = 1;
	assert(InvariantC(0));
	if(pBuf) {
		const size_t sz = MIN(Size, bufSize);
		if(sz)
			memcpy(pBuf, P_Data, sz);
		else
			ok = -1;
	}
	return ok;
}

int FASTCALL BDbTable::Buffer::Get(int32 * pBuf) const
{
	return Get(pBuf, sizeof(*pBuf));
}

int FASTCALL BDbTable::Buffer::Get(uint32 * pBuf) const
{
	return Get(pBuf, sizeof(*pBuf));
}

int FASTCALL BDbTable::Buffer::Get(int64 * pBuf) const
{
	return Get(pBuf, sizeof(*pBuf));
}
//
//
//
//BDbTable::Config::Config(const char * pName, int idxType /*= idxtypDefault*/, long flags /*= 0*/)
BDbTable::Config::Config(const char * pName, int idxType, long flags, uint32 pageSize, uint32 cacheSizeKb)
{
	Clear();
	IdxType = idxType;
	Flags = flags;
	PageSize = pageSize;
	CacheSize = cacheSizeKb;
	DataChunk = 1024;
	Name = pName;
}

void BDbTable::Config::Clear()
{
	IdxType = idxtypDefault;
	Flags = 0;
	DataChunk = 0;
	CacheSize = 0;
	PageSize = 0;
	Name = 0;
}
//
//
//
BDbTable::SecondaryIndex::SecondaryIndex()
{
	P_MainT = 0;
}

BDbTable::SecondaryIndex::~SecondaryIndex()
{
}

//virtual
int BDbTable::SecondaryIndex::Implement_Cmp(BDbTable * pMainTbl, const BDbTable::Buffer * pKey1, const BDbTable::Buffer * pKey2)
{
	return 0;
}
//
//
//
//static
int BDbTable::ScndIdxCallback(DB * pSecondary, const DBT * pKey, const DBT * pData, DBT * pResult)
{
	int    r = DB_DONOTINDEX;
	const  DbThreadLocalArea::DbRegList & r_reg = DBS.GetConstTLA().GetBDbRegList_Const();
	BDbTable * p_taget_tbl = (BDbTable *)r_reg.GetBySupplementPtr(pSecondary);
#ifndef NDEBUG
	// testing {
	for(int i = 1; i <= r_reg.GetMaxEntries(); i++) {
		BDbTable * p_tbl = (BDbTable *)r_reg.GetPtr(i);
		if(p_tbl && p_tbl->H == pSecondary) {
			assert(p_taget_tbl == p_tbl);
			//p_taget_tbl = p_tbl;
			break;
		}
	}
	// }
#endif
	if(p_taget_tbl) {
		if(p_taget_tbl->P_IdxHandle) {
			BDbTable::Buffer key(pKey);
			BDbTable::Buffer data(pData);
			r = p_taget_tbl->P_IdxHandle->Cb(key, data, p_taget_tbl->P_IdxHandle->ResultBuf);
			p_taget_tbl->P_IdxHandle->ResultBuf.Get(pResult);
		}
	}
	return r;
}

//static
int BDbTable::CmpCallback(DB * pDb, const DBT * pDbt1, const DBT * pDbt2)
{
	int    c = 0;
	if(pDbt1 != 0 || pDbt2 != 0) {
		const DbThreadLocalArea::DbRegList & r_reg = DBS.GetConstTLA().GetBDbRegList_Const();
		BDbTable * p_taget_tbl = (BDbTable *)r_reg.GetBySupplementPtr(pDb);
#ifndef NDEBUG
		// testing {
		for(int i = 1; i <= r_reg.GetMaxEntries(); i++) {
			BDbTable * p_tbl = (BDbTable *)r_reg.GetPtr(i);
			if(p_tbl && p_tbl->H == pDb) {
				assert(p_taget_tbl == p_tbl);
				//p_taget_tbl = p_tbl;
				break;
			}
		}
		// }
#endif
		if(p_taget_tbl) {
			BDbTable::Buffer key1(pDbt1);
			BDbTable::Buffer key2(pDbt2);
			c = p_taget_tbl->Implement_Cmp(&key1, &key2);
		}
	}
	return c;
}

int BDbTable::InitInstance(BDbDatabase * pDb)
{
	int    ok = 1;
	BDbTable::VerifyStatic();
	Handle = 0;
	State = 0;
	H = 0;
	P_Db = pDb;
	P_MainT = 0;
	P_IdxHandle = 0;
	//
	// �������������� �������� ������������ // {
	//
	P_SCtx = 0;
	if(P_Db)
		P_SCtx = P_Db->GetSCtx();
	if(Cfg.Flags & cfNeedSCtx && !P_SCtx) {
		P_SCtx = new SSerializeContext;
		State |= stOwnSCtx;
	}
	// }
	if(Cfg.Name.NotEmptyS() && P_Db) {
		if(!P_Db->IsFileExists(Cfg.Name)) {
			THROW(P_Db->CreateDataFile(Cfg.Name, 0, &Cfg));
		}
		THROW(Open(Cfg.Name));
	}
	CATCHZOK
	return ok;
}

#define BDBT_SIGNATURE 0x3a0491f2U

BDbTable::BDbTable(const Config & rCfg, BDbDatabase * pDb) : Cfg(rCfg)
{
	Sign = BDBT_SIGNATURE;
	InitInstance(pDb);
}

BDbTable::BDbTable(const Config & rCfg, BDbDatabase * pDb, SecondaryIndex * pIdxHandle, BDbTable * pMainTbl) : Cfg(rCfg)
{
	Sign = BDBT_SIGNATURE;
	if(InitInstance(pDb)) {
		if(pIdxHandle) {
			THROW_D_S(GetState(stOpened), BE_FILNOPEN, Cfg.Name);
			THROW_D_S(pMainTbl, BE_BDB_IDX_ZEROMAINTBL, Cfg.Name);
			THROW_D_S(pMainTbl->GetState(stOpened), BE_BDB_IDX_MAINTBLNOPENED, Cfg.Name);
			P_IdxHandle = pIdxHandle;
			P_IdxHandle->P_MainT = pMainTbl;
			P_MainT = pMainTbl;
			State |= stIndex;
			THROW(BDbDatabase::ProcessError(P_MainT->H->associate(P_MainT->H, 0 /*TXN*/, H, BDbTable::ScndIdxCallback, 0)));
			P_MainT->IdxList.insert(this); // this ���������� �������������� P_MainT
		}
	}
	CATCH
		Close();
	ENDCATCH
}

BDbTable::~BDbTable()
{
	Close();
	Sign = 0;
	if(P_SCtx && State & stOwnSCtx)
		delete P_SCtx;
}

int BDbTable::operator ! () const
{
	return !GetState(stOpened);
}

BDbTable::operator DB * ()
{
	return H;
}

BDbTable::operator DB_TXN * ()
{
	return P_Db ? (DB_TXN *)*P_Db : (DB_TXN *)0;
}

TSCollection <BDbTable> & BDbTable::GetIdxList()
{
	return IdxList;
}

int FASTCALL BDbTable::GetState(long stateFlag) const
{
	return BIN(State & stateFlag);
}

int BDbTable::IsConsistent() const
{
	return (Sign == BDBT_SIGNATURE) ? 1 : DBS.SetError(BE_BDB_INVALID_TABLE, 0);
}

//virtual
int BDbTable::Implement_Cmp(const BDbTable::Buffer * pKey1, const BDbTable::Buffer * pKey2)
{
	return P_IdxHandle ? P_IdxHandle->Implement_Cmp(this, pKey1, pKey2) : 0;
}

SSerializeContext * BDbTable::GetSCtx() const
{
	return P_SCtx;
}

int BDbTable::Helper_GetConfig(BDbTable * pT, Config & rCfg)
{
	int    ok = 1;
	rCfg.Clear();
	if(pT && pT->H) {
		SString temp_buf;
		DB * p_db = pT->H;
		{
			DBTYPE _t = DB_UNKNOWN;
			THROW(BDbDatabase::ProcessError(p_db->get_type(p_db, &_t)));
			rCfg.IdxType = _t;
			switch(_t) {
				case DB_BTREE: rCfg.IdxType = BDbTable::idxtypBTree; break;
				case DB_HASH: rCfg.IdxType = BDbTable::idxtypHash; break;
				case DB_HEAP: rCfg.IdxType = BDbTable::idxtypHeap; break;
				case DB_QUEUE: rCfg.IdxType = BDbTable::idxtypQueue; break;
				case DB_UNKNOWN: rCfg.IdxType = BDbTable::idxtypDefault; break;
				default:
					DBS.SetError(BE_BDB_UNKNDBTYPE, (temp_buf = 0).Cat(_t));
					CALLEXCEPT();
			}
		}
		{
			uint32 _f = 0;
			THROW(BDbDatabase::ProcessError(p_db->get_flags(p_db, &_f)));
			SETFLAG(rCfg.Flags, cfEncrypt, _f & DB_ENCRYPT);
			SETFLAG(rCfg.Flags, cfDup, _f & DB_DUP);
		}
		{
			const char * p_db_name = 0;
			const char * p_file_name = 0;
			THROW(BDbDatabase::ProcessError(p_db->get_dbname(p_db, &p_file_name, &p_db_name)));
			rCfg.Name = p_file_name;
		}
		{
			uint32 _gb = 0;
			uint32 _b = 0;
			int    _c = 0;
			THROW(BDbDatabase::ProcessError(p_db->get_cachesize(p_db, &_gb, &_b, &_c)));
			rCfg.CacheSize = (_gb * 1024 * 1024) + _b / 1024;
		}
		{
			uint32 _ps = 0;
			THROW(BDbDatabase::ProcessError(p_db->get_pagesize(p_db, &_ps)));
			rCfg.PageSize = _ps;
		}
	}
	CATCHZOK
	return ok;
}

int BDbTable::GetConfig(int idx, Config & rCfg)
{
	int    ok = 1;
	THROW(State & stOpened);
	if(idx == 0) {
		THROW(Helper_GetConfig(this, rCfg));
	}
	else {
		THROW_D(idx <= (int)IdxList.getCount(), BE_INVKEY);
		{
			BDbTable * p_idx_tbl = IdxList.at(idx-1);
			THROW(Helper_GetConfig(p_idx_tbl, rCfg));
		}
	}
	CATCHZOK
	return ok;
}

int BDbTable::Open(const char * pFileName)
{
	int    ok = 1;
	int    r = 0;
	if(State & stOpened) {
		THROW(Close());
	}
	THROW(P_Db);
	THROW(P_Db->Implement_Open(this, pFileName, 0, 0));
	{
		DbThreadLocalArea::DbRegList & r_reg = DBS.GetTLA().GetBDbRegList();
		THROW(Handle = r_reg.AddEntry(this, H));
	}
	CATCHZOK
	return ok;
}

int BDbTable::Close()
{
	int    ok = 1;
	IdxList.freeAll();
	P_MainT = 0;
	ZDELETE(P_IdxHandle);
	THROW(P_Db);
	THROW(P_Db->Implement_Close(this));
	if(Handle) {
		DbThreadLocalArea::DbRegList & r_reg = DBS.GetTLA().GetBDbRegList();
		r_reg.FreeEntry(Handle, H);
		Handle = 0;
	}
	CATCHZOK
	return ok;
}

int BDbTable::Helper_EndTransaction()
{
	return 1;
}

static uint32 FASTCALL _GetBDBSearchFlags(int sp)
{
	uint32 flags = 0;
	switch(sp) {
		case spEq:      flags = DB_SET; break;
		case spNext:    flags = DB_NEXT; break;
		case spPrev:    flags = DB_PREV; break;
		case spFirst:   flags = DB_FIRST; break;
		case spLast:    flags = DB_LAST; break;
		case spNextDup: flags = DB_NEXT_DUP; break;
		case spPrevDup: flags = DB_PREV_DUP; break;
		case spGe:      flags = DB_SET_RANGE; break;
		default:        DBS.SetError(BE_BDB_INVALSP); break;
	}
	return flags;
}

int BDbTable::Helper_Search(Buffer & rKey, Buffer & rData, uint32 flags)
{
	DB_TXN * p_txn = P_Db ? (DB_TXN *)*P_Db : (DB_TXN *)0;
	int    r = BDbDatabase::ProcessError(H->get(H, p_txn, rKey, rData, flags));
	if(!r && BtrError == BE_UBUFLEN) {
		int    r2 = 0;
		r2 = rData.Realloc();
		if(r2 > 0)
			r = BDbDatabase::ProcessError(H->get(H, p_txn, rKey, rData, flags));
	}
	return r;
}

int BDbTable::Search(Buffer & rKey, Buffer & rData)
{
	return Helper_Search(rKey, rData, 0);
}

int BDbTable::SearchPair(Buffer & rKey, Buffer & rData)
{
	return Helper_Search(rKey, rData, DB_GET_BOTH);
}

int BDbTable::Search(int idx, Buffer & rKey, Buffer & rData)
{
	int    ok = 1;
	if(idx == 0) {
		ok = Helper_Search(rKey, rData, 0);
	}
	else {
		THROW_D(idx <= (int)IdxList.getCount(), BE_INVKEY);
		{
			BDbTable * p_idx_tbl = IdxList.at(idx-1);
			Buffer pkey;
			pkey.Alloc(256);
			uint32 flags = 0;
			THROW_D(p_idx_tbl, BE_INVKEY);
			{
				DB_TXN * p_txn = P_Db ? (DB_TXN *)*P_Db : (DB_TXN *)0;
				int r = BDbDatabase::ProcessError(p_idx_tbl->H->pget(p_idx_tbl->H, p_txn, rKey, pkey, rData, flags));
				if(!r && BtrError == BE_UBUFLEN) {
					int r1 = 0, r2 = 0;
					THROW(r1 = pkey.Realloc());
					THROW(r2 = rData.Realloc());
					if(r1 > 0 || r2 > 0)
						r = BDbDatabase::ProcessError(p_idx_tbl->H->pget(p_idx_tbl->H, p_txn, rKey, pkey, rData, flags));
				}
				THROW(r);
			}
			rKey = pkey;
		}
	}
	CATCHZOK
	return ok;
}

int BDbTable::Helper_Put(Buffer & rKey, Buffer & rData, uint32 flags)
{
	DB_TXN * p_txn = P_Db ? (DB_TXN *)*P_Db : (DB_TXN *)0;
	return BDbDatabase::ProcessError(H->put(H, p_txn, rKey, rData, flags));
}

int BDbTable::InsertRec(Buffer & rKey, Buffer & rData)
{
	return Helper_Put(rKey, rData, DB_NOOVERWRITE);
}

int BDbTable::UpdateRec(Buffer & rKey, Buffer & rData)
{
	return Helper_Put(rKey, rData, 0);
}

int BDbTable::DeleteRec(Buffer & rKey)
{
	int    ok = 1;
	DB_TXN * p_txn = P_Db ? (DB_TXN *)*P_Db : (DB_TXN *)0;
	//int DB->del(DB *db, DB_TXN *txnid, DBT *key, uint32 flags);
	THROW(BDbDatabase::ProcessError(H->del(H, p_txn, rKey, 0 /*flags*/)));
	CATCHZOK
	return ok;
}
//
//
//
DB * FASTCALL BDbCursor::GetIntTbl(int idx)
{
	DB * p_db = (DB *)R_Tbl;
	if(idx == 0)
		p_db = (DB *)R_Tbl;
	else {
		TSCollection <BDbTable> & r_idx_list = R_Tbl.GetIdxList();
		if(idx <= (int)r_idx_list.getCount()) {
			BDbTable * p_tbl = r_idx_list.at(idx-1);
			p_db = p_tbl ? *p_tbl : (DB *)0;
		}
		else
			DBS.SetError(BE_INVKEY, 0);
	}
	return p_db;
}

BDbCursor::BDbCursor(BDbTable & rT, int idx) : R_Tbl(rT)
{
	C = 0;
	Idx = idx;
	if(R_Tbl.IsConsistent()) {
		DB_TXN * p_txn = (DB_TXN *)R_Tbl;
		DB * p_db = GetIntTbl(idx);
		if(p_db)
			if(p_db->cursor(p_db, p_txn, &C, 0 /*flags*/) != 0) {
				C = 0;
			}
	}
}

BDbCursor::~BDbCursor()
{
	if(C) {
		C->close(C);
		C = 0;
	}
}

int BDbCursor::operator !() const
{
	return (C == 0);
}

int BDbCursor::Search(BDbTable::Buffer & rKey, BDbTable::Buffer & rData, int sp)
{
	int    ok = 1;
	uint32 flags = _GetBDBSearchFlags(sp);
	THROW(R_Tbl.IsConsistent());
	THROW_D(C, BE_BDB_INVALID_CURSOR);
	THROW(flags);
	if(Idx == 0) {
		ok = BDbDatabase::ProcessError(C->get(C, rKey, rData, flags));
		if(!ok && BtrError == BE_UBUFLEN) {
			int    r2 = rData.Realloc();
			if(r2 > 0)
				ok = BDbDatabase::ProcessError(C->get(C, rKey, rData, flags));
		}
	}
	else {
		BDbTable::Buffer pkey;
		pkey.Alloc(256);
		ok = BDbDatabase::ProcessError(C->pget(C, rKey, pkey, rData, flags));
		if(!ok && BtrError == BE_UBUFLEN) {
			int r1 = 0, r2 = 0;
			THROW(r1 = pkey.Realloc());
			THROW(r2 = rData.Realloc());
			if(r1 > 0 || r2 > 0)
				ok = BDbDatabase::ProcessError(C->pget(C, rKey, pkey, rData, flags));
		}
		THROW(ok);
		rKey = pkey;
	}
	CATCHZOK
	return ok;
}

int BDbCursor::Insert(BDbTable::Buffer & rKey, BDbTable::Buffer & rData, int current)
{
	int    ok = 1;
	THROW(R_Tbl.IsConsistent());
	THROW_D(C, BE_BDB_INVALID_CURSOR);
	THROW(BDbDatabase::ProcessError(C->put(C, rKey, rData, current ? DB_CURRENT : DB_KEYLAST)));
	CATCHZOK
	return ok;
}

int BDbCursor::Delete()
{
	int    ok = 1;
	THROW(R_Tbl.IsConsistent());
	THROW_D(C, BE_BDB_INVALID_CURSOR);
	THROW(BDbDatabase::ProcessError(C->del(C, 0)));
	CATCHZOK
	return ok;
}
//
//
//
BDbTransaction::BDbTransaction(BDbDatabase * pDb, int use_ta)
{
	Ta = 0;
	Err = 0;
	P_Db = pDb;
	if(use_ta && P_Db) {
		int    r = P_Db->StartTransaction();
		if(r > 0)
			Ta = 1;
		else if(r == 0) {
			Err = 1;
		}
	}
}

BDbTransaction::~BDbTransaction()
{
	if(Ta && P_Db)
		P_Db->RollbackWork();
}

int BDbTransaction::operator !() const
{
	return BIN(Err);
}

int BDbTransaction::Commit()
{
	if(Ta && P_Db) {
		if(P_Db->CommitWork())
			Ta = 0;
		else {
			Err = 1;
			return 0;
		}
	}
	return 1;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(BerkeleyDB)
{
	int    ok = 1;
	uint   i;
	SString home_dir, test_file;
	(home_dir = GetSuiteEntry()->OutPath).SetLastSlash().Cat("BDB");
	(test_file = GetSuiteEntry()->InPath).SetLastSlash().Cat("city-enru-pair.csv");
	::RemoveDir(home_dir);
	THROW(SLTEST_CHECK_NZ(createDir(home_dir)));
	{
		SFile f_in(test_file, SFile::mRead);
		SString line_buf, en_buf, ru_buf, test_buf;
		THROW(SLTEST_CHECK_NZ(f_in.IsValid()));
		{
			/*
			BDbDatabase::Config bdb_cfg;
			bdb_cfg.CacheSize = 64 * 1024 * 1024;
			bdb_cfg.CacheSize = 1;
			bdb_cfg.MaxLockers = 256 * 1024;
			bdb_cfg.LogFileSize = 256 * 1024 * 1024;
			*/
			BDbDatabase bdb(home_dir);
			THROW(SLTEST_CHECK_NZ(bdb));
			{
				const  uint  test_ta_count = 20;
				uint   count = 0;
				BDbTable::Buffer key_buf, val_buf;
				BDbTable tbl(BDbTable::Config("city-enru-pair", BDbTable::idxtypHash, 0, /*1024*/0, 0), &bdb);
				THROW(SLTEST_CHECK_NZ(tbl));
				{
					//
					// ���� ������������� ������� ���������� �������
					//
					StrAssocArray test_list;
					THROW(SLTEST_CHECK_NZ(bdb.StartTransaction()));
					while(f_in.ReadLine(line_buf) && count < test_ta_count) {
						count++;
						line_buf.Chomp();
						test_list.Add(count, line_buf);
						line_buf.Divide(';', en_buf, ru_buf);

						key_buf = en_buf;
						val_buf = ru_buf;
						THROW(SLTEST_CHECK_NZ(tbl.InsertRec(key_buf, val_buf)));
					}
					THROW(SLTEST_CHECK_NZ(bdb.CommitWork()));
					for(i = 0; i < test_list.getCount(); i++) {
						line_buf = test_list.at(i).Txt;
						line_buf.Divide(';', en_buf, ru_buf);
						key_buf = en_buf;
						THROW(SLTEST_CHECK_NZ(tbl.Search(key_buf, val_buf)));
						val_buf.Get(test_buf);
						THROW(SLTEST_CHECK_Z(test_buf.Cmp(ru_buf, 0)));
					}
					{
						//
						// ���� �������������� �������� ����������� �������
						//
						THROW(SLTEST_CHECK_NZ(bdb.StartTransaction()));
						for(i = 0; i < test_list.getCount(); i++) {
							line_buf = test_list.at(i).Txt;
							line_buf.Divide(';', en_buf, ru_buf);
							key_buf = en_buf;
							THROW(SLTEST_CHECK_NZ(tbl.Search(key_buf, val_buf)));
							THROW(SLTEST_CHECK_NZ(tbl.DeleteRec(key_buf)));
						}
						THROW(SLTEST_CHECK_NZ(bdb.CommitWork()));
						for(i = 0; i < test_list.getCount(); i++) {
							line_buf = test_list.at(i).Txt;
							line_buf.Divide(';', en_buf, ru_buf);
							key_buf = en_buf;
							THROW(SLTEST_CHECK_Z(tbl.Search(key_buf, val_buf)));
							THROW(SLTEST_CHECK_EQ(BtrError, (long)BE_KEYNFOUND));
						}
					}
				}
				{
					//
					// ���� ������ ���������� �� ����� ������� �������
					//
					StrAssocArray test_list;
					THROW(SLTEST_CHECK_NZ(bdb.StartTransaction()));
					while(f_in.ReadLine(line_buf) && count < 2*test_ta_count) {
						count++;
						line_buf.Chomp();
						test_list.Add(count, line_buf);
						line_buf.Divide(';', en_buf, ru_buf);

						key_buf = en_buf;
						val_buf = ru_buf;
						THROW(SLTEST_CHECK_NZ(tbl.InsertRec(key_buf, val_buf)));
						//
						// �� ������ ���������� �� ������ ������ ����������� ������
						//
						THROW(SLTEST_CHECK_NZ(tbl.Search(key_buf, val_buf)));
						val_buf.Get(test_buf);
						THROW(SLTEST_CHECK_Z(test_buf.Cmp(ru_buf, 0)));
					}
					THROW(SLTEST_CHECK_NZ(bdb.RollbackWork()));
					//
					// �� ����� ������ �� ����������� ������� �� ������ ���� � ���� ������
					//
					for(i = 0; i < test_list.getCount(); i++) {
						line_buf = test_list.at(i).Txt;
						line_buf.Divide(';', en_buf, ru_buf);
						key_buf = en_buf;
						THROW(SLTEST_CHECK_Z(tbl.Search(key_buf, val_buf)));
						THROW(SLTEST_CHECK_EQ(BtrError, (long)BE_KEYNFOUND));
					}
				}
				{
					//
					// �������, �� ������ ��������� ��� ������ �� ����� � ������� ��.
					//
					StrAssocArray test_list;
					count = 0;
					f_in.Seek(0, SEEK_SET);
					THROW(SLTEST_CHECK_NZ(bdb.StartTransaction()));
					while(f_in.ReadLine(line_buf)) {
						count++;
						line_buf.Chomp();
						test_list.Add(count, line_buf);
						line_buf.Divide(';', en_buf, ru_buf);

						key_buf = en_buf;
						val_buf = ru_buf;
						THROW(SLTEST_CHECK_NZ(tbl.InsertRec(key_buf, val_buf)));
					}
					THROW(SLTEST_CHECK_NZ(bdb.CommitWork()));
					for(i = 0; i < test_list.getCount(); i++) {
						line_buf = test_list.at(i).Txt;
						line_buf.Divide(';', en_buf, ru_buf);
						key_buf = en_buf;
						THROW(SLTEST_CHECK_NZ(tbl.Search(key_buf, val_buf)));
						val_buf.Get(test_buf);
						THROW(SLTEST_CHECK_Z(test_buf.Cmp(ru_buf, 0)));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING
