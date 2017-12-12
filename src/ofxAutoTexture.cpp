//
//  ofxAutoTexture.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 14/1/16.
//
//

#include "ofxAutoTexture.h"

float ofxAutoTexture::totalLoadedMbytes = 0.0f;
float ofxAutoTexture::currentlyLoadedMBytes = 0.0f;

ofxAutoTexture::ofxAutoTexture() {
	loaded = false;
	lastCheckTime = 0.0f;
	#if !defined(DISABLE_TEXTURE_AUTOLOAD)
	ofAddListener(ofEvents().update, this, &ofxAutoTexture::_update, OF_EVENT_ORDER_BEFORE_APP);
	#endif
	nextCheckInterval = textureFileCheckInterval + ofRandom(0.2);
	lastModified = 0;
	nChannels = 0;
}

ofxAutoTexture::~ofxAutoTexture() {
#if !defined(DISABLE_TEXTURE_AUTOLOAD)
	ofRemoveListener(ofEvents().update, this, &ofxAutoTexture::_update, OF_EVENT_ORDER_BEFORE_APP);
#endif
}

bool ofxAutoTexture::preloadPixelsFromFile(const string &filePath){
	this->filePath = filePath;
	pixelsPreloaded = false;
	preloadingPixels = true;
	bool loaded_ = ofLoadImage(preloadedPixels, filePath); // load file to pixels
	if(!loaded_){
		ofLogError("ofxAutoTexture") << "failed to preload pixels from: " << filePath;
	}
	pixelsPreloaded = true;
	preloadingPixels = false;
	return loaded_;
}

bool ofxAutoTexture::arePixelPreLoaded(){
	return preloadedPixels.isAllocated();
}


bool ofxAutoTexture::isPreloadingPixels(){
	return preloadingPixels && !pixelsPreloaded;
}

void ofxAutoTexture::_update(ofEventArgs &e) {

	if(loaded || liveLoadError) {
		float timeNow = ofGetElapsedTimef();
		if(timeNow - lastCheckTime > nextCheckInterval) { // time to check again

			nextCheckInterval = textureFileCheckInterval + ofRandom(0.2);
			lastCheckTime = timeNow;
			std::time_t modif = getLastModified(filePath);

			if(lastModified != modif) { // file has been modified!
				// reload file!
				lastModified = modif;

				// ARB
				#ifndef TARGET_OPENGLES
				bool arbState = ofGetUsingArbTex();
				bool ourTexIsArb = getTextureData().textureTarget == GL_TEXTURE_RECTANGLE_ARB;
				if(ourTexIsArb != arbState) {
					if(GL_TEXTURE_RECTANGLE_ARB == getTextureData().textureTarget) {
						ofEnableArbTex();
					} else {
						ofDisableArbTex();
					}
				}
				#else
				bool arbState = false;
				bool ourTexIsArb = false;
				#endif

				// mipmap
				bool needsMipMap = hasMipmap();
				loaded = _loadFromFile(filePath);
				if(loaded) {
					liveLoadError = false;
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
					liveLoadError = true;
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
		ofLogNotice("ofxAutoTexture") << "loadFromFile(\"" << this->filePath << "\")";
	}else{
		ofLogError("ofxAutoTexture") << "Cant load tex from file '" << this->filePath << "'";
	}
	return loaded;
}

bool ofxAutoTexture::_loadFromFile(const string &filePath) {

	ofPixels pixels;
	bool loaded_;
	if(preloadedPixels.isAllocated()){
		loaded_ = true;
		pixels = preloadedPixels;
		preloadedPixels.clear();
	}else{
		loaded_ = ofLoadImage(pixels, filePath); // load file to pixels
	}
	
	if(loaded_) {
		int nChan = pixels.getNumChannels();
		// alloc texture space
		if(getWidth() != pixels.getWidth() || getHeight() != pixels.getHeight() || nChan != nChannels){
			allocate(pixels.getWidth(), pixels.getHeight(), ofGetGlInternalFormat(pixels), ofGetUsingArbTex());
		}

		// see if we need post-processing
		string type = ofFilePath::getFileExt(filePath);
		if(ofToLower(type) == "psd") { // psd's get a special treatment - remove white halo
			if(pixels.getNumChannels() == 4){
				ofLogNotice("ofxAutoTexture") << "Loading PSD file - removing white matte from file at '" << this->filePath << "'";
				removeWhiteMatte(pixels);
			}
		}else{
			makeTransparentPixelsThisColor(pixels, ofColor::black);
		}

		//support for overriding the RGB value of fully transparent pixels; user can name the file "xxx_transpFFFFFF.png"
		// where FFFFFF is an hex RGB value for the transparent pixels to be painted with
		string fileName = ofFilePath::getBaseName(filePath);
		if(ofIsStringInString(fileName, paintTransaprentPiexelsCommand)){ //if file contains "_transpWhite"
			auto strings = ofSplitString(fileName, paintTransaprentPiexelsCommand);
			if (strings.size() > 1){
				string hexStr = strings.back();
				if(hexStr.size() == 6){
					int hex = ofHexToInt(hexStr);
					int r = (hex >> 16) & 0xff;
					int g = (hex >> 8) & 0xff;
					int b = (hex >> 0) & 0xff;
					//process al a==0 pixels into user supplied RGB (mostly useful for mipmaps)
					makeTransparentPixelsThisColor(pixels, ofColor(r,g,b));
				}
			}
		}

		// copy pixel data to texture data
		ofTexture::loadData(pixels);
		float memUsed = memUse(this);
		totalLoadedMbytes += memUsed;
		currentlyLoadedMBytes += memUsed;
		
		nChannels = nChan;

		ofNotifyEvent(eventTextureReloaded, *this);
	} else {
		ofLogError("ofxAutoTexture") << "Can't load file at '" << this->filePath << "'";
	}
	return loaded_;
}

void ofxAutoTexture::removeWhiteMatte(ofPixels &pixels, bool makeTransparentPixelsBlack) {

	const int nChan = pixels.getNumChannels();
	if(nChan == 4){
		const size_t total = pixels.getWidth() * pixels.getHeight();
		unsigned char * data = pixels.getData();
		for(size_t i = 0; i < total; ++i) {
			const size_t k = i * 4;
			const unsigned char a = data[k + 3];
			if(a) {
				const float na = a / 255.0f; // normalized alpha
				const float ina = 1.0f - na; // inverse normalized alpha
				data[k    ] = (data[k    ] - 255.0f * ina) / na;
				data[k + 1] = (data[k + 1] - 255.0f * ina) / na;
				data[k + 2] = (data[k + 2] - 255.0f * ina) / na;
			}else{
				if(makeTransparentPixelsBlack){
					data[k    ] = 0;
					data[k + 1] = 0;
					data[k + 2] = 0;
				}
			}
		}
	}
}

float ofxAutoTexture::memUse(ofTexture * tex) {

	if (tex && tex->isAllocated()) {
		int w, h;
		#ifndef TARGET_OPENGLES
		if (tex->texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB) {
			w = ofNextPow2(tex->getWidth());
			h = ofNextPow2(tex->getHeight());
		} else {
			w = tex->getWidth();
			h = tex->getHeight();
		}
		#else
		w = tex->getWidth();
		h = tex->getHeight();
		#endif


		int numC = ofGetNumChannelsFromGLFormat(ofGetGLFormatFromInternal(tex->texData.glInternalFormat));
		float mem = w * h * numC;
		if (tex->hasMipmap()) {
			mem *= 1.3333; //mipmaps take 33% more memory
		}
		return mem / float(1024 * 1024); //return MBytes
	}
	return 0;
}

/*
float ofxAutoTexture::getCurrentlyLoadedMBytes(){
	
}*/

float ofxAutoTexture::getTotalLoadedMBytes(){
	return totalLoadedMbytes;
}


void ofxAutoTexture::makeTransparentPixelsThisColor(ofPixels &pixels, const ofColor & color){

	const int nChan = pixels.getNumChannels();
	if(nChan == 4){
		const size_t total = pixels.getWidth() * pixels.getHeight();
		unsigned char * data = pixels.getData();
		for(size_t i = 0; i < total; ++i) {
			const size_t k = i * 4;
			const unsigned char a = data[k + 3];
			if(a == 0) { //pixels with 0 alpha showuld be pure black to avoid weird mipmaps artifacts
				data[k    ] = color.r;
				data[k + 1] = color.g;
				data[k + 2] = color.b;
			}
		}
	}
}