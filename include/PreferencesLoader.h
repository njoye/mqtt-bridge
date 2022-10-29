/*
 *  Preferences parser class for .conf or json files
 *  V 1.7.3
 *  Copyright 2017-2019 by labs | project 2010
 *  created by Susanna Miller-Grove
 *
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <jsoncpp/json/json.h>
#include <sys/stat.h>
#include <mutex>
 
/*
 * A class to read data from a UNIX/Linux conf file.
 */

class PreferencesLoader
{
private:
	struct pref {
		std::string key;
		std::string value;
	};
	
	std::vector<PreferencesLoader::pref> p;
	Json::Value root;
	std::string fileName;
	std::string delimeter;
	std::mutex mtx;
 
public:

	PreferencesLoader(std::string filename, std::string delm = "=") :
			fileName(filename), delimeter(delm)
	{ }
 
	/*
	*
	*
	*
	*/
	void load()
	{
		std::ifstream file(fileName);
		std::string line = "";
    	mtx.lock();
		// Iterate through each line and split the content using delimeter
    	int i=0;
		while( getline(file, line) ) {
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find(delimeter);
            p.push_back(PreferencesLoader::pref());
            p[i].key = line.substr(0, delimiterPos);
            p[i].value = line.substr(delimiterPos + 1);
            p.push_back(PreferencesLoader::pref());

            #if defined(__DEBUG__)
            std::cout << p[i].key << " " << p[i].value << '\n';
            #endif
            i++;
        }
		// Close the File
		file.close();
		mtx.unlock();
	}

	//
	// TODO
	// Error Handling
	//

	void loadJSON()
	{
		mtx.lock();
		std::ifstream ifs(fileName);
    	Json::Reader reader;

    	if ( !reader.parse(ifs, root) ) {
    		std::cout << reader.getFormattedErrorMessages();
    		//exit(1);
    		std::cout << "json sytnax error" << std::endl;
    	}
    	mtx.unlock();
	}

	std::string valueForKey(std::string v)
	{
    	std::string ret = "";
    	uint i = 0;

    	for ( i=0; i < p.size(); i++ ) {
        	if ( v.compare(p[i].key) == 0 ) ret = p[i].value;
    	}

    	return ret;
	}

	std::vector<std::string> splitPrefs(std::string str, char *d)
	{
		std::vector<std::string> v;

		boost::split(v, str, boost::is_any_of(d));

		return v;
	}

	const Json::Value& getJSONConfig(std::string name)
	{
		return root[name];
	}

	bool fileExists(const std::string& name) {
		struct stat buffer;

		return (stat(name.c_str(), &buffer) == 0);
	}

	~PreferencesLoader() {}
};
