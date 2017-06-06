
// -*- C++ -*-
//
// Package:     Services
// Class  :     MessageLogger
// 
//
// Original Author:  Marc Paterno
// $Id: JobReport.cc,v 1.23 2007/09/28 18:56:58 evansde Exp $
//


#include "FWCore/MessageLogger/interface/JobReport.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/EDMException.h"

#include <sstream>

using std::string;
using std::vector;
using std::ostream;
      
namespace edm
{
    /*
     * Note that output formatting is spattered across these classes
     * If something outside these classes requires access to the 
     * same formatting then we need to refactor it into a common library
     */
    ostream& 
    operator<< (ostream& os, JobReport::InputFile const& f) {
      
      os << "\n<InputFile>";
      formatFile<JobReport::InputFile>(f, os);
      os << "\n<InputSourceClass>" << f.inputSourceClassName 
	 << "</InputSourceClass>";
      os << "\n<EventsRead>" << f.numEventsRead << "</EventsRead>";
      os << "\n</InputFile>";
      return os;
    }


    ostream& 
    operator<< (ostream& os, JobReport::OutputFile const& f) {
      formatFile<JobReport::OutputFile>(f, os);           
      os << "\n<OutputModuleClass>" 
			<< f.outputModuleClassName 
			<< "</OutputModuleClass>";
      os << "\n<TotalEvents>" 
			<< f.numEventsWritten 
			<< "</TotalEvents>\n";
      os << "\n<DataType>" 
			<< f.dataType 
			<< "</DataType>\n";
      os << "\n<BranchHash>" 
			<< f.branchHash 
			<< "</BranchHash>\n";
      
      return os;      
    }

    ostream&
    operator<< (std::ostream& os, 
		JobReport::LumiSectionReport const& rep){
      os << "\n<LumiSection>\n"
	 << "<LumiSectionNumber Value=\""
	 << rep.lumiSectionId
	 << "\"/>\n"
	 << "<RunNumber Value=\""
	 << rep.runNumber
	 << "\"/>\n"
	 << "</LumiSection>\n";

      
	return os;
     }


    JobReport::InputFile& JobReport::JobReportImpl::getInputFileForToken(JobReport::Token t) {
	if (t >= inputFiles_.size() ) {
	    throw edm::Exception(edm::errors::LogicError)
	      << "Access reported for input file with token "
	      << t
	      << " but no matching input file is found\n";
	}

	if (inputFiles_[t].fileHasBeenClosed) {
	    throw edm::Exception(edm::errors::LogicError)
	      << "Access reported for input file with token "
	      << t
	      << " after this file has been closed.\n"
	      << "File record follows:\n"
	      << inputFiles_[t]
	      << '\n';
	}
	 
      return inputFiles_[t];
    }

    JobReport::OutputFile& JobReport::JobReportImpl::getOutputFileForToken(JobReport::Token t) {
	if (t >= outputFiles_.size() ) {
	    throw edm::Exception(edm::errors::LogicError)
	      << "Access reported for output file with token "
	      << t
	      << " but no matching output file is found\n";
	}
	if (outputFiles_[t].fileHasBeenClosed) {
	    throw edm::Exception(edm::errors::LogicError)
	      << "Access reported for output file with token "
	      << t
	      << " after this file has been closed.\n"
	      << "File record follows:\n"
	      << outputFiles_[t]
	      << '\n';
	}
      return outputFiles_[t];
    }
    
    /*
     * Add the input file token provided to every output 
     * file currently available.
     * Used whenever a new input file is opened, it's token
     * is added to all open output files as a contributor
     */
    void JobReport::JobReportImpl::insertInputForOutputs(JobReport::Token t) {
	std::vector<JobReport::OutputFile>::iterator outFile;
	for (outFile = outputFiles_.begin(); 
	     outFile != outputFiles_.end();
	     outFile++){
	  outFile->contributingInputs.push_back(t);
	}
    }
    /*
     * get a vector of Tokens for all currently open
     * input files. 
     * Used when a new output file is opened, all currently open
     * input file tokens are used to initialise its list of contributors
     */
    std::vector<JobReport::Token> JobReport::JobReportImpl::openInputFiles(void) {
	std::vector<JobReport::Token> result;
	for (unsigned int i = 0; i < inputFiles_.size(); ++i) {
	  JobReport::InputFile inFile = inputFiles_[i];
	  if ( inFile.fileHasBeenClosed == false){
	    result.push_back(i);
	  }
	}
	return result;
    }

    /*
     * get a vector of Tokens for all currently open
     * output files. 
     * 
     */
    std::vector<JobReport::Token> JobReport::JobReportImpl::openOutputFiles(void) {
	std::vector<JobReport::Token> result;
	for (unsigned int i = 0; i < outputFiles_.size(); ++i) {
	  JobReport::OutputFile outFile = outputFiles_[i];
	  if ( outFile.fileHasBeenClosed == false){
	    result.push_back(i);
	  }
	}
	return result;
    }

    /*
     * Write anJobReport::InputFile object to the Logger 
     * Generate XML string forJobReport::InputFile instance and dispatch to 
     * job report via MessageLogger
     */
    void JobReport::JobReportImpl::writeInputFile(JobReport::InputFile const& f){
	LogInfo("FwkJob") << f;
    }
    
    /*
     * Write an OutputFile object to the Logger 
     * Generate an XML string for the OutputFile provided and
     * dispatch it to the logger
     * Contributing input tokens are resolved to the input LFN and PFN
     * 
     * TODO: We have not yet addressed the issue where we cleanup not
     * contributing input files. 
     * Also, it is possible to get fake input to output file mappings
     * if an input file is open already when a new output file is opened
     * but the input gets closed without contributing events to the
     * output file due to filtering etc.
     *
     */
    void JobReport::JobReportImpl::writeOutputFile(JobReport::OutputFile const& f) {
	LogInfo("FwkJob") << "\n<File>";
	LogInfo("FwkJob") << f;
	
	LogInfo("FwkJob") << "\n<LumiSections>";
	std::vector<JobReport::LumiSectionReport>::const_iterator iLumi;
	for (iLumi = f.lumiSections.begin();
	     iLumi != f.lumiSections.end(); iLumi++){
	  LogInfo("FwkJob") << *iLumi;
	}
	LogInfo("FwkJob") << "\n</LumiSections>\n";
	  
	LogInfo("FwkJob") << "\n<Inputs>";
 	std::vector<JobReport::Token>::const_iterator iInput;
 	for (iInput = f.contributingInputs.begin(); 
 	     iInput != f.contributingInputs.end(); iInput++) {
 	    JobReport::InputFile inpFile = inputFiles_[*iInput];
 	    LogInfo("FwkJob") <<"\n<Input>";
 	    LogInfo("FwkJob") <<"\n  <LFN>" << inpFile.logicalFileName << "</LFN>";
 	    LogInfo("FwkJob") <<"\n  <PFN>" << inpFile.physicalFileName << "</PFN>";
 	    LogInfo("FwkJob") <<"\n</Input>";
 	}
 	LogInfo("FwkJob") << "\n</Inputs>";
 	LogInfo("FwkJob") << "\n</File>";
    }
    
    /*
     *  Flush all open files to logger in event of a problem.
     *  Called from JobReport dtor to flush any remaining open files
     */ 
    void JobReport::JobReportImpl::flushFiles(void) {
      std::vector<JobReport::InputFile>::iterator ipos;
      std::vector<JobReport::OutputFile>::iterator opos;
      for (ipos = inputFiles_.begin(); ipos != inputFiles_.end(); ++ipos) {
          if (!(ipos->fileHasBeenClosed)) {
            writeInputFile(*ipos);
          }
      }
      for (opos = outputFiles_.begin(); opos != outputFiles_.end(); ++opos) {
	if (!(opos->fileHasBeenClosed)) {
	  writeOutputFile(*opos);
	}
      }
    }

  void JobReport::JobReportImpl::addGeneratorInfo(std::string const& name, 
						  std::string const& value){
    
    generatorInfo_[name] = value;
  }

  void JobReport::JobReportImpl::writeGeneratorInfo(void){
    LogInfo("FwkJob") << "\n<GeneratorInfo>";
    std::map<std::string, std::string>::iterator pos;
    for (pos = generatorInfo_.begin(); pos != generatorInfo_.end(); ++pos){
      std::ostringstream msg;
      msg << "\n<Data Name=\"" << pos->first
			<< "\" Value=\"" << pos->second << "\"/>";
      LogInfo("FwkJob") << msg.str();
    }
    LogInfo("FwkJob") << "</GeneratorInfo>";
    
  }

  void JobReport::JobReportImpl::associateLumiSection(JobReport::LumiSectionReport const&  rep){
    std::vector<Token> openFiles = openOutputFiles();
    std::vector<Token>::iterator iToken;
    for (iToken = openFiles.begin(); iToken != openFiles.end(); iToken++){
      //
      // Loop over all open output files
      //
      JobReport::OutputFile & theFile = outputFiles_[*iToken];
      //
      // check known lumi sections for each file
      //
      std::vector<JobReport::LumiSectionReport>::iterator iLumi;
      bool lumiKnownByFile = false;
      for (iLumi = theFile.lumiSections.begin();
	   iLumi != theFile.lumiSections.end(); iLumi++){
	
	if ( (iLumi->lumiSectionId == rep.lumiSectionId) && 
	     (iLumi->runNumber == rep.runNumber) ){
	  //
	  // This file already has this lumi section associated to it
	  // dont report it twice
	  
	  lumiKnownByFile = true;
	}
      }
      if (lumiKnownByFile == false){
	//
	// New lumi section for file: associate lumi section with it.
	//
	JobReport::LumiSectionReport newReport;
	newReport.runNumber = rep.runNumber;
	newReport.lumiSectionId = rep.lumiSectionId;
	theFile.lumiSections.push_back(newReport);
      }
    }
    
    
  }

  JobReport::~JobReport() {
    impl_->writeGeneratorInfo(); 
    impl_->flushFiles();
  }

    JobReport::JobReport() :
      impl_(new JobReportImpl) {
    }

    JobReport::Token
    JobReport::inputFileOpened(string const& physicalFileName,
			       string const& logicalFileName,
			       string const& catalog,
			       string const& inputSourceClassName,
			       string const& moduleLabel,
			       string const& guid,
			       vector<string> const& branchNames)
    {
      // Do we have to worry about thread safety here? Or is this
      // service used in a way to make this safe?
      impl_->inputFiles_.push_back(JobReport::InputFile());
      JobReport::InputFile& r = impl_->inputFiles_.back();

      r.logicalFileName      = logicalFileName;
      r.physicalFileName     = physicalFileName;
      r.catalog              = catalog;
      r.inputSourceClassName = inputSourceClassName;
      r.moduleLabel          = moduleLabel;
      r.guid                 = guid;
      // r.runsSeen is not modified
      r.numEventsRead        = 0;
      r.branchNames          = branchNames;
      r.fileHasBeenClosed    = false;
    
      JobReport::Token newToken = impl_->inputFiles_.size()-1;
        //
       // Add the new input file token to all output files
      //  currently open.
      impl_->insertInputForOutputs(newToken);
      return newToken;
    }

    JobReport::Token
    JobReport::inputFileOpened(string const& physicalFileName,
			       string const& logicalFileName,
			       string const& catalog,
			       string const& inputSourceClassName,
			       string const& moduleLabel,
			       vector<string> const& branchNames)
    {
      return this->inputFileOpened(physicalFileName,
				   logicalFileName,
				   catalog,
				   inputSourceClassName,
				   moduleLabel,
				   "",
				   branchNames);
    }
  
    void
    JobReport::eventReadFromFile(JobReport::Token fileToken, unsigned int run, unsigned int)
    {
      JobReport::InputFile& f = impl_->getInputFileForToken(fileToken);
      f.numEventsRead++;
      f.runsSeen.insert(run);
    }

    void
    JobReport::inputFileClosed(JobReport::Token fileToken)
    {
      JobReport::InputFile& f = impl_->getInputFileForToken(fileToken);
      // Dump information to the MessageLogger's JobSummary
      // about this file.
      // After setting the file to 'closed', we will no longer be able
      // to reference it by ID.
      f.fileHasBeenClosed = true;
      impl_->writeInputFile(f);
    }

    JobReport::Token 
    JobReport::outputFileOpened(string const& physicalFileName,
				string const& logicalFileName,
				string const& catalog,
				string const& outputModuleClassName,
				string const& moduleLabel,
				string const& guid,
				string const& dataType,
				string const& branchHash,
				vector<string> const& branchNames)
    {
      impl_->outputFiles_.push_back(JobReport::OutputFile());
      JobReport::OutputFile& r = impl_->outputFiles_.back();
      
      r.logicalFileName       = logicalFileName;
      r.physicalFileName      = physicalFileName;
      r.catalog               = catalog;
      r.outputModuleClassName = outputModuleClassName;
      r.moduleLabel           = moduleLabel;
      r.guid           = guid;
      r.dataType = dataType;
      r.branchHash = branchHash;
      // r.runsSeen is not modified
      r.numEventsWritten      = 0;
      r.branchNames           = branchNames;
      r.fileHasBeenClosed     = false;
        //
       // Init list of contributors to list of open input file Tokens
      //
      r.contributingInputs = std::vector<JobReport::Token>(impl_->openInputFiles());
      return impl_->outputFiles_.size()-1;
    }

    JobReport::Token 
    JobReport::outputFileOpened(string const& physicalFileName,
				string const& logicalFileName,
				string const& catalog,
				string const& outputModuleClassName,
				string const& moduleLabel,
				string const& guid,
				string const& dataType,
				vector<string> const& branchNames)
    {
      return this->outputFileOpened(physicalFileName,
				    logicalFileName,
				    catalog,
				    outputModuleClassName,
				    moduleLabel,
				    guid,
				    "",
				    "NO_BRANCH_HASH",
				    branchNames);
      
      
    }
  
    JobReport::Token 
    JobReport::outputFileOpened(string const& physicalFileName,
				string const& logicalFileName,
				string const& catalog,
				string const& outputModuleClassName,
				string const& moduleLabel,
				string const& guid,
				vector<string> const& branchNames)
    {
      return this->outputFileOpened(physicalFileName,
				    logicalFileName,
				    catalog,
				    outputModuleClassName,
				    moduleLabel,
				    guid,
				    "",
				    branchNames);

    }
  

  
  JobReport::Token 
  JobReport::outputFileOpened(string const& physicalFileName,
			      string const& logicalFileName,
			      string const& catalog,
			      string const& outputModuleClassName,
			      string const& moduleLabel,
			      vector<string> const& branchNames)
  {
   
    return this->outputFileOpened(physicalFileName,
				  logicalFileName,
				  catalog,
				  outputModuleClassName,
				  moduleLabel,
				  "",
				  "",
				  branchNames);
  }
  


    void
    JobReport::eventWrittenToFile(JobReport::Token fileToken, unsigned int run, unsigned int)
    {
      JobReport::OutputFile& f = impl_->getOutputFileForToken(fileToken);
      f.numEventsWritten++;
      f.runsSeen.insert(run);
    }


    void
    JobReport::outputFileClosed(JobReport::Token fileToken)
    {
      JobReport::OutputFile& f = impl_->getOutputFileForToken(fileToken);
      // Dump information to the MessageLogger's JobSummary
      // about this file.
      // After setting the file to 'closed', we will no longer be able
      // to reference it by ID.
      f.fileHasBeenClosed = true;
      impl_->writeOutputFile(f);

    }

    void 
    JobReport::overrideEventsWritten(Token fileToken, const int eventsWritten)
    {
      // Get the required output file instance using the token
      JobReport::OutputFile& f = impl_->getOutputFileForToken(fileToken);
      // set the eventsWritten parameter to the provided value
      f.numEventsWritten = eventsWritten;

    }

    void 
    JobReport::overrideEventsRead(Token fileToken, const int eventsRead)
    {
      // Get the required input file instance using the token
      JobReport::InputFile& f = impl_->getInputFileForToken(fileToken);
      // set the events read parameter to the provided value
      f.numEventsRead = eventsRead;

    }

    void
    JobReport::overrideContributingInputs(Token outputToken, 
					  std::vector<Token> const& inputTokens)
    {
       // Get the required output file instance using the token
      JobReport::OutputFile& f = impl_->getOutputFileForToken(outputToken);
      // override its contributing inputs data
      f.contributingInputs = inputTokens;
    }

    void 
    JobReport::reportSkippedEvent(unsigned int run, unsigned int event)
    {
      std::ostringstream msg;
      msg << "<SkippedEvent Run=\"" << run << "\"";
      msg << " Event=\"" << event << "\" />\n";
      LogInfo("FwkJob") << msg.str();
    }

  void 
  JobReport::reportLumiSection(unsigned int run, unsigned int lumiSectId){
    JobReport::LumiSectionReport lumiRep;
    lumiRep.runNumber = run;
    lumiRep.lumiSectionId = lumiSectId;
    impl_->associateLumiSection(lumiRep);

  }


  void
  JobReport::reportError(std::string const& shortDesc,
  			 std::string const& longDesc)
  {
   std::ostringstream msg;
   msg << "<FrameworkError ExitStatus=\"1\" Type=\"" << shortDesc <<"\" >\n";
   msg << "  " << longDesc << "\n";
   msg << "</FrameworkError>\n";
   LogError("FwkJob") << msg.str();
  }
   


  void 
  JobReport::reportError(std::string const& shortDesc,
			 std::string const& longDesc,
			 int const& exitCode)
  {
    std::ostringstream msg;
    msg << "<FrameworkError ExitStatus=\""<< exitCode 
    	<<"\" Type=\"" << shortDesc <<"\" >\n";
    msg << "  " << longDesc << "\n";
    msg << "</FrameworkError>\n";
    LogError("FwkJob") << msg.str();
  }

  void 
  JobReport::reportSkippedFile(std::string const& pfn, 
			       std::string const& lfn) {

    std::ostringstream msg;
    msg << "<SkippedFile Pfn=\"" << pfn << "\"";
    msg << " Lfn=\"" << lfn << "\" />\n";
    LogInfo("FwkJob") << msg.str();
  }

  void 
  JobReport::reportTimingInfo(std::map<std::string, double> const& timingData){


    std::ostringstream msg;
    msg << "<TimingService>\n";
    

    std::map<std::string, double>::const_iterator pos;
    for (pos = timingData.begin(); pos != timingData.end(); ++pos){
      msg <<  "  <" << pos->first 
	  <<  "  Value=\"" << pos->second  << "\" />"
	  <<  "\n";
    }
    msg << "</TimingService>\n";
    LogInfo("FwkJob") << msg.str();
  }
  
  void
  JobReport::reportStorageStats(std::string const& data)
  {
    
    std::ostringstream msg;
    msg << "<StorageStatistics>\n"
        << data << "\n"
	<<  "</StorageStatistics>\n";
    LogInfo("FwkJob") << msg.str();    
  }
  void
  JobReport::reportGeneratorInfo(std::string const&  name, std::string const&  value)
  {
    
    impl_->addGeneratorInfo(name, value);
  }

  void
  JobReport::reportPSetHash(std::string const& hashValue)
  {
    std::ostringstream msg;
    msg << "<PSetHash>"
        <<  hashValue 
	<<  "</PSetHash>\n";
    LogInfo("FwkJob") << msg.str();    
  }


  void 
  JobReport::reportPerformanceSummary(std::string const& metricClass,
				      std::map<std::string, std::string> const& metrics)
  {
    std::ostringstream msg;
    msg << "<PerformanceReport>\n"
        << "  <PerformanceSummary Metric=\"" << metricClass << "\">\n";
    
    std::map<std::string, std::string>::const_iterator iter;
    for( iter = metrics.begin(); iter != metrics.end(); ++iter ) {
      msg << "    <Metric Name=\"" << iter->first << "\" " 
	  <<"Value=\"" << iter->second << "\"/>\n";
    }

    msg << "  </PerformanceSummary>\n"
	<< "</PerformanceReport>\n";
    LogInfo("FwkJob") << msg.str();    
  }
      
  void 
  JobReport::reportPerformanceForModule(std::string const&  metricClass,
					std::string const&  moduleName,
					std::map<std::string, std::string> const& metrics)
  {
    std::ostringstream msg;
    msg << "<PerformanceReport>\n"
        << "  <PerformanceModule Metric=\"" << metricClass << "\" "
	<< " Module=\""<< moduleName << "\" >\n";
    
    std::map<std::string, std::string>::const_iterator iter;
    for( iter = metrics.begin(); iter != metrics.end(); ++iter ) {
      msg << "    <Metric Name=\"" << iter->first << "\" " 
	  <<"Value=\"" << iter->second << "\"/>\n";
    }

    msg << "  </PerformanceModule>\n"
	<< "</PerformanceReport>\n";
    LogInfo("FwkJob") << msg.str();    
  }
  

  std::string
  JobReport::dumpFiles(void){
    
    std::ostringstream msg;
    
    std::vector<JobReport::OutputFile>::iterator f;
    
    for (f = impl_->outputFiles_.begin();
	 f != impl_->outputFiles_.end(); f++){
      
      msg << "\n<File>";
      msg << *f;
      
      msg << "\n<LumiSections>";
      std::vector<JobReport::LumiSectionReport>::iterator iLumi;
      for (iLumi = f->lumiSections.begin();
	   iLumi != f->lumiSections.end(); iLumi++){
	msg << *iLumi;
      }
      msg << "\n</LumiSections>\n";
      msg << "\n<Inputs>";
      std::vector<JobReport::Token>::iterator iInput;
      for (iInput = f->contributingInputs.begin(); 
	   iInput != f->contributingInputs.end(); iInput++) {
	JobReport::InputFile inpFile = impl_->inputFiles_[*iInput];
	msg <<"\n<Input>";
	msg <<"\n  <LFN>" << inpFile.logicalFileName << "</LFN>";
	msg <<"\n  <PFN>" << inpFile.physicalFileName << "</PFN>";
	msg <<"\n</Input>";
      }
      msg << "\n</Inputs>";
      msg << "\n</File>";
      
    }

    return msg.str();

  }
  

} //namspace edm
