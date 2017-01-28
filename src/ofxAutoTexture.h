//
//  ofxAutoTexture.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 14/1/16.
//
//

#pragma once

#include "ofMain.h"

// add DISABLE_TEXTURE_AUTOLOAD to your project pre-processor macros
// to disable the auto-reloading entirely

class ofxAutoTexture : public ofTexture {

  public:
	ofxAutoTexture();
	virtual ~ofxAutoTexture();

	bool preloadPixelsFromFile(const string &filePath); //use this to parallel load several ofxAutoTexture from threads
	bool arePixelPreLoaded(); //will b e true when pixels are loaded.
	bool isPreloadingPixels();
	
	bool loadFromFile(const string &filePath);
	string getFilePath(){return filePath;}
	
	// to be used on PSDs to fix white halos
	static void removeWhiteMatte(ofPixels &pixels, bool makeTransparentPixelsBlack = true);
	static void makeTransparentPixelsBlack(ofPixels &pixels);
	static float memUse(ofTexture * tex); //return MBytes float

	//mem use stats
//	static float getCurrentlyLoadedMBytes(); //what's loaded right now
	static float getTotalLoadedMBytes(); //over app time


  protected:

	void _update(ofEventArgs &e);
	std::time_t getLastModified(const string &filePath);

	bool _loadFromFile(const string &filePath);

	bool loaded;
	std::time_t lastModified;
	float lastCheckTime;
	float nextCheckInterval;
	string filePath;
	int nChannels;

	const float textureFileCheckInterval = 0.8; // seconds
	
	bool preloadingPixels = false;
	ofPixels preloadedPixels;
	bool pixelsPreloaded = false;

	static float totalLoadedMbytes;
	static float currentlyLoadedMBytes;
};
