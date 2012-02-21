/*
 *  ofxVending.h
 *  ofxVendingExample
 *
 *  Created by Stellato, Anthony on 9/7/11.
 *  Copyright 2012 Arnold Worldwide.
 *
 *  An addon for OF to interface with vending machines
 *  https://github.com/astellato/ofxVending
 */

#pragma once

#include "ofMain.h"
#include "ofxSimpleSerial.h"
#include <vector>
#include "ofUtils.h"

class ofxVending {
	public:

	ofxVending();
	~ofxVending();

	bool setup(string _device, int _baud);
	void onMDBMessage(string & _message);
	
	void startSession(int _credits);
	string& getLastChoice();
	bool isVendSuccess();
	bool isInSession();
	bool isReady();
	bool isSetup();

	protected:
	void processMessage(string& _message);
	void decodeChoice(string& _choice);
	
	// Vending Commands
	void sendBeginSession();
	void sendBeginSession(int _price);
	void sendVendApproved();
	void sendEndSession();
	void sendRequestEndSession();
	void sendForceEndSession();
	void sendReset();
	void sendVendDeny();
	
	ofxSimpleSerial serial;
	bool bVendFailed, bSession, bReady, bSetup;
	string lastChoice, device;
	int baud;
};