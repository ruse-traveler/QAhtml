#include "GlobalQADraw.h"

#include <qahtml/QADrawClient.h>
#include <qahtml/QADrawDB.h>
#include <boost/format.hpp>

#include <TCanvas.h>
#include <TDatime.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#include <TPad.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TF1.h>
#include <TGaxis.h>
#include <TSQLServer.h>
#include <TSQLRow.h>
#include <TSQLResult.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void get_scaledowns(int runnumber, int scaledowns[])
{

  TSQLServer *db = TSQLServer::Connect("pgsql://sphnxdaqdbreplica:5432/daq","phnxro","");

  TSQLRow *row;
  TSQLResult *res;
  std::string sql = "";

  for (int is = 0; is < 64; is++)
  {
    sql = boost::str(boost::format("select scaledown%02d from gl1_scaledown where runnumber = %d;") % is % runnumber).c_str();
    const char * csql = sql.c_str();
    res = db->Query(csql);

    int nrows = res->GetRowCount();

    int nfields = res->GetFieldCount();
    for (int i = 0; i < nrows; i++) 
    {
      row = res->Next();
      for (int j = 0; j < nfields; j++) 
      {
        scaledowns[is] = std::stoi(row->GetField(j));
      }
      delete row;
    }

    delete res;
  }
  delete db;
}

  GlobalQADraw::GlobalQADraw(const std::string &name)
: QADraw(name)
{
  memset(TC, 0, sizeof(TC));
  memset(transparent, 0, sizeof(transparent));
  memset(Pad, 0, sizeof(Pad));
  DBVarInit();
  return;
}

GlobalQADraw::~GlobalQADraw()
{
  delete db;
  return;
}

int GlobalQADraw::Draw(const std::string &what)
{
  /* sPHENIX style
     gStyle->SetTitleSize(gStyle->GetTitleSize("X")*2.0, "X");
     gStyle->SetTitleSize(gStyle->GetTitleSize("Y")*2.0, "Y");
     gStyle->SetPadLeftMargin(0.15);
     gStyle->SetPadBottomMargin(0.15);
     gStyle->SetTitleOffset(0.85, "XY");
     gStyle->SetLabelSize(gStyle->GetLabelSize("X")*1.5, "X");
     gStyle->SetLabelSize(gStyle->GetLabelSize("Y")*1.5, "Y");
     gStyle->SetLabelSize(gStyle->GetLabelSize("Z")*1.5, "Z");
     TGaxis::SetMaxDigits(4);
     gStyle->SetPadTickX(1);
     gStyle->SetPadTickY(1);
     gStyle->SetOptStat(0);
     gROOT->ForceStyle();
     */
  int iret = 0;
  int idraw = 0;
  if (what == "ALL" || what == "MBD")
  {
    iret += DrawMBD(what);
    idraw++;
  }
  if (what == "ALL" || what == "ZDC")
  {
    iret += DrawZDC(what);
    idraw++;
  }
  if (what == "ALL" || what == "sEPD")
  {
    iret += DrawsEPD(what);
    idraw++;
  }
  if (!idraw)
  {
    std::cout << " Unimplemented Drawing option: " << what << std::endl;
    iret = -1;
  }
  return iret;
}


int GlobalQADraw::MakeCanvas(const std::string &name,int num)
{
  QADrawClient *cl = QADrawClient::instance();
  int xsize = cl->GetDisplaySizeX();
  int ysize = cl->GetDisplaySizeY();
  if (num == 0)
  {

    const double padWidth = 0.32; // width of each pad
    const double padHeight = 0.30; // height of each pad
    const double padSpacing = 0.02; // spacing between pads

    // total width and height needed for pads
    double totalWidth = padWidth * 3 + padSpacing * 2; 
    double totalHeight = padHeight * 3 + padSpacing * 2; 

    //ensure the total width and height do not exceed 1.0 (normalized coordinates)
    if (totalWidth > 1.0 || totalHeight > 1.0)
    {
      std::cerr << "Canvas cannot contain all pads without overlapping!" << std::endl;
      std::cerr << "Total width: " << totalWidth << ", Total height: " << totalHeight << std::endl;
      return -1;
    }

    // xpos (-1) negative: do not draw menu bar
    TC[num] = new TCanvas(name.c_str(), "MBD Plots", -1, 0, (int)(xsize * totalWidth ), (int)(ysize * totalHeight + 0.2));  
    TC[num]->UseCurrentStyle();
    gSystem->ProcessEvents();

    Pad[num][0] = new TPad("mypad00", "mbd_zvtx_wide", 0.05, 0.60, 0.32, 0.95); 
    Pad[num][1] = new TPad("mypad01", "mbd_zvtxq", 0.35, 0.60, 0.62, 0.95);    
    //Pad[num][2] = new TPad("mypad02", "mbd_charge_sum", 0.65, 0.60, 0.95, 0.95); 
    Pad[num][2] = new TPad("mypad03", "mbd_charesum_correlation", 0.05, 0.05, 0.32, 0.30);  
    Pad[num][3] = new TPad("mypad04", "mbd_nhit", 0.05, 0.30, 0.32, 0.60);  
    Pad[num][4] = new TPad("mypad05", "mbd_charge", 0.35, 0.30, 0.62, 0.60); 
    Pad[num][5] = new TPad("mypad06", "mbd_nhits_correlation", 0.35, 0.05, 0.62, 0.30);  


    Pad[num][0]->Draw();
    Pad[num][1]->Draw();
    Pad[num][2]->Draw();
    Pad[num][3]->Draw();
    Pad[num][4]->Draw();
    Pad[num][5]->Draw();
    //Pad[num][6]->Draw();
  }

  if (num == 1)
  {
    TC[num] = new TCanvas(name.c_str(),"ZDC Plots", -1, 0, (int) (xsize / 1.2), (int) (ysize / 1.2));
    TC[num]->UseCurrentStyle();
    gSystem->ProcessEvents();

    Pad[num][0] = new TPad("mypad10", "zdc_energy", 0.05, 0.52, 0.50, 0.97, 0);
    Pad[num][1] = new TPad("mypad11", "zdc_zvtx", 0.5, 0.52, 0.95, 0.97, 0);
    Pad[num][2] = new TPad("mypad12", "zdc_zvtx_wide", 0.5, 0.02, 0.95, 0.47, 0);

    Pad[num][0]->Draw();
    Pad[num][1]->Draw();
    Pad[num][2]->Draw();
  }

  if (num == 2)
  {
    TC[num] = new TCanvas(name.c_str(),"sEPD Plots", -1, 0, (int) (xsize / 1.2), (int) (ysize / 1.2));
    TC[num]->UseCurrentStyle();
    gSystem->ProcessEvents();

    Pad[num][0] = new TPad("mypad00", "sEPD South Map", 0.05, 0.52, 0.50, 0.97, 0);
    Pad[num][1] = new TPad("mypad01", "sEPD North Map", 0.5, 0.52, 0.95, 0.97, 0);
    Pad[num][2] = new TPad("mypad02", "sEPD NS ADC distribution", 0.5, 0.02, 0.95, 0.47, 0);
    Pad[num][3] = new TPad("mypad03", "sEPD NS Correlation", 0.05, 0.02, 0.50, 0.47, 0);

    Pad[num][0]->Draw();
    Pad[num][1]->Draw();
    Pad[num][2]->Draw();
    Pad[num][3]->Draw();
  }


  // this one is used to plot the run number on the canvas
  transparent[num] = new TPad("transparent0", "this does not show", 0, 0, 1, 1);
  transparent[num]->SetFillStyle(4000);
  transparent[num]->Draw();

  return 0;
}

int GlobalQADraw::DrawMBD(const std::string & /*what*/)
{
  QADrawClient *cl = QADrawClient::instance();
  TH1 *h_GlobalQA_mbd_zvtx_wide = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_zvtx_wide"));
  TH1 *h_GlobalQA_calc_zvtx_wide = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_calc_zvtx_wide"));
  TH1 *h_GlobalQA_mbd_charge_s = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_charge_s"));
  TH1 *h_GlobalQA_mbd_charge_n = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_charge_n"));
  TH1 *h_GlobalQA_mbd_nhit_s  = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_nhit_s"));
  TH1 *h_GlobalQA_mbd_nhit_n = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_nhit_n"));
  TH1 *h_GlobalQA_mbd_zvtxq = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_zvtxq"));
  TH1 *h_GlobalQA_mbd_charge_sum = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_mbd_charge_sum "));
  TH2 *h2_GlobalQA_mbd_charge_NS_correlation = dynamic_cast<TH2 *>(cl->getHisto("h2_GlobalQA_mbd_charge_NS_correlation"));
  TH2 *h2_GlobalQA_mbd_nhits_NS_correlation = dynamic_cast<TH2 *>(cl->getHisto("h2_GlobalQA_mbd_nhits_NS_correlation"));

  //check if the first attempt was unsuccessful
  if (!h_GlobalQA_mbd_charge_sum)
  {
    std::cerr << "h_GlobalQA_mbd_charge_sum not found with space, trying without space..." << std::endl;

    // now check if it can be retrieved without the space
    h_GlobalQA_mbd_charge_sum = dynamic_cast<TH1*>(cl->getHisto("h_GlobalQA_mbd_charge_sum")); 
  }

  //check to see if the histogram was found
  if (!h_GlobalQA_mbd_charge_sum)
  {
    std::cerr << "h_GlobalQA_mbd_charge_sum has not been found" << std::endl;
  } 
  else
  {
    std::cerr << "h_GlobalQA_mbd_charge_sum found." << std::endl;
  }

  if (!h2_GlobalQA_mbd_charge_NS_correlation)
  {
    std::cerr<<" h2_GlobalQA_mbd_charge_NS_correlation Not Found"<<std::endl;
    return -1;
  }

  if (!h2_GlobalQA_mbd_nhits_NS_correlation)
  {
    std::cerr<<" h2_GlobalQA_mbd_nhits_NS_correlation Not Found"<<std::endl;
    return -1;
  }

  //run type
  if(cl->RunNumber()>= 4200 && cl->RunNumber()<=53900)
  { 
    run_type = 1; //pp run 
    std::cout<<"MBDRun_pp :"<<cl->RunNumber()<<std::endl;
  }
  else
  {
    run_type = 0 ; //AuAu run 
    std::cout<<"MBDRun_AuAu :"<<cl->RunNumber()<<std::endl;
  }



  if (!gROOT->FindObject("Global1"))
  {
    MakeCanvas("Global1",0);
  }
  TC[0]->Clear("D");

  // Plot the z-vertices wide
  TLegend * leg00 = new TLegend(0.3, 0.7, 0.9, 0.9);
  Pad[0][0]->cd();
  if (h_GlobalQA_mbd_zvtx_wide && h_GlobalQA_calc_zvtx_wide)
  {

    TF1 * f = new TF1("f", "gaus", -100,100);
    f->SetParameters(h_GlobalQA_mbd_zvtx_wide->GetMaximum(), h_GlobalQA_mbd_zvtx_wide->GetMean(), h_GlobalQA_mbd_zvtx_wide->GetRMS() );
    h_GlobalQA_mbd_zvtx_wide->Fit("f");
    h_GlobalQA_mbd_zvtx_wide->SetMaximum(h_GlobalQA_mbd_zvtx_wide->GetMaximum() * 1.5);
    h_GlobalQA_mbd_zvtx_wide->SetLineColor(kRed+4);
    leg00->AddEntry(h_GlobalQA_mbd_zvtx_wide,"Provided z-vertex","l");
    h_GlobalQA_mbd_zvtx_wide->DrawCopy("hist");

    gPad->UseCurrentStyle();

    h_GlobalQA_mbd_zvtx_wide->SetLineColor(kRed+4);

    f->SetLineColor(kBlack);
    f->DrawCopy("same");

    float mean = f->GetParameter(1);
    float rms = f->GetParameter(2);
    leg00->AddEntry((TObject*)0, boost::str(boost::format("Mean: %.1f") % mean).c_str(), "");
    leg00->AddEntry((TObject*)0, boost::str(boost::format("RMS: %.1f") % rms).c_str(), "");

    leg00->Draw();
  }
  else
  {
    return -1;

  }
  // Plot the number of mbd plots available
  Pad[0][1]->cd();
  gPad->UseCurrentStyle();
  if (h_GlobalQA_mbd_zvtxq)
  {
    h_GlobalQA_mbd_zvtxq->Scale(1.0 / h_GlobalQA_mbd_zvtxq->GetEntries());
    h_GlobalQA_mbd_zvtxq->GetYaxis()->SetRangeUser(0,1);

    TText printyes;
    TText printno;
    printyes.SetTextFont(62);
    printno.SetTextFont(62);
    printyes.SetTextSize(0.06);
    printno.SetTextSize(0.06);
    printyes.SetNDC();
    printno.SetNDC();
    float yes = h_GlobalQA_mbd_zvtxq->GetBinContent(2) * 100;
    float no = h_GlobalQA_mbd_zvtxq->GetBinContent(1) * 100;

    h_GlobalQA_mbd_zvtxq->DrawCopy("hist");
    printyes.DrawText(0.55,0.7,boost::str(boost::format("zvtx provided:\n %.1f%s") % yes % "%").c_str());
    printno.DrawText(0.2,0.7, boost::str(boost::format("No zvtx:\n %.1f%s") % no % "%").c_str());

    gPad->UseCurrentStyle();
  }
  else
  {
    return -1;

  }


  // Plot the charge sum correlation distribution 
  TLegend * leg03 = new TLegend(0.6, 0.6, 0.9, 0.9);
  Pad[0][2]->cd();
  if (h2_GlobalQA_mbd_charge_NS_correlation && h_GlobalQA_mbd_charge_sum )
  {  
    Double_t nevents = h_GlobalQA_mbd_charge_sum->Integral();
    h_GlobalQA_mbd_charge_sum->Fill(-1000,nevents); // underflow bin keeps track of nevents
    Double_t norm = 1.0/nevents;
    h_GlobalQA_mbd_charge_sum ->Scale( norm );
    h2_GlobalQA_mbd_charge_NS_correlation->Scale( norm );

    gPad->UseCurrentStyle();
    gPad->SetLogz();

    if (run_type == 1)
    {
      // rebin the hist by a factor of 4 in both dimensions
      TH2F *h2_GlobalQA_mbd_charge_NS_correlation_rebinned = (TH2F*)h2_GlobalQA_mbd_charge_NS_correlation->Rebin2D(4,4, nullptr);

      h2_GlobalQA_mbd_charge_NS_correlation_rebinned->SetTitle("MBD North-South Charge Correlation");
      h2_GlobalQA_mbd_charge_NS_correlation_rebinned->SetXTitle("MBD south charge sum");
      h2_GlobalQA_mbd_charge_NS_correlation_rebinned->SetYTitle("MBD north charge sum");
      // h2_GlobalQA_mbd_charge_NS_correlation->DrawCopy("COLZ");
      h2_GlobalQA_mbd_charge_NS_correlation_rebinned->DrawCopy("COLZ");
    }
    else if (run_type == 0)
    {
      TH2F *h2_GlobalQA_mbd_charge_NS_correlation_auau = new TH2F("h2_GlobalQA_mbd_charge_NS_correlation_auau","h2_GlobalQA_mbd_charge_NS_correlation_auau", 150, 0, 1500, 150, 0, 1500);

      for (int xbin = 1; xbin <= h2_GlobalQA_mbd_charge_NS_correlation->GetNbinsX(); xbin++)
      {
        for (int ybin = 1; ybin <= h2_GlobalQA_mbd_charge_NS_correlation->GetNbinsY(); ybin++) 
        {

          double content = h2_GlobalQA_mbd_charge_NS_correlation->GetBinContent(xbin, ybin);
          double error = h2_GlobalQA_mbd_charge_NS_correlation->GetBinError(xbin, ybin);

          h2_GlobalQA_mbd_charge_NS_correlation_auau->Fill(h2_GlobalQA_mbd_nhits_NS_correlation->GetXaxis()->GetBinCenter(xbin), h2_GlobalQA_mbd_nhits_NS_correlation->GetYaxis()->GetBinCenter(ybin), content);
          //get the error 
          h2_GlobalQA_mbd_charge_NS_correlation_auau->SetBinError(h2_GlobalQA_mbd_charge_NS_correlation_auau->FindBin(h2_GlobalQA_mbd_charge_NS_correlation->GetXaxis()->GetBinCenter(xbin),h2_GlobalQA_mbd_charge_NS_correlation_auau->GetYaxis()->GetBinCenter(ybin)), error);

        }

      }

      h2_GlobalQA_mbd_charge_NS_correlation_auau->SetTitle("MBD North-South Charge Correlation");
      h2_GlobalQA_mbd_charge_NS_correlation_auau->SetXTitle("MBD south charge sum");
      h2_GlobalQA_mbd_charge_NS_correlation_auau->SetYTitle("MBD north charge sum");

      h2_GlobalQA_mbd_charge_NS_correlation_auau->DrawCopy("COLZ");
      //h2_GlobalQA_mbd_charge_NS_correlation_auau->DrawCopy("E");  


      leg03->Draw();
    }
  }
  else
  {
    return -1;

  }

  // Plot the hit distribution
  TLegend * leg04 = new TLegend(0.6, 0.6, 0.9, 0.9);
  Pad[0][3]->cd();
  if (h_GlobalQA_mbd_nhit_s && h_GlobalQA_mbd_nhit_n)
  {

    gPad->UseCurrentStyle();
    gPad->SetLogy();
    if (run_type == 1)
    {
      //pp run
      h_GlobalQA_mbd_nhit_s->SetLineColor(kRed);
      leg04->AddEntry(h_GlobalQA_mbd_nhit_s,"South","l");
      h_GlobalQA_mbd_nhit_s->DrawCopy("hist");

      h_GlobalQA_mbd_nhit_n->SetLineColor(kBlue);
      leg04->AddEntry(h_GlobalQA_mbd_nhit_n,"North","l");

      float means = h_GlobalQA_mbd_nhit_s->GetMean();
      float meann = h_GlobalQA_mbd_nhit_n->GetMean();
      leg04->AddEntry((TObject*)0, boost::str(boost::format("South mean: %.1f") % means).c_str(), ""); 
      leg04->AddEntry((TObject*)0, boost::str(boost::format("North mean: %.1f") % meann).c_str(), ""); 

      h_GlobalQA_mbd_nhit_n->DrawCopy("hist same");

    }
    else if(run_type == 0)
    {  
      //AuAu run 

      //MBD south arm
      TH1F *h_GlobalQA_mbd_nhit_s_auau = new TH1F("h_GlobalQA_mbd_nhit_s_auau","h_GlobalQA_mbd_nhit_s_auau", 64, 0, 64);
      //fill
      for (int xbin = 1; xbin <= h_GlobalQA_mbd_nhit_s->GetNbinsX(); xbin++)
      {
        double content = h_GlobalQA_mbd_nhit_s->GetBinContent(xbin);
        double Xvalue = h_GlobalQA_mbd_nhit_s->GetXaxis()->GetBinCenter(xbin);

        //check only fill if the original values are within the new range
        if (Xvalue >= 0 && Xvalue <= 64)
        {
          h_GlobalQA_mbd_nhit_s_auau->Fill(Xvalue,content);
        }
      }

      h_GlobalQA_mbd_nhit_s_auau->SetLineColor(kRed);
      leg04->AddEntry(h_GlobalQA_mbd_nhit_s_auau,"South","l");
      h_GlobalQA_mbd_nhit_s_auau->DrawCopy("hist");

      //MBD north arm
      TH1F *h_GlobalQA_mbd_nhit_n_auau = new TH1F("h_GlobalQA_mbd_nhit_n_auau","h_GlobalQA_mbd_nhit_n_auau", 64, 0, 64);
      //fill
      for (int xbin = 1; xbin <= h_GlobalQA_mbd_nhit_n->GetNbinsX(); xbin++)
      {
        double content = h_GlobalQA_mbd_nhit_n->GetBinContent(xbin);
        double Xvalue = h_GlobalQA_mbd_nhit_n->GetXaxis()->GetBinCenter(xbin);
        //check only fill if the original values are within the new range
        if (Xvalue >= 0 && Xvalue <= 64)
        {
          h_GlobalQA_mbd_nhit_n_auau->Fill(Xvalue,content);
        }
      }

      h_GlobalQA_mbd_nhit_n_auau->SetLineColor(kBlue);
      leg04->AddEntry(h_GlobalQA_mbd_nhit_n_auau,"North","l");

      float means = h_GlobalQA_mbd_nhit_s_auau->GetMean();
      float meann = h_GlobalQA_mbd_nhit_n_auau->GetMean();
      leg04->AddEntry((TObject*)0, boost::str(boost::format("South mean: %.1f") % means).c_str(), ""); 
      leg04->AddEntry((TObject*)0, boost::str(boost::format("North mean: %.1f") % meann).c_str(), ""); 

      h_GlobalQA_mbd_nhit_n_auau->DrawCopy("hist same");

    }

    leg04->Draw();
  }
  else
  {
    return -1;

  }

  // Plot the charge distribution 
  TLegend * leg01 = new TLegend(0.6, 0.6, 0.9, 0.9);
  Pad[0][4]->cd();
  if (h_GlobalQA_mbd_charge_s && h_GlobalQA_mbd_charge_n)
  {

    gPad->UseCurrentStyle();
    gPad->SetLogy();

    if (run_type == 1)
    {

      h_GlobalQA_mbd_charge_s->SetLineColor(kRed);
      leg01->AddEntry(h_GlobalQA_mbd_charge_s,"South","l");
      h_GlobalQA_mbd_charge_s->DrawCopy("hist");

      h_GlobalQA_mbd_charge_n->SetLineColor(kBlue);
      leg01->AddEntry(h_GlobalQA_mbd_charge_n,"North","l");
      h_GlobalQA_mbd_charge_n->DrawCopy("hist same");

      float means = h_GlobalQA_mbd_charge_s->GetMean();
      float meann = h_GlobalQA_mbd_charge_n->GetMean();
      leg01->AddEntry((TObject*)0, boost::str(boost::format("South mean: %.1f") % means).c_str(), ""); 
      leg01->AddEntry((TObject*)0, boost::str(boost::format("North mean: %.1f") % meann).c_str(), ""); 
    }
    else if (run_type == 0)
    {
      //MBD south arm
      TH1F *h_GlobalQA_mbd_charge_s_auau = new TH1F("h_GlobalQA_mbd_charge_s_auau","h_GlobalQA_mbd_charge_s_auau", 150, 0, 1500);
      //fill
      for (int xbin = 1; xbin <= h_GlobalQA_mbd_charge_s->GetNbinsX(); xbin++)
      {
        double content = h_GlobalQA_mbd_charge_s->GetBinContent(xbin);
        double Xvalue = h_GlobalQA_mbd_charge_s->GetXaxis()->GetBinCenter(xbin);

        //check only fill if the original values are within the new range
        if (Xvalue >= 0 && Xvalue <= 1500)
        {
          h_GlobalQA_mbd_charge_s_auau->Fill(Xvalue,content);
        }
      }

      h_GlobalQA_mbd_charge_s_auau->SetLineColor(kRed);
      leg01->AddEntry(h_GlobalQA_mbd_charge_s_auau,"South","l");
      h_GlobalQA_mbd_charge_s_auau->DrawCopy("hist");

      //MBD north arm
      TH1F *h_GlobalQA_mbd_charge_n_auau = new TH1F("h_GlobalQA_mbd_charge_n_auau","h_GlobalQA_mbd_charge_n_auau", 150, 0, 1500);
      //fill
      for (int xbin = 1; xbin <= h_GlobalQA_mbd_charge_n->GetNbinsX(); xbin++)
      {
        double content = h_GlobalQA_mbd_charge_n->GetBinContent(xbin);
        double Xvalue = h_GlobalQA_mbd_charge_n->GetXaxis()->GetBinCenter(xbin);

        //check only fill if the original values are within the new range
        if (Xvalue >= 0 && Xvalue <= 1500)
        {
          h_GlobalQA_mbd_charge_n_auau->Fill(Xvalue,content);
        }
      }

      h_GlobalQA_mbd_charge_n_auau->SetLineColor(kBlue);
      leg01->AddEntry(h_GlobalQA_mbd_charge_n_auau,"North","l");

      float means = h_GlobalQA_mbd_charge_s_auau->GetMean();
      float meann = h_GlobalQA_mbd_charge_n_auau->GetMean();
      leg01->AddEntry((TObject*)0, boost::str(boost::format("South mean: %.1f") % means).c_str(), ""); 
      leg01->AddEntry((TObject*)0, boost::str(boost::format("North mean: %.1f") % meann).c_str(), ""); 

      h_GlobalQA_mbd_charge_n_auau->DrawCopy("hist same");



    }
    leg01->Draw();

  }
  else
  {
    return -1;

  }

  // Plot the hits correlation
  TLegend * leg05 = new TLegend(0.6, 0.6, 0.9, 0.9);
  Pad[0][5]->cd();
  if (h2_GlobalQA_mbd_nhits_NS_correlation)
  {

    gPad->UseCurrentStyle();
    gPad->SetLogz();
    if (run_type==1)
    {
      h2_GlobalQA_mbd_nhits_NS_correlation->SetTitle("Run_pp_MBD South-North nhits Correlation");
      h2_GlobalQA_mbd_nhits_NS_correlation->SetXTitle("MBD south nhits(Run_pp)");
      h2_GlobalQA_mbd_nhits_NS_correlation->SetYTitle("MBD north nhits(Run_pp)");

      h2_GlobalQA_mbd_nhits_NS_correlation->DrawCopy("COLZ");

      leg05->Draw();
    }
    else if(run_type==0)
    {
      TH2F *h2_GlobalQA_mbd_nhits_NS_correlation_rebinned = (TH2F*)h2_GlobalQA_mbd_nhits_NS_correlation->Rebin2D(4,4, nullptr);

      h2_GlobalQA_mbd_nhits_NS_correlation_rebinned->SetTitle("Run_AuAu_MBD South-North nhits Correlation");
      h2_GlobalQA_mbd_nhits_NS_correlation_rebinned->SetXTitle("MBD south nhits(Run_AuAu)");
      h2_GlobalQA_mbd_nhits_NS_correlation_rebinned->SetYTitle("MBD north nhits(Run_AuAu)");

      h2_GlobalQA_mbd_nhits_NS_correlation_rebinned->DrawCopy("COLZ");

      leg05->Draw();
    }
  }
  else
  {
    return -1;

  }

  //db->DBcommit();

  TText PrintRun;
  PrintRun.SetTextFont(62);
  PrintRun.SetTextSize(0.04);
  PrintRun.SetNDC();          // set to normalized coordinates
  PrintRun.SetTextAlign(23);  // center/top alignment
  std::ostringstream runnostream;
  std::string runstring;
  runnostream << Name() << "_MBD Run " << cl->RunNumber() << ", build " << cl->build();
  runstring = runnostream.str();
  transparent[0]->cd();
  PrintRun.DrawText(0.5, 1., runstring.c_str());
  TC[0]->Update();

  return 0;
}


int GlobalQADraw::DrawZDC(const std::string & /*what*/)
{
  QADrawClient *cl = QADrawClient::instance();
  TH1 *h_GlobalQA_zdc_zvtx = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_zdc_zvtx"));
  TH1 *h_GlobalQA_zdc_zvtx_wide = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_zdc_zvtx_wide"));
  TH1 *h_GlobalQA_zdc_energy_s = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_zdc_energy_s"));
  TH1 *h_GlobalQA_zdc_energy_n = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_zdc_energy_n"));

  if (!gROOT->FindObject("Global2"))
  {
    MakeCanvas("Global2",1);
  }
  // Plot the ZDC energy distributions
  TLegend * leg10 = new TLegend(0.7, 0.7, 0.9 ,0.9);
  Pad[1][0]->cd();
  if (h_GlobalQA_zdc_energy_s && h_GlobalQA_zdc_energy_n)
  {
    h_GlobalQA_zdc_energy_s->Scale(1/h_GlobalQA_zdc_energy_s->Integral());
    h_GlobalQA_zdc_energy_n->Scale(1/h_GlobalQA_zdc_energy_n->Integral());

    gPad->UseCurrentStyle();

    h_GlobalQA_zdc_energy_s->SetLineColor(kRed);
    h_GlobalQA_zdc_energy_s->SetMarkerColor(kRed);
    h_GlobalQA_zdc_energy_s->SetMarkerStyle(20);
    h_GlobalQA_zdc_energy_s->SetMarkerSize(0.8);

    leg10->AddEntry(h_GlobalQA_zdc_energy_s,"South","l");
    h_GlobalQA_zdc_energy_s->DrawCopy();

    // TGraph *gr_1n = new TGraph();
    // gr_1n->SetPoint(0, 70, 0);
    // gr_1n->SetPoint(1, 70, 1e7);
    // gr_1n->SetLineColor(kBlack);
    // gr_1n->SetLineWidth(2);
    // gr_1n->SetLineStyle(9);
    // gr_1n->Draw("l");

    h_GlobalQA_zdc_energy_n->SetLineColor(kBlue);
    h_GlobalQA_zdc_energy_n->SetMarkerColor(kBlue);
    h_GlobalQA_zdc_energy_n->SetMarkerStyle(20);
    h_GlobalQA_zdc_energy_n->SetMarkerSize(0.8);

    leg10->AddEntry(h_GlobalQA_zdc_energy_n,"North","l");
    h_GlobalQA_zdc_energy_n->DrawCopy("same");


    leg10->Draw();
  }
  else
  {
    return -1;
  }

  // Plot the ZDC z-vertex
  Pad[1][1]->cd();
  if (h_GlobalQA_zdc_zvtx)
  {
    h_GlobalQA_zdc_zvtx->DrawCopy();
    gPad->UseCurrentStyle();
  }
  else 
  {
    return -1;
  }

  // Plot the ZDC z-vertex wide
  Pad[1][2]->cd();
  if (h_GlobalQA_zdc_zvtx_wide)
  {
    TF1 * f = new TF1("f", "gaus", -100,100);
    f->SetParameters(h_GlobalQA_zdc_zvtx_wide->GetMaximum(), h_GlobalQA_zdc_zvtx_wide->GetMean(), h_GlobalQA_zdc_zvtx_wide->GetRMS() );
    h_GlobalQA_zdc_zvtx_wide->Fit("f");
    h_GlobalQA_zdc_zvtx_wide->DrawCopy();
    gPad->UseCurrentStyle();

    f->SetLineColor(kBlack);
    f->DrawCopy("same");

    TText printmean;
    TText printrms;
    printmean.SetTextFont(62);
    printrms.SetTextFont(62);
    printmean.SetTextSize(0.06);
    printrms.SetTextSize(0.06);
    printmean.SetNDC();
    printrms.SetNDC();
    float mean = f->GetParameter(1);
    float rms = f->GetParameter(2);
    printmean.DrawText(0.2,0.7,boost::str(boost::format("Mean: %.1f") % mean).c_str());
    printrms.DrawText(0.2,0.55, boost::str(boost::format("RMS: %.1f") % rms).c_str());
  }
  else 
  {
    return -1;
  }

  //db->DBcommit();
  TText PrintRun;
  PrintRun.SetTextFont(62);
  PrintRun.SetTextSize(0.04);
  PrintRun.SetNDC();          // set to normalized coordinates
  PrintRun.SetTextAlign(23);  // center/top alignment
  std::ostringstream runnostream;
  std::string runstring;
  runnostream << Name() << "_ZDC Run " << cl->RunNumber() << ", build " << cl->build();
  runstring = runnostream.str();
  transparent[1]->cd();
  PrintRun.DrawText(0.5, 1., runstring.c_str());
  TC[1]->Update();
  return 0;
}

int GlobalQADraw::DrawsEPD(const std::string & /*what*/)
{
  QADrawClient *cl = QADrawClient::instance();
  TH1 *h_GlobalQA_sEPD_adcsum_s = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_sEPD_adcsum_s"));
  TH1 *h_GlobalQA_sEPD_adcsum_n = dynamic_cast<TH1 *>(cl->getHisto("h_GlobalQA_sEPD_adcsum_n"));
  TH2 *h2_GlobalQA_sEPD_adcsum_ns = dynamic_cast<TH2 *>(cl->getHisto("h2_GlobalQA_sEPD_adcsum_ns"));
  TProfile2D *h2Profile_GlobalQA_sEPD_tiles_north = dynamic_cast<TProfile2D *>(cl->getHisto("h2Profile_GlobalQA_sEPD_tiles_north"));
  TProfile2D *h2Profile_GlobalQA_sEPD_tiles_south = dynamic_cast<TProfile2D *>(cl->getHisto("h2Profile_GlobalQA_sEPD_tiles_south"));
  TH2 *h2_GlobalQA_sEPD_ADC_channel_north = dynamic_cast<TH2 *>(cl->getHisto("h2_GlobalQA_sEPD_ADC_channel_north"));
  TH2 *h2_GlobalQA_sEPD_ADC_channel_south = dynamic_cast<TH2 *>(cl->getHisto("h2_GlobalQA_sEPD_ADC_channel_south"));

  if (!gROOT->FindObject("Global3"))
  {
    MakeCanvas("Global3",2);
  }
  TLegend * leg10 = new TLegend(0.7, 0.7, 0.9 ,0.9);
  Pad[2][0]->cd();
  if (h_GlobalQA_sEPD_adcsum_s && h_GlobalQA_sEPD_adcsum_n)
  {
    double _max1 = h_GlobalQA_sEPD_adcsum_s->GetMaximum();
    double _max2 = h_GlobalQA_sEPD_adcsum_n->GetMaximum();
    double _add = 500.0;
    double _histmax = _max2;

    if(_max2 < _max1) _histmax = _max1;

    h_GlobalQA_sEPD_adcsum_n->GetXaxis()->SetRangeUser(-10, 30000);
    h_GlobalQA_sEPD_adcsum_s->GetXaxis()->SetRangeUser(-10, 30000);
    h_GlobalQA_sEPD_adcsum_n->GetYaxis()->SetRangeUser(0, _histmax + _add);
    h_GlobalQA_sEPD_adcsum_s->GetYaxis()->SetRangeUser(0, _histmax + _add);

    // h_GlobalQA_sEPD_adcsum_s->Scale(1/h_GlobalQA_sEPD_adcsum_s->Integral());
    // h_GlobalQA_sEPD_adcsum_n->Scale(1/h_GlobalQA_sEPD_adcsum_n->Integral());

    h_GlobalQA_sEPD_adcsum_s->SetYTitle("Counts");

    gPad->UseCurrentStyle();

    h_GlobalQA_sEPD_adcsum_s->SetLineColor(kRed);
    h_GlobalQA_sEPD_adcsum_s->SetMarkerColor(kRed);
    h_GlobalQA_sEPD_adcsum_s->SetMarkerStyle(20);
    h_GlobalQA_sEPD_adcsum_s->SetMarkerSize(0.8);

    leg10->AddEntry(h_GlobalQA_sEPD_adcsum_s,"South","l");
    h_GlobalQA_sEPD_adcsum_s->DrawCopy();


    h_GlobalQA_sEPD_adcsum_n->SetLineColor(kBlue);
    h_GlobalQA_sEPD_adcsum_n->SetMarkerColor(kBlue);
    h_GlobalQA_sEPD_adcsum_n->SetMarkerStyle(20);
    h_GlobalQA_sEPD_adcsum_n->SetMarkerSize(0.8);

    leg10->AddEntry(h_GlobalQA_sEPD_adcsum_n,"North","l");
    h_GlobalQA_sEPD_adcsum_n->DrawCopy("same");

    leg10->Draw();
  }
  else
  {
    return -1;
  }

  Pad[2][1]->cd();
  if (h2_GlobalQA_sEPD_adcsum_ns)
  {

    h2_GlobalQA_sEPD_adcsum_ns->SetTitle("sEPD North-South Correlation");
    h2_GlobalQA_sEPD_adcsum_ns->GetXaxis()->SetRangeUser(-10, 30000);
    h2_GlobalQA_sEPD_adcsum_ns->GetYaxis()->SetRangeUser(-10, 30000);
    h2_GlobalQA_sEPD_adcsum_ns->SetXTitle("sEPD south ADC sum");
    h2_GlobalQA_sEPD_adcsum_ns->SetYTitle("sEPD north ADC sum");
    h2_GlobalQA_sEPD_adcsum_ns->DrawCopy("COLZ");
    gPad->UseCurrentStyle();
    gPad->SetRightMargin(0.15);

  }
  else
  {
    return -1;
  }


  Pad[2][2]->cd();
  if (h2Profile_GlobalQA_sEPD_tiles_south && h2_GlobalQA_sEPD_ADC_channel_south)
  {
    h2Profile_GlobalQA_sEPD_tiles_south->GetYaxis()->SetNdivisions(527);
    h2Profile_GlobalQA_sEPD_tiles_south->GetXaxis()->SetNdivisions(527);
    h2_GlobalQA_sEPD_ADC_channel_south->GetXaxis()->SetNdivisions(527);
    h2_GlobalQA_sEPD_ADC_channel_south->GetYaxis()->SetNdivisions(527);
    h2Profile_GlobalQA_sEPD_tiles_south->SetTitle("sEPD South Tile Mean ADC");
    h2_GlobalQA_sEPD_ADC_channel_south->SetTitle("sEPD South Tile Mean ADC");
    h2_GlobalQA_sEPD_ADC_channel_south->GetZaxis()->SetTitle("Mean ADC");
    h2Profile_GlobalQA_sEPD_tiles_south->SetXTitle("#eta bin");
    h2Profile_GlobalQA_sEPD_tiles_south->SetYTitle("#phi bin");
    h2_GlobalQA_sEPD_ADC_channel_south->SetXTitle("#eta bin");
    h2_GlobalQA_sEPD_ADC_channel_south->SetYTitle("#phi bin");
    h2Profile_GlobalQA_sEPD_tiles_south->DrawCopy("COLZ");
    if(h2_GlobalQA_sEPD_ADC_channel_south->GetEntries() == 372)
    {
      h2_GlobalQA_sEPD_ADC_channel_south->Draw("text SAME");
    }


    TLatex l1;
    l1.SetNDC();
    l1.SetTextFont(43);
    l1.SetTextSize(20);
    l1.DrawLatex(0.3, 0.01, "sEPD South Tile <ADC>");
    gPad->UseCurrentStyle();
    gPad->SetRightMargin(0.15);
    // gPad->SetLogz();
  }
  else
  {
    return -1;
  }

  Pad[2][3]->cd();
  if (h2Profile_GlobalQA_sEPD_tiles_north && h2_GlobalQA_sEPD_ADC_channel_north)
  {
    h2Profile_GlobalQA_sEPD_tiles_north->GetYaxis()->SetNdivisions(527);
    h2Profile_GlobalQA_sEPD_tiles_north->GetXaxis()->SetNdivisions(527);
    h2_GlobalQA_sEPD_ADC_channel_north->GetXaxis()->SetNdivisions(527);
    h2_GlobalQA_sEPD_ADC_channel_north->GetYaxis()->SetNdivisions(527);
    h2Profile_GlobalQA_sEPD_tiles_north->SetTitle("sEPD North Tile Mean ADC");
    h2_GlobalQA_sEPD_ADC_channel_north->SetTitle("sEPD North Tile Mean ADC");
    h2_GlobalQA_sEPD_ADC_channel_north->GetZaxis()->SetTitle("Mean ADC");
    h2Profile_GlobalQA_sEPD_tiles_north->SetXTitle("#eta bin");
    h2Profile_GlobalQA_sEPD_tiles_north->SetYTitle("#phi bin");
    h2_GlobalQA_sEPD_ADC_channel_north->SetXTitle("#eta bin");
    h2_GlobalQA_sEPD_ADC_channel_north->SetYTitle("#phi bin");
    h2Profile_GlobalQA_sEPD_tiles_north->DrawCopy("COLZ ");
    if(h2_GlobalQA_sEPD_ADC_channel_north->GetEntries() == 372)
    {
      h2_GlobalQA_sEPD_ADC_channel_north->Draw("text SAME");
    }
    TLatex l2;
    l2.SetNDC();
    l2.SetTextFont(43);
    l2.SetTextSize(20);
    l2.DrawLatex(0.3, 0.01, "sEPD North Tile <ADC>");
    gPad->UseCurrentStyle();
    gPad->SetRightMargin(0.15);
    // gPad->SetLogz();
  }
  else
  {
    return -1;
  }

  //db->DBcommit();
  TText PrintRun;
  PrintRun.SetTextFont(62);
  PrintRun.SetTextSize(0.04);
  PrintRun.SetNDC();          // set to normalized coordinates
  PrintRun.SetTextAlign(23);  // center/top alignment
  std::ostringstream runnostream;
  std::string runstring;
  runnostream << Name() << "_sEPD Run " << cl->RunNumber() << ", build " << cl->build();
  runstring = runnostream.str();
  transparent[2]->cd();
  PrintRun.DrawText(0.5, 1., runstring.c_str());
  TC[2]->Update();
  return 0;
}

int GlobalQADraw::MakeHtml(const std::string &what)
{
  int iret = Draw(what);
  if (iret)  // on error no html output please
  {
    return iret;
  }

  QADrawClient *cl = QADrawClient::instance();

  // Register the 1st canvas png file to the menu and produces the png file.
  std::string pngfile = cl->htmlRegisterPage(*this, "MBD", "global1", "png");
  cl->CanvasToPng(TC[0], pngfile);
  pngfile = cl->htmlRegisterPage(*this, "ZDC", "global2", "png");
  cl->CanvasToPng(TC[1], pngfile);
  pngfile = cl->htmlRegisterPage(*this, "sEPD", "global3", "png");
  cl->CanvasToPng(TC[2], pngfile);



  return 0;
}

int GlobalQADraw::DBVarInit()
{
  //db = new QADrawDB(this);
  //db->registerVar("meanpx");
  //db->registerVar("rms");
  //db->DBInit();
  return 0;
}
