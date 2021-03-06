#include "MODEL/Main/LesHouches_Interface.H"
#include "ATOOLS/Org/Run_Parameter.H"
#include "ATOOLS/Org/Message.H"
#include "ATOOLS/Org/MyStrStream.H"
#include "ATOOLS/Math/MathTools.H"
#include "MODEL/Main/Running_AlphaQED.H"
#include "MODEL/Main/Running_AlphaS.H"
#include "ATOOLS/Org/Exception.H"
#include "ATOOLS/Org/Shell_Tools.H"
#include "PDF/Main/ISR_Handler.H"

using namespace MODEL;
using namespace ATOOLS;
using namespace std;


LesHouches_Interface::LesHouches_Interface(Data_Reader * _dataread,
					   Model_Base * _model, std::string _dir) :
  Spectrum_Generator_Base(_dataread,_model),m_dir(_dir) { }



LesHouches_Interface::~LesHouches_Interface() 
{ 
  exh->RemoveTerminatorObject(this);
}



void LesHouches_Interface::PrepareTerminate()
{
  std::string path(rpa->gen.Variable("SHERPA_STATUS_PATH")+"/");
  if (path=="/") return;
  Copy(m_dir+"/"+m_inputfile,path+m_inputfile);
}

void LesHouches_Interface::Run(const PDF::ISR_Handler_Map& isr) {
  m_inputfile = p_dataread->GetValue<std::string>("SLHA_INPUT",std::string("LesHouches.dat"));

  msg_Tracking()<<"================================================================ "<<std::endl;
  msg_Tracking()<<"MSSM spectrum generated according to the SUSY Les Houches Accord! "<<std::endl;
  msg_Tracking()<<"Les Houches input file is: "<<m_dir+m_inputfile<<std::endl;
  
  p_reader = new ATOOLS::Data_Reader(" ",";","#","=");
  p_reader->AddWordSeparator("\t");
  p_reader->SetAddCommandLine(false);
  p_reader->SetInputPath(m_dir);
  p_reader->SetInputFile(m_inputfile);
  if (p_reader->InputPath()=="" && p_reader->InputFile()=="")
    msg_Error()<<"ERROR in LesHouches_Interface::Run : "
	       <<"The input file for the SLHA has not been set correctly!"<<std::endl;
  
  p_reader->SetIgnoreCase(true);
  p_reader->AddFileEnd(std::string("Block"));
  
  p_reader->OpenInFile();
  
  if (p_reader->InFileMode()==3) 
    throw(ATOOLS::Exception(ATOOLS::ex::fatal_error,
			    "The SLHA input file "+m_dir+m_inputfile+" could not be read!",
			    "LesHouches_Interface","Run"));
    
  Info();
  
  //SetSMInput();
  SetMasses();
  if (p_dataread->GetValue<int>("LESHOUCHES_WIDTHS",1)) SetWidths();
  SetHiggsParameters();
  SetInoParameters();
  SetSquarkParameters();
  SetSleptonParameters();
  
  msg_Tracking()<<"================================================================ "<<std::endl;

  delete p_reader;
  exh->AddTerminatorObject(this);
}

void LesHouches_Interface::Info() {
  
  msg_Tracking()<<std::endl<<"  Reading Block SPINFO: "<<std::endl;
  
  std::vector<std::vector<std::string> > vs;
  p_reader->SetFileBegin(std::string("Block SPINFO"));
  p_reader->RereadInFile();
  if(p_reader->MatrixFromFile(vs,"")) {
  
    for (unsigned int i=0;i<vs.size();++i) {
      if (vs[i][0]=="1") msg_Tracking()<<"   The Spectrum was generated by "<<vs[i][1];
      if (vs[i][0]=="2") msg_Tracking()<<" version "<<vs[i][1]<<std::endl;
      if (vs[i][0]=="3") msg_Tracking()<<"   A warning was produced saying "<<vs[i][1]<<std::endl;
    }
  }
  else msg_Tracking()<<"Could not read Block SPINFO from SLHA file: "<<m_dir+m_inputfile<<std::endl;
  
  std::vector<std::vector<int> > vi;
  p_reader->SetFileBegin(std::string("Block MODSEL"));
  p_reader->RereadInFile();
  if (p_reader->MatrixFromFile(vi,"")) {
    for (unsigned int i=0;i<vi.size();++i) {
      if (vi[i][0]==1) {
      if (vi[i][1]==0) msg_Tracking()<<"   Consider a general MSSM simulation!"<<std::endl;
      if (vi[i][1]==1) msg_Tracking()<<"   A (m)SUGRA scenario is considered!"<<std::endl;
      if (vi[i][1]==2) msg_Tracking()<<"   A (m)GMSB  scenario is considered!"<<std::endl;
      if (vi[i][1]==3) msg_Tracking()<<"   A (m)AMSB  scenario is considered!"<<std::endl;
      }
      else {
	msg_Tracking()<<"   Switch "<<vi[i][0]<<" in Block MODSEL not yet supported."<<std::endl;
      }
    }
  }
  else msg_Error()<<std::endl<<"Could not read Block MODSEL from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
}

void LesHouches_Interface::SetSMInput(const PDF::ISR_Handler_Map& isr) {
  
  msg_Tracking()<<std::endl<<"  Reading Block SMINPUTS: "<<std::endl;

  std::vector<std::vector<double> >      vd;
  p_reader->SetFileBegin(std::string("Block SMINPUTS"));
  p_reader->RereadInFile();
  
  if(p_reader->MatrixFromFile(vd,"")) {
    double alphaQED=0., MZ=0., GF=0.,alphaS=0., sin2TW=0., MW=0.;
    for (unsigned int i=0;i<vd.size();++i) {
      if (vd[i][0]==1) alphaQED = 1./vd[i][1]; 
      if (vd[i][0]==2) GF       = vd[i][1]; 
      if (vd[i][0]==3) alphaS   = vd[i][1]; 
      if (vd[i][0]==4) MZ       = vd[i][1]; 
    }
    MW = sqrt((sqr(MZ)/2.)+sqrt(pow(MZ,4.)/4.-sqr(MZ)*(alphaQED*M_PI)/(sqrt(2.)*GF)));
    sin2TW = 1.-sqr(MW/MZ);
        
    aqed = new Running_AlphaQED(1./137.0359895);
    aqed->SetDefault(alphaQED);
    
    p_model->GetScalarFunctions()->insert(std::make_pair(std::string("alpha_QED"),aqed));
    
    int    order_alphaS	= p_dataread->GetValue<int>("ORDER_ALPHAS",1);
    int    th_alphaS	= p_dataread->GetValue<int>("THRESHOLD_ALPHAS",1);
    double alphaS_default = p_dataread->GetValue<double>("ALPHAS(default)",alphaS);
    
    as = new Running_AlphaS(alphaS,sqr(MZ),order_alphaS,th_alphaS,isr);
    as->SetDefault(alphaS_default);
  
    p_model->GetScalarFunctions()->insert(std::make_pair(std::string("alpha_S"),as));
    
    (*p_model->GetScalarConstants())["alpha_S(MZ)"]=alphaS;
    (*p_model->GetScalarConstants())["sin2_thetaW"]=sin2TW;
    (*p_model->GetScalarConstants())["cos2_thetaW"]=1.-sin2TW;
    (*p_model->GetScalarConstants())["MW"]=MW;
    (*p_model->GetScalarConstants())["MZ"]=MZ;
    (*p_model->GetScalarConstants())["GF"]=GF; 
  }
  else {
    msg_Tracking()<<std::endl<<"Could not read Block SMINPUTS from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
  }
}

void LesHouches_Interface::SetMasses() {

  msg_Tracking()<<std::endl<<"  Reading Block MASS: "<<std::endl;
  std::vector<std::vector<double> >   vd;
  p_reader->SetFileBegin(std::string("Block MASS"));
  p_reader->RereadInFile();
   
  if(p_reader->MatrixFromFile(vd,"")) {
    Flavour flav;    
    for (unsigned int i=0;i<vd.size();++i) {
      flav.FromHepEvt(int(vd[i][0]));
      if (flav!=Flavour(kf_Wplus)) {
	flav.SetMass(dabs(vd[i][1]));
	flav.SetHadMass(dabs(vd[i][1]));
      }
      if (vd[i][1]<0) flav.SetMassSign(-1);
      if (flav!=Flavour(kf_Wplus))
	msg_Tracking()<<"   Set mass of "<<flav<<" to "<<vd[i][1]<<std::endl;
    }
  }
  else msg_Error()<<std::endl<<"Could not read Block MASS from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
}

void LesHouches_Interface::SetWidths() {
  msg_Tracking()<<std::endl<<"  Reading decay data: "<<std::endl;
  p_reader->SetFileEnd("dummy");
  std::vector<std::vector<std::string> >   vds;
  
  p_reader->SetIgnoreCase(false);
  
  //
  p_reader->SetFileBegin(std::string("DECAY"));
  p_reader->RereadInFile();
  //
  p_reader->MatrixFromFile(vds,"");

  if (vds.size()>0) vds.front().insert(vds.front().begin(),"DECAY");
  
  for (size_t k=0;k<vds.size();++k) {
    if (vds[k].size()>0) {
      if (vds[k][0]=="DECAY") {
       Flavour flav;    
       flav.FromHepEvt(ATOOLS::ToType<int>(vds[k][1]));
       if (flav!=Flavour(kf_Wplus) || flav!=Flavour(kf_Z)) {
         flav.SetWidth(ATOOLS::ToType<double>(vds[k][2]));
         msg_Tracking()<<"   Set width of "<<flav<<" to "<<flav.Width()<<std::endl;
         else {
           msg_Tracking()<<"   Do not change width of EW gauge bosons : "<<flav<<" at "<<flav.Width()<<std::endl;
         }
       }
      }
    }
  }
  p_reader->SetIgnoreCase(true);
  p_reader->AddFileEnd("Block"); 
}


void LesHouches_Interface::SetHiggsParameters() {

  msg_Tracking()<<std::endl<<"  Reading Block alpha and Block hmix: "<<std::endl;
  double alpha=0.,mu=0.,tanb=0.,vev=0.;
  std::vector<std::vector<double> >   vd;
  std::vector<std::vector<double> >   vd2;
  
  p_reader->SetFileBegin(std::string("Block MINPAR"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd2,"")) 
    msg_Error()<<std::endl<<"Could not read Block MINPAR from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;  
  
  p_reader->SetFileBegin(std::string("Block alpha"));
  p_reader->RereadInFile();
  if (!p_reader->ReadFromFile(alpha,"")) 
    msg_Error()<<std::endl<<"Could not read Block alpha from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  p_reader->SetFileBegin(std::string("Block hmix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,"")) 
    msg_Error()<<std::endl<<"Could not read Block hmix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;

  for (unsigned int i=0;i<vd2.size();++i) {
    if (vd2[i][0]==3) {
      tanb=vd2[i][1];
      msg_Tracking()<<"   tanb : "<<tanb<<std::endl;
    }
  }

  for (unsigned int i=0;i<vd.size();++i) {
    if (vd[i][0]==1) {
      mu    =vd[i][1];
      msg_Tracking()<<"   Mu   : "<<mu<<std::endl; 
    }
    /*
    //if running tanb shall be used  
    if (vd[i][0]==2) {
    tanb=vd[i][1];
    msg_Tracking()<<"   tanb : "<<tanb<<std::endl;
    }
    */
    if (vd[i][0]==3) vev =vd[i][1];
  }    
 
  double sina = ::sin(alpha);
  double cosa = cos(alpha);
  
  CMatrix ZR  = CMatrix(2);
  
  ZR[0][0]    = Complex(-sina,0.);
  ZR[0][1]    = Complex(cosa,0.);
  ZR[1][0]    = Complex(cosa,0.);
  ZR[1][1]    = Complex(sina,0.);
  
  double cosb = sqrt(1./(1.+sqr(tanb)));
  double sinb = cosb*tanb;  

  CMatrix ZH  = CMatrix(2);
  
  ZH[0][0]    = Complex(sinb,0.);
  ZH[0][1]    = Complex(-cosb,0.);
  ZH[1][0]    = Complex(cosb,0.);
  ZH[1][1]    = Complex(sinb,0.);

  msg_Tracking()<<"   ZH is : "<<std::endl;
  for (unsigned int i=0;i<2;++i) {
    for (unsigned int j=0;j<2;++j) {
      msg_Tracking()<<"    "<<ZH[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  msg_Tracking()<<"   ZR is : "<<std::endl;
  for (unsigned int i=0;i<2;++i) {
    for (unsigned int j=0;j<2;++j) {
      msg_Tracking()<<"    "<<ZR[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
   
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_R"),ZR));
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_H"),ZH));
  
  (*p_model->GetScalarConstants())["mu"]=mu;
  (*p_model->GetScalarConstants())["tan(beta)"]=tanb;

  //better leave out setting the vev, 
  //it is determined by MZ already
  //(*p_model->GetScalarConstants())["vev"]=vev;
}

void LesHouches_Interface::SetInoParameters() {

  msg_Tracking()<<std::endl<<"  Reading Block nmix, Umix and Vmix"<<std::endl;
  std::vector<std::vector<double> >   vd;
  p_reader->SetFileBegin(std::string("Block nmix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block nmix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  Matrix<4> NMix;
  for (unsigned int k=0;k<vd.size();++k) {
    NMix[int(vd[k][0])-1][int(vd[k][1])-1]=vd[k][2];
  }
  CMatrix ZN = CMatrix(4);
  //transposed with respect to SLHA
  for (unsigned int i=0;i<4;i++) 
    for (unsigned int j=0;j<4;j++) ZN[i][j] = NMix[j][i];
  
  msg_Tracking()<<"   ZN is : "<<std::endl;
  for (unsigned int i=0;i<4;++i) {
    for (unsigned int j=0;j<4;++j) {
      msg_Tracking()<<"    "<<ZN[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  

  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z^N"),ZN));
  vd.clear();

  p_reader->SetFileBegin(std::string("Block Umix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block Umix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  Matrix<2> UMix;
  for (unsigned int k=0;k<vd.size();++k) {
    UMix[int(vd[k][0])-1][int(vd[k][1])-1]=vd[k][2];
  }
  //transposed with respect to SLHA
  CMatrix Zminus = CMatrix(2);
  for (unsigned int i=0;i<2;++i) 
    for (unsigned int j=0;j<2;++j) Zminus[i][j]=UMix[j][i];
    
  msg_Tracking()<<"   Zminus is : "<<std::endl;
  for (unsigned int i=0;i<2;++i) {
    for (unsigned int j=0;j<2;++j) {
      msg_Tracking()<<"    "<<Zminus[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z^-"),Zminus));	 
  vd.clear();

  p_reader->SetFileBegin(std::string("Block Vmix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block Vmix from SLHA file: "<<
      m_dir+m_inputfile<<std::endl;

  Matrix<2> VMix;
  for (unsigned int k=0;k<vd.size();++k) {
    VMix[int(vd[k][0])-1][int(vd[k][1])-1]=vd[k][2];
  }
  //transposed with respect to SLHA
  CMatrix Zplus = CMatrix(2);
  for (unsigned int i=0;i<2;++i)
    for (unsigned int j=0;j<2;++j) Zplus[i][j]=VMix[j][i];
    
  msg_Tracking()<<"   Zplus is : "<<std::endl;
  for (unsigned int i=0;i<2;++i) {
    for (unsigned int j=0;j<2;++j) {
      msg_Tracking()<<"    "<<Zplus[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z^+"),Zplus));	 
}

void LesHouches_Interface::SetSquarkParameters() {
  
  msg_Tracking()<<std::endl<<"  Reading Block stopmix, sbotmix, au, ad "<<std::endl;
  std::vector<std::vector<double> >   vd;
  //stops
  p_reader->SetFileBegin(std::string("Block stopmix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block stopmix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  //transposed with respect to SLHA
  CMatrix Zu = CMatrix(6);
  for (short int i=0;i<6;i++) {
    for (short int j=0;j<6;j++) Zu[i][j] = Complex(0.,0.);
    Zu[i][i] = Complex(1.,0.);
  }    
  
  for (unsigned int i=0;i<vd.size();++i) {
    if (vd[i][0]==1) {
      if (vd[i][1]==1) Zu[2][2]=vd[i][2];
      if (vd[i][1]==2) Zu[5][2]=vd[i][2];
    }
    if (vd[i][0]==2) {
      if (vd[i][1]==1) Zu[2][5]=vd[i][2];
      if (vd[i][1]==2) Zu[5][5]=vd[i][2];
    }
  }
  
  msg_Tracking()<<"   Zu is : "<<std::endl;
  for (unsigned int i=0;i<6;++i) {
    for (unsigned int j=0;j<6;++j) {
      msg_Tracking()<<"    "<<Zu[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_u"),Zu));
  vd.clear();
  
  CMatrix ws = CMatrix(3);
  CMatrix us = CMatrix(3);
  for (short int i=0;i<2;i++) 
    for (short int j=0;j<2;j++) ws[i][j] = Complex(0.,0.);   
    
  for (short int i=0;i<3;i++) 
    for (short int j=0;j<3;j++) us[i][j] = Complex(0.,0.);
  
  double v2 = (*p_model->GetScalarConstants())["vev"] * 
    (*p_model->GetScalarConstants())["tan(beta)"] * 
    sqrt(1./(1.+sqr((*p_model->GetScalarConstants())["tan(beta)"])));
  double YU = Flavour((kf_code)(6)).Yuk()/v2*sqrt(2.);
  
  p_reader->SetFileBegin(std::string("Block au"));
  p_reader->RereadInFile();  

  if (p_reader->MatrixFromFile(vd,"")) {
    for (unsigned int i=0;i<vd.size();++i) {
      if (vd[i][0]==3 && vd[i][1]==3) us[2][2]=-vd[i][2]*YU;
    }
  }
  else msg_Error()<<std::endl<<"Could not read Block au from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
  
  msg_Tracking()<<"   ws is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<ws[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  msg_Tracking()<<"   us is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<us[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("ws"),ws));
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("us"),us));
  vd.clear();
    
  //sbottoms
  p_reader->SetFileBegin(std::string("Block sbotmix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block sbotmix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  CMatrix Zd = CMatrix(6);
  for (short int i=0;i<6;i++) {
    for (short int j=0;j<6;j++) Zd[i][j] = Complex(0.,0.);
    Zd[i][i] = Complex(1.,0.);
  }    
  
  for (unsigned int i=0;i<vd.size();++i) {
    if (vd[i][0]==1) {
      if (vd[i][1]==1) Zd[2][2]=vd[i][2];
      if (vd[i][1]==2) Zd[5][2]=vd[i][2];
    }
    if (vd[i][0]==2) {
      if (vd[i][1]==1) Zd[2][5]=vd[i][2];
      if (vd[i][1]==2) Zd[5][5]=vd[i][2];
    }
  }
  
  msg_Tracking()<<"   Zd is : "<<std::endl;
  for (unsigned int i=0;i<6;++i) {
    for (unsigned int j=0;j<6;++j) {
      msg_Tracking()<<"    "<<Zd[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_d"),Zd));
  vd.clear();
  
  CMatrix es = CMatrix(3);
  CMatrix ds = CMatrix(3);
  for (short int i=0;i<2;i++) 
    for (short int j=0;j<2;j++) es[i][j] = Complex(0.,0.);   
  
  for (short int i=0;i<3;i++) 
    for (short int j=0;j<3;j++) ds[i][j] = Complex(0.,0.);
  
  double v1 = (*p_model->GetScalarConstants())["vev"] * 
    sqrt(1./(1.+sqr((*p_model->GetScalarConstants())["tan(beta)"])));
  double YD = -Flavour((kf_code)(5)).Yuk()/v1*sqrt(2.);
  
  p_reader->SetFileBegin(std::string("Block ad"));
  p_reader->RereadInFile();   
  
  if (p_reader->MatrixFromFile(vd,"")) {
    for (unsigned int i=0;i<vd.size();++i) {
      if (vd[i][0]==3 && vd[i][1]==3) ds[2][2] = -vd[i][2]*YD;  
    }
  }
  else msg_Error()<<std::endl<<"Could not read Block ad from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
  vd.clear();
  
  msg_Tracking()<<"   es is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<es[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  msg_Tracking()<<"   ds is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<ds[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("es"),es));
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("ds"),ds));
  vd.clear();
  
}

void LesHouches_Interface::SetSleptonParameters() {
  
  msg_Tracking()<<std::endl<<"  Reading Block staumix, ae"<<std::endl;
  std::vector<std::vector<double> >   vd;
  //staus
  p_reader->SetFileBegin(std::string("Block staumix"));
  p_reader->RereadInFile();
  if (!p_reader->MatrixFromFile(vd,""))
    msg_Error()<<std::endl<<"Could not read Block staumix from SLHA file: "
	       <<m_dir+m_inputfile<<std::endl;
  
  CMatrix Zl = CMatrix(6);
  for (short int i=0;i<6;i++) {
    for (short int j=0;j<6;j++) Zl[i][j] = Complex(0.,0.);
    Zl[i][i] = Complex(1.,0.);
  }    

  //new case
  for (unsigned int i=0;i<vd.size();++i) {
    if (vd[i][0]==1) {
      if (vd[i][1]==1) Zl[2][2]=vd[i][2];
      if (vd[i][1]==2) Zl[5][2]=vd[i][2];
    }
    if (vd[i][0]==2) {
      if (vd[i][1]==1) Zl[2][5]=vd[i][2];
      if (vd[i][1]==2) Zl[5][5]=vd[i][2];
    }
  }

  msg_Tracking()<<"   Zl is : "<<std::endl;
  for (unsigned int i=0;i<6;++i) {
    for (unsigned int j=0;j<6;++j) {
      msg_Tracking()<<"    "<<Zl[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_l"),Zl));
  vd.clear();

  CMatrix ls = CMatrix(3);
  CMatrix ks = CMatrix(3);
  for (short int i=0;i<2;i++) 
    for (short int j=0;j<2;j++) ks[i][j] = Complex(0.,0.);   
  
  for (short int i=0;i<3;i++) 
    for (short int j=0;j<3;j++) ls[i][j] = Complex(0.,0.);
  
  double v1 = (*p_model->GetScalarConstants())["vev"] * 
    sqrt(1./(1.+sqr((*p_model->GetScalarConstants())["tan(beta)"])));
  double YE = -Flavour((kf_code)(15)).Yuk()/v1*sqrt(2.); 
  
  p_reader->SetFileBegin(std::string("Block ae"));
  p_reader->RereadInFile();   
  
  if (p_reader->MatrixFromFile(vd,"")) {
    for (unsigned int i=0;i<vd.size();++i) {
      if (vd[i][0]==3 && vd[i][1]==3) ls[2][2] = -vd[i][2]*YE;  
    }
  }
  else msg_Error()<<std::endl<<"Could not read Block ae from SLHA file: "
		  <<m_dir+m_inputfile<<std::endl;
  vd.clear();
  
  msg_Tracking()<<"   ls is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<ls[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  msg_Tracking()<<"   ks is : "<<std::endl;
  for (unsigned int i=0;i<3;++i) {
    for (unsigned int j=0;j<3;++j) {
      msg_Tracking()<<"    "<<ks[i][j]<<" ";
    }
    msg_Tracking()<<std::endl;
  }
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("ls"),ls));
  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("ks"),ks));
  
  //sneutrinos
  CMatrix ZNue = CMatrix(3);
  for (short int i=0;i<3;i++) {
    for (short int j=0;j<3;j++) ZNue[i][j] = Complex(0.,0.);
    ZNue[i][i] = Complex(1.,0.);
  }    

  p_model->GetComplexMatrices()->insert(std::make_pair(std::string("Z_nu"),ZNue));
}



