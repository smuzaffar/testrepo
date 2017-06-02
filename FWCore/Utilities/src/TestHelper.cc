//------------------------------------------------------------
// $Id:$
//------------------------------------------------------------
#include <iostream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
#include "boost/filesystem/convenience.hpp"

#include "FWCore/Utilities/interface/TestHelper.h"

namespace bf=boost::filesystem;


int run_script(const std::string& shell, const std::string& script)
{
  pid_t pid;
  int status=0;

  if ((pid=fork())<0)
    {
      std::cerr << "fork failed, to run " << script << std::endl;;
      return -1;
    }

  if (pid==0) // child
    {
      execlp(shell.c_str(), "sh", "-c", script.c_str(), 0);
      _exit(127);
    }
  else // parent
    {
      while(waitpid(pid,&status,0)<0)
	{
	  if (errno!=EINTR)
	    {
	      status=-1;
	      break;
	    }
	}
    }
  return status;
}

int ptomaine(int argc, char* argv[])
{
  bf::path currentPath = bf::initial_path();
  
  if (argc<4)
    {
      std::cout << "Usage: " << argv[0] << " shell subdir script1 script2 ... scriptN\n\n"
		<< "where shell is the path+shell (e.g., /bin/bash) intended to run the scripts\n"
		<< "and subdir is the subsystem/package/subdir in which the scripts are found\n"
		<< "(e.g., FWCore/Utilities/test)\n"
		<< std::endl;

      std::cout << "Current directory is: " << currentPath.native_directory_string() << '\n';
      std::cout << "Current environment:\n";
      std::cout << "---------------------\n";
      for (int i = 0; environ[i] != 0; ++i) std::cout << environ[i] << '\n';
      std::cout << "---------------------\n";
      std::cout << "Executable name: " << argv[0] << '\n';
      return -1;
    }

  for (int i = 0; i < argc; ++i)
    {
      std::cout << "argument " << i << ": " << argv[i] << '\n';
    }

  
  std::string shell(argv[1]);
  std::cerr << "shell is: " << shell << '\n';

  std::cout << "Current directory is: " << currentPath.native_directory_string() << '\n';
  const char* topdir  = getenv("SCRAMRT_LOCALRT");
  const char* arch    = getenv("SCRAM_ARCH");

  if ( !arch )
    {
      // Try to synthesize SCRAM_ARCH value.
      bf::path exepath(argv[0]);
      std::string maybe_arch = exepath.branch_path().leaf();
      if (setenv("SCRAM_ARCH", maybe_arch.c_str(), 1) != 0)
	{
	  std::cerr << "SCRAM_ARCH not set and attempt to set it failed\n";
	  return -1;
	}
    }

  int rc=0;

  if (!topdir)
    {
      std::cout << "SCRAMRT_LOCALRT is not defined" << std::endl;;
      return -1;
    }


  std::string testdir(topdir); testdir += "/src/"; testdir += argv[2];
  std::string tmpdir(topdir);  tmpdir+="/tmp/";   tmpdir+=arch;
  std::string testbin(topdir); testbin+="/test/"; testbin+=arch;

  std::cout << "topdir is: " << topdir << '\n';
  std::cout << "testdir is: " << testdir << '\n';
  std::cout << "tmpdir is: " << tmpdir << '\n';
  std::cout << "testbin is: " << testbin << '\n';


  if (setenv("LOCAL_TEST_DIR",testdir.c_str(),1)!=0)
    {
      std::cerr << "Could not set LOCAL_TEST_DIR to " << testdir << std::endl;;
      return -1;
    }
  if (setenv("LOCAL_TMP_DIR",tmpdir.c_str(),1)!=0)
    {
      std::cerr << "Could not set LOCAL_TMP_DIR to " << tmpdir << std::endl;;
      return -1;
    }
  if (setenv("LOCAL_TOP_DIR",topdir,1)!=0)
    {
      std::cerr << "Could not set LOCAL_TOP_DIR to " << topdir << std::endl;;
      return -1;
    }
  if (setenv("LOCAL_TEST_BIN",testbin.c_str(),1)!=0)
    {
      std::cerr << "Could not set LOCAL_TEST_BIN to " << testbin << std::endl;;
      return -1;
    }

  testdir+="/";

  for(int i=3; i<argc && rc==0; ++i)
    {
      std::string scriptname(testdir);
      scriptname += argv[i];
      std::cout << "Running script: " << scriptname << std::endl;
      rc = run_script(shell, scriptname);
    }

  std::cout << "status = " << rc << std::endl;;
  return rc == 0 ? 0 : -1;
}
