#ifndef FWCore_Utilities_InputTag_h
#define FWCore_Utilities_InputTag_h

#include <string>
#include <iosfwd>

namespace edm {

  class InputTag {
  public:
    InputTag();
    InputTag(const std::string & label, const std::string & instance, const std::string & processName = "");
    /// the input string is of the form:
    /// label
    /// label:instance
    /// label:instance:process
    InputTag(std::string const& s);
    std::string encode() const;

    std::string const& label() const {return label_;} 
    std::string const& instance() const {return instance_;}
    ///an empty string means find the most recently produced 
    ///product with the label and instance
    std::string const& process() const {return process_;} 
    
    bool operator==(InputTag const& tag) const;

  private:
    std::string label_;
    std::string instance_;
    std::string process_;
  };

  std::ostream& operator<<(std::ostream& ost, InputTag const& tag);

}

#endif

