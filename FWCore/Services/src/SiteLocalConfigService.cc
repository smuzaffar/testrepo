//<<<<<< INCLUDES                                                       >>>>>>

#include "FWCore/Services/src/SiteLocalConfigService.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <sstream>
#include <memory>

using namespace xercesc;

//<<<<<< PRIVATE DEFINES                                                >>>>>>
//<<<<<< PRIVATE CONSTANTS                                              >>>>>>
//<<<<<< PRIVATE TYPES                                                  >>>>>>
//<<<<<< PRIVATE VARIABLE DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC VARIABLE DEFINITIONS                                    >>>>>>
//<<<<<< CLASS STRUCTURE INITIALIZATION                                 >>>>>>
//<<<<<< PRIVATE FUNCTION DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC FUNCTION DEFINITIONS                                    >>>>>>
//<<<<<< MEMBER FUNCTION DEFINITIONS                                    >>>>>>

inline std::string _toString(const XMLCh *toTranscode)
{
    std::string tmp(XMLString::transcode(toTranscode));
    return tmp;
}

inline unsigned int _toUInt(const XMLCh *toTranscode)
{
   std::istringstream iss(_toString(toTranscode));
   unsigned int returnValue;
   iss >> returnValue;
   return returnValue;
}

inline double _toDouble(const XMLCh *toTranscode)
{
   std::istringstream iss(_toString(toTranscode));
   double returnValue;
   iss >> returnValue;
   return returnValue;
}

inline XMLCh*  _toDOMS( std::string temp )
{
    XMLCh* buff = XMLString::transcode(temp.c_str());    
    return  buff;
}

// concatenate all the XML node attribute/value pairs into a
// paren-separated string (for use by CORAL and frontier_client)
inline std::string _toParenString(const DOMNode &nodeToConvert)
{
    std::ostringstream oss;

    DOMNodeList *childList = nodeToConvert.getChildNodes();

    unsigned int numNodes = childList->getLength ();
    for (unsigned int i = 0; i < numNodes; ++i)
    {
	DOMNode *childNode = childList->item(i);
	if (childNode->getNodeType() != DOMNode::ELEMENT_NODE)
	    continue;
	DOMElement *child = static_cast <DOMElement *> (childNode);

	DOMNamedNodeMap *attributes = child->getAttributes();
	unsigned int numAttributes = attributes->getLength ();
	for (unsigned int j = 0; j < numAttributes; ++j)
	{
	    DOMNode *attributeNode = attributes->item(j);
	    if (attributeNode->getNodeType() != DOMNode::ATTRIBUTE_NODE)
		continue;
	    DOMAttr *attribute = static_cast <DOMAttr *> (attributeNode);

	    oss << "(" << _toString(child->getTagName()) << 
	    		_toString(attribute->getName()) << "=" << 
			_toString(attribute->getValue()) << ")";
	}
    }
    return oss.str();
}

template <typename T>
static
void overrideFromPSet(const char* iName, const edm::ParameterSet& iPSet,
                      T& iHolder, const T* iPointer)
{
   if(iPSet.exists(iName)) {
      iHolder = iPSet.getUntrackedParameter<T>(iName);
      iPointer = &iHolder;
   }
}

edm::service::SiteLocalConfigService::SiteLocalConfigService (const edm::ParameterSet &pset,
							      const edm::ActivityRegistry &activityRegistry)
    : m_connected (false),
      m_cacheTempDirPtr(0),
      m_cacheMinFreePtr(0),
      m_cacheHintPtr(0),
      m_readHintPtr(0),
      m_ttreeCacheSizePtr(0),
      m_nativeProtocolsPtr(0)

{
    m_rfioType = "castor";
    m_url = "/SITECONF/local/JobConfig/site-local-config.xml";
    char * tmp = getenv ("CMS_PATH");
    
    if (tmp)
	m_url = tmp + m_url;
    
    this->parse (m_url);
   
   //apply overrides
   overrideFromPSet("overrideSourceCacheTempDir",pset,m_cacheTempDir, m_cacheTempDirPtr);
   overrideFromPSet("overrideSourceCacheMinFree",pset,m_cacheMinFree, m_cacheMinFreePtr);
   overrideFromPSet("overrideSourceCacheHintDir",pset,m_cacheHint,m_cacheHintPtr);
   overrideFromPSet("overrideSourceReadHint",pset,m_readHint,m_readHintPtr);
   overrideFromPSet("overrideSourceNativeProtocols",pset,m_nativeProtocols,m_nativeProtocolsPtr);
   overrideFromPSet("overrideSourceTTreeCacheSize",pset,m_ttreeCacheSize,m_ttreeCacheSizePtr);
   
}

const std::string
edm::service::SiteLocalConfigService::dataCatalog (void) const
{
    if (! m_connected) {
	//throw cms::Exception("Incomplete configuration") 
	//    << "Valid site-local-config not found at " << m_url ;
        // Return PoolFileCatalog.xml for now
        return "file:PoolFileCatalog.xml";
    }

    if (m_dataCatalog == "")
    {
	throw cms::Exception("Incomplete configuration")
	    << "Did not find catalog in event-data section in " << m_url ;
    }

    return  m_dataCatalog;    
}

const std::string
edm::service::SiteLocalConfigService::frontierConnect (const std::string& servlet) const
{
    if (! m_connected)
	throw cms::Exception("Incomplete configuration") 
	    << "Valid site-local-config not found at " << m_url ;
    
    if (m_frontierConnect == "")
    {
	throw cms::Exception("Incomplete configuration")
	    << "Did not find frontier-connect in calib-data section in " << m_url ;
    }

    if (servlet == "")
	return m_frontierConnect;

    // Replace the last component of every "serverurl=" piece (up to the
    //   next close-paren) with the servlet
    std::string::size_type nextparen = 0;
    std::string::size_type serverurl, lastslash;
    std::string complexstr = "";
    while ((serverurl = m_frontierConnect.find("(serverurl=", nextparen)) != std::string::npos)
    {
	complexstr.append(m_frontierConnect, nextparen, serverurl - nextparen);
	nextparen = m_frontierConnect.find(')', serverurl);
	lastslash = m_frontierConnect.rfind('/', nextparen);
	complexstr.append(m_frontierConnect, serverurl, lastslash - serverurl + 1);
	complexstr.append(servlet);
    }
    complexstr.append(m_frontierConnect, nextparen, m_frontierConnect.length()-nextparen);

    return complexstr;
}

const std::string
edm::service::SiteLocalConfigService::lookupCalibConnect (const std::string& input) const
{
    static const std::string proto = "frontier://";

    if (input.substr(0,proto.length()) == proto)
    {
	// Replace the part after the frontier:// and before either an open-
	//  parentheses (which indicates user-supplied options) or the last
	//  slash (which indicates start of the schema) with the complex
	//  parenthesized string returned from frontierConnect() (which
	//  contains all the information needed to connect to frontier),
	//  if that part is a simple servlet name (non-empty and not
	//  containing special characters)
	// Example connect strings where servlet is replaced:
	//  frontier://cms_conditions_data/CMS_COND_ECAL
	//  frontier://FrontierInt/CMS_COND_ECAL
	//  frontier://FrontierInt(retrieve-ziplevel=0)/CMS_COND_ECAL
	// Example connect strings left untouched:
	//  frontier://cmsfrontier.cern.ch:8000/FrontierInt/CMS_COND_ECAL
	//  frontier://(serverurl=cmsfrontier.cern.ch:8000/FrontierInt)/CMS_COND_ECAL
	std::string::size_type startservlet = proto.length();
	// if user supplied extra parenthesized options, stop servlet there
	std::string::size_type endservlet = input.find("(", startservlet);
	if (endservlet == std::string::npos)
	    endservlet = input.rfind('/', input.length());
	std::string servlet = input.substr(startservlet, endservlet - startservlet);
	if ((servlet != "") && (servlet.find_first_of(":/)[]") == std::string::npos))
	{
	    if (servlet == "cms_conditions_data")
		// use the default servlet from site-local-config.xml
		servlet = "";

	    return proto + frontierConnect(servlet) + input.substr(endservlet);
	}
    }
    return input;
}

const std::string
edm::service::SiteLocalConfigService::rfioType (void) const
{
    return m_rfioType;
}

const std::string* 
edm::service::SiteLocalConfigService::sourceCacheTempDir() const
{
   return m_cacheTempDirPtr;
}

const double*
edm::service::SiteLocalConfigService::sourceCacheMinFree() const
{
   return m_cacheMinFreePtr;
}

const std::string* 
edm::service::SiteLocalConfigService::sourceCacheHint() const
{
   return m_cacheHintPtr;
}

const std::string* 
edm::service::SiteLocalConfigService::sourceReadHint() const
{
   return m_readHintPtr;
}

const unsigned int* 
edm::service::SiteLocalConfigService::sourceTTreeCacheSize() const
{
   return m_ttreeCacheSizePtr;
}

const std::vector<std::string>* 
edm::service::SiteLocalConfigService::sourceNativeProtocols() const
{
   return m_nativeProtocolsPtr;
}


void
edm::service::SiteLocalConfigService::parse (const std::string &url)
{
    XMLPlatformUtils::Initialize();  
    std::auto_ptr<XercesDOMParser> parser(new XercesDOMParser);
    try 
    {
	parser->setValidationScheme(XercesDOMParser::Val_Auto);
	parser->setDoNamespaces(false);

	parser->parse(url.c_str());	
	DOMDocument* doc = parser->getDocument();
	if (! doc)
	{
	    return;
	}
	
	// The Site Config has the following format
	// <site-local-config>
	// <site name="FNAL">
	//   <event-data>
	//     <catalog url="trivialcatalog_file:/x/y/z.xml"/>
	//     <rfiotype value="castor"/>
	//   </event-data>
	//   <calib-data>
	//     <catalog url="trivialcatalog_file:/x/y/z.xml"/>
	//     <frontier-connect>
	//       ... frontier-interpreted server/proxy xml ...
        //     </frontier-connect>
	//   </calib-data>
        //   <source-config>
        //     <cache-temp-dir name="/a/b/c"/>
        //     <cache-hint value="..."/>
        //     <read-hint value="..."/>
        //     <ttree-cache-size value="0"/>
        //     <native-protocols>
        //        <protocol  prefix="dcache"/>
        //        <protocol prefix="file"/>
        //     </native-protocols>
        //   </source-config>
	// </site>
	// </site-local-config>
    
	// FIXME: should probably use the parser for validating the XML.
    
	DOMNodeList *sites = doc->getElementsByTagName (_toDOMS ("site"));
	unsigned int numSites = sites->getLength ();
	for (unsigned int i = 0;
	     i < numSites; 
	     ++i)
	{	
	    DOMElement *site = static_cast <DOMElement *> (sites->item (i));
	
	    // Parsing of the event data section
	    {
		DOMNodeList * eventDataList 
		    = site->getElementsByTagName (_toDOMS ("event-data"));
		if (eventDataList->getLength () > 0)
		{
		    DOMElement *eventData 
			= static_cast <DOMElement *> (eventDataList->item (0));
	    
		    DOMNodeList *catalogs 
			= eventData->getElementsByTagName (_toDOMS ("catalog"));
	    
		    if (catalogs->getLength () > 0)
		    {
			DOMElement * catalog 
			    = static_cast <DOMElement *> (catalogs->item (0));
	    
			m_dataCatalog = _toString (catalog->getAttribute (_toDOMS ("url")));
		    }
		    
		    DOMNodeList *rfiotypes 
			= eventData->getElementsByTagName (_toDOMS ("rfiotype"));
	    
		    if (rfiotypes->getLength () > 0)
		    {
			DOMElement * rfiotype 
			    = static_cast <DOMElement *> (rfiotypes->item (0));
	    
			m_rfioType = _toString (rfiotype->getAttribute (_toDOMS ("value")));
		    }
		    
		}
	    }
	
	    // Parsing of the calib-data section
	    {
		DOMNodeList * calibDataList 
		    = site->getElementsByTagName (_toDOMS ("calib-data"));
	    
		if (calibDataList->getLength () > 0)
		{
		    DOMElement *calibData 
			= static_cast <DOMElement *> (calibDataList->item (0));
	    
		    DOMNodeList *frontierConnectList
			= calibData->getElementsByTagName (_toDOMS ("frontier-connect"));

		    if (frontierConnectList->getLength () > 0)
		    {
			DOMElement *frontierConnect
			    = static_cast <DOMElement *> (frontierConnectList->item (0));

			m_frontierConnect = _toParenString(*frontierConnect);
		    }
		}
	    }
            // Parsing of the source config section
            {
               DOMNodeList * sourceConfigList 
               = site->getElementsByTagName (_toDOMS ("source-config"));
               
               if (sourceConfigList->getLength () > 0)
               {
                  DOMElement *sourceConfig 
                  = static_cast <DOMElement *> (sourceConfigList->item (0));
                  
                  DOMNodeList *cacheTempDirList
                  = sourceConfig->getElementsByTagName (_toDOMS ("cache-temp-dir"));
                  
                  if (cacheTempDirList->getLength () > 0)
                  {
                     DOMElement *cacheTempDir
                     = static_cast <DOMElement *> (cacheTempDirList->item (0));
                     
                     m_cacheTempDir = _toString(cacheTempDir->getAttribute (_toDOMS ("name")));
                     m_cacheTempDirPtr = &m_cacheTempDir;
                  }
                  
                  DOMNodeList *cacheMinFreeList
                  = sourceConfig->getElementsByTagName (_toDOMS ("cache-min-free"));
                  
                  if (cacheMinFreeList->getLength () > 0)
                  {
                     DOMElement *cacheMinFree
                     = static_cast <DOMElement *> (cacheMinFreeList->item (0));
                     
                     m_cacheMinFree = _toDouble(cacheMinFree->getAttribute (_toDOMS ("value")));
                     m_cacheMinFreePtr = &m_cacheMinFree;
                  }

                  DOMNodeList *cacheHintList
                  = sourceConfig->getElementsByTagName (_toDOMS ("cache-hint"));
                  
                  if (cacheHintList->getLength () > 0)
                  {
                     DOMElement *cacheHint
                     = static_cast <DOMElement *> (cacheHintList->item (0));
                     
                     m_cacheHint = _toString(cacheHint->getAttribute (_toDOMS ("value")));
                     m_cacheHintPtr = &m_cacheHint;
                  }

                  DOMNodeList *readHintList
                  = sourceConfig->getElementsByTagName (_toDOMS ("read-hint"));
                  
                  if (readHintList->getLength () > 0)
                  {
                     DOMElement *readHint
                     = static_cast <DOMElement *> (readHintList->item (0));
                     
                     m_readHint = _toString(readHint->getAttribute (_toDOMS ("value")));
                     m_readHintPtr = &m_readHint;
                  }
                  
                  DOMNodeList *ttreeCacheSizeList
                  = sourceConfig->getElementsByTagName (_toDOMS ("ttree-cache-size"));
                  
                  if (ttreeCacheSizeList->getLength () > 0)
                  {
                     DOMElement *ttreeCacheSize
                     = static_cast <DOMElement *> (ttreeCacheSizeList->item (0));
                     
                     m_ttreeCacheSize = _toUInt(ttreeCacheSize->getAttribute (_toDOMS ("value")));
                     m_ttreeCacheSizePtr = &m_ttreeCacheSize;
                  }
                  
                  DOMNodeList *nativeProtocolsList
                  = sourceConfig->getElementsByTagName (_toDOMS ("native-protocols"));
                  
                  if (nativeProtocolsList->getLength () > 0)
                  {
                     DOMElement *nativeProtocol
                     = static_cast <DOMElement *> (nativeProtocolsList->item (0));
                     
                     DOMNodeList *childList = nativeProtocol->getChildNodes();
                     
                     XMLCh* prefixXMLCh = _toDOMS ("prefix");
                     unsigned int numNodes = childList->getLength ();
                     for (unsigned int i = 0; i < numNodes; ++i)
                     {
                        DOMNode *childNode = childList->item(i);
                        if (childNode->getNodeType() != DOMNode::ELEMENT_NODE)
                           continue;
                        DOMElement *child = static_cast <DOMElement *> (childNode);
                        
                        m_nativeProtocols.push_back( _toString(child->getAttribute(prefixXMLCh)));
                     }
                     
                     m_nativeProtocolsPtr = &m_nativeProtocols;
                  }
                  
               }
            }
	}
	m_connected = true;
    } 
    catch (xercesc::DOMException &e)
    {
    }       
}	
