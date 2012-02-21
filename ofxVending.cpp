/*
 *  ofxVending.cpp
 *  ofxVendingExample
 *
 *  Created by Stellato, Anthony on 9/7/11.
 *  Copyright 2012 Arnold Worldwide.
 *
 *  An addon for OF to interface with vending machines
 */

#include "ofxVending.h"

const bool bDebug = true;

ofxVending::ofxVending(){
	
}

ofxVending::~ofxVending(){
	
}

bool ofxVending::setup(string _device, int _baud){
	device = _device;
	baud = _baud;
	
	bSetup = serial.setup(device, baud);
	if(bSetup){
		serial.startContinuesRead(false);
		ofAddListener(serial.NEW_MESSAGE,this,&ofxVending::onMDBMessage);
	} else {
		ofLog(OF_LOG_ERROR, "ofxVending: No MDB connection.");
	}
	
	lastChoice = "";

	bVendFailed = false;
	bSession = false;
	
	if(!bDebug){
		bReady = false;
		
		//Force Machine Reset
		sendReset();
	} else {
		if(bSetup)
			bReady = true;
	}
	
	return bSetup;
}

void ofxVending::onMDBMessage(string & _message){
	processMessage(_message);
}

void ofxVending::startSession(int _credits){
	if(isReady() && isSetup()){
		sendBeginSession(_credits);
		ofSendMessage("ofxVending: SESSION BEGUN");
	} else {
		ofLog(OF_LOG_WARNING, "ofxVending: Cannot start session, not ready.");
	}
}

string& ofxVending::getLastChoice(){
	return lastChoice;
}

bool ofxVending::isSetup(){
	return bSetup;
}

bool ofxVending::isVendSuccess(){
	return !bVendFailed;
}

bool ofxVending::isInSession(){
	return bSession;
}

bool ofxVending::isReady(){
	return bReady;
}

// ---------- PROTECTED ------------

void ofxVending::processMessage(string& _message){
	vector<string> split = ofSplitString(_message, " " );
	int sSize = split.size() - 1;
	
	switch (sSize) {
		case -1:
			ofLog(OF_LOG_FATAL_ERROR, "ofxVending: Something is truly fucked up.");
			break;
		case 0:
			ofLog(OF_LOG_FATAL_ERROR, "ofxVending: Something is really fucked up.");
			break;
		case 1: //ACK NACK
			if(split[0] == "00"){
				ofLog(OF_LOG_VERBOSE, "ofxVending: V: ACK.");
				if(bVendFailed && !bSession){
					ofLog(OF_LOG_WARNING, "ofxVending: D: Vend Failed.  Please attempt a new session.");
					bVendFailed = false;
				}
			} else {
				ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			}
			
			break;
		case 2: //NOT USED
			ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			break;
		case 3: //READER ENABLE, READER DISABLE, VEND FAILED, VEND COMPLETE
			if(split[0] == "13"){ //VEND
				if(split[1] == "03"){ //FAILED
					ofLog(OF_LOG_WARNING, "ofxVending: V: Vend Failed.");
					bVendFailed = true;
				} else if(split[1] == "04"){ //COMPLETE
					ofLog(OF_LOG_VERBOSE, "ofxVending: V: Session Complete.");
					//debug(split);
					sendEndSession();
					ofSendMessage("ofxVending: SESSION COMPLETE");
				} else {
					ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
				}
				
			} else if(split[0] == "14"){ //READER
				if(split[1] == "01"){ //ENABLE
					ofLog(OF_LOG_VERBOSE, "ofxVending: V: Reader Enable.");
					bReady = true;
				} else if(split[1] == "00"){ //DISABLE
					ofLog(OF_LOG_VERBOSE, "ofxVending: V: Reader Disable.");
					bReady = false;
				} else {
					ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
				}
				
			} else {
				ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			}
			break;
		case 4:
			ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			break;
		case 5: //VEND SUCCESS, VEND FAILED
			if(split[0] == "13"){ //VEND
				if(split[1] == "02"){ //SUCCESS
					ofLog(OF_LOG_VERBOSE, "ofxVending: V: Vend Success.");
				} else {
					ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
				}
			} else {
				ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			}
			break;
		case 6:
			ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			break;
		case 7: //VEND REQUEST
			if(split[0] == "13"){ //VEND
				if(split[1] == "00"){ //REQUEST
					decodeChoice(split[5]);
				} else {
					ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
				}
				
			} else {
				ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			}
			break;
		default:
			ofLog(OF_LOG_WARNING, "ofxVending: Unknown message: " + _message);
			break;
	}
}

void ofxVending::decodeChoice(string& _choice){
	string hex = "0x" + _choice;
	string ret = "V: They chose ";
	
	string lc = "";
	//debug(hex);
	int n = ofHexToInt(hex);
	bool bInBounds = true;
	//string d = "n = " + ofToString(n);
	//debug(d);
	
	if(n < 10){
		ret += "A-" + ofToString(n);
		lc = "A-" + ofToString(n);
	} else if(n < 19){
		ret += "B-" + ofToString(n - 9);
		lc = "B-" + ofToString(n - 9);
	} else if(n < 28){
		ret += "C-" + ofToString(n - 18);
		lc = "C-" + ofToString(n - 18);
	} else if(n < 37) {
		ret += "D-" + ofToString(n - 27);
		lc = "D-" + ofToString(n - 27);
	} else if(n < 46){
		ret += "E-" + ofToString(n - 36);
		lc = "E-" + ofToString(n - 36);
	} else {
		ret += "out of bounds";
		lc = "error";
		bInBounds = false;
	}
	
	ret += ".";
	ofLog(OF_LOG_VERBOSE, "ofxVending: " + ret);
	
	lastChoice = lc;
	
	if(bInBounds){
		sendVendApproved();
	} else {
		sendVendDeny();
	}
	
	
}

void ofxVending::sendBeginSession(){
	if(bReady){
		serial.writeByte(0x03);
		serial.writeByte(0x00);
		serial.writeByte(0x14); //nickel scale $1.00: 0x14
		
		ofLog(OF_LOG_VERBOSE, "ofxVending: S: Begin Session.");
		bSession = true;
	} else {
		ofLog(OF_LOG_WARNING, "ofxVending: D: Machine is not ready.  Aborting.");
	}
}

void ofxVending::sendBeginSession(int _price){
	if(bReady){
		serial.writeByte(0x03);
		serial.writeByte(0x00);
		
		if(_price == 2){
			serial.writeByte(0x28);
		} else {
			serial.writeByte(0x14);
		}
		
		
		string d = "S: Begin Session " + ofToString(_price) + ".";
		ofLog(OF_LOG_VERBOSE, "ofxVending: " + d);
		bSession = true;
	} else {
		ofLog(OF_LOG_VERBOSE, "ofxVending: D: Machine is not ready.  Aborting.");
	}
}

void ofxVending::sendVendApproved(){
	serial.writeByte(0x05);
	serial.writeByte(0x00);
	serial.writeByte(0x07);
	
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: Vend Approved.");
}

void ofxVending::sendEndSession(){
	serial.writeByte(0x07);	
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: End Session.");
	
	bSession = false;
}

void ofxVending::sendRequestEndSession(){
	serial.writeByte(0x04);
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: Request End Session.");
	
	//bSession = false;
}

void ofxVending::sendForceEndSession(){
	serial.writeByte(0x00);
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: Force End Session.");
	
	bSession = false;
}

void ofxVending::sendReset(){
	// is this the same as above?
	serial.writeByte(0x00);
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: Reset.");
	
	bSession = false;
}

void ofxVending::sendVendDeny(){
	serial.writeByte(0x06);
	ofLog(OF_LOG_VERBOSE, "ofxVending: S: Vend Denied.");
}
