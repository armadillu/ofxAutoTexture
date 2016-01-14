//
//  ofxAutoTexture.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 14/1/16.
//
//

#include "ofxAutoTexture.h"

ofxAutoTexture::ofxAutoTexture() {
#if !defined(DISABLE_TEXTURE_AUTOLOAD)
	loaded = false;
	ofAddListener(ofEvents().update, this, &ofxAutoTexture::_update);
#endif
}


ofxAutoTexture::~ofxAutoTexture() {
	#if !defined(DISABLE_TEXTURE_AUTOLOAD)
	ofRemoveListener(ofEvents().update, this, &ofxAutoTexture::_update);
	#endif
}

void ofxAutoTexture::_update(ofEventArgs &e) {

	if(loaded){
		float timeNow = ofGetElapsedTimef();
		if(timeNow - lastCheckTime > textureFileCheckInterval){ //time to check again
			lastCheckTime = ofGetElapsedTimef();
			std::time_t modif = getLastModified(filePath);
			if(lastModified != modif){ //file has been modified!
				//reload file!
				lastModified = modif;
				loaded = ofLoadImage(*this, filePath);
				ofLogNotice("ofxAutoTexture") << "reloading texture at " << filePath;
			}
		}
	}
}

std::time_t ofxAutoTexture::getLastModified(string path) {
	if(std::filesystem::exists(path)) {
		return std::filesystem::last_write_time(path);
	} else {
		return 0;
	}
}


void ofxAutoTexture::loadFromFile(string filePath){

	this->filePath = ofToDataPath(filePath, true);
	loaded = ofLoadImage(*this, filePath);
	lastModified = getLastModified(filePath);
	lastCheckTime = ofGetElapsedTimef();
}