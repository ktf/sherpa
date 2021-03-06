#include "MCATNLO/Main/CS_Gamma.H"

#include "MCATNLO/Main/CS_MCatNLO.H"
#include "MCATNLO/Showers/Splitting_Function_Base.H"
#include "PHASIC++/Process/MCatNLO_Process.H"
#include "PHASIC++/Main/Process_Integrator.H"
#include "PDF/Main/Jet_Criterion.H"
#include "PHASIC++/Process/ME_Generator_Base.H"
#include "PHASIC++/Process/Single_Process.H"
#include "PHASIC++/Selectors/Combined_Selector.H"
#include "ATOOLS/Org/Run_Parameter.H"
#include "ATOOLS/Math/Random.H"
#include "ATOOLS/Org/MyStrStream.H"
#include "ATOOLS/Org/Exception.H"

using namespace MCATNLO;
using namespace PHASIC;
using namespace PDF;
using namespace ATOOLS;

#define DEBUG__Trial_Weight

Weight_Key::Weight_Key(const size_t &ij,const size_t &k,
		       const ATOOLS::Flavour &flij,
		       const ATOOLS::Flavour &fli,
		       const ATOOLS::Flavour &flj):
  m_ij(ij), m_k(k)
{
}

CS_Gamma::CS_Gamma(CS_MCatNLO *const css,Shower *const shower,
		   CS_Cluster_Definitions *const cluster):
  p_css(css), p_shower(shower), p_cluster(cluster),
  m_on(0), m_oef(9.0)
{
}

Weight_Map CS_Gamma::CalculateWeight
(Cluster_Amplitude *const ampl,const int mode)
{
  Cluster_Amplitude *rampl(ampl->Copy());
#ifdef DEBUG__Trial_Weight
  DEBUG_FUNC(ampl);
  msg_Debugging()<<*rampl<<"\n";
#endif
  rampl->SetIdNew(0);
  std::map<size_t,size_t> idmap;
  for (size_t i(0);i<rampl->Legs().size();++i) {
    idmap[1<<i]=rampl->Leg(i)->Id();
    rampl->Leg(i)->SetId(1<<i);
  }
  Weight_Map ws;
  int stat(CalculateWeights(rampl,idmap,ws,mode));
  rampl->Delete();
  if (stat==-1) ws.clear();
  return ws;
}

int CS_Gamma::CalculateWeights(Cluster_Amplitude *const ampl,
			       const std::map<size_t,size_t> &idmap,
			       Weight_Map &ws,const int mode)
{
  Parton *const *cur(mode?NULL:p_shower->GetLast());
  Flavour_Vector cf(1,p_shower->GetOld(0)->Flav());
  for (size_t i(0);i<ampl->Legs().size();++i) {
    Cluster_Leg *li(ampl->Leg(i));
    for (size_t j(Max((size_t)2,i+1));j<ampl->Legs().size();++j) {
      Cluster_Leg *lj(ampl->Leg(j));
      for (size_t f(0);f<cf.size();++f) {
	for (size_t k(0);k<ampl->Legs().size();++k) {
	  Cluster_Leg *lk(ampl->Leg(k));
	  if (k==i || k==j) continue;
	  if (!CheckColors(li,lj,lk,cf[f])) continue;
	  if (cur) {
	    if (((idmap.find(li->Id())->second|
		  idmap.find(lj->Id())->second)&cur[0]->Id())==0 ||
		idmap.find(lk->Id())->second!=cur[2]->Id()) continue; 
	    if (((idmap.find(li->Id())->second|idmap.find(lj->Id())->second)&
		 (1<<(ampl->Legs().size()-1)))==0) continue; 
	  }
	  CS_Parameters cs(p_cluster->KT2(ampl,li,lj,lk,cf[f],p_ms));
	  if (cs.p_sf==NULL || !cs.p_sf->On()) continue;
	  Vec4D_Vector p(p_cluster->Combine(*ampl,i,j,k,cf[f],p_ms,cs.m_kin));
	  if (p.empty()) {
	    msg_Debugging()<<"combine failed for "<<ID(li->Id())<<"&"
			   <<ID(lj->Id())<<" <-> "<<ID(lk->Id())<<"\n";
	    continue;
	  }
	  ampl->SetKT2(cs.m_kt2);
	  ampl->SetZ(cs.m_z);
	  ampl->SetPhi(cs.m_phi);
	  ampl->SetKin(cs.m_kin);
	  Cluster_Amplitude *nampl(ampl->InitNext());
	  nampl->SetNIn(ampl->NIn());
	  nampl->SetMuF2(cs.m_kt2);
	  nampl->SetMuR2(ampl->MuR2());
	  nampl->ColorMap()=ampl->ColorMap();
	  nampl->Decays()=ampl->Decays();
	  nampl->SetProcs(ampl->Procs<void>());
	  nampl->SetDInfo(ampl->DInfo<void>());
	  Cluster_Leg *lijt(NULL), *lkt(NULL);
	  for (size_t l(0), m(0);l<ampl->Legs().size();++l) {
	    if (l==j) continue;
	    else if (l==i) {
	      nampl->CreateLeg(p[m],cf[f],CombineColors
			       (li,lj,lk,cf[f]),li->Id()|lj->Id());
	      nampl->Legs().back()->SetK(lk->Id());
	      lijt=nampl->Legs().back();
	    }
	    else {
	      Cluster_Leg *cl(ampl->Leg(l));
	      nampl->CreateLeg(p[m],cl->Flav(),cl->Col(),cl->Id());
	      if (cl==lk) lkt=nampl->Legs().back();
	    }
	    ++m;
	  }
	  std::string pname(Process_Base::GenerateName(ampl));
	  const DDip_Set &dinfo((*nampl->DInfo<StringDDipSet_Map>())[pname]);
	  if (dinfo.find(DDip_ID(i,j,k))!=dinfo.end()) {
	    int stat(SingleWeight(nampl,li,lj,lk,cs,idmap,ws,mode));
	    if (stat==-1) return -1;
	  }
	  ampl->DeleteNext();
	}
      }
    }
  }
  return 1;
}

int CS_Gamma::SingleWeight
(Cluster_Amplitude *const ampl,Cluster_Leg *const li,
 Cluster_Leg *const lj,Cluster_Leg *const lk,const CS_Parameters &cs,
 const std::map<size_t,size_t> &idmap,Weight_Map &ws,const int mode)
{
#ifdef DEBUG__Trial_Weight
  DEBUG_FUNC(ID(li->Id())<<","<<ID(lj->Id())<<"<->"<<ID(lk->Id()));
#endif
  Splitting_Function_Base *cdip(cs.p_sf);
  if (cdip==NULL) return 0;
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"B config -> "<<*ampl<<" -> "<<cs<<" ( "
		 <<cdip->GetFlavourA()<<" -> "<<cdip->GetFlavourB()
		 <<" "<<cdip->GetFlavourC()<<" )\n";
  Cluster_Amplitude *pampl(ampl);
  while ((pampl=pampl->Next())!=NULL) msg_Debugging()<<*pampl<<"\n";
#endif
  cdip->SetFlavourSpec((lk->Id()&((1<<ampl->NIn())-1))?
		       lk->Flav().Bar():lk->Flav());
  double eta=1.0, Q2=(cs.m_mode==1||cs.m_mode==2)?-cs.m_q2:cs.m_q2;
  if (cs.m_mode==1) eta=p_cluster->GetX(li,cdip)*cs.m_z;
  else if (cs.m_mode==2) eta=p_cluster->GetX(lk,cdip)*(1.0-cs.m_y);
  else if (cs.m_mode==3) eta=p_cluster->GetX(li,cdip)*cs.m_z;
  Weight_Value meps(Differential(ampl));
  meps.p_sf=cdip;
  meps.m_me*=cdip->SymFac()/
    cdip->AsymmetryFactor(cs.m_z,cs.m_y,Q2);
#ifdef DEBUG__Trial_Weight
  double me=meps.m_me;
#endif
  Color_Info ci(li->Col(),lj->Col(),lk->Col(),0);
  meps.m_me*=(*cdip)(cs.m_z,cs.m_y,eta,cs.m_kt2,Q2,ci,ampl)*
    cdip->MEPSWeight(cs.m_z,cs.m_y,eta,cs.m_kt2,Q2,ampl);
  if (meps.m_me==0.0) {
#ifdef DEBUG__Trial_Weight
    msg_Debugging()<<"zero matrix element\n";
#endif
    return 0;
  }
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"add ( z = "<<cs.m_z<<", y = "<<cs. m_y
		 <<", kt = "<<sqrt(cs.m_kt2)<<" ) {\n  "<<*li
		 <<"\n  "<<*lj<<"\n  "<<*lk<<"\n} -> w = "
		 <<me<<" * "<<meps.m_me/me<<" -> "<<meps.m_me
		 <<" ( S = "<<cdip->AsymmetryFactor(cs.m_z,cs.m_y,Q2)<<" )\n";
#endif
  ws[Weight_Key(idmap.find(li->Id())->second|idmap.find(lj->Id())->second,
		idmap.find(lk->Id())->second,cdip->GetFlavourA(),
		cdip->GetFlavourB(),cdip->GetFlavourC())]=meps;
  return 1;
}

bool CS_Gamma::Reject()
{
  if (p_css->PSMode()) {
    m_weight=1.0;
    return false;
  }
  if (m_on==0) return false;
  Cluster_Amplitude *rampl=p_css->GetRealEmissionAmplitude(1);
  Trial_Weight wgt(TrialWeight(rampl));
  rampl->Delete();
  if (wgt.MC()>ran->Get()) {
    m_weight=wgt.Accept();
    msg_Debugging()<<"w = "<<wgt.MC()<<" -> accept\n";
    return false;
  }
  m_weight=wgt.Reject();
  msg_Debugging()<<"w = "<<wgt.MC()<<" -> reject\n";
  return true;
}

Trial_Weight CS_Gamma::TrialWeight(Cluster_Amplitude *const ampl)
{
  DEBUG_FUNC("");
  p_ms=ampl->MS();
  p_shower->SetMS(p_ms);
  Weight_Map ws(CalculateWeight(ampl,0));
  if (ws.empty()) THROW(fatal_error,"Invalid amplitude");
  Parton *const *cur(p_shower->GetLast());
  size_t idij(0), idk(0);
  double wgt(0.0);
  Weight_Value wact;
  Weight_Map::const_iterator ait;
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"Accumulate weights {\n";
#endif
  for (Weight_Map::const_iterator
	 wit(ws.begin());wit!=ws.end();++wit) {
#ifdef DEBUG__Trial_Weight
    msg_Debugging()<<"  "<<wit->first<<" -> "<<wit->second;
#endif
    wgt+=wit->second.m_me;
    if ((wit->first.m_ij==(cur[0]->Id()|ampl->IdNew())) &&
	(wit->first.m_k==cur[2]->Id())) {
      ait=wit;
      wact=ait->second;
      idij=wit->first.m_ij;
      idk=wit->first.m_k;
#ifdef DEBUG__Trial_Weight
      msg_Debugging()<<" <- active";
#endif
    }
#ifdef DEBUG__Trial_Weight
    msg_Debugging()<<"\n";
#endif
  }
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"} -> w = "<<wgt<<"\n";
#endif
  if (!wact.p_sf || wact.m_me==-1.0)
    THROW(fatal_error,"No active splitting weight");
  ampl->SetMuF2(wact.m_muf2);
  ampl->SetMuR2(wact.m_mur2);
  int i(-1), j(-1), k(-1);
  for (size_t l(0);l<ampl->Legs().size();++l)
    if (ampl->Leg(l)->Id()&idk) k=l;
    else if (ampl->Leg(l)->Id()&idij) {
      if (i<0) i=l;
      else j=l;
    }
  std::string nadd("__QCD(S)_RS");
  nadd+=ToString(i)+"_"+ToString(j)+"_"+ToString(k);
  double rme(Differential(ampl,nlo_type::rsub,nadd).m_me);
  msg_Debugging()<<"me / ecss = "<<rme<<" / "<<wact.m_me
		 <<" = "<<rme/wact.m_me<<"\n";
  double h(wact.m_me), g(m_oef*rme);
  if (m_oef>0.0) g*=Max(1.0,h/dabs(rme));
  if (IsEqual(rme,h,1.0e-6) || rme==0.0) g=h;
  return Trial_Weight(rme,g,h);
}

void CS_Gamma::AddRBPoint(Cluster_Amplitude *const ampl)
{
  DEBUG_FUNC("");
  p_ms=ampl->MS();
  p_shower->SetMS(p_ms);
  Weight_Map ws(CalculateWeight(ampl,1));
  if (ws.empty()) return;
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"Accumulate weights {\n";
#endif
  for (Weight_Map::const_iterator
	 wit(ws.begin());wit!=ws.end();++wit) {
#ifdef DEBUG__Trial_Weight
    msg_Debugging()<<"  "<<wit->first<<" -> "<<wit->second<<"\n";
#endif
    int i(-1), j(-1), k(-1);
    for (size_t l(0);l<ampl->Legs().size();++l)
      if (ampl->Leg(l)->Id()&wit->first.m_k) k=l;
      else if (ampl->Leg(l)->Id()&wit->first.m_ij) {
	if (i<0) i=l;
	else j=l;
      }
    std::string nadd("__QCD(S)_RS"+ToString(i)+
		     "_"+ToString(j)+"_"+ToString(k));
    double rme(Differential(ampl,nlo_type::rsub,nadd).m_me);
    double wgt(wit->second.m_me);
    Process_Base *bproc(wit->second.p_proc);
    msg_Debugging()<<"  Set weight for '"<<bproc->Name()
		   <<"'"<<wit->first<<" -> ";
    msg_Debugging()<<"me / ecss = "<<rme<<" / "<<wgt
		   <<" = "<<rme/wgt<<"\n";
  }
#ifdef DEBUG__Trial_Weight
  msg_Debugging()<<"}\n";
#endif
}

Weight_Value CS_Gamma::Differential
(Cluster_Amplitude *const ampl,const nlo_type::code type,
 const std::string add) const
{
#ifndef DEBUG__Differential
  int olv(msg->Level());
  msg->SetLevel(2);
#endif
  NLOTypeStringProcessMap_Map *procs
    (ampl->Procs<NLOTypeStringProcessMap_Map>());
  Process_Base::SortFlavours(ampl);
  int rm(ampl->Leg(0)->Mom()[3]<0.0?0:1024);
  std::string pname(Process_Base::GenerateName(ampl));
  StringProcess_Map::const_iterator pit((*(*procs)[type]).find(pname+add));
  if (pit==(*(*procs)[nlo_type::lo]).end()) 
    THROW(fatal_error,"Process '"+pname+"' not found");
  Weight_Value meps(pit->second);
  meps.m_b=meps.m_me=pit->second->Differential(*ampl,2|4|rm);
  meps.m_me*=pit->second->SymFac();
  meps.m_muf2=ampl->MuF2();
  meps.m_mur2=ampl->MuR2();
#ifndef DEBUG__Differential
  msg->SetLevel(olv);
#endif
  return meps;
}

bool CS_Gamma::CheckColors
(const ATOOLS::Cluster_Leg *li,const ATOOLS::Cluster_Leg *lj,
 const ATOOLS::Cluster_Leg *lk,const ATOOLS::Flavour &mo) const
{
  if (mo.Strong()) {
    if (!lk->Flav().Strong()) return false;
  }
  else {
    if (lk->Flav().StrongCharge()==8) return false;
    if (li->Col().m_i==-1 && lj->Col().m_i==-1 &&
	lk->Col().m_i==-1) return true;
    ColorID ci(li->Col()), cj(lj->Col());
    if (ci.m_i==cj.m_j && ci.m_j==0 && cj.m_i==0) return true;
    if (ci.m_j==cj.m_i && ci.m_i==0 && cj.m_j==0) return true;
    return false;
  }
  if (li->Col().m_i==-1 && lj->Col().m_i==-1 &&
      lk->Col().m_i==-1) return true;
  ColorID ci(li->Col()), cj(lj->Col()), ck(lk->Col());
  if (ci.m_i<0 && cj.m_i<0 && ck.m_i<0) return true;
  if (li->Flav().StrongCharge()==3) {
    if (lj->Flav().StrongCharge()==-3) {
      if (lk->Flav().StrongCharge()==0) return true;
      if (ci.m_i==ck.m_j || cj.m_j==ck.m_i ||
	  (ci.m_i==cj.m_j && (ck.m_i>0 || ck.m_j>0))) return true;
    }
    else if (lj->Flav().StrongCharge()==8) {
      if (lk->Flav().StrongCharge()==0) return false;
      if (ci.m_i==cj.m_j && 
	  (cj.m_i==ck.m_j || ck.Singlet())) return true;
      if ((ci.m_i==ck.m_j || ck.Singlet()) && 
	  cj.Singlet()) return true;
    }
    else {
      if (lk->Flav().StrongCharge()==8) return false;
      return true;
    }
  }
  else if (li->Flav().StrongCharge()==-3) {
    if (lj->Flav().StrongCharge()==3) {
      if (lk->Flav().StrongCharge()==0) return true;
      if (ci.m_j==ck.m_i || cj.m_i==ck.m_j ||
	  (ci.m_j==cj.m_i && (ck.m_i>0 || ck.m_j>0))) return true;
    }
    else if (lj->Flav().StrongCharge()==8) {
      if (lk->Flav().StrongCharge()==0) return false;
      if (ci.m_j==cj.m_i && 
	  (cj.m_j==ck.m_i || ck.Singlet())) return true;
      if ((ci.m_j==ck.m_i || ck.Singlet()) && 
	  cj.Singlet()) return true;
    }
    else {
      if (lk->Flav().StrongCharge()==8) return false;
      return true;
    }
  }
  else if (li->Flav().StrongCharge()==8) {
    if (lk->Flav().StrongCharge()==0) return false;
    if (lj->Flav().StrongCharge()==8) {
      if (ci.m_i==cj.m_j && 
	  (ci.m_j==ck.m_i || cj.m_i==ck.m_j ||
	   (ci.m_j==cj.m_i && lk->Flav().StrongCharge()!=8))) 
	return true;
      if (ci.m_j==cj.m_i && 
	  (ci.m_i==ck.m_j || cj.m_j==ck.m_i ||
	   (ci.m_i==cj.m_j && lk->Flav().StrongCharge()!=8)))
	return true;
    }
    else if (lj->Flav().StrongCharge()==3) {
      if (ci.m_j==cj.m_i &&
	  (ci.m_i==ck.m_j || ck.Singlet())) return true;
      if ((cj.m_i==ck.m_j || ck.Singlet()) &&
	  ci.Singlet()) return true;
    }
    else if (lj->Flav().StrongCharge()==-3) {
      if (ci.m_i==cj.m_j &&
	  (ci.m_j==ck.m_i || ck.Singlet())) return true;
      if ((cj.m_j==ck.m_i || ck.Singlet()) &&
	  ci.Singlet()) return true;
    }
    else {
      return false;
    }
  }
  else {
    if (lj->Flav().StrongCharge()==8 ||
	lk->Flav().StrongCharge()==8) {
      return false;
    }
    return true;
  }
  return false;
}

ColorID CS_Gamma::CombineColors
(const Cluster_Leg *li,const Cluster_Leg *lj,const Cluster_Leg *lk,
 const ATOOLS::Flavour &mo) const
{
  ColorID ci(li->Col()), cj(lj->Col()), ck(lk->Col());
  if (ci.m_i==-1 && cj.m_i==-1 && ck.m_i==-1) return ColorID();
  if (!mo.Strong()) return ColorID(0,0);
  if (li->Flav().StrongCharge()==3) {
    if (lj->Flav().StrongCharge()==-3) {
      return ColorID(ci.m_i,cj.m_j);
    }
    else if (lj->Flav().StrongCharge()==8) {
      if (cj.Singlet()) return ColorID(ci.m_i,0);
      return ColorID(cj.m_i,0);
    }
    else {
      return ColorID(ci.m_i,0);
    }
  }
  else if (li->Flav().StrongCharge()==-3) {
    if (lj->Flav().StrongCharge()==3) {
      return ColorID(cj.m_i,ci.m_j);
    }
    else if (lj->Flav().StrongCharge()==8) {
      if (cj.Singlet()) return ColorID(0,ci.m_j);
      return ColorID(0,cj.m_j);
    }
    else {
      return ColorID(0,ci.m_j);
    }
  }
  else if (li->Flav().StrongCharge()==8) {
    if (lj->Flav().StrongCharge()==8) {
      if (ci.m_i==cj.m_j && 
	  (ci.m_j==ck.m_i || cj.m_i==ck.m_j ||
	   (ci.m_j==cj.m_i && lk->Flav().StrongCharge()!=8))) 
	return ColorID(cj.m_i,ci.m_j);
      if (ci.m_j==cj.m_i && 
	  (ci.m_i==ck.m_j || cj.m_j==ck.m_i ||
	   (ci.m_i==cj.m_j && lk->Flav().StrongCharge()!=8)))
	return ColorID(ci.m_i,cj.m_j);
      THROW(fatal_error,"Invalid clustering");
    }
    else if (lj->Flav().StrongCharge()==3) {
      if (ci.Singlet()) return ColorID(cj.m_i,0);
      return ColorID(ci.m_i,0);
    }
    else if (lj->Flav().StrongCharge()==-3) {
      if (ci.Singlet()) return ColorID(0,cj.m_j);
      return ColorID(0,ci.m_j);
    }
    else {
      THROW(fatal_error,"Invalid combination");
    }
  }
  else {
    if (lj->Flav().StrongCharge()==3) {
      return ColorID(cj.m_i,0);
    }
    else if (lj->Flav().StrongCharge()==-3) {
      return ColorID(0,cj.m_j);
    }
    else {
      return ColorID(0,0);
    }
  }
  return ColorID();
}

namespace MCATNLO {

  std::ostream &operator<<(std::ostream &str,const Weight_Key &k)
  {
    return str<<"["<<ATOOLS::ID(k.m_ij)<<","<<ATOOLS::ID(k.m_k)<<"]";
  }

  std::ostream &operator<<(std::ostream &str,const Weight_Value &w)
  {
    return str<<w.m_me<<"  "<<w.p_proc->Name()<<" [ "
	      <<w.p_sf->GetFlavourA()<<" -> "<<w.p_sf->GetFlavourB()
	      <<" "<<w.p_sf->GetFlavourC()<<" ] ( \\mu_F = "
	      <<sqrt(w.m_muf2)<<", \\mu_R = "<<sqrt(w.m_mur2)<<" ) ";
  }

}
