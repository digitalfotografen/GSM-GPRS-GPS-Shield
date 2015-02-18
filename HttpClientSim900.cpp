#include "HttpClientSim900.h"

/*
AT+CSQ
AT+CGATT?
AT+CGATT=1
AT+SAPBR=3,1,"CONTYPE","GPRS"
AT+SAPBR=3,1,"APN","internet.telenor.se"
AT+SAPBR=1,1
AT+HTTPINIT
AT+HTTPPARA="URL","http://logger.brobild.se/conf/12345678.conf" 
AT+HTTPACTION=0
AT+HTTPREAD
AT+HTTPTERM
*/

HttpClientSim900::HttpClientSim900(){
  _cid = 1;
}

HttpClient_retval HttpClientSim900::begin(byte cid, 
                            const char *apn,
                            const char *user,
                            const char *pwd){
  char cmd[40] = "";
  char tmp[10] = "";
  //if (CLS_FREE != gsm.GetCommLineStatus()) return (ret_val);

  if (gsm.SendATCmdWaitResp(F("AT+CGATT=1"), 1000, 50, str_ok, 3) != AT_RESP_OK) {
    Serial.println(F("Couldn't connect to GPRS"));
    _status = HTTP_NO_GPRS;
    return _status;
  }
  
  if (apn){ // if apn defined, else use stored parameters
    strcpy(cmd, "AT+SAPBR=3,");
    strcat(cmd, itoa(cid, tmp, 10));
    strcat(cmd, ",\"CONTYPE\",\"GPRS\"");
    if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
      Serial.println(F("Couldn't set SAPBR GPRS"));
      _status = HTTP_ERROR;
      return _status;
    }

    strcpy(cmd, "AT+SAPBR=3,");
    strcat(cmd, itoa(cid, tmp, 10));
    strcat(cmd, ",\"APN\",\"");
    strcat(cmd, apn);
    strcat(cmd, "\"");
    if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
      Serial.println(F("Couldn't set SAPBR APN"));
      _status = HTTP_ERROR;
      return _status;
    }

    if (user){
      strcpy(cmd, "AT+SAPBR=3,");
      strcat(cmd, itoa(cid, tmp, 10));
      strcat(cmd, ",\"USER\",\"");
      strcat(cmd, user);
      strcat(cmd, "\"");
      if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
        Serial.println(F("Couldn't set SAPBR USER"));
        _status = HTTP_ERROR;
        return _status;
      }
    }

    if (pwd){
      strcpy(cmd, "AT+SAPBR=3,");
      strcat(cmd, itoa(cid, tmp, 10));
      strcat(cmd, ",\"PWD\",\"");
      strcat(cmd, pwd);
      strcat(cmd, "\"");
      if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
        Serial.println(F("Couldn't set SAPBR PWD"));
        _status = HTTP_ERROR;
        return _status;
      }
    }

  } else { // load stored parameters from NVRAM
    if (cid != 0) {
      strcpy(cmd, "AT+SAPBR=4,");
      strcat(cmd, itoa(cid, tmp, 10));
      if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
        Serial.println(F("Couldn't load SAPBR"));
        _status = HTTP_ERROR;
        return _status;
      }
    } else {
      cid = _cid;
    }
  }
  
  // connect
  strcpy(cmd, "AT+SAPBR=1,");
  strcat(cmd, itoa(cid, tmp, 10));
  if (gsm.SendATCmdWaitResp(cmd, 3000, 50, str_ok, 3) != AT_RESP_OK) {
    _status = HTTP_NO_CONNECTION;
    return _status;
  }

  // check connection
  strcpy(cmd, "AT+SAPBR=2,");
  strcat(cmd, itoa(cid, tmp, 10));
  if (gsm.SendATCmdWaitResp(cmd, 1000, 50, ",1,", 3) != AT_RESP_OK){
    Serial.println(F("Waiting for connection"));
    _status = HTTP_NO_CONNECTION;
    return _status;
  };

  Serial.println(F("Connected"));
  gsm.setStatus(GSM::ATTACHED);
  _status = HTTP_CONNECTED;
  return _status;
}

boolean HttpClientSim900::saveBearer(byte cid){
  char cmd[40] = "AT+SAPBR=5,";
  char tmp[10] = "";

  strcat(cmd, itoa(cid, tmp, 10));
  if (gsm.SendATCmdWaitResp(cmd, 1000, 50, str_ok, 3) != AT_RESP_OK) {
    return true;
  } else {
    return false;
  }
}

int HttpClientSim900::httpGet(const char *url,
                              boolean allowRedirect){
  int ret_val;
  
  if (!httpRequest(url, allowRedirect)){
    Serial.println(F("httpRequest returned false"));
    return 0;
  }
  delay(100);
  //Serial.println(F("Time for action"));
  Serial.flush();
  ret_val = httpAction(0);
  return ret_val;
}

int HttpClientSim900::httpPost(const char *url,
                              const char *data,
                              boolean allowRedirect){
  int ret_val = 0;
  char tmp[10] = "";
  char cmd[40] = "AT+HTTPDATA=";
  strcat(cmd, itoa(strlen(data), tmp, 10));
  strcat(cmd, ",5000");
  if (!httpRequest(url, allowRedirect)){
    return 0;
  }

  gsm.WhileSimpleRead();
  if (gsm.SendATCmdWaitResp(cmd, 1000, 50, "DOWNLOAD", 3) != AT_RESP_OK) {
    Serial.println(F("Couldn't start data send"));
    ret_val = 0;
    return ret_val;
  }
  gsm.SimpleWrite(data);
  gsm.SimpleWrite("\r\n");
  gsm.WaitResp(5000, 50, "OK");
  return httpAction(1);
}

boolean HttpClientSim900::httpRequest(const char *url,
                                      boolean allowRedirect){
  //if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  //SetCommLineStatus(CLS_ATCMD);
  
  gsm.setStatus(GSM::TCPCONNECTEDCLIENT);
  _status = HTTP_ACTIVE;
  _responseCode = 0;
  _responseLength = -1;
  _pos = 0;
  
  if (gsm.SendATCmdWaitResp(F("AT+HTTPINIT"), 1000, 50, str_ok, 3) != AT_RESP_OK) {
    Serial.println(F("Couldn't initialize http"));
    return false;
  }
  Serial.println(F("Http get initialized"));
  
  gsm.SimpleWrite(F("AT+HTTPPARA=\"URL\",\"http://"));
  gsm.SimpleWrite(url);
  gsm.SimpleWriteln(F("\""));
  if (gsm.WaitResp(1000, 50, str_ok) != RX_FINISHED_STR_RECV) {
    Serial.println(F("Couldn't set url and path"));
    return false;
  }
  //Serial.println(F("Url and path set"));

  // activate redirect
  if (allowRedirect){
    if (gsm.SendATCmdWaitResp(F("AT+HTTPPARA=\"REDIR\",1"), 1000, 50, str_ok, 3) != AT_RESP_OK) {
      Serial.println(F("Couldn't set redirect"));
      return false;
    }
  }

  return true;
}

int HttpClientSim900::httpAction(int method){
  //Serial.println(F("HTTPACTION"));
  byte status;
  int ret_val = 0;
  gsm.WhileSimpleRead();
  gsm.SimpleWrite(F("AT+HTTPACTION="));
  gsm.SimpleWriteln(method);
  
  int tries = 0;
  do {
    tries++;
    //Serial.println(F("HTTPACTION waiting for response"));
    status = gsm.WaitResp(1000, 50);
    if (status == RX_FINISHED) {
      // response format +HTTPACTION:<Method>,<StatusCode>,<DataLen>
      char * pch;
      pch = strstr((char *)gsm.comm_buf, "+HTTPACTION:");
      if (pch){
        //Serial.println(pch);
        pch = strtok(pch+12, ",");
        //Serial.println(pch);
        pch = strtok(NULL, ",");
        _responseCode = atoi(pch);
        ret_val = _responseCode;
        pch = strtok(NULL, ",:");
        //Serial.println(pch);
        _responseLength = atoi(pch);
      }
    }
  } while ((tries < 30) && (ret_val == 0));
  
  return ret_val;
}

int HttpClientSim900::readResponse(char *buff, int start, int length){
  char * pch;
  byte status = 0;
  int resultLength = -1;
  length = min(COMM_BUF_LEN-25, length-1); // 
  
  if (start > _responseLength){
    return -1;
  }
  if (start + length > _responseLength){
    length = _responseLength - start;
  }

  gsm.WhileSimpleRead();
  gsm.SimpleWrite("AT+HTTPREAD=");
  gsm.SimpleWrite(start);
  gsm.SimpleWrite(",");
  gsm.SimpleWriteln(length);
  
  status = gsm.WaitResp(1000, 10, "+HTTPREAD:");
  
  if (status == RX_FINISHED_STR_RECV){
    pch = strstr((char *)gsm.comm_buf, "+HTTPREAD:");
    pch = strtok(pch, ":");
    pch = strtok(NULL, ":\n");
    resultLength = atoi(pch);
    //Serial.print("Resultln:");
    //Serial.println(resultLength);
    pch = strtok(NULL, "");
    memcpy(buff, pch, resultLength);
    buff[resultLength] = 0x00;
    //Serial.print("buff:");
    //Serial.println(buff);
  }

  int remaining = resultLength - strlen(buff);
  //Serial.print("Remaining:");
  //Serial.println(remaining);
  if (remaining > 0){
    gsm.read(buff+strlen(buff), remaining);
  }

  return resultLength;
}

int HttpClientSim900::readRow(char *buff, int length){
  if (_pos >= _responseLength) return 0;
  
  if (readResponse(buff, _pos, length)){
    char *pch = strtok(buff, "\r\n");
    int len = strlen(buff);
    _pos += len ? len : 1;
    return _responseLength - _pos;
  } else {
    _pos = _responseLength;
    return 0;
  }
}

boolean HttpClientSim900::terminate(){
  if (gsm.SendATCmdWaitResp("AT+HTTPTERM", 1000, 50, str_ok, 5) == AT_RESP_OK){
    Serial.print("Http connection terminated");
    gsm.setStatus(GSM::ATTACHED);
    _status = HTTP_CONNECTED;
    return true;
  } else {
    return false;
  }
}

boolean HttpClientSim900::close(){
  if (gsm.SendATCmdWaitResp("AT+SAPBR=0,1", 1000, 50, str_ok, 5) == AT_RESP_OK){
    Serial.print("Http connection closed");
    gsm.setStatus(GSM::READY);
    _status = HTTP_CLOSED;
    return true;
  } else {
    return false;
  }
}
