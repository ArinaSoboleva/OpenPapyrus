// BILVATAX.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2005, 2006, 2007, 2009, 2010, 2015, 2016
// @codepage windows-1251
// ������ ��� �� ����������, ��������������� ��������� �������
//
#include <pp.h>
#pragma hdrstop

IMPL_CMPFUNC(BVATAccm, p1, p2)
{
	int    r = cmp_long(((BVATAccm*)p2)->IsVatFree, ((BVATAccm*)p1)->IsVatFree); // descending order
	return NZOR(r, cmp_double(((BVATAccm*)p1)->PRate, ((BVATAccm*)p2)->PRate));
}

IMPL_CMPFUNC(BVATAccm_DiffByCVat, p1, p2)
{
	int    r = cmp_long(((BVATAccm*)p2)->IsVatFree, ((BVATAccm*)p1)->IsVatFree); // descending order
	return NZOR(r, cmp_double(((BVATAccm*)p1)->CRate, ((BVATAccm*)p2)->CRate));
}

SLAPI BVATAccm::BVATAccm()
{
	THISZERO();
}

SLAPI BVATAccmArray::BVATAccmArray(uint aFlags) : TSArray <BVATAccm> (), Flags(aFlags)
{
}

int SLAPI BVATAccmArray::Add(const BVATAccm * pItem, int dontRound)
{
	int    ok = 1;
	uint   p = 0;
	BVATAccm item = *pItem;
	if(!dontRound) {
		item.Cost     = R2(item.Cost);
		item.Price    = R2(item.Price);
		item.CVATSum  = R2(item.CVATSum);
		item.PVATSum  = R2(item.PVATSum);
		item.PTrnovr  = R2(item.PTrnovr);
		item.Discount = R2(item.Discount);
	}
	CompFunc cfunc = (Flags & BVATF_DIFFBYCRATE) ? PTR_CMPFUNC(BVATAccm_DiffByCVat) : PTR_CMPFUNC(BVATAccm);
	if(bsearch(&item, &p, cfunc)) {
		BVATAccm & r_item = at(p);
		r_item.Cost     += item.Cost;
		r_item.Price    += item.Price;
		r_item.CVATSum  += item.CVATSum;
		r_item.PVATSum  += item.PVATSum;
		r_item.PTrnovr  += item.PTrnovr;
		r_item.Discount += item.Discount;
	}
	else if(!ordInsert(&item, 0, cfunc))
		ok = PPSetErrorSLib();
	return ok;
}

int SLAPI BVATAccmArray::Add(const PPTransferItem * pTI, PPID opID)
{
	int    ok = 1;
	BVATAccm item;
	GTaxVect vect;
	item.IsVatFree = IsVataxableSuppl(pTI->Suppl) ? 0 : 1;
	if(item.IsVatFree && flags & BVATF_SUMZEROVAT) {
	   	vect.CalcTI(pTI, opID, TIAMT_PRICE);
		item.PRate   = vect.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = 0.0;
		item.CVATSum = 0.0;
	   	item.Price   = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT | GTAXVF_EXCISE);
		item.PVATSum = vect.GetValue(GTAXVF_VAT);
		THROW(Add(&item));
		vect.CalcTI(pTI, opID, TIAMT_COST, GTAXVF_VAT);
		item.CRate   = vect.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT);
		item.CVATSum = vect.GetValue(GTAXVF_VAT);
	   	item.Price   = 0.0;
		item.PVATSum = 0.0;
		THROW(Add(&item));
	}
	else {
	   	vect.CalcTI(pTI, opID, TIAMT_PRICE);
		item.PRate   = vect.GetTaxRate(GTAX_VAT, 0);
		item.Price   = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT | GTAXVF_EXCISE);
		item.PVATSum = vect.GetValue(GTAXVF_VAT);
		vect.CalcTI(pTI, opID, TIAMT_COST);
		item.CRate   = vect.GetTaxRate(GTAX_VAT, 0);
		item.Cost    = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT);
		item.CVATSum = vect.GetValue(GTAXVF_VAT);
		THROW(Add(&item));
	}
	CATCHZOK
	return ok;
}

int SLAPI BVATAccmArray::Scale_(double part, int useRounding)
{
	BVATAccm * p_item;
	if(part != 1.0 /* @v6.7.12 && part != 0.0*/)
		for(uint i = 0; enumItems(&i, (void**)&p_item);) {
			p_item->Cost    = p_item->Cost    * part;
			p_item->Price   = p_item->Price   * part;
			p_item->CVATSum = p_item->CVATSum * part;
			p_item->PVATSum = p_item->PVATSum * part;
			if(useRounding) {
				p_item->Cost    = R2(p_item->Cost);
				p_item->Price   = R2(p_item->Price);
				p_item->CVATSum = R2(p_item->CVATSum);
				p_item->PVATSum = R2(p_item->PVATSum);
			}
		}
	return 1;
}

int SLAPI BVATAccmArray::CalcBill(const PPBillPacket * pPack)
{
	int    inited = 0;
	uint   i;
	PPID   op_type_id = GetOpType(pPack->Rec.OpID);
	//
	// ������� (!pPack->GetTCount() && pPack->Rec.Flags & BILLF_BILLF_FIXEDAMOUNTS)
	// ������� ��� ��������� ���������� ��������������� �������� �� ������ ��������.
	//
	if(oneof2(op_type_id, PPOPT_ACCTURN, PPOPT_PAYMENT) || (!pPack->GetTCount() && pPack->Rec.Flags & BILLF_FIXEDAMOUNTS)) {
		PPObjAmountType amtt_obj;
		PPAmountType amtt_rec;
		AmtEntry * p_ae;
		double amt = pPack->GetBaseAmount();
		int    num_vat_rates = 0;
		int    is_there_stax = 0;
		for(i = 0; pPack->Amounts.enumItems(&i, (void**)&p_ae);)
			if(p_ae->Amt != 0.0 && amtt_obj.Fetch(p_ae->AmtTypeID, &amtt_rec) > 0)
				if(amtt_rec.IsTax(GTAX_VAT))
					num_vat_rates++;
				else if(!is_there_stax && amtt_rec.IsTax(GTAX_SALES)) {
					amt -= p_ae->Amt;
					is_there_stax = 1;
				}
		if(num_vat_rates)
			for(i = 0; pPack->Amounts.enumItems(&i, (void**)&p_ae);)
				if(p_ae->Amt != 0.0 && amtt_obj.Fetch(p_ae->AmtTypeID, &amtt_rec) > 0)
					if(amtt_rec.IsTax(GTAX_VAT)) {
						BVATAccm item;
						if(amtt_rec.TaxRate) {
							double rate = fdiv100i(amtt_rec.TaxRate);
							double vat = p_ae->Amt;
							double a = (num_vat_rates > 1) ? (vat / SalesTaxMult(rate)) : amt;
							item.IsVatFree = 0;
							item.PRate   = item.CRate   = rate;
							item.CVATSum = item.PVATSum = vat;
							item.Cost    = item.Price   = a;
						}
						if(!Add(&item))
							return 0;
						inited = 1;
					}
	}
	if(!inited) {
		PPTransferItem * p_ti;
		for(i = 0; pPack->EnumTItems(&i, &p_ti);)
			if(!Add(p_ti, pPack->Rec.OpID))
				return 0;
			else
				inited = 1;
	}
	return inited ? 1 : -1;
}

int SLAPI BVATAccmArray::CalcBill(PPID id)
{
	int    r = -1;
	PPTransferItem ti;
	BillTbl::Rec bill_rec;
	if(BillObj->Search(id, &bill_rec) > 0)
		for(int rbybill = 0; (r = BillObj->trfr->EnumItems(id, &rbybill, &ti)) > 0;)
			if(!Add(&ti, bill_rec.OpID))
				return 0;
	return BIN(r);
}

int SLAPI BVATAccmArray::IsVataxableSuppl(PPID suppl)
{
	return (!(flags & BVATF_IGNORESUPPL) && suppl && IsSupplVATFree(suppl) > 0) ? 0 : 1;
}
