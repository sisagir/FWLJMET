import FWCore.ParameterSet.Config as cms
import os


relBase = os.environ['CMSSW_BASE']


## PARSE ARGUMENTS
from FWCore.ParameterSet.VarParsing import VarParsing
options = VarParsing('analysis')
options.register('isMC', '', VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'Is MC')
options.register('isTTbar', '', VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'Is TTbar')
options.register('isVLQsignal', '', VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'Is VLQ Signal')

## SET DEFAULT VALUES
## ATTENTION: THESE DEFAULT VALUES ARE SET FOR VLQ SIGNAL ! isMC=True, isTTbar=False, isVLQsignal=True 
options.isMC = ISMC
options.isTTbar = ISTTBAR
options.isVLQsignal = ISVLQSIGNAL
options.inputFiles = [
    #'root://cmsxrootd.fnal.gov//store/mc/RunIIFall17MiniAODv2/TTTT_TuneCP5_PSweights_13TeV-amcatnlo-pythia8/MINIAODSIM/PU2017_12Apr2018_94X_mc2017_realistic_v14-v1/70000/0ED34A55-DD52-E811-91CC-E0071B73B6B0.root'
    'root://cmsxrootd.fnal.gov//store/mc/RunIIFall17MiniAODv2/TprimeTprime_M-1400_TuneCP5_13TeV-madgraph-pythia8/MINIAODSIM/PU2017_12Apr2018_94X_mc2017_realistic_v14-v2/50000/F82DC089-5591-E811-9210-6C3BE5B58198.root'
    #'root://cmsxrootd-site.fnal.gov//store/mc/RunIIFall17MiniAODv2/WJetsToLNu_HT-200To400_TuneCP5_13TeV-madgraphMLM-pythia8/MINIAODSIM/PU2017_12Apr2018_94X_mc2017_realistic_v14-v1/10000/001187B6-E554-E811-89C7-24BE05C63721.root'
    ]
options.maxEvents = 100
options.parseArguments()

isMC= options.isMC
isTTbar = options.isTTbar
isVLQsignal = options.isVLQsignal

#Check arguments
print options

## LJMET
process = cms.Process("LJMET")

## MessageLogger
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 20

## Options and Output Report
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )

## Check memory and timing - TURN OFF when not testing!
import FWCore.ParameterSet.Config as cms
def customise(process):

    #Adding SimpleMemoryCheck service:
    process.SimpleMemoryCheck=cms.Service("SimpleMemoryCheck",
                                          ignoreTotal=cms.untracked.int32(1),
                                          oncePerEventMode=cms.untracked.bool(True))
    #Adding Timing service:
    process.Timing=cms.Service("Timing")

    #Add these 3 lines to put back the summary for timing information at the end of the logfile
    #(needed for TimeReport report)
    if hasattr(process,'options'):
        process.options.wantSummary = cms.untracked.bool(True)
    else:
        process.options = cms.untracked.PSet(
            wantSummary = cms.untracked.bool(True)
        )

    return(process)
# customise(process)

## Multithreading option
process.options.numberOfThreads=cms.untracked.uint32(1)
process.options.numberOfStreams=cms.untracked.uint32(0)

## Maximal Number of Events
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(options.maxEvents) )

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(cms.untracked.vstring(options.inputFiles),
    )
)

OUTFILENAME = "DATASET"
process.TFileService = cms.Service("TFileService", fileName = cms.string(OUTFILENAME+'.root'))

################################
## For updateJetCollection
################################
from PhysicsTools.PatAlgos.tools.helpers import getPatAlgosToolsTask
patAlgosToolsTask = getPatAlgosToolsTask(process)
# process.outpath = cms.EndPath(process.out, patAlgosToolsTask)


## Geometry and Detector Conditions (needed for a few patTuple production steps)
process.load("Configuration.Geometry.GeometryRecoDB_cff")
process.load('Configuration.StandardSequences.Services_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '94X_mc2017_realistic_v17', '')
if isMC == False: process.GlobalTag = GlobalTag(process.GlobalTag, '94X_dataRun2_v11')
print 'Using global tag', process.GlobalTag.globaltag

################################
## Produce DeepJet AK4 tags
################################
from PhysicsTools.PatAlgos.tools.jetTools import updateJetCollection

updateJetCollection(
    process,
    jetSource = cms.InputTag('slimmedJets'),
    pvSource = cms.InputTag('offlineSlimmedPrimaryVertices'),
    svSource = cms.InputTag('slimmedSecondaryVertices'),
    jetCorrections = ('AK4PFchs', cms.vstring(['L1FastJet', 'L2Relative', 'L3Absolute']), 'None'),
    btagDiscriminators = [
        'pfDeepFlavourJetTags:probb',
        'pfDeepFlavourJetTags:probbb',
        'pfDeepFlavourJetTags:problepb',
        'pfDeepFlavourJetTags:probc',
        'pfDeepFlavourJetTags:probuds',
        'pfDeepFlavourJetTags:probg'
        ],
    postfix='NewDFTraining',
    printWarning=False
    )

################################
## MC Weights Analyzer
################################
process.mcweightanalyzer = cms.EDAnalyzer(
    "WeightAnalyzerBEff",
    JetsTag = cms.InputTag("selectedUpdatedPatJetsNewDFTraining"),
    DiscriminatorValueTight = cms.double(0.7489),
    DiscriminatorValueMedium = cms.double(0.3033),
    DiscriminatorValueLoose = cms.double(0.0521),
    )

if(isMC):
    process.p = cms.Path(
        process.mcweightanalyzer
        )
else:
    process.p = cms.Path(
        )

process.p.associate(patAlgosToolsTask)


# process.ep = cms.EndPath(process.out)
# process.ep = cms.EndPath(process.out,patAlgosToolsTask)
# process.scedule = cms.Schedule(
#     process.p,
#     process.outpath)

