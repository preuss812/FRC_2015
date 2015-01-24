/*
 * ConfigReader.h
 *
 *  Singleton class for reading configuration file
 *
 */

#ifndef CONFIGREADER_H_
#define CONFIGREADER_H_

#include <string>
#include <map>

using namespace std;

class ConfigReader {
public:
	// make the implementation of this class a singleton
	static ConfigReader* getInstance();
	~ConfigReader();

	int getIntValue(string key);
	float getFloatValue(string key);
	string getStringValue(string key);

private:
	ConfigReader();
	static ConfigReader* instance;

	map<string, string>* keyValueMap;
};

#endif /* CONFIGREADER_H_ */
