#ifndef MODEL_Interaction_Models_Interaction_Model_sQCD_H
#define MODEL_Interaction_Models_Interaction_Model_sQCD_H

#include "MODEL/Interaction_Models/Interaction_Model_Base.H"
#include "ATOOLS/Math/Kabbala.H"

namespace MODEL {  
  class Interaction_Model_sQCD : public Interaction_Model_Base {
    ATOOLS::Kabbala g1,g3,PL,PR,M_I,root2,num_2;    
    ATOOLS::Kabbala K_Z_U(short int i,short int j);
    ATOOLS::Kabbala K_Z_D(short int i,short int j);
    int gen_sUp(ATOOLS::Flavour fl);
    int gen_sDown(ATOOLS::Flavour fl);
  public:
    Interaction_Model_sQCD(MODEL::Model_Base *,std::string,std::string);
    void c_FFV(std::vector<Single_Vertex>&,int &);
    void c_VVV(std::vector<Single_Vertex>&,int &)  {}
    void c_VVVV(std::vector<Single_Vertex>&,int &) {}

    void c_FFS(std::vector<Single_Vertex>&,int &);
    void c_SSV(std::vector<Single_Vertex>&,int &);
    void c_VVS(std::vector<Single_Vertex>&,int &)  {}
    void c_SSS(std::vector<Single_Vertex>&,int &)  {} 
    void c_SSVV(std::vector<Single_Vertex>&,int &);
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

