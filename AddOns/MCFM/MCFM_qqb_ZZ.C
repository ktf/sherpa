#include "PHASIC++/Process/Virtual_ME2_Base.H"
#include "AddOns/MCFM/MCFM_Wrapper.H"

namespace MCFM {
  class MCFM_qqb_ZZ: public PHASIC::Virtual_ME2_Base {
  private:
    int     m_pID;
    double  m_aqed,m_cplcorr, m_normcorr;
    bool    m_change_order;
    double *p_p, *p_msqv;

    double CallMCFM(const int & i,const int & j);
  public:
    MCFM_qqb_ZZ(const int & pID,const PHASIC::Process_Info& pi,
		const ATOOLS::Flavour_Vector& flavs, bool change_order);
    ~MCFM_qqb_ZZ();
    void Calc(const ATOOLS::Vec4D_Vector& momenta);
    double Eps_Scheme_Factor(const ATOOLS::Vec4D_Vector& mom);
  };

}// end of namespace MCFM

extern "C" { 
  void qqb_zz_v_(double *p,double *msqv); 
}

#include "MODEL/Main/Model_Base.H"
#include "ATOOLS/Org/Run_Parameter.H"

using namespace MCFM;
using namespace PHASIC;
using namespace ATOOLS;

MCFM_qqb_ZZ::MCFM_qqb_ZZ(const int & pID,const PHASIC::Process_Info& pi,
			 const Flavour_Vector& flavs, bool change_order):
  Virtual_ME2_Base(pi,flavs), m_pID(pID),
  m_aqed(MODEL::s_model->ScalarFunction(std::string("alpha_QED"))),
  m_cplcorr(ATOOLS::sqr(4.*M_PI*m_aqed/ewcouple_.esq)),
  m_normcorr(4.*9./qcdcouple_.ason2pi),
  m_change_order(change_order)
 {
  rpa->gen.AddCitation
    (1,"The NLO matrix elements have been taken from MCFM \\cite{Campbell:1999ah}.");
  p_p = new double[4*MCFM_NMX];
  p_msqv = new double[sqr(2*MCFM_NF+1)];
  m_drmode=m_mode=1;
  if (m_pID==82) m_normcorr /= 3.;
}

MCFM_qqb_ZZ::~MCFM_qqb_ZZ()
{
  delete [] p_p;
  delete [] p_msqv;
}

double MCFM_qqb_ZZ::CallMCFM(const int & i,const int & j) {
  qqb_zz_v_(p_p,p_msqv); 
  return p_msqv[mr(i,j)];
}

void MCFM_qqb_ZZ::Calc(const Vec4D_Vector &p)
{
  double corrfactor(m_cplcorr*m_normcorr);
  if (m_pID==87) corrfactor*=1./3.;
  
  for (int n(0);n<2;++n) GetMom(p_p,n,-p[n]);
  if (m_change_order){
    GetMom(p_p,2,p[4]); 
    GetMom(p_p,3,p[5]); 
    GetMom(p_p,4,p[2]); 
    GetMom(p_p,5,p[3]);   
  }
  else if (m_pID==81 || m_pID==82) {
    GetMom(p_p,4,p[2]); 
    GetMom(p_p,2,p[3]); 
    GetMom(p_p,5,p[4]);  
    GetMom(p_p,3,p[5]); 
  }
  else{
    GetMom(p_p,2,p[2]); 
    GetMom(p_p,3,p[3]); 
    GetMom(p_p,4,p[4]); 
    GetMom(p_p,5,p[5]); 
  }

  long int i(m_flavs[0]), j(m_flavs[1]);
  if (i==21) { i=0; }
  if (j==21) { j=0; }
  scale_.musq=m_mur2;
  scale_.scale=sqrt(scale_.musq);

  epinv_.epinv=epinv2_.epinv2=0.0;
  double res(CallMCFM(i,j)  * corrfactor);
  epinv_.epinv=1.0;
  double res1(CallMCFM(i,j) * corrfactor);
  epinv2_.epinv2=1.0;
  double res2(CallMCFM(i,j) * corrfactor);

  m_res.Finite() = res;
  m_res.IR()     = (res1-res);
  m_res.IR2()    = (res2-res1);
}

double MCFM_qqb_ZZ::Eps_Scheme_Factor(const ATOOLS::Vec4D_Vector& mom)
{
  return 4.*M_PI;
}

extern "C" { void chooser_(); }

DECLARE_VIRTUALME2_GETTER(MCFM_qqb_ZZ,"MCFM_qqb_ZZ")
Virtual_ME2_Base *ATOOLS::Getter
<Virtual_ME2_Base,Process_Info,MCFM_qqb_ZZ>::
operator()(const Process_Info &pi) const
{
  DEBUG_FUNC("");
  if (MODEL::s_model->Name()!=std::string("SM")) return NULL;
  if (pi.m_loopgenerator!="MCFM")                       return NULL;
  if (pi.m_fi.m_nloewtype!=nlo_type::lo)                return NULL;
  if (!pi.m_fi.m_nloqcdtype&nlo_type::loop)             return NULL;
  Flavour_Vector fl(pi.ExtractFlavours());
  // check for right flavour structure:
  // - two incoming quarks + 4 particle FS
  // - assume two propagators - check for alternatives (i.e. 4 lepton FS) later
  // - have W/Z
  if (!(fl[0].IsQuark() && fl[1].IsQuark()))            return NULL;
  if (fl.size()!=6)                                     return NULL;
  if (!(fl[2]==fl[3].Bar() && fl[4]==fl[5].Bar())
      && !(fl[2]==fl[4].Bar() && fl[3]==fl[5].Bar()))   return NULL;
  if (!(fl[2].IsLepton() && fl[3].IsLepton() &&
	fl[4].IsLepton() && fl[5].IsLepton())) {
    msg_Error()<<"Error in "<<METHOD<<":\n"
	       <<"   ZZ production for lepton/neutrino final states only.\n";
    THROW(fatal_error,"Not yet working."); 
  }
  int pID(0);
  bool change_order(false);
  if (pi.m_fi.m_ps.size()==2 &&
      pi.m_fi.m_ps[0].m_fl[0].Kfcode()==23 && pi.m_fi.m_ps[0].m_fl[1].Kfcode()==23) {
    if (Flavour(kf_b).Yuk()>0.) {
      msg_Error()<<"Error in "<<METHOD<<":\n"
		 <<"   Must switch off Yukawa couplings of b quarks.\n";
      THROW(fatal_error,"Wrong model assupmtions."); 
    }
    if (fl[2].IsDowntype() && fl[4].IsUptype())   pID = 87;
    if (fl[2].IsUptype() && fl[4].IsDowntype())   pID = 87;
    if (fl[2].IsDowntype() && fl[4].IsDowntype()) pID = 86;
    if (fl[2].IsUptype() && fl[4].IsDowntype())   change_order=true;
  }
  else if (pi.m_fi.m_ps.size()==4) {
    if (fl[2]==fl[3]){
      msg_Error()<<"Error in "<<METHOD<<":\n"
		 <<"   No interference allowed in this process.\n";
      THROW(fatal_error,"Not working."); 
    }
    if (abs(fl[2].Kfcode()-fl[3].Kfcode())==1){
      msg_Error()<<"Error in "<<METHOD<<":\n"
		 <<"   This final state not possible due to Z/W interference.\n";
      THROW(fatal_error,"Not yet working."); 
    }
    else if (fl[2].IsDowntype() && fl[3].IsDowntype()) pID=81;
    else if (fl[2].IsDowntype() && fl[3].IsUptype()) pID=82; 
    if (pID==82){
      msg_Error()<<"Error in "<<METHOD<<":\n"
    		 <<"   Not working for ll nu nu final state.\n";
      THROW(fatal_error,"Not yet working."); 
    }
  }
  
  if (pID!=0) {
    if (nproc_.nproc>=0) {
      if (nproc_.nproc!=pID)
	THROW(not_implemented,
	      "Only one process class allowed when using MCFM");
    }
    nproc_.nproc=pID;
    if (pID==86 || pID==87) zerowidth_.zerowidth=true;
    if (pID==81 || pID==82) zerowidth_.zerowidth=false;
    chooser_();
    msg_Info()<<"Initialise MCFM with nproc = "<<nproc_.nproc<<"\n";
    return new MCFM_qqb_ZZ(pID,pi,fl,change_order);
  } 
  return NULL;
}

