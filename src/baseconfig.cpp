#include "baseconfig.h"

BaseConfig::BaseConfig() : debuglevel(0) {  
  #ifdef ESP8266
    LittleFS.begin();
  #elif ESP32
    if (!LittleFS.begin(true)) { // true: format LittleFS/NVS if mount fails
      Serial.println("LittleFS Mount Failed");
    }
  #endif
  
  // Flash Write Issue
  // https://github.com/esp8266/Arduino/issues/4061#issuecomment-428007580
  // LittleFS.format();
  
  LoadJsonConfig();
}

void BaseConfig::StoreJsonConfig(String* json) {

  StaticJsonDocument<512> doc;
  deserializeJson(doc, *json);
  JsonObject root = doc.as<JsonObject>();

  if (!root.isNull()) {
    File configFile = LittleFS.open("/BaseConfig.json", "w");
    if (!configFile) {
      if (this->GetDebugLevel() >=0) {Serial.println("failed to open BaseConfig.json file for writing");}
    } else {  
      serializeJsonPretty(doc, Serial);
      if (serializeJson(doc, configFile) == 0) {
        if (this->GetDebugLevel() >=0) {Serial.println(F("Failed to write to file"));}
      }
      configFile.close();
  
      LoadJsonConfig();
    }
  }
}

void BaseConfig::LoadJsonConfig() {
  bool loadDefaultConfig = false;
  if (LittleFS.exists("/BaseConfig.json")) {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = LittleFS.open("/BaseConfig.json", "r");
    if (configFile) {
      Serial.println("opened config file");
      //size_t size = configFile.size();

      StaticJsonDocument<512> doc; // TODO Use computed size??
      DeserializationError error = deserializeJson(doc, configFile);
      
      if (!error) {
        serializeJsonPretty(doc, Serial);
        
        if (doc.containsKey("mqttroot"))         { this->mqtt_root = doc["mqttroot"].as<String>();} else {this->mqtt_root = "solax";}
        if (doc.containsKey("mqttserver"))       { this->mqtt_server = doc["mqttserver"].as<String>();} else {this->mqtt_server = "test.mosquitto.org";}
        if (doc.containsKey("mqttport"))         { this->mqtt_port = (int)(doc["mqttport"]);} else {this->mqtt_port = 1883;}
        if (doc.containsKey("mqttuser"))         { this->mqtt_username = doc["mqttuser"].as<String>();} else {this->mqtt_username = "";}
        if (doc.containsKey("mqttpass"))         { this->mqtt_password = doc["mqttpass"].as<String>();} else {this->mqtt_password = "";}
        if (doc.containsKey("mqttbasepath"))     { this->mqtt_basepath = doc["mqttbasepath"].as<String>();} else {this->mqtt_basepath = "home/";}
        if (doc.containsKey("UseRandomClientID")){ if (strcmp(doc["UseRandomClientID"], "none")==0) { this->mqtt_UseRandomClientID=false;} else {this->mqtt_UseRandomClientID=true;}} else {this->mqtt_UseRandomClientID = true;}
        if (doc.containsKey("SelectConnectivity")){ if (strcmp(doc["SelectConnectivity"], "wifi")==0) { this->useETH=false;} else {this->useETH=true;}} else {this->useETH = false;}
        if (doc.containsKey("debuglevel"))       { this->debuglevel = _max((int)(doc["debuglevel"]), 0);} else {this->debuglevel = 0; }
        if (doc.containsKey("SelectLAN"))        {this->LANBoard = doc["SelectLAN"].as<String>();} else {this->LANBoard = "";}
      } else {
        if (this->GetDebugLevel() >=1) {Serial.println("failed to load json config, load default config");}
        loadDefaultConfig = true;
      }
    }
  } else {
    if (this->GetDebugLevel() >=3) {Serial.println("BaseConfig.json config File not exists, load default config");}
    loadDefaultConfig = true;
  }

  if (loadDefaultConfig) {
    this->mqtt_server = "test.mosquitto.org";
    this->mqtt_port  = 1883;
    this->mqtt_username = "";
    this->mqtt_password = "";
    this->mqtt_root = "Solax";
    this->mqtt_basepath = "home/";
    this->mqtt_UseRandomClientID = true;
    this->useETH = false;
    this->debuglevel = 0;
    this->LANBoard = "";
    
    loadDefaultConfig = false; //set back
  }

  // Data Cleaning
  if(this->mqtt_basepath.endsWith("/")) {
    this->mqtt_basepath = this->mqtt_basepath.substring(0, this->mqtt_basepath.length()-1); 
  }

}

void BaseConfig::GetWebContent(AsyncResponseStream *response) {
  char buffer[200] = {0};
  memset(buffer, 0, sizeof(buffer));

  response->print("<form id='DataForm'>\n");
  response->print("<table id='maintable' class='editorDemoTable'>\n");
  response->print("<thead>\n");
  response->print("<tr>\n");
  response->print("<td style='width: 250px;'>Name</td>\n");
  response->print("<td style='width: 200px;'>Wert</td>\n");
  response->print("</tr>\n");
  response->print("</thead>\n");
  response->print("<tbody>\n");

  response->print("<tr>\n");
  response->print("<td>Device Name</td>\n");
  response->printf("<td><input size='30' maxlength='40' name='mqttroot' type='text' value='%s'/></td>\n", this->mqtt_root.c_str());
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td colspan='2'>\n");
  
  response->print("<div class='inline'>");
  response->printf("<input type='radio' id='sel_wifi' name='SelectConnectivity' value='wifi' %s onclick=\"radioselection([''],['SelectLAN'])\"/>", (this->useETH)?"":"checked");
  response->print("<label for='sel_wifi'>use WIFI</label></div>\n");
  
  response->print("<div class='inline'>");
  response->printf("<input type='radio' id='sel_eth' name='SelectConnectivity' value='eth' %s onclick=\"radioselection(['SelectLAN'],[''])\"/>", (this->useETH)?"checked":"");
  response->print("<label for='sel_eth'>use wired ethernet</label></div>\n");
    
  response->print("</td>\n");
  response->print("</tr>\n");

  response->printf("<tr id='SelectLAN' class='%s'>\n", (this->useETH?"":"hide"));
  response->print("<td>Select LAN Board</td>\n");
  response->print("<td><select name='SelectLAN' size='1'> \n");
  response->printf("<option %s value='WT32-ETH01'/>WT32-ETH01</option>\n", (this->LANBoard=="WT32-ETH01"?"selected":""));
  response->print("</select></td>\n");
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td>MQTT Server IP</td>\n");
  response->printf("<td><input size='30' name='mqttserver' type='text' value='%s'/></td>\n", this->mqtt_server.c_str());
  response->print("</tr>\n");
  
  response->print("<tr>\n");
  response->print("<td>MQTT Server Port</td>\n");
  response->printf("<td><input maxlength='5' name='mqttport' type='text' style='width: 6em' value='%d'/></td>\n", this->mqtt_port);
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td>MQTT Authentification: Username (optional)</td>\n");
  response->printf("<td><input size='30' name='mqttuser' type='text' value='%s'/></td>\n", this->mqtt_username.c_str());
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td>MQTT Authentification: Password (optional)</td>\n");
  response->printf("<td><input size='30' name='mqttpass' type='text' value='%s'/></td>\n", this->mqtt_password.c_str());
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td>MQTT Topic Base Path (example: home/inverter)</td>\n");
  response->printf("<td><input size='30' maxlength='40' name='mqttbasepath' type='text' value='%s'/></td>\n", this->mqtt_basepath.c_str());
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("  <td colspan='2'>\n");
  
  response->print("    <div class='inline'>");
  response->printf("<input type='radio' id='sel_URCID1' name='UseRandomClientID' value='none' %s />", (this->mqtt_UseRandomClientID)?"":"checked");
  response->print("<label for='sel_URCID1'>benutze statische MQTT ClientID</label></div>\n");
  
  response->print("    <div class='inline'>");
  response->printf("<input type='radio' id='sel_URCID2' name='UseRandomClientID' value='yes' %s />", (this->mqtt_UseRandomClientID)?"checked":"");
  response->print("<label for='sel_URCID2'>benutze dynamische MQTT ClientID</label></div>\n");
    
  response->print("  </td>\n");
  response->print("</tr>\n");

  response->print("<tr>\n");
  response->print("<td>DebugMode (0 [off] .. 5 [max]</td>\n");
  response->printf("<td><input min='0' max='5' name='debuglevel' type='number' style='width: 6em' value='%d'/></td>\n", this->debuglevel);
  response->print("</tr>\n");

  response->print("</tbody>\n");
  response->print("</table>\n");


  response->print("</form>\n\n<br />\n");
  response->print("<form id='jsonform' action='StoreBaseConfig' method='POST' onsubmit='return onSubmit(\"DataForm\", \"jsonform\")'>\n");
  response->print("  <input type='text' id='json' name='json' />\n");
  response->print("  <input type='submit' value='Speichern' />\n");
  response->print("</form>\n\n");

}
