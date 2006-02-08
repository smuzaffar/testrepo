// -*- C++ -*-
//
// Package:     Services
// Class  :     Timing
// 
// Implementation:
//
// Original Author:  Jim Kowalkowski
// $Id: Memory.cc,v 1.4 2006/02/07 07:16:46 wmtan Exp $
//

#include "FWCore/Services/src/Memory.h"
#include "DataFormats/Common/interface/ModuleDescription.h"
#include "DataFormats/Common/interface/EventID.h"
#include "DataFormats/Common/interface/Timestamp.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

#ifdef __linux__
#define LINUX 1
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/procfs.h>

using namespace std;

namespace edm {
  namespace service {

    struct linux_proc {
      int pid; // %d
      char comm[400]; // %s
      char state; // %c
      int ppid; // %d
      int pgrp; // %d
      int session; // %d
      int tty; // %d
      int tpgid; // %d
      unsigned int flags; // %u
      unsigned int minflt; // %u
      unsigned int cminflt; // %u
      unsigned int majflt; // %u
      unsigned int cmajflt; // %u
      int utime; // %d
      int stime; // %d
      int cutime; // %d
      int cstime; // %d
      int counter; // %d
      int priority; // %d
      unsigned int timeout; // %u
      unsigned int itrealvalue; // %u
      int starttime; // %d
      unsigned int vsize; // %u
      unsigned int rss; // %u
      unsigned int rlim; // %u
      unsigned int startcode; // %u
      unsigned int endcode; // %u
      unsigned int startstack; // %u
      unsigned int kstkesp; // %u
      unsigned int kstkeip; // %u
      int signal; // %d
      int blocked; // %d
      int sigignore; // %d
      int sigcatch; // %d
      unsigned int wchan; // %u
    };
      
    procInfo SimpleMemoryCheck::fetch()
    {
      procInfo ret;
      double pr_size=0.0, pr_rssize=0.0;
      
#ifdef LINUX
      linux_proc pinfo;
      int cnt;

      lseek(fd_,0,SEEK_SET);
    
      if((cnt=read(fd_,buf_,sizeof(buf_)))<0)
	{
	  perror("Read of Proc file failed:");
	  return procInfo();
	}
    
      if(cnt>0)
	{
	  buf_[cnt]='\0';
	  
	  sscanf(buf_,
		 "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
		 &pinfo.pid, // %d
		 pinfo.comm, // %s
		 &pinfo.state, // %c
		 &pinfo.ppid, // %d
		 &pinfo.pgrp, // %d
		 &pinfo.session, // %d
		 &pinfo.tty, // %d
		 &pinfo.tpgid, // %d
		 &pinfo.flags, // %u
		 &pinfo.minflt, // %u
		 &pinfo.cminflt, // %u
		 &pinfo.majflt, // %u
		 &pinfo.cmajflt, // %u
		 &pinfo.utime, // %d
		 &pinfo.stime, // %d
		 &pinfo.cutime, // %d
		 &pinfo.cstime, // %d
		 &pinfo.counter, // %d
		 &pinfo.priority, // %d
		 &pinfo.timeout, // %u
		 &pinfo.itrealvalue, // %u
		 &pinfo.starttime, // %d
		 &pinfo.vsize, // %u
		 &pinfo.rss, // %u
		 &pinfo.rlim, // %u
		 &pinfo.startcode, // %u
		 &pinfo.endcode, // %u
		 &pinfo.startstack, // %u
		 &pinfo.kstkesp, // %u
		 &pinfo.kstkeip, // %u
		 &pinfo.signal, // %d
		 &pinfo.blocked, // %d
		 &pinfo.sigignore, // %d
		 &pinfo.sigcatch, // %d
		 &pinfo.wchan // %u
		 );

	  // resident set size in pages
	  pr_size = (double)pinfo.vsize;
	  pr_rssize = (double)pinfo.rss;
	  
	  ret.vsize = pr_size   / 1000000.0;
	  ret.rss   = pr_rssize * pg_size_ / 1000000.0;
	}
#else
      ret.vsize=0;
      ret.rss=0;
#endif
      return ret;
    }
    
    SimpleMemoryCheck::SimpleMemoryCheck(const ParameterSet& iPS,
					 ActivityRegistry&iReg):
      a_(),b_(),
      current_(&a_),previous_(&b_),
      pg_size_(sysconf(_SC_PAGESIZE)), // getpagesize();
      num_to_skip_(iPS.getUntrackedParameter<int>("ignoreTotal",1)),
      count_()
    {
      // pg_size = (double)getpagesize();
      ostringstream ost;
	
#ifdef LINUX
      ost << "/proc/" << getpid() << "/stat";
      fname_ = ost.str();
      
      if((fd_=open(ost.str().c_str(),O_RDONLY))<0)
	{
	  throw cms::Exception("Configuration")
	    << "Memory checker server: Failed to open " << ost.str() << endl;
	}
#endif
      iReg.watchPostBeginJob(this,&SimpleMemoryCheck::postBeginJob);
      iReg.watchPostEndJob(this,&SimpleMemoryCheck::postEndJob);
      
      iReg.watchPreProcessEvent(this,&SimpleMemoryCheck::preEventProcessing);
      iReg.watchPostProcessEvent(this,&SimpleMemoryCheck::postEventProcessing);

      iReg.watchPreModule(this,&SimpleMemoryCheck::preModule);
      iReg.watchPostModule(this,&SimpleMemoryCheck::postModule);
    }

    SimpleMemoryCheck::~SimpleMemoryCheck()
    {
#ifdef LINUX
      close(fd_);
#endif
    }

    void SimpleMemoryCheck::postBeginJob()
    {
    }

    void SimpleMemoryCheck::postEndJob()
    {
    }

    void SimpleMemoryCheck::preEventProcessing(const edm::EventID& iID,
					       const edm::Timestamp& iTime)
    {
    }
    void SimpleMemoryCheck::postEventProcessing(const Event& e,
						const EventSetup&)
    {
	  ++count_;
    }

    void SimpleMemoryCheck::preModule(const ModuleDescription& md)
    {
    }

    void SimpleMemoryCheck::postModule(const ModuleDescription& md)
    {
      swap(current_,previous_);
      *current_ = fetch();

      if(*current_ > max_)
	{
	  if(count_ >= num_to_skip_)
	  LogWarning("MemoryIncrease")
	    << "Memory increased from "
	    << "VSIZE=" << max_.vsize << "MB "
	    << "and RSS=" << max_.rss << "MB "
	    << "to VSIZE=" << current_->vsize << "MB"
	    << " and RSS=" << current_->rss << "MB\n"
	    << "after module "
	    << md.moduleLabel_ << "/"
	    << md.moduleName_ << "\n";
	  
	  max_ = *current_;
	}

    }

  }
}
