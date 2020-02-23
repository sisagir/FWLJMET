#include "FWLJMET/LJMet/interface/HOTTaggerCalc.h"



int HOTTaggerCalc::BeginJob(edm::ConsumesCollector && iC){
    
    ak4ptCut_ = mPset.getParameter<double>("ak4PtCut");
    qgTaggerKey_ = mPset.getParameter<std::string>("qgTaggerKey");
    deepCSVBJetTags_ = mPset.getParameter<std::string>("deepCSVBJetTags");
    bTagKeyString_ = mPset.getParameter<std::string>("bTagKeyString");    
    taggerCfgFile_ = mPset.getParameter<edm::FileInPath>("taggerCfgFile").fullPath();
    discriminatorCut_ = mPset.getParameter<double>("discriminatorCut");
    genParticlesToken = iC.consumes<reco::GenParticleCollection>(mPset.getParameter<edm::InputTag>("genParticles"));
    
    //configure the top tagger
    try{
		//For working directory use cfg file location
		size_t splitLocation = taggerCfgFile_.rfind("/");
		std::string workingDir = taggerCfgFile_.substr(0, splitLocation);
		std::string configName = taggerCfgFile_.substr(splitLocation + 1);
		tt.setWorkingDirectory(workingDir);
		tt.setCfgFile(configName);
	}
    catch(const TTException& e){
      //Convert the TTException into a cms::Exception
      throw cms::Exception(e.getFileName() + ":" + std::to_string(e.getLineNumber()) + ", in function \"" + e.getFunctionName() + "\" -- " + e.getMessage());
    }
    
    return 0;
  }


int HOTTaggerCalc::AnalyzeEvent(edm::Event const & event, BaseEventSelector * selector){

  //Get lepton-cleaned and JEC-corrected jet collection
  std::vector<pat::Jet> const & vSelCorrJets = selector->GetSelCorrJets();

  //container holding input jet info for top tagger
  std::vector<Constituent> constituents;
    
  // Get the generated particle collection
  std::vector<TLorentzVector> mygenTops, mygenTopDaughtersTemp_;
  std::vector<std::vector<TLorentzVector>> mygenTopDaughters_;
  mygenTops.clear();
  mygenTopDaughters_.clear();
  TLorentzVector topLV,topdauLV;
    
  edm::Handle<reco::GenParticleCollection> genParticles;
  if(event.getByToken(genParticlesToken, genParticles)){
  	// loop over all gen particles in event
  	for(size_t igen = 0; igen < genParticles->size(); igen++){
  		const reco::GenParticle &p = (*genParticles).at(igen);
  		int id = p.pdgId();
  		
  		// find tops
  		if(abs(id) != 6) continue;
  		
  		mygenTopDaughtersTemp_.clear();
  		bool foundB = false;
  		bool foundW = false;
  		for(size_t idau = 0; idau < p.numberOfDaughters(); idau++){
  			int dauId = p.daughter(idau)->pdgId();
  			if(abs(dauId) == 5){
  				topdauLV.SetPtEtaPhiE(p.daughter(idau)->pt(),p.daughter(idau)->eta(),p.daughter(idau)->phi(),p.daughter(idau)->energy());
  				mygenTopDaughtersTemp_.push_back(topdauLV);
  				foundB = true;
  				}
  			else if(abs(dauId) == 24){
  				const reco::Candidate *W = p.daughter(idau);
  				bool isHardProcess = false;
  				while(!isHardProcess){
  					isHardProcess = true;
  					for(size_t iWdau = 0; iWdau < W->numberOfDaughters(); iWdau++){
  						int WdauId = W->daughter(iWdau)->pdgId();
  						if(abs(WdauId) == 24){
  							W = W->daughter(iWdau);
  							isHardProcess = false;
  							}
  						}
  					}
  				for(size_t iWdau = 0; iWdau < W->numberOfDaughters(); iWdau++){
					int WdauId = W->daughter(iWdau)->pdgId();
					if(abs(WdauId) > 0 && abs(WdauId) < 6){
						topdauLV.SetPtEtaPhiE(W->daughter(iWdau)->pt(),W->daughter(iWdau)->eta(),W->daughter(iWdau)->phi(),W->daughter(iWdau)->energy());
						mygenTopDaughtersTemp_.push_back(topdauLV);
						foundW = true;
						}
					}//W daughter loop
  				}
  			}//top daughter loop
  		if(foundB && foundW){ // only save hadronicly decaying tops
  			mygenTopDaughters_.push_back(mygenTopDaughtersTemp_);
  			topLV.SetPtEtaPhiE(p.pt(),p.eta(),p.phi(),p.energy());
  			mygenTops.push_back(topLV);
  			}
		}//gen loop
    }

  std::vector<std::vector<const TLorentzVector*>> mygenTopDaughters;
  for(size_t itop = 0; itop < mygenTops.size(); itop++){
  	std::vector<const TLorentzVector*> mygenTopsTemp;
  	for(size_t idau = 0; idau < mygenTopDaughters_[itop].size(); idau++){
  		mygenTopsTemp.push_back(&mygenTopDaughters_[itop][idau]);
  		}
  	mygenTopDaughters.push_back(mygenTopsTemp);
  	}

  std::vector<double> topGenPt;
  std::vector<double> topGenPhi;
  std::vector<double> topGenEta;
  std::vector<double> topGenMass;
  std::vector<double> topD1GenPt;
  std::vector<double> topD1GenPhi;
  std::vector<double> topD1GenEta;
  std::vector<double> topD1GenMass;
  std::vector<double> topD2GenPt;
  std::vector<double> topD2GenPhi;
  std::vector<double> topD2GenEta;
  std::vector<double> topD2GenMass;
  std::vector<double> topD3GenPt;
  std::vector<double> topD3GenPhi;
  std::vector<double> topD3GenEta;
  std::vector<double> topD3GenMass;

  for (unsigned int igtop = 0; igtop < mygenTops.size(); igtop++){
  	topGenPt.push_back(mygenTops[igtop].Pt());
  	topGenPhi.push_back(mygenTops[igtop].Phi());
  	topGenEta.push_back(mygenTops[igtop].Eta());
  	topGenMass.push_back(mygenTops[igtop].M());
    double pt1 = -999;
    double phi1 = -999;
    double eta1 = -999;
    double mass1 = -999;
    double pt2 = -999;
    double phi2 = -999;
    double eta2 = -999;
    double mass2 = -999;
    double pt3 = -999;
    double phi3 = -999;
    double eta3 = -999;
    double mass3 = -999;
  	if(mygenTopDaughters_[igtop].size()>0){
  		pt1 = mygenTopDaughters_[igtop][0].Pt();
  		phi1 = mygenTopDaughters_[igtop][0].Phi();
  		eta1 = mygenTopDaughters_[igtop][0].Eta();
  		mass1 = mygenTopDaughters_[igtop][0].M();
  		}
  	if(mygenTopDaughters_[igtop].size()>1){
  		pt2 = mygenTopDaughters_[igtop][1].Pt();
  		phi2 = mygenTopDaughters_[igtop][1].Phi();
  		eta2 = mygenTopDaughters_[igtop][1].Eta();
  		mass2 = mygenTopDaughters_[igtop][1].M();
  		}
  	if(mygenTopDaughters_[igtop].size()>2){
  		pt3 = mygenTopDaughters_[igtop][2].Pt();
  		phi3 = mygenTopDaughters_[igtop][2].Phi();
  		eta3 = mygenTopDaughters_[igtop][2].Eta();
  		mass3 = mygenTopDaughters_[igtop][2].M();
  		}

	topD1GenPt.push_back(pt1);
	topD1GenPhi.push_back(phi1);
	topD1GenEta.push_back(eta1);
	topD1GenMass.push_back(mass1);
	topD2GenPt.push_back(pt2);
	topD2GenPhi.push_back(phi2);
	topD2GenEta.push_back(eta2);
	topD2GenMass.push_back(mass2);
	topD3GenPt.push_back(pt3);
	topD3GenPhi.push_back(phi3);
	topD3GenEta.push_back(eta3);
	topD3GenMass.push_back(mass3);
  	}
  
  std::vector<TLorentzVector> perJetLVec_;
  std::vector<float> qgLikelihood_;
  std::vector<float> qgPtD;
  std::vector<float> qgAxis1;
  std::vector<float> qgAxis2;
  std::vector<float> qgMult;
  std::vector<float> deepCSVb;
  std::vector<float> deepCSVc;
  std::vector<float> deepCSVl;
  std::vector<float> deepCSVbb;
  std::vector<float> deepCSVcc;
  std::vector<float> recoJetsBtag_;
  std::vector<float> chargedHadronEnergyFraction;
  std::vector<float> neutralHadronEnergyFraction;
  std::vector<float> chargedEmEnergyFraction;
  std::vector<float> neutralEmEnergyFraction;
  std::vector<float> muonEnergyFraction;
  std::vector<float> photonEnergyFraction;
  std::vector<float> electronEnergyFraction;
  std::vector<float> recoJetsHFHadronEnergyFraction;
  std::vector<float> recoJetsHFEMEnergyFraction;
  std::vector<float> chargedHadronMultiplicity;
  std::vector<float> neutralHadronMultiplicity;
  std::vector<float> photonMultiplicity;
  std::vector<float> electronMultiplicity;
  std::vector<float> muonMultiplicity;
  
  perJetLVec_.clear();
  qgLikelihood_.clear();
  qgPtD.clear();
  qgAxis1.clear();
  qgAxis2.clear();
  qgMult.clear();
  deepCSVb.clear();
  deepCSVc.clear();
  deepCSVl.clear();
  deepCSVbb.clear();
  deepCSVcc.clear();
  recoJetsBtag_.clear();
  chargedHadronEnergyFraction.clear();
  neutralHadronEnergyFraction.clear();
  chargedEmEnergyFraction.clear();
  neutralEmEnergyFraction.clear();
  muonEnergyFraction.clear();
  photonEnergyFraction.clear();
  electronEnergyFraction.clear();
  recoJetsHFHadronEnergyFraction.clear();
  recoJetsHFEMEnergyFraction.clear();
  chargedHadronMultiplicity.clear();
  neutralHadronMultiplicity.clear();
  photonMultiplicity.clear();
  electronMultiplicity.clear();
  muonMultiplicity.clear();
      
  int nAK4 = 0; // to check against the number of jets from singleLepCalc, they should be the same
  for (std::vector<pat::Jet>::const_iterator ijet = vSelCorrJets.begin(); ijet != vSelCorrJets.end(); ijet++){
      
    const pat::Jet jet = *ijet;
      
    //Apply pt cut on jets -- this should do nothing if the cut is left at 20
    if(jet.pt() < ak4ptCut_) continue;
    nAK4++;

    TLorentzVector perJetLVecTemp(jet.p4().X(), jet.p4().Y(), jet.p4().Z(), jet.p4().T());
      
    perJetLVec_.push_back(perJetLVecTemp);
    qgLikelihood_.push_back(0.0);
    qgPtD.push_back(jet.userFloat("QGTagger:ptD"));
    qgAxis1.push_back(jet.userFloat("QGTagger:axis1"));
    qgAxis2.push_back(jet.userFloat("QGTagger:axis2"));
    qgMult.push_back(static_cast<double>(jet.userInt("QGTagger:mult")));
    deepCSVb.push_back(jet.bDiscriminator((deepCSVBJetTags_+":probb").c_str()));
    deepCSVc.push_back(jet.bDiscriminator((deepCSVBJetTags_+":probc").c_str()));
    deepCSVl.push_back(jet.bDiscriminator((deepCSVBJetTags_+":probudsg").c_str()));
    deepCSVbb.push_back(jet.bDiscriminator((deepCSVBJetTags_+":probbb").c_str()));
    deepCSVcc.push_back(jet.bDiscriminator((deepCSVBJetTags_+":probcc").c_str()));
    recoJetsBtag_.push_back(jet.bDiscriminator(bTagKeyString_.c_str()));
    chargedHadronEnergyFraction.push_back(jet.chargedHadronEnergyFraction());
    neutralHadronEnergyFraction.push_back(jet.neutralHadronEnergyFraction());
    chargedEmEnergyFraction.push_back(jet.chargedEmEnergyFraction());
    neutralEmEnergyFraction.push_back(jet.neutralEmEnergyFraction());
    muonEnergyFraction.push_back(jet.muonEnergyFraction());
    photonEnergyFraction.push_back(jet.photonEnergyFraction());
    electronEnergyFraction.push_back(jet.electronEnergyFraction());
    recoJetsHFHadronEnergyFraction.push_back(jet.HFHadronEnergyFraction());
    recoJetsHFEMEnergyFraction.push_back(jet.HFEMEnergyFraction());
    chargedHadronMultiplicity.push_back(jet.chargedHadronMultiplicity());
    neutralHadronMultiplicity.push_back(jet.neutralHadronMultiplicity());
    photonMultiplicity.push_back(jet.photonMultiplicity());
    electronMultiplicity.push_back(jet.electronMultiplicity());
    muonMultiplicity.push_back(jet.muonMultiplicity());
    
  }

  //New Tagger starts here
  ttUtility::ConstAK4Inputs<float> *myConstAK4Inputs = nullptr;
  
  std::vector<TLorentzVector> *genTops;
  std::vector<std::vector<const TLorentzVector*>> *genTopDaughters = nullptr;
  genTops = new std::vector<TLorentzVector>(std::move(mygenTops));
  genTopDaughters = new std::vector<std::vector<const TLorentzVector*>>(std::move(mygenTopDaughters));
  
  const std::vector<TLorentzVector>& jetsLVec = perJetLVec_;
  const std::vector<float>& recoJetsBtag      = recoJetsBtag_;
  const std::vector<float>& qgLikelihood      = qgLikelihood_;
  myConstAK4Inputs = new ttUtility::ConstAK4Inputs<float>(jetsLVec, recoJetsBtag, qgLikelihood, *genTops, *genTopDaughters);
  
  myConstAK4Inputs->addSupplamentalVector("qgMult"                              , qgMult);
  myConstAK4Inputs->addSupplamentalVector("qgPtD"                               , qgPtD);
  myConstAK4Inputs->addSupplamentalVector("qgAxis1"                             , qgAxis1);
  myConstAK4Inputs->addSupplamentalVector("qgAxis2"                             , qgAxis2);
  myConstAK4Inputs->addSupplamentalVector("recoJetschargedHadronEnergyFraction" , chargedHadronEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetschargedEmEnergyFraction"     , chargedEmEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetsneutralEmEnergyFraction"     , neutralEmEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetsmuonEnergyFraction"          , muonEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetsHFHadronEnergyFraction"      , recoJetsHFHadronEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetsHFEMEnergyFraction"          , recoJetsHFEMEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("recoJetsneutralEnergyFraction"       , neutralHadronEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("PhotonEnergyFraction"                , photonEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("ElectronEnergyFraction"              , electronEnergyFraction);
  myConstAK4Inputs->addSupplamentalVector("ChargedHadronMultiplicity"           , chargedHadronMultiplicity);
  myConstAK4Inputs->addSupplamentalVector("NeutralHadronMultiplicity"           , neutralHadronMultiplicity);
  myConstAK4Inputs->addSupplamentalVector("PhotonMultiplicity"                  , photonMultiplicity);
  myConstAK4Inputs->addSupplamentalVector("ElectronMultiplicity"                , electronMultiplicity);
  myConstAK4Inputs->addSupplamentalVector("MuonMultiplicity"                    , muonMultiplicity);
  myConstAK4Inputs->addSupplamentalVector("DeepCSVb"                            , deepCSVb);
  myConstAK4Inputs->addSupplamentalVector("DeepCSVc"                            , deepCSVc);
  myConstAK4Inputs->addSupplamentalVector("DeepCSVl"                            , deepCSVl);
  myConstAK4Inputs->addSupplamentalVector("DeepCSVbb"                           , deepCSVbb);
  myConstAK4Inputs->addSupplamentalVector("DeepCSVcc"                           , deepCSVcc);
  
  constituents = ttUtility::packageConstituents(*myConstAK4Inputs);

  //run top tagger
  try{
    tt.runTagger(std::move(constituents));
  }
  catch(const TTException& e){
    //Convert the TTException into a cms::Exception
    throw cms::Exception(e.getFileName() + ":" + std::to_string(e.getLineNumber()) + ", in function \"" + e.getFunctionName() + "\" -- " + e.getMessage());
  }
  
  //retrieve the top tagger results object
  const TopTaggerResults& ttr = tt.getResults();

  //get reconstructed top
  const std::vector<TopObject*>& tops = ttr.getTops();
  
  std::vector<double> topPt;
  std::vector<double> topPhi;
  std::vector<double> topEta;
  std::vector<double> topMass;
  std::vector<double> topDRmax;
  std::vector<double> topDThetaMin;
  std::vector<double> topDThetaMax;
  std::vector<double> topDiscriminator;
  std::vector<double> topType;
  std::vector<double> topNconstituents;
  std::vector<int>    topJet1Index;
  std::vector<int>    topJet2Index;
  std::vector<int>    topJet3Index;
  std::vector<double> topBestGen2dPt;
  std::vector<double> topBestGen2dPhi;
  std::vector<double> topBestGen2dEta;
  std::vector<double> topBestGen2dEnergy;
  std::vector<double> topBestGen3dPt;
  std::vector<double> topBestGen3dPhi;
  std::vector<double> topBestGen3dEta;
  std::vector<double> topBestGen3dEnergy;

  int topNtops = tops.size();
  for (unsigned int itop = 0; itop < tops.size(); itop++){
    TopObject *thistop = tops.at(itop);
    topPt.push_back(thistop->p().Pt());
    topPhi.push_back(thistop->p().Phi());
    topEta.push_back(thistop->p().Eta());
    topMass.push_back(thistop->p().M());
    topDRmax.push_back(thistop->getDRmax());
    topDThetaMin.push_back(thistop->getDThetaMin());
    topDThetaMax.push_back(thistop->getDThetaMax());
    topDiscriminator.push_back(thistop->getDiscriminator());
    topType.push_back(thistop->getType());
    topNconstituents.push_back(thistop->getNConstituents());
  
    std::vector<Constituent const*> thistopConsts = thistop->getConstituents();
    topJet1Index.push_back(thistopConsts.at(0)->getIndex());
    topJet2Index.push_back(thistopConsts.at(1)->getIndex());
    topJet3Index.push_back(thistopConsts.at(2)->getIndex());
  
    double pt_2d = -999;
    double phi_2d = -999;
    double eta_2d = -999;
    double energy_2d = -999;
    double pt_3d = -999;
    double phi_3d = -999;
    double eta_3d = -999;
    double energy_3d = -999;
    const TLorentzVector *bestGen2d = thistop->getBestGenTopMatch(0.6, 2, 2);
    if(bestGen2d){
      pt_2d = bestGen2d->Pt();
      phi_2d = bestGen2d->Phi();
      eta_2d = bestGen2d->Eta();
      energy_2d = bestGen2d->Energy();
    }
    topBestGen2dPt.push_back(pt_2d);
    topBestGen2dPhi.push_back(phi_2d);
    topBestGen2dEta.push_back(eta_2d);
    topBestGen2dEnergy.push_back(energy_2d);
    const TLorentzVector *bestGen3d = thistop->getBestGenTopMatch(0.6, 3, 3);
    if(bestGen3d){
      pt_3d = bestGen3d->Pt();
      phi_3d = bestGen3d->Phi();
      eta_3d = bestGen3d->Eta();
      energy_3d = bestGen3d->Energy();
    }
    topBestGen3dPt.push_back(pt_3d);
    topBestGen3dPhi.push_back(phi_3d);
    topBestGen3dEta.push_back(eta_3d);
    topBestGen3dEnergy.push_back(energy_3d);
  }

  SetValue("topNAK4",         nAK4);
  SetValue("topNtops",        topNtops);
  SetValue("topPt",		      topPt);		  
  SetValue("topPhi",	      topPhi);	  
  SetValue("topEta",	      topEta);	  
  SetValue("topMass",	      topMass);	  
  SetValue("topDRmax",	      topDRmax);	  
  SetValue("topDThetaMin",	  topDThetaMin);	  
  SetValue("topDThetaMax",	  topDThetaMax);	  
  SetValue("topDiscriminator",topDiscriminator);
  SetValue("topType",	      topType);	  
  SetValue("topNconstituents",topNconstituents);
  SetValue("topJet1Index",	  topJet1Index);	  
  SetValue("topJet2Index",	  topJet2Index);	  
  SetValue("topJet3Index",	  topJet3Index);	  
  SetValue("topBestGen2dPt",	topBestGen2dPt);	  
  SetValue("topBestGen2dPhi",	topBestGen2dPhi);	  
  SetValue("topBestGen2dEta",	topBestGen2dEta);	  
  SetValue("topBestGen2dEnergy",topBestGen2dEnergy);
  SetValue("topBestGen3dPt",	topBestGen3dPt);	  
  SetValue("topBestGen3dPhi",	topBestGen3dPhi);	  
  SetValue("topBestGen3dEta",	topBestGen3dEta);	  
  SetValue("topBestGen3dEnergy",topBestGen3dEnergy);
  
  SetValue("topGenPt",    topGenPt);
  SetValue("topGenPhi",   topGenPhi);
  SetValue("topGenEta",   topGenEta);
  SetValue("topGenMass",  topGenMass);
  SetValue("topD1GenPt",  topD1GenPt);
  SetValue("topD1GenPhi", topD1GenPhi);
  SetValue("topD1GenEta", topD1GenEta);
  SetValue("topD1GenMass",topD1GenMass);
  SetValue("topD2GenPt",  topD2GenPt);
  SetValue("topD2GenPhi", topD2GenPhi);
  SetValue("topD2GenEta", topD2GenEta);
  SetValue("topD2GenMass",topD2GenMass);
  SetValue("topD3GenPt",  topD3GenPt);
  SetValue("topD3GenPhi", topD3GenPhi);
  SetValue("topD3GenEta", topD3GenEta);
  SetValue("topD3GenMass",topD3GenMass);
  
  return 0;
}

