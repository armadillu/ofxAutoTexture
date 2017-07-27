# ofxAutoTexture

[![Build Status](https://travis-ci.org/armadillu/ofxAutoTexture.svg?branch=master)](https://travis-ci.org/armadillu/ofxAutoTexture)

OpenFrameworks addon to handle rapid prototyping through images. ofxAutoTexture is an ofTexture subclass that adds one extra feature; once loaded, it will check the filesystem for modifications to the src file, and re-load the texture from disk if the original file is modified.

It also has some other features like keeping track of VRAM used, cleaning up transparent pixels and others.
