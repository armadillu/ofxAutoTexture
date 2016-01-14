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
	lastCheckTime = 0.0f;
	ofAddListener(ofEvents().update, this, &ofxAutoTexture::_update, OF_EVENT_ORDER_BEFORE_APP);
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

				//ARB
				bool arbState = ofGetUsingArbTex;
				bool ourTexIsArb = getTextureData().textureTarget == GL_TEXTURE_RECTANGLE_ARB;
				if(ourTexIsArb != arbState){
					if(GL_TEXTURE_RECTANGLE_ARB == getTextureData().textureTarget){
						ofEnableArbTex();
					}else{
						ofDisableArbTex();
					}
				}

				//mipmap
				bool needsMipMap = getTextureData().minFilter == GL_LINEAR_MIPMAP_LINEAR; //this is an ugly guess...
																							//todo OF should allow me to query a texture for its mipmap existance
				loaded = ofLoadImage(*this, filePath);
				if(needsMipMap) generateMipmap();

				if(ourTexIsArb != arbState){
					if(arbState){
						ofEnableArbTex();
					}else{
						ofDisableArbTex();
					}
				}


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
	lastModified = getLastModified(this->filePath);
	lastCheckTime = ofGetElapsedTimef();
}