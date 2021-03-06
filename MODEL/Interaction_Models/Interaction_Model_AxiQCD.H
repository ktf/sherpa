#ifndef Interaction_Model_AxiQCD_h
#define Interaction_Model_AxiQCD_h

#include "MODEL/Interaction_Models/Interaction_Model_Base.H"
#include "MODEL/Interaction_Models/Interaction_Model_QCD.H"
#include "ATOOLS/Math/Kabbala.H"

namespace MODEL {  
  class Interaction_Model_AxiQCD : public Interaction_Model_Base {
    Interaction_Model_QCD    * p_moqcd;
    ATOOLS::Kabbala g3,PL,PR,M_I;    
  public:
    Interaction_Model_AxiQCD(MODEL::Model_Base *,std::string,std::string);
    void c_FFV(std::vector<Single_Vertex>&,int &);
    void c_VVV(std::vector<Single_Vertex>&,int &);
    void c_VVVV(std::vector<Single_Vertex>&,int &);

    void c_FFS(std::vector<Single_Vertex>&,int &)  {}
    void c_SSV(std::vector<Single_Vertex>&,int &)  {}
    void c_VVS(std::vector<Single_Vertex>&,int &)  {}
    void c_SSS(std::vector<Single_Vertex>&,int &)  {}
    void c_SSVV(std::vector<Single_Vertex>&,int &) {}
    void c_SSSS(std::vector<Single_Vertex>&,int &) {}
  
    void c_FFT(std::vector<Single_Vertex>&,int&)  {}
    void c_VVT(std::vector<Single_Vertex>&,int&)  {} 
    void c_SST(std::vector<Single_Vertex>&,int&)  {} 
    void c_VVVT(std::vector<Single_Vertex>&,int&)  {} 
    void c_FFVT(std::vector<Single_Vertex>&,int&)  {} 
    void c_SSST(std::vector<Single_Vertex>&,int&)  {} 
  };
}

#endif


