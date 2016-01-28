//
//  ofxAutoTexture.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 14/1/16.
//
//

#ifndef __BaseApp__ofxAutoTexture__
#define __BaseApp__ofxAutoTexture__

#include "ofMain.h"

// add DISABLE_TEXTURE_AUTOLOAD to your project pre-processor macros
// to disable the auto-reloading entirely

class ofxAutoTexture : public ofTexture {

  public:
	ofxAutoTexture();
	virtual ~ofxAutoTexture();

	bool loadFromFile(const string &filePath);

	// to be used on PSDs to fix white halos
	static void removeWhiteMatte(ofPixels &pixels);

  protected:
	void _update(ofEventArgs &e);
	std::time_t getLastModified(const string &filePath);

	bool _loadFromFile(const string &filePath);

	bool loaded;
	std::time_t lastModified;
	float lastCheckTime;
	string filePath;

	const float textureFileCheckInterval = 1.0; // seconds
};

#endif /* defined(__BaseApp__ofxAutoTexture__) */
