/*    
ESPHelperWebConfig.cpp
Copyright (c) 2017 ItKindaWorks All right reserved.
github.com/ItKindaWorks

This file is part of ESPHelper

ESPHelper is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ESPHelper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ESPHelper.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ESPHelperWebConfig.h"

ESPHelperWebConfig::ESPHelperWebConfig(int port, const char* URI) : _localServer(port){
  _server = &_localServer;
  _runningLocal = true;
  _pageURI = URI;
}

ESPHelperWebConfig::ESPHelperWebConfig(ESP8266WebServer *server, const char* URI){
  _server = server;
  _runningLocal = false;
  _pageURI = URI;
}

bool ESPHelperWebConfig::begin(const char* _hostname){
  MDNS.begin(_hostname);
  return begin();
}

bool ESPHelperWebConfig::begin(){
  //setup server handlers
  //these handler function definitions use lambdas to pass the funtion... more information can be found here:
  //https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type
  _server->on(_pageURI, HTTP_GET, [this](){handleGet();});        // Call the 'handleRoot' function when a client requests URI "/"
  _server->on(_pageURI, HTTP_POST, [this](){handlePost();}); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  _server->onNotFound([this](){handleNotFound();});           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  if(_runningLocal){
    _server->begin(); // Actually start the server
  }  


	return true;
}

bool ESPHelperWebConfig::handle(){
	_server->handleClient();
	return _configLoaded;
}

netInfo ESPHelperWebConfig::getConfig(){
	_configLoaded = false;
	return _config;
}




//main config page that allows user to enter in configuration info
void ESPHelperWebConfig::handleGet() {                      
  _server->send(200, "text/html", 
    String("<form action=\"" + String(_pageURI) + "\" method=\"POST\">\
    <input type=\"text\" name=\"hostname\" placeholder=\"Device Hostname  (Required)\"></br>\
    <input type=\"text\" name=\"ssid\" placeholder=\"SSID  (Required)\"></br>\
    <input type=\"password\" name=\"netPass\" placeholder=\"Network Password\"></br>\
    <input type=\"password\" name=\"otaPassword\" placeholder=\"OTA Password\"></br>\
    <input type=\"text\" name=\"mqttHost\" placeholder=\"MQTT Host\"></br>\
    <input type=\"text\" name=\"mqttUser\" placeholder=\"MQTT Username\"></br>\
    <input type=\"text\" name=\"mqttPort\" placeholder=\"MQTT Port\"></br>\
    <input type=\"password\" name=\"mqttPass\" placeholder=\"MQTT Password\"></br>\
    <input type=\"submit\" value=\"Submit\"></form>\
    <p>Press Submit to update ESP8266 config file</p>"));
}

// If a POST request is made to URI /config
void ESPHelperWebConfig::handlePost() {   

  //make sure that all the arguments exist and that at least an SSID and hostname have been entered                      
  if( ! _server->hasArg("ssid") || ! _server->hasArg("netPass")
      || ! _server->hasArg("hostname") || ! _server->hasArg("mqttHost")
      || ! _server->hasArg("mqttUser") || ! _server->hasArg("mqttPass")
      || ! _server->hasArg("mqttPort") || ! _server->hasArg("otaPassword")
      || _server->arg("ssid") == NULL || _server->arg("hostname") == NULL){ // If the POST request doesn't have username and password data

    _server->send(400, "text/plain", "400: Invalid Request - Did you make sure to specify an SSID and Hostname?");  // The request is invalid, so send HTTP status 400
    return;
  }

  //if there is an mqtt user/pass/port entered then there better also be a host!
  if((_server->arg("mqttUser") != NULL || _server->arg("mqttPass") != NULL
      || _server->arg("mqttPort") != NULL) && _server->arg("mqttHost") == NULL){

    _server->send(400, "text/plain", "400: Invalid Request - MQTT info specified without host");  // The request is invalid, so send HTTP status 400
    return;
  }


  //convert the Strings returned by _server->arg to char arrays that can be entered into netInfo
  

  _server->arg("ssid").toCharArray(_newSsid, 64);
  _server->arg("netPass").toCharArray(_newNetPass, 64);
  _server->arg("mqttHost").toCharArray(_newMqttHost, 64);
  _server->arg("mqttUser").toCharArray(_newMqttUser, 64);
  _server->arg("mqttPass").toCharArray(_newMqttPass, 64);
  _server->arg("hostname").toCharArray(_newHostname, 64);
  _server->arg("otaPassword").toCharArray(_newOTAPass, 64);

  //the port is special because it doesnt get stored as a string so we take care of that
  
  if(_server->arg("mqttPort") != NULL){_newMqttPort = _server->arg("mqttPort").toInt();}
  else{_newMqttPort = 1883;}

  //tell the user that the config is loaded in and the module is restarting
  _server->send(200, "text/plain", "Config Info Loaded");

  //enter in the new data 
  _config = {mqttHost : _newMqttHost,     
             mqttUser : _newMqttUser,   
             mqttPass : _newMqttPass,   
             mqttPort : _newMqttPort,
             ssid : _newSsid, 
             pass : _newNetPass,
             otaPassword : _newOTAPass,
             hostname : _newHostname};


  _configLoaded = true;
}

void ESPHelperWebConfig::handleNotFound(){
  _server->send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}