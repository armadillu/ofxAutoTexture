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
	nextCheckInterval = textureFileCheckInterval + ofRandom(0.2);
	lastModified = 0;
#endif
}

ofxAutoTexture::~ofxAutoTexture() {
#if !defined(DISABLE_TEXTURE_AUTOLOAD)
	ofRemoveListener(ofEvents().update, this, &ofxAutoTexture::_update, OF_EVENT_ORDER_BEFORE_APP);
#endif
}

void ofxAutoTexture::_update(ofEventArgs &e) {

	if(loaded) {
		float timeNow = ofGetElapsedTimef();
		if(timeNow - lastCheckTime > nextCheckInterval) { // time to check again

			nextCheckInterval = textureFileCheckInterval + ofRandom(0.2);
			lastCheckTime = timeNow;
			std::time_t modif = getLastModified(filePath);

			if(lastModified != modif) { // file has been modified!
				// reload file!
				lastModified = modif;

				// ARB
				bool arbState = ofGetUsingArbTex;
				bool ourTexIsArb = getTextureData().textureTarget == GL_TEXTURE_RECTANGLE_ARB;
				if(ourTexIsArb != arbState) {
					if(GL_TEXTURE_RECTANGLE_ARB == getTextureData().textureTarget) {
						ofEnableArbTex();
					} else {
						ofDisableArbTex();
					}
				}

				// mipmap
				bool needsMipMap = hasMipmap();
				loaded = _loadFromFile(filePath);
				if(loaded) {
					if(needsMipMap)
						generateMipmap();

					if(ourTexIsArb != arbState) {
						if(arbState) {
							ofEnableArbTex();
						} else {
							ofDisableArbTex();
						}
					}

					ofLogNotice("ofxAutoTexture") << "reloading texture at " << filePath;
				} else {
					ofLogError("ofxAutoTexture") << "failed to reload texture at " << filePath;
				}
			}
		}
	}
}

std::time_t ofxAutoTexture::getLastModified(const string &path) {
	if(std::filesystem::exists(path)) {
		return std::filesystem::last_write_time(path);
	} else {
		return 0;
	}
}

bool ofxAutoTexture::loadFromFile(const string &filePath) {

	this->filePath = ofToDataPath(filePath, true);
	loaded = _loadFromFile(filePath);
	if(loaded){
		lastModified = getLastModified(this->filePath);
	}else{
		ofLogError("ofxAutoTexture") << "Cant load tex from file '" << this->filePath << "'";
	}
	return loaded;
}

bool ofxAutoTexture::_loadFromFile(const string &filePath) {

	ofPixels pixels;
	bool loaded_ = ofLoadImage(pixels, filePath); // load file to pixels
	if(loaded_) {
		// alloc texture space
		if(getWidth() != pixels.getWidth() || getHeight() != pixels.getHeight()){
			allocate(pixels.getWidth(), pixels.getHeight(), ofGetGlInternalFormat(pixels), ofGetUsingArbTex());
		}

		// see if we need post-processing
		string type = ofFilePath::getFileExt(filePath);
		if(ofToLower(type) == "psd") { // psd's get a special treatment - remove white halo
			ofLogNotice("ofxAutoTexture") << "Loading PSD file - removing white matte from file at '" << this->filePath
										  << "'";
			removeWhiteMatte(pixels);
		}

		// copy pixel data to texture data
		ofTexture::loadData(pixels);
	} else {
		ofLogError("ofxAutoTexture") << "Can't load file at '" << this->filePath << "'";
	}
	return loaded_;
}

void ofxAutoTexture::removeWhiteMatte(ofPixels &pixels) {

	int total = pixels.getWidth() * pixels.getHeight();
	unsigned char * data = pixels.getData();
	for(int i = 0; i < total; ++i) {
		const int k = i * 4;
		const unsigned char a = data[k + 3];
		if(a) {
			const float na = a / 255.0f; // normalized alpha
			const float ina = 1.0f - na; // inverse normalized alpha
			data[k    ] = (data[k    ] - 255.0f * ina) / na;
			data[k + 1] = (data[k + 1] - 255.0f * ina) / na;
			data[k + 2] = (data[k + 2] - 255.0f * ina) / na;
		}
	}
}
